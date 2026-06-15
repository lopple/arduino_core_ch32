#include "USBHID.h"

extern "C" {
#include "rv003usb.h"
#include "ch32fun.h"
}

static volatile uint32_t _rv003usbSysTickWrapCount = 0;
static volatile uint64_t _rv003usbMicrosBase = 0;
static volatile uint32_t _rv003usbMicrosCycleRemainder = 0;
static volatile uint64_t _rv003usbMillisBase = 0;
static volatile uint32_t _rv003usbMillisCycleRemainder = 0;
static bool _rv003usbStarted = false;

#define RV003USB_SYSTICK_WRAP_CYCLES (1ULL << 32)
#define RV003USB_CYCLES_PER_MICROSECOND (F_CPU / 1000000U)
#define RV003USB_CYCLES_PER_MILLISECOND (F_CPU / 1000U)
#define RV003USB_MICROS_PER_WRAP (RV003USB_SYSTICK_WRAP_CYCLES / RV003USB_CYCLES_PER_MICROSECOND)
#define RV003USB_MICROS_WRAP_CYCLE_REMAINDER (RV003USB_SYSTICK_WRAP_CYCLES % RV003USB_CYCLES_PER_MICROSECOND)
#define RV003USB_MILLIS_PER_WRAP (RV003USB_SYSTICK_WRAP_CYCLES / RV003USB_CYCLES_PER_MILLISECOND)
#define RV003USB_MILLIS_WRAP_CYCLE_REMAINDER (RV003USB_SYSTICK_WRAP_CYCLES % RV003USB_CYCLES_PER_MILLISECOND)

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
} __attribute__((packed)) RV003USBMouseReport;

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__((packed)) RV003USBKeyboardReport;

// Keep wrap accounting in ISR-sized chunks to avoid 64-bit division in millis()/micros().
static void rv003usbAccumulateSysTickWrap(uint64_t *timeBase, uint32_t *cycleRemainder, uint64_t unitsPerWrap, uint32_t cyclesRemainderPerWrap, uint32_t cyclesPerUnit)
{
    uint32_t remainder = *cycleRemainder + cyclesRemainderPerWrap;
    *timeBase += unitsPerWrap;
    if (remainder >= cyclesPerUnit) {
        *timeBase += 1;
        remainder -= cyclesPerUnit;
    }
    *cycleRemainder = remainder;
}

static uint32_t rv003usbCyclesToUnits(uint32_t ticks, uint32_t cycleRemainder, uint32_t cyclesPerUnit)
{
    uint32_t units = ticks / cyclesPerUnit;
    uint32_t remainder = ticks - units * cyclesPerUnit;
    return units + ((remainder + cycleRemainder) / cyclesPerUnit);
}

static uint32_t rv003usbReadSysTick(uint64_t *microsBase, uint32_t *microsRemainder, uint64_t *millisBase, uint32_t *millisRemainder)
{
    uint32_t wrap0;
    uint32_t wrap1;
    uint32_t ticks;
    uint32_t pending;

    for (;;) {
        wrap0 = _rv003usbSysTickWrapCount;
        *microsBase = _rv003usbMicrosBase;
        *microsRemainder = _rv003usbMicrosCycleRemainder;
        *millisBase = _rv003usbMillisBase;
        *millisRemainder = _rv003usbMillisCycleRemainder;
        ticks = (uint32_t)SysTick->CNT;
        pending = (uint32_t)SysTick->SR & 1U;
        wrap1 = _rv003usbSysTickWrapCount;

        if (wrap0 != wrap1) {
            continue;
        }

        if (pending != 0U) {
            ticks = (uint32_t)SysTick->CNT;
            wrap1 = _rv003usbSysTickWrapCount;
            if (wrap0 != wrap1) {
                continue;
            }
            if (ticks < 0x80000000U) {
                rv003usbAccumulateSysTickWrap(microsBase, microsRemainder, RV003USB_MICROS_PER_WRAP, RV003USB_MICROS_WRAP_CYCLE_REMAINDER, RV003USB_CYCLES_PER_MICROSECOND);
                rv003usbAccumulateSysTickWrap(millisBase, millisRemainder, RV003USB_MILLIS_PER_WRAP, RV003USB_MILLIS_WRAP_CYCLE_REMAINDER, RV003USB_CYCLES_PER_MILLISECOND);
            }
        }

        return ticks;
    }
}

static void rv003usbArduinoBegin()
{
    if (_rv003usbStarted) {
        return;
    }

    _rv003usbStarted = true;
    _rv003usbSysTickWrapCount = 0;
    _rv003usbMicrosBase = 0;
    _rv003usbMicrosCycleRemainder = 0;
    _rv003usbMillisBase = 0;
    _rv003usbMillisCycleRemainder = 0;

    // Switch SysTick from the core's 1 ms interrupt to a free-running counter.
    // This happens only after USBHID.begin(), so non-USB sketches stay byte-identical.
    SysTick->SR = 0;
    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = 0xFFFFFFFFU;
    SysTick->CTLR = 0xF;

    usb_setup();
}

RV003USBHID_ USBHID;

void RV003USBHID_::begin()
{
    rv003usbArduinoBegin();
}

void RV003USBHID_::end()
{
}

extern "C" {
// Strong timebase overrides used only when this library is linked into the sketch.
uint64_t GetTick(void)
{
    uint64_t microsBase;
    uint32_t microsRemainder;
    uint64_t millisBase;
    uint32_t millisRemainder;
    uint32_t ticks = rv003usbReadSysTick(&microsBase, &microsRemainder, &millisBase, &millisRemainder);
    return millisBase + rv003usbCyclesToUnits(ticks, millisRemainder, RV003USB_CYCLES_PER_MILLISECOND);
}

uint32_t getCurrentMicros(void)
{
    uint64_t microsBase;
    uint32_t microsRemainder;
    uint64_t millisBase;
    uint32_t millisRemainder;
    uint32_t ticks = rv003usbReadSysTick(&microsBase, &microsRemainder, &millisBase, &millisRemainder);
    return (uint32_t)(microsBase + rv003usbCyclesToUnits(ticks, microsRemainder, RV003USB_CYCLES_PER_MICROSECOND));
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    uint64_t microsBase = _rv003usbMicrosBase;
    uint32_t microsRemainder = _rv003usbMicrosCycleRemainder;
    uint64_t millisBase = _rv003usbMillisBase;
    uint32_t millisRemainder = _rv003usbMillisCycleRemainder;

    rv003usbAccumulateSysTickWrap(&microsBase, &microsRemainder, RV003USB_MICROS_PER_WRAP, RV003USB_MICROS_WRAP_CYCLE_REMAINDER, RV003USB_CYCLES_PER_MICROSECOND);
    rv003usbAccumulateSysTickWrap(&millisBase, &millisRemainder, RV003USB_MILLIS_PER_WRAP, RV003USB_MILLIS_WRAP_CYCLE_REMAINDER, RV003USB_CYCLES_PER_MILLISECOND);

    _rv003usbMicrosBase = microsBase;
    _rv003usbMicrosCycleRemainder = microsRemainder;
    _rv003usbMillisBase = millisBase;
    _rv003usbMillisCycleRemainder = millisRemainder;
    _rv003usbSysTickWrapCount++;
    SysTick->SR = 0;
}

void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal *ist)
{
    (void)e;
    (void)scratchpad;
    (void)ist;

    if (endp == 1) {
        static const RV003USBMouseReport mouseReport = {0, 0, 0, 0};
        usb_send_data(&mouseReport, sizeof(mouseReport), 0, sendtok);
        return;
    }

    if (endp == 2) {
        static const RV003USBKeyboardReport keyboardReport = {0, 0, {0, 0, 0, 0, 0, 0}};
        usb_send_data(&keyboardReport, sizeof(keyboardReport), 0, sendtok);
        return;
    }

    usb_send_empty(sendtok);
}

}

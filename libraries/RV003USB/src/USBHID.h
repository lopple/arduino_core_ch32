#ifndef _USBHID_H
#define _USBHID_H

#include "Arduino.h"

#if !defined(CH32V003F4)
#error "RV003USB HID support is currently available only for CH32V003F4."
#endif

// rv003usb reconfigures SysTick as a free-running counter. Keep the delay
// replacement local to sketches that include this library.
static inline void ch32v003_rv003usb_delayMicroseconds(uint32_t us)
{
    const uint32_t cyclesPerMicrosecond = F_CPU / 1000000U;
    uint32_t nbTicks = (us - ((us > 0) ? 1 : 0)) * cyclesPerMicrosecond;
    uint32_t startTicks = (uint32_t)SysTick->CNT;
    uint32_t currentTicks;

    do {
        currentTicks = (uint32_t)SysTick->CNT;
    } while ((uint32_t)(currentTicks - startTicks) < nbTicks);
}

#define delayMicroseconds(us) ch32v003_rv003usb_delayMicroseconds(us)

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__((packed)) KeyboardReport_t;

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
} __attribute__((packed)) MouseReport_t;

typedef bool (*RV003USBKeyboardReportProvider)(KeyboardReport_t *report);
typedef bool (*RV003USBMouseReportProvider)(MouseReport_t *report);

class RV003USBHID_ {
public:
    void begin();
    void end();
};

extern RV003USBHID_ USBHID;

extern "C" void rv003usbSetKeyboardReportProvider(RV003USBKeyboardReportProvider provider);
extern "C" void rv003usbSetMouseReportProvider(RV003USBMouseReportProvider provider);

#endif // _USBHID_H

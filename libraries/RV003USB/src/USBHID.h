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

class RV003USBHID_ {
public:
    void begin();
    void end();
};

extern RV003USBHID_ USBHID;

#endif // _USBHID_H

#define ARDUINO_MAIN

#include "Arduino.h"
#include "debug.h"

extern "C" void rv003usb_core_poll(void) __attribute__((weak));


/*
 * \brief Main entry point of Arduino application
 */
int main( void )
{
    pre_init( );
#if defined(USE_TINYUSB)
    if (TinyUSB_Device_Init) {
        TinyUSB_Device_Init(0);
    }
#endif
    setup( );
  
    do {
        loop( );
#if defined(USE_TINYUSB)
        if (TinyUSB_Device_Task) {
            TinyUSB_Device_Task();
        }
        if (TinyUSB_Device_FlushCDC) {
            TinyUSB_Device_FlushCDC();
        }
#endif
        if (rv003usb_core_poll) {
            rv003usb_core_poll();
        }
    } while (1);

    return 0;
}

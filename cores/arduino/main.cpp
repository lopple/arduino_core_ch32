#define ARDUINO_MAIN

#include "Arduino.h"
#include "debug.h"

#if defined(CH32V003_RV003USB)
extern "C" {
void usb_setup(void);
void usb_poll(void);
}
#endif

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
#elif defined(CH32V003_RV003USB)
    usb_setup();
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
#elif defined(CH32V003_RV003USB)
        usb_poll();
#endif
    } while (1);

    return 0;
}

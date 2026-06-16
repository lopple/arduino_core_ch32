#include <USBHID.h>
#include <USBHIDMonitor.h>

void setup()
{
    USBHIDMonitor.begin();
    delay(1000);
    USBHIDMonitor.println("RV003USB HID monitor echo ready.");
}

void loop()
{
    while (USBHIDMonitor.available() > 0) {
        int c = USBHIDMonitor.read();
        if (c >= 0) {
            USBHIDMonitor.write((uint8_t)c);
        }
    }
}

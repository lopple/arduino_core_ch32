#include <USBHID.h>
#include <USBKeyboard_US.h>

void setup()
{
    Keyboard.begin();
    delay(3000);

    // Use USBKeyboard_JIS.h instead when the host OS uses a Japanese layout.
    Keyboard.println("Hello world!");
}

void loop()
{
}

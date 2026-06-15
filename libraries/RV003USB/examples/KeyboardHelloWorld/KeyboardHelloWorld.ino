#include <USBHID.h>
#include <Keyboard_US.h>

void setup()
{
    Keyboard.begin();
    delay(3000);

    // ASCII punctuation follows the host OS keyboard layout.
    Keyboard.println("Hello world!");
}

void loop()
{
}

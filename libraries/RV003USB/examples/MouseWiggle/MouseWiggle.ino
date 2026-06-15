#include <USBHID.h>
#include <USBMouse.h>

void setup()
{
    Mouse.begin();
    delay(3000);

    for (int i = 0; i < 5; i++) {
        Mouse.move(20, 0);
        delay(200);
        Mouse.move(-20, 0);
        delay(800);
    }
}

void loop()
{
}

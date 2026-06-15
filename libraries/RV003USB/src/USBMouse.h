#ifndef _RV003USB_USB_MOUSE_H
#define _RV003USB_USB_MOUSE_H

#include "USBHID.h"

#define MOUSE_LEFT   0x01
#define MOUSE_RIGHT  0x02
#define MOUSE_MIDDLE 0x04

class Mouse_ {
private:
    uint8_t _buttons;
    int8_t _x;
    int8_t _y;
    int8_t _wheel;
    bool _dirty;

    void sendReport();

public:
    Mouse_() : _buttons(0), _x(0), _y(0), _wheel(0), _dirty(false) {}

    void begin();
    void end();
    void click(uint8_t button = MOUSE_LEFT);
    void move(int8_t x, int8_t y, int8_t wheel = 0);
    void press(uint8_t button = MOUSE_LEFT);
    void release(uint8_t button = MOUSE_LEFT);
    bool isPressed(uint8_t button = MOUSE_LEFT);
    bool getReport(MouseReport_t *report);
};

// Keep the Mouse object lazy so USBHID.begin()-only sketches do not link the
// mouse API implementation.
extern Mouse_ &rv003usbMouse();

#define Mouse rv003usbMouse()

#endif // _RV003USB_USB_MOUSE_H

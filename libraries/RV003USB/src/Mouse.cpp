#include "USBMouse.h"

static bool rv003usbMouseGetReport(MouseReport_t *report)
{
    return Mouse.getReport(report);
}

Mouse_ &rv003usbMouse()
{
    static Mouse_ mouse;
    return mouse;
}

void Mouse_::begin()
{
    rv003usbSetMouseReportProvider(rv003usbMouseGetReport);
    USBHID.begin();
    _buttons = 0;
    _x = 0;
    _y = 0;
    _wheel = 0;
    _dirty = false;
}

void Mouse_::end()
{
    release(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE);
    rv003usbSetMouseReportProvider(nullptr);
}

void Mouse_::sendReport()
{
    _dirty = true;
    uint32_t start = millis();
    while (_dirty) {
        if ((uint32_t)(millis() - start) > 100U) {
            _dirty = false;
            break;
        }
    }
}

void Mouse_::click(uint8_t button)
{
    press(button);
    release(button);
}

void Mouse_::move(int8_t x, int8_t y, int8_t wheel)
{
    _x = x;
    _y = y;
    _wheel = wheel;
    sendReport();
}

void Mouse_::press(uint8_t button)
{
    uint8_t newButtons = _buttons | button;
    if (newButtons != _buttons) {
        _buttons = newButtons;
        sendReport();
    }
}

void Mouse_::release(uint8_t button)
{
    uint8_t newButtons = _buttons & (uint8_t)~button;
    if (newButtons != _buttons) {
        _buttons = newButtons;
        sendReport();
    }
}

bool Mouse_::isPressed(uint8_t button)
{
    return (_buttons & button) != 0;
}

bool Mouse_::getReport(MouseReport_t *report)
{
    report->buttons = _buttons;
    report->x = _x;
    report->y = _y;
    report->wheel = _wheel;

    _x = 0;
    _y = 0;
    _wheel = 0;
    _dirty = false;
    return true;
}

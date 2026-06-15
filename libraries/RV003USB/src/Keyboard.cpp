#include "Keyboard_US.h"

#include <string.h>

#define RV003USB_KEY_SHIFT 0x80

// Arduino Keyboard-style ASCII input is mapped for a US keyboard layout.
// The host OS layout still decides which character each HID key position emits.
static const uint8_t _asciiMap[128] __attribute__((section(".rodata"))) = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2a, 0x2b, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x1e | RV003USB_KEY_SHIFT, 0x34 | RV003USB_KEY_SHIFT, 0x20 | RV003USB_KEY_SHIFT,
    0x21 | RV003USB_KEY_SHIFT, 0x22 | RV003USB_KEY_SHIFT, 0x24 | RV003USB_KEY_SHIFT, 0x34,
    0x26 | RV003USB_KEY_SHIFT, 0x27 | RV003USB_KEY_SHIFT, 0x25 | RV003USB_KEY_SHIFT, 0x2e | RV003USB_KEY_SHIFT,
    0x36, 0x2d, 0x37, 0x38,
    0x27, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x33 | RV003USB_KEY_SHIFT, 0x33, 0x36 | RV003USB_KEY_SHIFT, 0x2e,
    0x37 | RV003USB_KEY_SHIFT, 0x38 | RV003USB_KEY_SHIFT, 0x1f | RV003USB_KEY_SHIFT, 0x04 | RV003USB_KEY_SHIFT,
    0x05 | RV003USB_KEY_SHIFT, 0x06 | RV003USB_KEY_SHIFT, 0x07 | RV003USB_KEY_SHIFT, 0x08 | RV003USB_KEY_SHIFT,
    0x09 | RV003USB_KEY_SHIFT, 0x0a | RV003USB_KEY_SHIFT, 0x0b | RV003USB_KEY_SHIFT, 0x0c | RV003USB_KEY_SHIFT,
    0x0d | RV003USB_KEY_SHIFT, 0x0e | RV003USB_KEY_SHIFT, 0x0f | RV003USB_KEY_SHIFT, 0x10 | RV003USB_KEY_SHIFT,
    0x11 | RV003USB_KEY_SHIFT, 0x12 | RV003USB_KEY_SHIFT, 0x13 | RV003USB_KEY_SHIFT, 0x14 | RV003USB_KEY_SHIFT,
    0x15 | RV003USB_KEY_SHIFT, 0x16 | RV003USB_KEY_SHIFT, 0x17 | RV003USB_KEY_SHIFT, 0x18 | RV003USB_KEY_SHIFT,
    0x19 | RV003USB_KEY_SHIFT, 0x1a | RV003USB_KEY_SHIFT, 0x1b | RV003USB_KEY_SHIFT, 0x1c | RV003USB_KEY_SHIFT,
    0x1d | RV003USB_KEY_SHIFT, 0x2f, 0x31, 0x30,
    0x23 | RV003USB_KEY_SHIFT, 0x2d | RV003USB_KEY_SHIFT, 0x35, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
    0x1d, 0x2f | RV003USB_KEY_SHIFT, 0x31 | RV003USB_KEY_SHIFT, 0x30 | RV003USB_KEY_SHIFT,
    0x35 | RV003USB_KEY_SHIFT, 0x00
};

static bool rv003usbMapKeyboardKey(uint8_t key, uint8_t *hidKey, uint8_t *modifierMask)
{
    *modifierMask = 0;

    if (key >= KEY_LEFT_CTRL && key <= KEY_RIGHT_GUI) {
        *hidKey = 0;
        *modifierMask = (uint8_t)(1U << (key - KEY_LEFT_CTRL));
        return true;
    }

    if (key >= KEY_F13 && key <= KEY_F24) {
        *hidKey = (uint8_t)(0x68U + (key - KEY_F13));
        return true;
    }

    if (key >= KEY_F1 && key <= KEY_F12) {
        *hidKey = (uint8_t)(0x3aU + (key - KEY_F1));
        return true;
    }

    switch (key) {
    case KEY_RETURN:
        *hidKey = 0x28;
        return true;
    case KEY_ESC:
        *hidKey = 0x29;
        return true;
    case KEY_BACKSPACE:
        *hidKey = 0x2a;
        return true;
    case KEY_TAB:
        *hidKey = 0x2b;
        return true;
    case KEY_CAPS_LOCK:
        *hidKey = 0x39;
        return true;
    case KEY_RIGHT_ARROW:
        *hidKey = 0x4f;
        return true;
    case KEY_LEFT_ARROW:
        *hidKey = 0x50;
        return true;
    case KEY_DOWN_ARROW:
        *hidKey = 0x51;
        return true;
    case KEY_UP_ARROW:
        *hidKey = 0x52;
        return true;
    case KEY_INSERT:
        *hidKey = 0x49;
        return true;
    case KEY_HOME:
        *hidKey = 0x4a;
        return true;
    case KEY_PAGE_UP:
        *hidKey = 0x4b;
        return true;
    case KEY_DELETE:
        *hidKey = 0x4c;
        return true;
    case KEY_END:
        *hidKey = 0x4d;
        return true;
    case KEY_PAGE_DOWN:
        *hidKey = 0x4e;
        return true;
    default:
        break;
    }

    if (key >= 128) {
        return false;
    }

    uint8_t mapped = _asciiMap[key];
    if (mapped == 0) {
        return false;
    }

    if ((mapped & RV003USB_KEY_SHIFT) != 0) {
        *modifierMask = 0x02;
        mapped &= (uint8_t)~RV003USB_KEY_SHIFT;
    }

    *hidKey = mapped;
    return true;
}

static bool rv003usbKeyboardGetReport(KeyboardReport_t *report)
{
    return Keyboard.getReport(report);
}

Keyboard_ &rv003usbKeyboard()
{
    static Keyboard_ keyboard;
    return keyboard;
}

void Keyboard_::begin()
{
    rv003usbSetKeyboardReportProvider(rv003usbKeyboardGetReport);
    USBHID.begin();
    memset(&_keyReport, 0, sizeof(_keyReport));
    _dirty = false;
}

void Keyboard_::end()
{
    releaseAll();
    rv003usbSetKeyboardReportProvider(nullptr);
}

void Keyboard_::sendReport()
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

size_t Keyboard_::press(uint8_t key)
{
    uint8_t hidKey;
    uint8_t modifierMask;

    if (!rv003usbMapKeyboardKey(key, &hidKey, &modifierMask)) {
        return 0;
    }

    if (hidKey == 0) {
        _keyReport.modifiers |= modifierMask;
        sendReport();
        return 1;
    }

    if (modifierMask != 0) {
        _keyReport.modifiers |= modifierMask;
    }

    for (uint8_t i = 0; i < sizeof(_keyReport.keys); i++) {
        if (_keyReport.keys[i] == hidKey) {
            return 1;
        }
    }

    for (uint8_t i = 0; i < sizeof(_keyReport.keys); i++) {
        if (_keyReport.keys[i] == 0) {
            _keyReport.keys[i] = hidKey;
            sendReport();
            return 1;
        }
    }

    return 0;
}

size_t Keyboard_::release(uint8_t key)
{
    uint8_t hidKey;
    uint8_t modifierMask;

    if (!rv003usbMapKeyboardKey(key, &hidKey, &modifierMask)) {
        return 0;
    }

    if (modifierMask != 0) {
        _keyReport.modifiers &= (uint8_t)~modifierMask;
    }

    if (hidKey == 0) {
        sendReport();
        return 1;
    }

    for (uint8_t i = 0; i < sizeof(_keyReport.keys); i++) {
        if (_keyReport.keys[i] == hidKey) {
            _keyReport.keys[i] = 0;
            sendReport();
            return 1;
        }
    }

    return 0;
}

void Keyboard_::releaseAll()
{
    memset(&_keyReport, 0, sizeof(_keyReport));
    sendReport();
}

size_t Keyboard_::write_char(uint8_t key)
{
    if (!press(key)) {
        return 0;
    }

    release(key);
    return 1;
}

bool Keyboard_::getReport(KeyboardReport_t *report)
{
    if (!_dirty) {
        return false;
    }

    memcpy(report, &_keyReport, sizeof(_keyReport));
    _dirty = false;
    return true;
}

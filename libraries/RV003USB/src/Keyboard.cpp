#include "USBKeyboard.h"

#include <string.h>

#define RV003USB_KEY_SHIFT 0x0100U

static Keyboard_ *_rv003usbActiveKeyboard = nullptr;

bool Keyboard_::mapKey(uint8_t key, uint8_t *hidKey, uint8_t *modifierMask)
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

    RV003USBKeyboardMapEntry mapped = _asciiMap[key];
    if (mapped == 0) {
        return false;
    }

    if ((mapped & RV003USB_KEY_SHIFT) != 0) {
        *modifierMask = 0x02;
        mapped &= (RV003USBKeyboardMapEntry)~RV003USB_KEY_SHIFT;
    }

    *hidKey = (uint8_t)mapped;
    return true;
}

static bool rv003usbKeyboardGetReport(KeyboardReport_t *report)
{
    return (_rv003usbActiveKeyboard != nullptr) && _rv003usbActiveKeyboard->getReport(report);
}

void Keyboard_::begin()
{
    _rv003usbActiveKeyboard = this;
    rv003usbSetKeyboardReportProvider(rv003usbKeyboardGetReport);
    USBHID.begin();
    memset(&_keyReport, 0, sizeof(_keyReport));
    _dirty = false;
}

void Keyboard_::end()
{
    releaseAll();
    if (_rv003usbActiveKeyboard == this) {
        _rv003usbActiveKeyboard = nullptr;
    }
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

    if (!mapKey(key, &hidKey, &modifierMask)) {
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

    if (!mapKey(key, &hidKey, &modifierMask)) {
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
    if (key == '\r') {
        return 1;
    }

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

#if defined(CH32V003_RV003USB)

#include "USBHID.h"

extern "C" {
#include "rv003usb.h"
#include "ch32fun.h"
}

// Global instances
Keyboard_ Keyboard;
Mouse_ Mouse;

#define SHIFT 0x80
static const uint8_t _asciiMap[128] __attribute__((section(".rodata"))) = {
    0x00,             // NUL
    0x00,             // SOH
    0x00,             // STX
    0x00,             // ETX
    0x00,             // EOT
    0x00,             // ENQ
    0x00,             // ACK
    0x00,             // BEL
    0x2a,             // BS  Backspace
    0x2b,             // TAB Tab
    0x28,             // LF  Enter
    0x00,             // VT
    0x00,             // FF
    0x00,             // CR
    0x00,             // SO
    0x00,             // SI
    0x00,             // DLE
    0x00,             // DC1
    0x00,             // DC2
    0x00,             // DC3
    0x00,             // DC4
    0x00,             // NAK
    0x00,             // SYN
    0x00,             // ETB
    0x00,             // CAN
    0x00,             // EM
    0x00,             // SUB
    0x29,             // ESC Escape
    0x00,             // FS
    0x00,             // GS
    0x00,             // RS
    0x00,             // US
    0x2c,             //  ' '
    0x1e|SHIFT,       // !
    0x34|SHIFT,       // "
    0x20|SHIFT,       // #
    0x21|SHIFT,       // $
    0x22|SHIFT,       // %
    0x24|SHIFT,       // &
    0x34,             // '
    0x26|SHIFT,       // (
    0x27|SHIFT,       // )
    0x25|SHIFT,       // *
    0x2e|SHIFT,       // +
    0x36,             // ,
    0x2d,             // -
    0x37,             // .
    0x38,             // /
    0x27,             // 0
    0x1e,             // 1
    0x1f,             // 2
    0x20,             // 3
    0x21,             // 4
    0x22,             // 5
    0x23,             // 6
    0x24,             // 7
    0x25,             // 8
    0x26,             // 9
    0x33|SHIFT,       // :
    0x33,             // ;
    0x36|SHIFT,       // <
    0x2e,             // =
    0x37|SHIFT,       // >
    0x38|SHIFT,       // ?
    0x1f|SHIFT,       // @
    0x04|SHIFT,       // A
    0x05|SHIFT,       // B
    0x06|SHIFT,       // C
    0x07|SHIFT,       // D
    0x08|SHIFT,       // E
    0x09|SHIFT,       // F
    0x0a|SHIFT,       // G
    0x0b|SHIFT,       // H
    0x0c|SHIFT,       // I
    0x0d|SHIFT,       // J
    0x0e|SHIFT,       // K
    0x0f|SHIFT,       // L
    0x10|SHIFT,       // M
    0x11|SHIFT,       // N
    0x12|SHIFT,       // O
    0x13|SHIFT,       // P
    0x14|SHIFT,       // Q
    0x15|SHIFT,       // R
    0x16|SHIFT,       // S
    0x17|SHIFT,       // T
    0x18|SHIFT,       // U
    0x19|SHIFT,       // V
    0x1a|SHIFT,       // W
    0x1b|SHIFT,       // X
    0x1c|SHIFT,       // Y
    0x1d|SHIFT,       // Z
    0x2f,             // [
    0x31,             // \
    0x30,             // ]
    0x23|SHIFT,       // ^
    0x2d|SHIFT,       // _
    0x35,             // `
    0x04,             // a
    0x05,             // b
    0x06,             // c
    0x07,             // d
    0x08,             // e
    0x09,             // f
    0x0a,             // g
    0x0b,             // h
    0x0c,             // i
    0x0d,             // j
    0x0e,             // k
    0x0f,             // l
    0x10,             // m
    0x11,             // n
    0x12,             // o
    0x13,             // p
    0x14,             // q
    0x15,             // r
    0x16,             // s
    0x17,             // t
    0x18,             // u
    0x19,             // v
    0x1a,             // w
    0x1b,             // x
    0x1c,             // y
    0x1d,             // z
    0x2f|SHIFT,       // {
    0x31|SHIFT,       // |
    0x30|SHIFT,       // }
    0x35|SHIFT,       // ~
    0x00              // DEL
};

// --- Keyboard_ implementation ---

void Keyboard_::begin() {
    memset(&_keyReport, 0, sizeof(KeyboardReport_t));
    _dirty = false;
}

void Keyboard_::end() {
    releaseAll();
}

void Keyboard_::sendReport() {
    _dirty = true;
    uint32_t start = millis();
    while (_dirty) {
        if (millis() - start > 100) {
            _dirty = false;
            break;
        }
    }
}

size_t Keyboard_::press(uint8_t k) {
    uint8_t i;
    if (k >= 0x80 && k <= 0x87) { // Modifier keys
        _keyReport.modifiers |= (1 << (k - 0x80));
        sendReport();
        return 1;
    }

    // Map special keys
    if (k >= 0xDA && k <= 0xDF) {
        // DA->Up, D9->Down, D8->Left, D7->Right
        static const uint8_t arrowKeys[] = { 0x52, 0x51, 0x50, 0x4F }; // Up, Down, Left, Right
        k = arrowKeys[0xDA - k];
    } else if (k >= 0xB0 && k <= 0xB3) {
        // B0->Return, B1->Esc, B2->Backspace, B3->Tab
        static const uint8_t specialKeys[] = { 0x28, 0x29, 0x2A, 0x2B };
        k = specialKeys[k - 0xB0];
    } else if (k >= 0xC1 && k <= 0xCD) {
        // C1->CapsLock, C2..CD -> F1..F12
        if (k == 0xC1) k = 0x39;
        else k = 0x3A + (k - 0xC2);
    } else if (k >= 0xD1 && k <= 0xD6) {
        // D1->Insert, D2->Home, D3->PageUp, D4->Delete, D5->End, D6->PageDown
        static const uint8_t navKeys[] = { 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E };
        k = navKeys[k - 0xD1];
    } else if (k < 128) {
        // Regular ASCII key
        uint8_t val = _asciiMap[k];
        if (val == 0) return 0;
        if (val & SHIFT) {
            _keyReport.modifiers |= 0x02; // Left Shift
            val &= 0x7F;
        }
        k = val;
    } else {
        return 0;
    }

    // Add key to report if not already present
    for (i = 0; i < 6; i++) {
        if (_keyReport.keys[i] == k) return 1; // Already pressed
    }
    for (i = 0; i < 6; i++) {
        if (_keyReport.keys[i] == 0) {
            _keyReport.keys[i] = k;
            sendReport();
            return 1;
        }
    }
    return 0;
}

size_t Keyboard_::release(uint8_t k) {
    uint8_t i;
    if (k >= 0x80 && k <= 0x87) {
        _keyReport.modifiers &= ~(1 << (k - 0x80));
        sendReport();
        return 1;
    }

    // Map special keys (same mapping as press)
    if (k >= 0xDA && k <= 0xDF) {
        static const uint8_t arrowKeys[] = { 0x52, 0x51, 0x50, 0x4F };
        k = arrowKeys[0xDA - k];
    } else if (k >= 0xB0 && k <= 0xB3) {
        static const uint8_t specialKeys[] = { 0x28, 0x29, 0x2A, 0x2B };
        k = specialKeys[k - 0xB0];
    } else if (k >= 0xC1 && k <= 0xCD) {
        if (k == 0xC1) k = 0x39;
        else k = 0x3A + (k - 0xC2);
    } else if (k >= 0xD1 && k <= 0xD6) {
        static const uint8_t navKeys[] = { 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E };
        k = navKeys[k - 0xD1];
    } else if (k < 128) {
        uint8_t val = _asciiMap[k];
        if (val == 0) return 0;
        if (val & SHIFT) {
            _keyReport.modifiers &= ~0x02; // Release Left Shift
            val &= 0x7F;
        }
        k = val;
    } else {
        return 0;
    }

    // Remove key from report
    for (i = 0; i < 6; i++) {
        if (_keyReport.keys[i] == k) {
            _keyReport.keys[i] = 0;
            sendReport();
            return 1;
        }
    }
    return 0;
}

void Keyboard_::releaseAll() {
    memset(&_keyReport, 0, sizeof(KeyboardReport_t));
    sendReport();
}

size_t Keyboard_::write_char(uint8_t k) {
    if (press(k)) {
        release(k);
        return 1;
    }
    return 0;
}

bool Keyboard_::getReport(KeyboardReport_t* report) {
    if (_dirty) {
        memcpy(report, &_keyReport, sizeof(KeyboardReport_t));
        _dirty = false;
        return true;
    }
    return false;
}

// --- Mouse_ implementation ---

Mouse_::Mouse_() {
    begin();
}

void Mouse_::begin() {
    memset(&_mouseReport, 0, sizeof(MouseReport_t));
    _dirty = false;
}

void Mouse_::end() {
    // Release all buttons
    _mouseReport.buttons = 0;
    sendReport();
}

void Mouse_::sendReport() {
    _dirty = true;
    uint32_t start = millis();
    while (_dirty) {
        if (millis() - start > 100) {
            _dirty = false;
            break;
        }
    }
}

void Mouse_::click(uint8_t b) {
    press(b);
    release(b);
}

void Mouse_::move(int8_t x, int8_t y, int8_t wheel) {
    _mouseReport.x = x;
    _mouseReport.y = y;
    _mouseReport.wheel = wheel;
    sendReport();
}

void Mouse_::press(uint8_t b) {
    _mouseReport.buttons |= b;
    _mouseReport.x = 0;
    _mouseReport.y = 0;
    _mouseReport.wheel = 0;
    sendReport();
}

void Mouse_::release(uint8_t b) {
    _mouseReport.buttons &= ~b;
    _mouseReport.x = 0;
    _mouseReport.y = 0;
    _mouseReport.wheel = 0;
    sendReport();
}

bool Mouse_::isPressed(uint8_t b) {
    return (_mouseReport.buttons & b) != 0;
}

bool Mouse_::getReport(MouseReport_t* report) {
    if (_dirty) {
        memcpy(report, &_mouseReport, sizeof(MouseReport_t));
        // Reset relative movements after sending
        _mouseReport.x = 0;
        _mouseReport.y = 0;
        _mouseReport.wheel = 0;
        _dirty = false;
        return true;
    }
    return false;
}

// --- Interrupt request handler integration ---

extern "C" {
void usb_handle_user_in_request(struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist) {
    if (endp == 1) { // Mouse EP
        MouseReport_t report;
        if (Mouse.getReport(&report)) {
            usb_send_data(&report, sizeof(MouseReport_t), 0, sendtok);
            return;
        }
    } else if (endp == 2) { // Keyboard EP
        KeyboardReport_t report;
        if (Keyboard.getReport(&report)) {
            usb_send_data(&report, sizeof(KeyboardReport_t), 0, sendtok);
            return;
        }
    }
    usb_send_empty(sendtok);
}
}

#endif // CH32V003_RV003USB

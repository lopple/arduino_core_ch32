#ifndef _USBHID_H
#define _USBHID_H

#if defined(CH32V003_RV003USB)

#include "Arduino.h"
#include "Print.h"

// Modifier keys
#define KEY_LEFT_CTRL   0x80
#define KEY_LEFT_SHIFT  0x81
#define KEY_LEFT_ALT    0x82
#define KEY_LEFT_GUI    0x83
#define KEY_RIGHT_CTRL  0x84
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT   0x86
#define KEY_RIGHT_GUI   0x87

#define KEY_UP_ARROW    0xDA
#define KEY_DOWN_ARROW  0xD9
#define KEY_LEFT_ARROW  0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_BACKSPACE   0xB2
#define KEY_TAB         0xB3
#define KEY_RETURN      0xB0
#define KEY_ESC         0xB1
#define KEY_INSERT      0xD1
#define KEY_DELETE      0xD4
#define KEY_PAGE_UP     0xD3
#define KEY_PAGE_DOWN   0xD6
#define KEY_HOME        0xD2
#define KEY_END         0xD5
#define KEY_CAPS_LOCK   0xC1
#define KEY_F1          0xC2
#define KEY_F2          0xC3
#define KEY_F3          0xC4
#define KEY_F4          0xC5
#define KEY_F5          0xC6
#define KEY_F6          0xC7
#define KEY_F7          0xC8
#define KEY_F8          0xC9
#define KEY_F9          0xCA
#define KEY_F10         0xCB
#define KEY_F11         0xCC
#define KEY_F12         0xCD
#define KEY_F13         0xF0
#define KEY_F14         0xF1
#define KEY_F15         0xF2
#define KEY_F16         0xF3
#define KEY_F17         0xF4
#define KEY_F18         0xF5
#define KEY_F19         0xF6
#define KEY_F20         0xF7
#define KEY_F21         0xF8
#define KEY_F22         0xF9
#define KEY_F23         0xFA
#define KEY_F24         0xFB

// Keyboard Report
typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__((packed)) KeyboardReport_t;

// Mouse Report
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
} __attribute__((packed)) MouseReport_t;

class Keyboard_ : public Print {
private:
    KeyboardReport_t _keyReport;
    bool _dirty;
    void sendReport();
    
    static size_t write_callback(void* ctx, uint8_t c) {
        return ((Keyboard_*)ctx)->write_char(c);
    }
    size_t write_char(uint8_t c);

public:
#if defined(CORE_LIGHTWEIGHT_PRINT)
    Keyboard_() : Print(write_callback, this) { begin(); }
#else
    Keyboard_() : Print() { begin(); }
    size_t write(uint8_t k) override { return write_char(k); }
#endif

    void begin();
    void end();
    size_t press(uint8_t k);
    size_t release(uint8_t k);
    void releaseAll();
    
    // Internal API for USB stack
    bool getReport(KeyboardReport_t* report);
};

#define MOUSE_LEFT    0x01
#define MOUSE_RIGHT   0x02
#define MOUSE_MIDDLE  0x04
#define MOUSE_ALL     (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

class Mouse_ {
private:
    MouseReport_t _mouseReport;
    bool _dirty;
    void sendReport();
public:
    Mouse_();
    void begin();
    void end();
    void click(uint8_t b = MOUSE_LEFT);
    void move(int8_t x, int8_t y, int8_t wheel = 0);
    void press(uint8_t b = MOUSE_LEFT);
    void release(uint8_t b = MOUSE_LEFT);
    bool isPressed(uint8_t b = MOUSE_LEFT);
    
    // Internal API for USB stack
    bool getReport(MouseReport_t* report);
};

extern Keyboard_ Keyboard;
extern Mouse_ Mouse;

#endif // CH32V003_RV003USB
#endif // _USBHID_H

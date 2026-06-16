#ifndef _RV003USB_USB_KEYBOARD_US_H
#define _RV003USB_USB_KEYBOARD_US_H

#if defined(RV003USB_KEYBOARD_LAYOUT_SELECTED)
#error "Include only one RV003USB keyboard layout header."
#endif
#define RV003USB_KEYBOARD_LAYOUT_SELECTED 1

#include "USBKeyboard.h"

// Keep the Keyboard object lazy so USBHID.begin()-only sketches do not link
// the US ASCII key map or keyboard report code.
extern Keyboard_ &rv003usbKeyboardUS();
extern Keyboard_ &rv003usbKeyboard();

#define Keyboard rv003usbKeyboardUS()

#endif // _RV003USB_USB_KEYBOARD_US_H

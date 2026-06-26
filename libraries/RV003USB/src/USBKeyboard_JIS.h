#ifndef _RV003USB_USB_KEYBOARD_JIS_H
#define _RV003USB_USB_KEYBOARD_JIS_H

#if defined(RV003USB_KEYBOARD_LAYOUT_SELECTED)
#error "Include only one RV003USB keyboard layout header."
#endif
#define RV003USB_KEYBOARD_LAYOUT_SELECTED 1

#include "USBKeyboard.h"

// Keep the Keyboard object lazy so USBHID.begin()-only sketches do not link
// the JIS ASCII key map or keyboard report code.
extern Keyboard_ &rv003usbKeyboardJIS();

#define Keyboard rv003usbKeyboardJIS()

#endif // _RV003USB_USB_KEYBOARD_JIS_H

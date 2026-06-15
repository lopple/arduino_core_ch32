# RV003USB TODO

## Keyboard layout selection

`Keyboard.write()` and `Keyboard.print()` currently use a US keyboard layout
for ASCII-to-HID key mapping. The actual output still depends on the host OS
keyboard layout, so punctuation may differ on JIS keyboards.

Keyboard support is intentionally exposed through an explicit layout header:

```cpp
#include <USBHID.h>
#include <Keyboard_US.h>
```

Consider adding `Keyboard_JIS.h` in a future change. The goal is for sketches
to include exactly one keyboard layout header, making the selected layout
explicit and avoiding multiple ASCII maps in flash.

Do not attempt automatic layout detection in the device firmware. A normal USB
HID keyboard cannot reliably query the host OS keyboard layout.

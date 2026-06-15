# RV003USB TODO

## Keyboard layout selection

`Keyboard.write()` and `Keyboard.print()` use the explicitly included layout
header for ASCII-to-HID key mapping. The actual output still depends on the
host OS keyboard layout, so the sketch should include a header that matches
the host setting.

Keyboard support is intentionally exposed through an explicit layout header:

```cpp
#include <USBHID.h>
#include <USBKeyboard_US.h>
```

Use `USBKeyboard_JIS.h` instead for Japanese OADG/JIS host layouts. Sketches
should include exactly one keyboard layout header, making the selected layout
explicit and avoiding multiple ASCII maps in flash.

Do not attempt automatic layout detection in the device firmware. A normal USB
HID keyboard cannot reliably query the host OS keyboard layout.

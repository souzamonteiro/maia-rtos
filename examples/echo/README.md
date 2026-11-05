# MaiaRTOS Tab5 LCD + keyboard smoke test

`main.c` is a C99, RTOS-independent terminal echo test. It is **not a standalone flashable firmware**: it deliberately uses six small adapter functions, to be implemented in `tab5_port.c` against the board's already-working LCD and keyboard drivers.

Required functions:

- `tab5_display_init`: initialize board-specific MIPI-DSI host, panel, reset, backlight, framebuffer and DMA/cache handling. Return 0 on success.
- `tab5_keyboard_init`: initialize the physical keyboard transport (e.g. USB HID host if that is the connected keyboard). Return 0 on success.
- `tab5_fill_rect`: clip and paint RGB565 rectangle, ensuring the change is visible.
- `tab5_draw_text`: render ASCII text and ensure it becomes visible (flush if necessary).
- `tab5_keyboard_poll_ascii`: nonblocking ASCII key poll; -1 when no key. Convert HID keycodes to ASCII in adapter, not main.c.
- `tab5_log`: serial diagnostic messages, or no-op.

Expected sequence: LCD test header appears immediately. If not, stop debugging the shell/keyboard and investigate LCD panel revision, DSI initialization, reset, backlight, framebuffer address and cache flushing. If header appears but typing doesn't, debug keyboard transport and HID-to-ASCII conversion.

Build `main.c` together with your existing platform sources and `tab5_port.c` in your own MaiaRTOS build. Do not link with Arduino, FreeRTOS, or LVGL. Do not invent register addresses or silently stub these functions as successful.

**Hardware caveat:** Tab5 revisions use ILI9881C, ST7123 or ST7121 panels with different initialization requirements. The panel and transport must be identified before implementing a bare-metal driver.

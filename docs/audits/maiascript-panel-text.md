# MaiaScript panel line breaks — 2026-11-10

## Defect and correction

SciCalc and Alarm draw their lower information panels with the MaiaScript
`draw_text` native binding, not LVGL labels. Their bundled strings already
contain newline bytes, but `draw_text_px` replaced every control character,
including newline, with a space. The entire panel was drawn on one line.

The shared renderer now resets the horizontal position to the original `x`
and advances by the existing 32-pixel character height on newline. No app,
compiler, font, layout, runtime dependency or module binary was changed.

## Host evidence

Command: `python3 tests/host/test_maiascript_text.py` (macOS, system C++ compiler).
The test compiles the actual target rasterizer with a pixel-recording sink
and uses the actual strings in the bundled `.mobj` files. It checks the
complete pixel positions, the original left margin on every line, panel
bounds, blank lines, empty strings and null input.

Before the fix: `FAIL: scicalc`. After the fix:
`PASS: panel pixel bounds, line origins, blank/empty/null text`.

SciCalc: three lines, maximum width 544 pixels; available width from text
origin to right border 674 pixels. Alarm: four lines, maximum width 576
pixels; available width 654 pixels. Both fit vertically as well.

## Target evidence

Environment: connected Tab5, `/dev/cu.usbmodem1301`, existing working ESP-IDF
5.4 firmware baseline restored at the user's request. No new runtime
dependency was introduced. Commands from `platforms/tab5`:

```sh
source /Users/roberto/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem1301 flash
```

Build and flash succeeded. Existing configuration/initializer/deprecation
warnings remain outside this change. Firmware SHA-256:
`21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd`.
Local logs: `/tmp/maia-visual-build.log`, `/tmp/maia-visual-flash.log`.

From the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python \
  tests/hardware/test_maia_apps.py --log /tmp/maia-visual-hardware.log
```

Result: PASS. Both apps opened via desktop shortcuts. Serial output confirmed
`2+3*4 = 14`, `2.5^2 = 6.25`, `9^0.5 = 3`, `sin(0) = 0` and a division-by-zero
domain error. The ten-second timer fired, a second timer was cancelled and
did not fire during the subsequent twelve seconds, and the test returned
to the desktop. The raw serial capture is `/tmp/maia-visual-hardware.log`.

## Scope and limitations

This implements explicit newlines, not automatic wrapping. The current
`draw_text` API has no width argument. Future longer or translated strings
need deliberate line breaks or a separate bounded-text API. Serial tests
can verify app operation but cannot verify physical display appearance or
audible output. Final appearance on the panel requires visual confirmation.

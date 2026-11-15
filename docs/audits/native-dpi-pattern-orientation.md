# Pattern orientation correction — 2026-11-14

The owner clarified that the stable bars observed during the native test were
**horizontal**. Earlier questions/descriptions said vertical because the SDK calls
VPG_ORIENTATION24=0 `BAR_VERTICAL`. That name does not establish the orientation
seen by a person holding the Tab5. Preserve the original UART/image evidence.

Production source at commit58e8f47, `platforms/tab5/main/hal/hal_esp32.cpp:104`,
explicitly selects `LV_DISPLAY_ROTATION_90` with `sw_rotate=true`. The physical
panel geometry remains720x1280 and the logical desktop is1280x720. The native
host pattern bypasses LVGL/software rotation. No rotation implementation was
added by the pattern diagnostic.

Future native framebuffer presentation must preserve the production orientation,
including input-coordinate mapping. Determine physical direction using a labeled,
asymmetric pattern rather than changing panel timings or assuming the enum name
means the observed direction. This corrects the earlier statement that the
owner confirmed vertical bars; the confirmed visible orientation is horizontal.

Documentation-only change: audit, observation, manifest clarification, evidence,
open questions, status and handoff. No firmware/register/parser changes, no new
hardware test. Validation: checked production rotation call and preserved hardware
log VPG word0001BF02, compared documentation diff and whitespace. Original recovery
manifest/source hashes remain an immutable pre-correction snapshot. This correction
is saved separately in the recovery directory; firmware/image hashes unchanged.
Next task remains guarded bridge/DPI latch, then native framebuffer presentation.

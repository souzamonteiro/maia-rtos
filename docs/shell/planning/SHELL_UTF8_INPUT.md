# UTF-8 canonical input editing

> Follow-up native dead-key capture is implemented in
> [SHELL_DEAD_KEY_INPUT.md](SHELL_DEAD_KEY_INPUT.md). Composition exclusions below
> describe this initial input-only slice, not the current browser transport.

> Subsequent user-visible glyph correction:
> [SHELL_ACCENT_GLYPHS.md](SHELL_ACCENT_GLYPHS.md). The original font exclusions
> below describe the input-only slice, not the corrected platform label font.

2026-10-23. Owner requested continued shell development after confirming wc
options in the simulator. Next bounded improvement: Unicode input in the existing
I/O guest, whose key event already defines data0 as a codepoint. Status: implemented and verified locally. No new ABI
signature, event layout, capability, process lifecycle or component boundary.

I/O encodes accepted Unicode scalar values as UTF-8 in the canonical line.
Reject C0/DEL/C1 controls, surrogate values and values above U+10FFFF. Keep 255
editable bytes plus a terminating NUL, 256 canonical bytes including LF, existing
history/display limits and stream backpressure. Insert each scalar atomically:
if all its bytes do not fit, ignore the key without changing the line or cursor.
Left/right, Backspace and Delete traverse complete UTF-8 codepoints; Home/End,
one-line history, queued keys under backpressure and EOF retain current behavior.
This is codepoint editing, not grapheme-cluster navigation or normalization.

The browser keyboard adapter forwards a single Unicode scalar in event.key using
the existing event import. Supplementary characters may occupy two UTF-16 units.
Keep focus, shortcuts, queue errors and ordinary navigation unchanged. Ignore
composition-in-progress keys; this step does not implement IME/paste/beforeinput
or guarantee every character can be produced by every platform keyboard.
Real I/O/LVGL continues to own editing, caret and rendering. Host supplies only
key packets; no shell parsing, byte editing or text rendering moves to JavaScript.

Modify only apps/io/io.cpp, its artifact, keyboard adapter and focused tests/docs.
No grammar/generated parser, compiler/dependency, kernel, worker, shell execution,
font set, npm/version or commit/push changes. Glyph availability remains a font
concern; exact UTF-8 transport does not demonstrate CJK/emoji glyph coverage.

Check: strict MaiaCpp C++98 -> C89 -> MaiaC/WASM I/O build; actual I/O WASM key
events/editing snapshots and canonical bytes over tiny rings; 1/2/3/4-byte scalar
boundaries, controls/surrogates/out-of-range rejection, byte limits/atomic insertion,
history, UTF-8 validity, prompt/caret and stable memory/cleanup. Browser adapter
unit tests for focus/modifiers/composition/scalars. Actual production kernel/
shell/I/O/LVGL commands using directly typed accented/quoted paths and wc -c;
existing unit, boot, stream and LVGL regressions. Browser interaction, physical
device and MaiaWASMVM validation are separate checks.

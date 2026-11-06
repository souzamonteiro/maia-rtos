# Native dead-key text capture

Status: implemented and verified locally with event fixtures and actual guest
WASM integration. Physical macOS dead-key/browser confirmation remains pending;
the in-app browser is unavailable in this session (browser list is empty).

2026-10-23. User confirmed cedilla displays after the font fix but tilde then a
does not produce ã. Existing code ignores composing key events and has no native
text commit listener. macOS dead keys use composition; canvas keyboard events
alone do not supply a reliable committed text stream. References:
https://www.w3.org/TR/uievents/#keys-dead and
https://github.com/w3c/uievents/issues/343 .

Correct browser input transport with a focusable invisible text receiver inside
the device display wrapper. It lets the browser/OS compose characters, then
forwards committed scalars through existing maia.events KEY packets. Its value
is cleared after each commit; it never contains/edits the guest command line.
The visible display/caret/history, codepoint editing, UTF-8 encoding and parsing
remain in I/O/shell WASM and real LVGL. No DOM terminal, handwritten composition
table, native keycode/layout assumption, normalization or new guest ABI.

Canvas focus redirects to the receiver only while a keyboard-capable process is
active. Navigation and Enter outside composition retain existing guest events;
composing Enter does not submit. Ordinary text uses native input, avoiding a
duplicate keydown path. Ignore preedit updates; forward compositionend once,
deduplicate its corresponding final input notification in either order. Handle
cancel/focus loss without forwarding stale preedit into another process.

Bound native receiver and pending scalar transport to 256 UTF-16 units/scalars.
Reuse existing per-process event FIFO and WOULD_BLOCK: retain unsent committed
scalars, retry bounded batches on animation frames, discard stopped/replaced
owner data, report permanent failures/overflow. No guest scheduling policy in
this retry queue. Keep shortcuts, focus outside display and controls intact;
paste/drop/cut and full IME candidate UI remain out of scope.

Change browser platform capture/routing, focused tests and docs only; no guest,
font, compiler/dependency or syscall/contract changes. Tests: native event-order
fixtures for tilde+a/acute/decomposed commits, cancellation and deduplication,
focus/owner isolation, queue backpressure and bounded overflow; actual browser
adapter -> I/O/shell/kernel/LVGL WASM integration, exact command/file bytes and
existing regressions. Synthetic event fixtures do not establish physical macOS
keyboard/browser conformance; user/browser interactive confirmation remains a
separate gate. Preserve all local work; no commit/push/npm/version changes.

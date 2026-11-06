# Pipeline file input — approved implementation contract

Date: 2026-10-23. Status: owner approved the exact contract and implementation on 2026-10-23;
implemented and validated locally through native C++98, ASan/UBSan and actual
WASM/LVGL reference tests. Interactive browser validation remains pending.
Baseline main b13f168 plus the locally validated wc, Regex pins and
simple-command input slice. No publication is included in this proposal.

## Scope and architectural reason

Support `cat < file | cat`, `cat < file | wc` and two-to-four-stage variants,
optionally ending in `> output` or `>> output`. Require the first command to
be operand-free cat when `<` is present; later stages retain existing cat/file
operand rules and optional final operand-free wc. One input redirect must belong
to the first SimpleCommand and one optional output redirect to the last.
Unsupported positions, repeated/numbered redirects, descriptor duplication,
heredocs, standalone wc and general execution remain rejected before effects.

Existing simple-command `<` reads a shell-owned VFS handle. Existing pipelines
give the first worker an EOF-only stream and require first-stage file operands.
Simply translating `< file` into a cat argument would defer the open to a child,
changing missing-input/output-truncation order and ownership. Do not use that
shortcut. Instead, the shell feeds its opened input through a kernel-owned pipe.
The kernel remains responsible for resources, runnable order, cancellation and
reaping. The host only executes existing PIPE/RELEASE actions and copies bytes;
it does not read file contents for the shell or interpret syntax. I/O retains
the real LVGL terminal and keyboard. Workers remain unchanged and separate WASM
instances; cat already reads stream 0 when argv is exactly ["cat"].

## Proposed compatible plan version 3

Keep existing import/export signatures and capabilities. Add plan version 3 to
the copied binary argument accepted by pipeline_start. Versions 1 and 2 retain
their exact validation, layout and behavior; ordinary/output-only pipelines
continue emitting v1/v2. Worker argv and completion packets remain version 1.

All fields are little-endian u32:

| Offset | Meaning | Version 3 constraint |
| --- | --- | --- |
| 0 | version | 3 |
| 4 | stageCount | 2..4 |
| 8 | wordCount | stageCount..12; argv only |
| 12 | stringBytes | Exact packed argv byte count |
| 16 | streamFlags | 2 = input from owner; 3 = input and output via owner |
| 20 | reserved | 0 |
| 24 | stages, then words, then strings | Existing packed pair tables |

Bit 0 means last stdout returns to owner stream 3; bit 1 means first stdin
receives owner stream 4. Input is mandatory in v3; flags 0/1 and unknown bits
are invalid. Table base is selected by version, independent of output flag.
First argv must be exactly ["cat"]. Later command/name/option checks, packed
UTF-8/no NUL, <=95-byte words, exact contiguous ranges and <=1536-byte packet
bounds remain enforced independently by the kernel before any reservation.
V1/v2 retain their first-cat-with-files and stageCount+1 minimum word rules.

The shell retains its twelve-expanded-word limit INCLUDING both redirect
targets. File targets stay in shell memory and never enter worker argv/plans.
Existing shell identity, process.pipeline, VFS/stream permission checks and
wc's stream-only package profile remain. V3 output requires vfs.write as v2 does;
input uses the existing owner vfs.open/read/close authority. No new capability,
arbitrary descriptor selector, package index or cross-guest handle transfer.

## Binding, scheduling and cleanup

Keep owner terminal streams 0/1/2. Reserve owner stream 4 as a writer to the first
child's stream 0 via existing PIPE action kind 1: reader=(first child,0),
writer=(owner,4), capacity <=4096. It replaces EOF_INPUT only for v3 input.
Optional final output uses the existing pipe to owner stream 3. Other edges and
stderr delegation are unchanged. At most one unreaped group per owner remains.
The shell preflights unused handle 4 with a zero-length stream.write and unused
handle 3, if requested, with zero-length stream.read: only -7 means unbound.
An endpoint of either direction is a collision. Reject before opening files;
kernel binding failure must never close the unrelated colliding endpoint.

Track successfully bound owner input/output endpoints independently, including
PIPE acknowledgement after in-flight cancellation. Keep owner blocked during
startup; after all bindings/starts succeed, schedule it alongside workers for
both input-only and combined input/output plans. No guest runs early.

Use fixed, separate input-feed and output-drain buffers. Each owner turn makes
bounded attempts in both directions; WOULD_BLOCK on input must not skip output
drain, and WOULD_BLOCK on output read must not skip input feed. Retain partially
written bytes and do not reread the input until that chunk is accepted. Yield
under backpressure; never wait for group completion before pumping its input.
On file EOF, close input and stream 4 to signal EOF, then continue draining/waiting.
Report an input close error and cancel rather than presenting successful input.

Normal completion must wait for acknowledged RELEASE of every successfully
acquired owner endpoint (3 and/or 4), even if already closed by the shell. A too
small completion buffer initiates no cleanup and does not reap. Failed release
acknowledgements retain ownership and retry. Rollback/trap/cancel/owner exit must
release both owner endpoints and all children before completion/reuse. Repeated
cancel and late START/binding acknowledgements retain existing idempotent rules.
EOF is not group completion; prompt follows wait/reap and file closure.

For input-only plans the owner also polls the existing nonblocking wait after
each bounded feed attempt. This observes rollback even when VFS read continues
returning WOULD_BLOCK; completion before input EOF closes the file and reports
early input termination. Plans with output never wait/reap before output drain,
because that would discard buffered stdout. This uses the existing wait contract.

## Expansion, opens and failure effects

Use only the generated shell AST and existing quote-preserving MaiaGlob stage.
Validate all syntax, placements, commands, argv/path lengths, target cardinality,
plan size and local endpoint availability before opening targets. Then open
redirects in AST source order, using existing VFS flags. A missing/denied first
input prevents a later output from being created/truncated. No children launch
if any required file open fails; close any successfully opened earlier handle.
Opening a shared source followed by truncating that same path yields empty input,
consistent with existing shared backing records; no snapshot semantics are added.
Append reads/writes retain current live-file semantics and existing store quotas.

A reservation/start failure after opens can leave output created/truncated.
Input read error, feed transport error, output drain/write failure or unexpected
closed peer reports terminal stderr, stops feeding, closes local endpoints/files,
cancels the owned group and waits for kernel cleanup. Rollback may discard
buffered bytes; only bytes accepted by VFS are retained. No filesystem transaction
or new durability guarantee. A rolled-back missing endpoint is an error/cleanup
state, not successful input EOF. Do not consume subsequent terminal commands as
pipeline data. Failure must leave the shell usable for the next command.

## Files and required verification

Expected changes: kernel processes.hpp; shell.cpp/shell-glob.hpp/shell-pipeline.hpp;
plan encoder runtime/maia-kernel-processes.js; focused tests; ABI/user/handoff
documentation; reproducibly rebuilt kernel.wasm and shell.wasm. Existing generic
host action transport should need no change; any discovered interface extension
outside this exact proposal requires review. No compiler/dependency/worker/I/O
source, npm, version or publication change is proposed.

Acceptance before claiming completion:

- Native C++98 kernel ASan/UBSan and actual kernel WASM: v1/v2 compatibility,
  v3 flags/layout/first-stage checks, malformed/truncated packets, authorization,
  quotas, owner scheduling, input/output binding acknowledgements, collision-safe
  rollback, foreign cancel/wait, short wait, retry release, cancellation at every
  startup action including in-flight starts, trap/owner exit and quota reuse.
- Actual production boot/scheduler under Node with kernel/shell/cat/wc/I/O/LVGL:
  two/four stages; text/empty/binary/128KiB input; relative/private/quoted/Unicode/
  glob targets; combined output/append; exact byte/count results through <=31-byte
  pipes; independent delays in both directions proving no pump deadlock.
- No open/launch for invalid placement/argv/ambiguous targets or occupied owner
  endpoints; missing/denied input leaves output intact. Exercise read/close/write
  failures, partial transfers, blocked operations, failed launch, trap/cancel/
  owner close, subsequent terminal commands, repeated groups and stable counts.
- Strict build:kernel and build:shell --check-c89; npm test; existing kernel,
  workers, wc, redirection, files/glob/directories, boot/LVGL regressions; exact
  generated JS/native C++98/WASM frontend equivalence after AST adapter changes.
- Interactive simulator checks when its browser backend is available. Node/LVGL
  integration alone makes no browser, device or MaiaWASMVM claim.

Approval is required by AGENTS.md section 4.6 before implementing the plan format
and execution-model extension. The owner supplied that explicit approval on 2026-10-23. Publication remains excluded.

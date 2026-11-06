# Bounded ls visibility options

Status: implemented and validated locally. Strict guest build produces a
1276388-byte shell.wasm with unchanged 18-page initial memory; generated parser
and grammar remain unchanged. Directory/glob/redirection/LVGL integration and
existing unit, boot, stream and pipeline regressions pass. Interactive browser,
MaiaWASMVM and physical-device conformance are separate checks.

2026-10-23. Continue the shell using existing guest command dispatch and VFS.
Add leading grouped/repeated -a and -A flags, and -- to end option processing.
Stop processing options at the first operand; '-' alone is an operand. -a takes
precedence over -A when combined. Unknown options fail with usage before directory
enumeration (existing simple-command redirects still open before builtin usage).

Intentional change from the initial directory slice: default ls omits child
names beginning with '.'. -A includes actual hidden children. -a also prefixes
each successfully opened directory listing with ./ and ../. These two display
markers are synthesized in the guest after authorization; they do not enumerate
the parent or change the VFS snapshot contract. Actual children retain sorted
UTF-8 byte order. Explicitly named hidden files/directories remain accessible
without flags. No special root-user behavior. No full POSIX-ls claim.

Options apply to all operands, with headings only for multiple operands (flags
are not operands). No operands defaults to cwd. -- permits literal dash-prefixed
names; quoting alone does not disable option interpretation. Existing MaiaGlob
expansion precedes command option handling and retains its limits/hidden rules.
Validate complete directory packets before hiding entries; skipped entries still
consume bounded scheduler work. Escaping, namespace isolation, handle cleanup,
redirection and stream backpressure remain. Reset options for every command.

Modify shell.cpp, rebuild shell.wasm through MaiaCpp AST-strict -> C89 ->
MaiaC/MaiaWASM, add actual guest option/visibility/glob/stream/LVGL checks and docs.
No public host ABI, capability, execution model, parser/grammar, VFS policy,
dependency/compiler, version/npm or commit/push changes. Preserve all local work.
Verify empty/default/all views, grouped flags/precedence/--, explicit hidden and
dash-prefixed operands, invalid options, multiple headings, hidden malformed
packets, large hidden directories, privacy, redirection and existing regressions.

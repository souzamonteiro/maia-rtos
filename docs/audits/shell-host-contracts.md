# Host contract build restored

The root host build now compiles and passes all five CTest entries. This
repairs the test/adapter wiring against the existing `msh_host_v1` header;
it does not change that header, the ABI layout, the lexer/parser or its EBNF.

## Changes

- Added a separate, pure C/C++98 validation helper. It checks the known prefix,
  major version and every required callback before accepting a host table.
  A newer minor version and additional trailing fields are accepted. Invalid
  tables return the existing `MSH_ERR_INVALID`; no new public error numbers.
- Updated the console diagnostic adapter and fixtures to current native v1
  names/flags/errors. Old richer errors, invalid-ID constants and the glob sink
  type are not part of the current header and were not reintroduced as aliases.
  The fake backend owns its sentinel and opaque sink protocol locally. The
  current v1 table does not define a would-block status; the test instead checks
  propagation of its declared I/O error. This does not certify async I/O.
- The full C++ shell regression now links the real host implementations of
  assembler, compiler, loader, regex and glob, restricted to the hosted CMake
  branch. Removed obsolete ESP/M5 mocks. Added the missing POSIX declaration
  include and selected C++17 for these existing tool sources.
- Quoted `(`, `)` and `*` in the `expr` fixture so they reach `expr` as operands
  rather than shell grouping/glob syntax. All ten expression/control assertions
  remain, including division by zero, substitution, loops and branching.

The console adapter remains the existing limited diagnostic console. No new
shell syntax or replacement parser was implemented. The public host header
is byte-for-byte unchanged from checkpoint `f543344`.

## Verification

Environment: local macOS host, CMake and AppleClang. Commands from repository root:

```sh
cmake -S . -B /tmp/maia-host-contracts-20261110 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
```

Result: **5/5 PASS**: metadata, host ABI/fixture, register constants, kernel/MVP,
and full-shell expression/control tests. The latter runs its ten assertions.

The host ABI and kernel/MVP tests also pass under AddressSanitizer and UBSan:

```sh
cmake -S . -B /tmp/maia-host-contracts-sanitized-20261110 \
  -DMAIA_BUILD_HOST_TESTS=ON \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-host-contracts-sanitized-20261110 \
  --target shell_host_contract kernel_mvp_test -j4
ctest --test-dir /tmp/maia-host-contracts-sanitized-20261110 \
  -R '^(shell_host_contract|kernel_mvp_test)$' --output-on-failure
```

Result: **2/2 PASS**. Negative cases include null/truncated tables, wrong major
version, every missing callback, invalid console binding, I/O failures and
invalid handles; extended/minor-version tables are accepted.

The new validation header and MVP header compile with
`c++ -std=c++98 -Wall -Wextra -Wpedantic -Werror -I include -fsyntax-only`.
The standard Tab5 firmware also builds with ESP-IDF and the default two-core
profile. No firmware was flashed and no device interaction was needed here.
Logs: `/tmp/maia-host-contracts-*.log`, copied into the recovery checkpoint's
`experiments/host-contracts/` directory.

## Limits and next step

These are host tests, not proof of real VFS/process/glob implementations or
hardware preemption/SMP. The hosted tool library must not enter the independent
kernel build. The separate full native shell target is still incomplete;
restoring the hosted build does not repair its freestanding dependencies.

Next: build a separate native scheduler diagnostic that cannot fabricate ticks
or call cooperative yields. Use it to verify actual timer IRQ/context switching,
preserving the complete working desktop and its flash image.

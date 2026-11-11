# CST ownership and the on-device compiler fault — 2026-11-10

## Result

The 32-addition program that previously crashed the on-device compiler now
compiles, validates as MNM-1, and executes with exit code **32** on the Tab5
one-core experimental profile. Desktop/SciCalc/Alarm regression tests pass.
Both firmware profiles build, and the host compiler passes 32/32 cases plus
the new lifetime test. The preserved original application was restored after
testing; this is not promotion of the experimental profile.

## Evidence and diagnosis limits

The earlier [stage-one report](tab5-singlecore-stage-1-2026-11-10.md) records the
reproducer failing in both the one-core experiment and the preserved original
firmware. The fault passed through pthread mutex destruction/FreeRTOS semaphore
operations while releasing temporary CST nodes.

Preprocessing the actual target compiler commands showed that libstdc++ selects
`__gnu_cxx::__default_lock_policy = _S_mutex` for both parser and supervisor.
Each `make_shared<CSTNode>` therefore creates a mutex-bearing control block.
The local GCC 14.2.0 `ext/concurrence.h` constructor invokes pthread mutex init
without checking its result; the ESP-IDF implementation can return allocation
errors. Its mutex creates a FreeRTOS semaphore, whose allocator requests
`MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`.

Relevant primary implementations, inspected in the local SDK/toolchain:

- [ESP-IDF v5.4.2 pthread mutex implementation](https://github.com/espressif/esp-idf/blob/v5.4.2/components/pthread/pthread.c), `pthread_mutex_init` and `pthread_mutex_destroy`.
- [ESP-IDF v5.4.2 FreeRTOS allocator](https://github.com/espressif/esp-idf/blob/v5.4.2/components/freertos/heap_idf.c), `portFREERTOS_HEAP_CAPS` and `pvPortMalloc`.
- Local GCC 14.2.0 `include/c++/14.2.0/ext/concurrence.h` and
  `bits/shared_ptr_base.h`; actual preprocessor output is archived with the test.

This establishes an unnecessary dependency and a plausible internal-memory
exhaustion path. Failed-allocation counters were not captured, so exhaustion
itself is not claimed as directly measured. Removing the CST control blocks
resolves the recorded reproducer without changing the runtime mutex code.
The ELF had seven symbols for shared CST control blocks before this change
and none afterwards. AST shared control blocks remain: this is not complete
removal of pthread/FreeRTOS dependencies from the compiler or firmware.

## Change and invariants

Only the private CST ownership model in `maiascript/src/parser.cpp` changed.
`CSTStorage` owns every node via `unique_ptr`. Tree edges and converter
arguments borrow pointers. Storage lives through CST construction and the
entire conversion to AST, then releases all nodes once, without recursive
shared-control-block destruction or per-CST-node mutexes.

AST strings/values retain their own data, and the existing public AST and
`Parser` signatures retain their `shared_ptr` types. Grammar, generated parser,
AST semantics, module ABI and code generator were not modified. The storage
belongs to each parse invocation; it is not global.

`test_parser_lifetime.cpp` keeps an AST alive across eight large parses and
syntax errors, checking its long identifier, heap-backed literal and return
value afterwards. It also checks statement counts in the returned large AST.
The normal `make test` now runs this test. Its generated executable is ignored;
already-versioned compiler binaries were rebuilt through the normal Makefile.

## Checks actually run

```sh
# From maiascript/: builds both host tools and runs the full suite + lifetime case.
make -j4 all test

# From the repository root: instrument the real parser and regex implementation.
c++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Imaiascript/include -Ilib/maiaregex \
  maiascript/tests/test_parser_lifetime.cpp maiascript/src/parser.cpp \
  -x c++ lib/maiaregex/maia_regex.c -o /tmp/maia-parser-lifetime
/tmp/maia-parser-lifetime
```

Results: 32/32 host compiler cases, lifetime/negative syntax/recovery test PASS;
AddressSanitizer and UBSan run PASS, with no reported errors. This is not a
certification of behavior on allocation failure.

From `platforms/tab5`, with `/Users/roberto/esp-idf/export.sh` sourced:

```sh
idf.py -B /tmp/maia-tab5-singlecore-20261110 -D MAIA_TAB5_SINGLECORE_DRIVERS=ON build
idf.py -D MAIA_TAB5_SINGLECORE_DRIVERS=OFF build
idf.py -B /tmp/maia-tab5-singlecore-20261110 -D MAIA_TAB5_SINGLECORE_DRIVERS=ON \
  -p /dev/cu.usbmodem1301 app-flash
```

Both builds and experimental app-only flash PASS. Then, using the Espressif
Python environment and the existing `maia.py` client:

```sh
python tests/hardware/test_tab5_workers.py --boot-wait 20 --log /tmp/maia-cst-workers.log
python tests/hardware/test_maia_apps.py --log /tmp/maia-cst-apps.log
```

Hardware results: background job completes; uploaded source verified chunk by
chunk; compilation emits a valid 680-byte ELF32 module; execution returns 32;
temporary files removed; desktop restored. The client observed 2.31 seconds
for the compilation transaction, a single measurement rather than a bound.
SciCalc arithmetic/powers/domain error and Alarm fire/cancel regressions PASS.
Serial tests do not certify physical pixels or audible sound.

| Built application | SHA-256 |
|---|---|
| One-core CST fix, tested on Tab5 | `d112036642ab7c0157a4cda64bb7283dd6a2f5a91918f46997fd4804818b97d8` |
| Default two-core CST fix, build tested only | `1da5fb184a4c09a07bb806b4507b89ea8a93111f0024928799942608c889b2b5` |
| Preserved application restored after testing | `21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd` |

Raw logs, target preprocessor outputs, symbols, firmware/map and source snapshot:
`../maia-rtos-recovery/bf044d3-20261110/experiments/cst-ownership-20261110/`.
Restore log: `maia-cst-restore.log`. Bootloader/partition table were not flashed.

## Remaining work

Large on-device application compilation, allocation-failure recovery and
two-core execution of this fix were not hardware-tested. Other AST/runtime
mutexes remain. Native timer/preemption, FP context, synchronization and driver
service migration remain separate requirements. The one-core FreeRTOS profile
is still experimental and does not meet R-002. The next kernel step is a
separately verified native scheduling target, preserving the working desktop.

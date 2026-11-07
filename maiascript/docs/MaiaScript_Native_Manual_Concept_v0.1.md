# MaiaScript Native
## Conceptual Language and Application Programming Manual for MaiaRTOS

**Version:** 0.1 — draft proposal, prior to EBNF formalization  
**Target:** MaiaRTOS / ESP32-P4 / RISC-V RV32 (exact ISA to be confirmed)  
**Status:** exploratory design; examples are not currently executable code  
**Manual language:** English; identifiers and API in English.

> **Note of rigor.** This document describes how MaiaScript Native *could* be. The existing `MaiaScript.ebnf` grammar was examined. Library names, commands, APIs, paths, executable formats, and ABI presented below are **proposals**, not affirmations about the current project. The desktop, LVGL as backend, and the recent separation of standalone applications were informed by the author. None of the examples should be interpreted as completed hardware tests.

---

# Part I — The Language

## 1. Objective

MaiaScript Native is a small, expressive language compiled **directly on the device** into RISC-V instructions. Its goal is not to replicate JavaScript, Rust, C++, or Python, but to allow a user to write a text file, compile it inside MaiaRTOS, and run the resulting program—including as a graphical desktop application.

Proposed principles:

1. Small compiler footprint, with no runtime LLVM or GCC dependency.
2. Static typing with optional local inference; no mandatory dynamic typing.
3. Syntax familiar to the MaiaScript/C/JavaScript family; avoiding Rust-like surface complexity.
4. Predictable memory layout for objects and methods compiled into native calls.
5. Resources managed by scope or explicit ownership; no mandatory garbage collector.
6. Versioned system APIs accessed via imports; user applications should never need raw hardware register addresses.
7. True standalone applications, without mandatory linkage to the desktop executable.
8. Explicit errors and deterministic resource reclamation even when an application fails.

## 2. Hello, MaiaRTOS

File `hello.maia`:

```javascript
import maia.console;

i32 main() {
    console.println("Hello from MaiaScript Native!");
    return 0;
}
```

**Proposed** workflow:

```text
maia> maiasc hello.maia -o hello.elf
maia> ./hello.elf
Hello from MaiaScript Native!
maia>
```

`maiasc` is the proposed native compiler. `hello.elf` represents an executable compatible with the MaiaRTOS loader; the file extension, binary format, and launch command depend on the chosen ABI.

## 3. Comments, Statements, and Scope

```javascript
// Single-line comment
/* Block comment: proposed extension to original EBNF */

i32 main() {
    i32 x = 10;
    {
        i32 x = 20;  // local shadow, permitted with optional compiler warning
        console.println(x);
    }
    return x;
}
```

Decision point: require `;` at the end of statements and expressions; code blocks delimited by `{}`. The original MaiaScript grammar does not mandate this consistently.

## 4. Fundamental Types

| Type | Meaning | Notes |
|---|---|---|
| `bool` | boolean (true/false) | distinct from integers |
| `i8`, `i16`, `i32`, `i64` | signed integers | explicit width |
| `u8`, `u16`, `u32`, `u64` | unsigned integers | explicit width |
| `f32`, `f64` | floating point | subject to target ISA/ABI; software fallback possible |
| `char` | Unicode codepoint | representation to be specified |
| `string` | UTF-8 string | explicit length, not an implicit C buffer |
| `void` | absence of return | functions without a return value |
| `T[]` | typed array/slice | semantics detailed below |
| `T*` | low-level pointer | restricted to `unsafe` modules |
| `T?` | optional value | proposed extension |
| `Result<T, E>` | success or error | special generic type or standard library type |

```javascript
import maia.console;

i32 main() {
    i32 temperature = 27;
    u32 count = 1024;
    f32 voltage = 3.3;
    bool ready = true;
    string title = "MaiaOS";
    auto next = count + 1u; // local inference, if adopted

    console.printf("%s: %d C\n", title, temperature);
    return 0;
}
```

`auto` is optional and requires strict rules. It must not introduce arbitrary implicit conversions. Integer overflow, casting, division by zero, NaN handling, and bitwise shift semantics must be formally defined before compiler implementation.

## 5. Expressions and Operators

```javascript
i32 a = 12;
i32 b = 5;
i32 sum = a + b;
i32 rem = a % b;
bool ok = a > b && b != 0;
u32 flags = 0x01 | 0x04;
i32 selected = ok ? a : b;
```

Proposed precedence (highest → lowest): member access / function call / array indexing; unary operators; multiplication / division / modulo; addition / subtraction; bit shifts; relational comparisons; equality; bitwise AND / XOR / OR; logical AND; logical OR; ternary conditional; assignment. The exponentiation operator `**`, present in the original grammar, may be exposed as `math.pow` in v1 to prevent surprising type promotion or associativity rules.

Assignment requires a valid **lvalue**; `3 = 5` is a compile error. `&&` and `||` must short-circuit.

## 6. Control Flow

```javascript
i32 classify(i32 value) {
    if (value < 0) {
        return -1;
    } elseif (value == 0) {
        return 0;
    } else {
        return 1;
    }
}

void loops() {
    for (i32 i = 0; i < 10; i += 1) {
        if (i == 3) continue;
        if (i == 8) break;
    }

    i32 n = 3;
    while (n > 0) { n -= 1; }

    do { n += 1; } while (n < 2);
}
```

The keyword `elseif` from the original MaiaScript is preserved. `switch` statements can initially be restricted to integer and enum constants.

## 7. Functions and Native Calls

```javascript
i32 add(i32 a, i32 b) {
    return a + b;
}

void greet(string who) {
    console.println("Hello, " + who);
}

i32 main() {
    return add(10, 20);
}
```

String concatenation may be a library function involving heap allocation; it must not appear deceptively cheap. External native functions must be imported explicitly through declared system modules, avoiding indiscriminate kernel symbol resolution.

## 8. Arrays, Slices, and Strings

```javascript
i32 main() {
    i32[] samples = [12, 15, 18, 20];
    i32 total = 0;
    foreach (i32 sample in samples) {
        total += sample;
    }
    console.println("Average: " + (total / samples.length));
    return 0;
}
```

**Proposed decision:** `T[N]` for fixed-size static arrays, and `T[]` for dynamic slices (pointer + length). Array literals `[...]` must be parsed distinctly from matrix constructs in the original EBNF. Bounds checking is enabled by default; unchecked indexing is permitted only inside explicit `unsafe` blocks.

Strings are UTF-8 encoded with length metadata; C ABI boundaries receive `(pointer, length)` or an explicit NUL-terminated copy. Temporary string buffers must never outlive asynchronous callbacks.

## 9. Structs: Zero-Cost Data Layout

```javascript
struct Point {
    i32 x;
    i32 y;
}

Point translate(Point p, i32 dx, i32 dy) {
    return Point { x: p.x + dx, y: p.y + dy };
}
```

A `struct` has deterministic memory layout, ABI-defined alignment/padding, and pass-by-value semantics. `Point { ... }` denotes compile-time field initialization, not a dynamic heap allocation.

## 10. Classes and Methods: Minimal Object Orientation

```javascript
class Counter {
    private i32 value = 0;

    void increment() {
        value += 1;
    }

    i32 getValue() const {
        return value;
    }
}

i32 main() {
    Counter counter = Counter();
    counter.increment();
    counter.increment();
    return counter.getValue();
}
```

**Proposed initial semantics:** a class is a statically known layout type; methods receive an implicit `this`; method dispatch is resolved statically at compile time; `private` access is verified at compile time; stack-allocated instances are destroyed upon exiting scope; no inheritance, RTTI, reflection, GC, or dynamic virtual dispatch in v1. A `const` method prevents mutating fields via `this`.

### 10.1 Composition over Inheritance

```javascript
class BatteryWidget {
    private ui.Label label;
    private i32 percent = 0;

    BatteryWidget(ui.Container parent) {
        label = ui.Label(parent, "Battery: --%");
    }

    void update(i32 next) {
        percent = next;
        label.setText("Battery: " + percent + "%");
    }
}
```

This is an illustrative example: `ui.Label` acts as an opaque widget handle rather than a raw C structure stored directly in application memory.

### 10.2 Heap Objects

```javascript
Counter* counter = new Counter();
counter.increment();
delete counter;
```

This model should only exist if an explicit dynamic heap is genuinely required. A safer alternative is `owned Counter` with automatic destruction. **Open decision:** avoid manual `new/delete` in the initial release, favoring stack-allocated objects and managed system resource handles. This manual documents both alternatives for design discussion.

## 11. Modules, Namespaces, and Libraries

```javascript
namespace mathutil {
    i32 clamp(i32 v, i32 lo, i32 hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }
}

export mathutil.clamp;
```

Usage:

```javascript
import app.mathutil;

i32 main() {
    return mathutil.clamp(120, 0, 100);
}
```

`include()` from the original grammar should be treated as compile-time source textual inclusion if retained; `import` resolves compiled binary modules and API contracts. Arbitrary runtime execution during imports is prohibited.

## 12. Errors, Options, and Exceptions

For an embedded native compiler, **errors as values** are preferred over mandatory stack unwinding:

```javascript
Result<string, FileError> readConfig(string path) {
    File f = fs.open(path, "r")?;
    defer f.close();
    return Ok(f.readAllText());
}

i32 main() {
    Result<string, FileError> r = readConfig("/config/app.ini");
    if (r.isError()) {
        console.error("Cannot read config: " + r.error());
        return 1;
    }
    console.println(r.value());
    return 0;
}
```

`?`, `Result`, `Ok`, and `defer` are **proposals** requiring explicit semantics. The `try/catch/throw` syntax of original MaiaScript could be omitted from the native v1 subset or restricted to local synchronous scopes without unwinding. Fatal process crashes must never be masked by a catch block.

## 13. Resources, Memory, and Safety

```javascript
void copyFile(string from, string to) {
    File src = fs.open(from, "rb")?;
    defer src.close();
    File dst = fs.open(to, "wb")?;
    defer dst.close();

    u8[4096] buffer;
    while (true) {
        i32 n = src.read(buffer);
        if (n == 0) break;
        if (n < 0) return; // real implementation should propagate error
        dst.writeAll(buffer[0:n])?;
    }
}
```

`defer` runs upon normal scope exit and during error propagation **if** local unwinding is implemented; it does not run during hard resets, hardware exceptions, or stack corruption. For kernel-terminated processes, OS resources must be tracked per process and cleaned up via a system handle table.

Direct MMIO access, arbitrary raw pointers, and privileged instructions should not be accessible to standard user applications. Native code on microcontrollers without hardware memory isolation can still compromise the system; API permission boundaries are helpful but do not substitute for a hardware sandbox.

---

# Part II — MaiaRTOS Programming

## 14. Standalone Application Execution Model

A MaiaScript Native application compiles into a standalone RISC-V binary independent of the desktop executable. The MaiaRTOS loader validates binary headers, target ISA, ABI version, relocations, memory requirements, and imported symbols before branching to the application entry point.

**Proposed lifecycle:** `load → validate → relocate → create process/task → appStart → event loop → appStop → release handles`.

Each application receives an `AppContext` containing process identity, permissions, arguments, current working directory, and IPC service endpoints. The desktop is not a mandatory dependency; headless CLI applications have zero LVGL overhead.

```javascript
import maia.app;
import maia.console;

i32 main(string[] args) {
    console.println("App: " + app.name());
    console.println("Arguments: " + args.length);
    return 0;
}
```

**To verify in codebase:** current standalone binary formats (`.mobj`, `.elf`), entry point conventions, relocation styles, symbol registration, memory bounds, resource ownership, scheduling, and executing code from RAM.

## 15. Console and Serial API

```javascript
import maia.console;
import maia.serial;

i32 main() {
    console.println("Ready");
    serial.Port debug = serial.open("uart0", 115200)?;
    defer debug.close();
    debug.write("MaiaScript started\n");

    while (true) {
        string line = console.readLine();
        if (line == "exit") break;
        console.println("echo: " + line);
    }
    return 0;
}
```

The console API maps transparently to kernel serial output, a graphic terminal emulator, or a remote session. An application must not assume exclusive ownership of UART0. Raw serial bus access requires explicit permissions.

## 16. Filesystem and Mounted Storage Devices

```javascript
import maia.fs;
import maia.console;

i32 main() {
    fs.mkdir("/home/demo", recursive: true)?;
    File file = fs.open("/home/demo/note.txt", "w")?;
    defer file.close();
    file.writeAll("Created on device!\n")?;

    foreach (fs.Entry e in fs.list("/home/demo")) {
        console.println(e.name + " " + e.size);
    }
    return 0;
}
```

Access to `/dev`, `/mnt`, SD cards, and USB storage proceeds through the VFS and kernel drivers. Commands like `fs.mount` and `fs.unmount` require permissions and cache flushing. An application cannot unmount a volume currently held open by other tasks.

## 17. Processes, Tasks, and Timers

```javascript
import maia.process;
import maia.time;
import maia.console;

i32 main() {
    process.Handle child = process.spawn("/apps/clock.elf", [])?;
    time.sleep(1000); // milliseconds; fixed unit defined by API
    console.println("Child PID: " + child.id());
    return 0;
}
```

```javascript
import maia.time;

i32 main() {
    time.Timer timer = time.every(500, () => {
        console.println("tick");
    });
    defer timer.cancel();
    app.runUntilExit();
    return 0;
}
```

Closures/lambdas require compiler runtime support; in v1, callbacks can be static global function pointers. The lambda syntax above reflects **intended future design**, not an immediate implementation commitment.

## 18. Inter-Application Communication (IPC)

```javascript
import maia.ipc;

i32 main() {
    ipc.Channel bus = ipc.connect("system.notifications")?;
    defer bus.close();
    bus.send({ title: "MaiaScript", message: "Compilation finished" });
    return 0;
}
```

IPC mechanisms must rely on copied/serialized messages or shared buffers with well-defined ownership semantics. Passing raw application pointers across process boundaries is prohibited.

## 19. Configuration and Permissions

**Proposed** standalone application manifest:

```toml
name = "org.maia.examples.notes"
version = "0.1.0"
entry = "notes.elf"
api = "maia-rtos/1"
permissions = ["fs:home", "ui:window", "timer"]
memory_kib = 512
```

On hardware lacking memory protection (MPU/MMU), service-level permissions do not constitute a cryptographic sandbox against malicious native code. The manifest acts as a capability declaration and resource contract.

---

# Part III — Graphical Applications with LVGL

## 20. Why the Public API Should Not Be Raw LVGL

MaiaRTOS incorporates its own desktop environment, window manager, and LVGL as the graphics rendering backend operating over a shared framebuffer. Standalone applications must interact through **window and widget handles** supplied by a system UI API, rather than directly manipulating raw framebuffers, internal mutexes, or calling `lv_timer_handler()`.

Proposed architecture:

```text
MaiaScript app (.elf)
  └── maia.ui / maia.events / maia.graphics (stable public API)
        └── Window Manager / UI service (object owner & compositor)
              └── LVGL backend (single-owner event thread)
                    └── display driver / framebuffer / DMA
```

A central UI service serializes user interface commands or executes them on the LVGL compositor thread. Application UI calls may be queued; applications must not hold compositor locks while performing disk I/O or waiting for external events.

## 21. First Window

File `hello_window.maia`:

```javascript
import maia.app;
import maia.ui;

i32 main() {
    ui.Window win = ui.Window("Hello MaiaOS", 420, 260);
    ui.Label title = ui.Label(win, "Hello from MaiaScript Native!");
    title.setPosition(24, 30);

    ui.Button close = ui.Button(win, "Close");
    close.setPosition(24, 90);
    close.onClick(onClose);

    win.show();
    return app.run();
}

void onClose(ui.Event event) {
    app.quit(0);
}
```

`app.run()` drives the event loop until termination; the runtime automatically reclaims widgets and window surfaces upon process exit. `onClick` invokes a callback within the application's event loop, never inside an LVGL internal critical section.

## 22. Layout and Styles

```javascript
import maia.ui;

i32 main() {
    ui.Window win = ui.Window("Preferences", 520, 360);
    ui.Column root = ui.Column(win);
    root.setPadding(16);
    root.setGap(12);

    ui.Label heading = ui.Label(root, "Display settings");
    heading.setFontSize(22);
    ui.Row row = ui.Row(root);
    ui.Label(row, "Brightness");
    ui.Slider brightness = ui.Slider(row, 0, 100, 70);
    brightness.onChange(onBrightnessChanged);

    win.show();
    return app.run();
}

void onBrightnessChanged(ui.ValueEvent e) {
    system.display.setBrightness(e.value)?;
}
```

The API must distinguish logical layout units from physical hardware pixels. Display scaling, fonts, visual themes, accessibility, and orientation must be cleanly abstracted. Adjusting display brightness is an OS service call requiring appropriate permissions.

## 23. Minimal Text Editor

```javascript
import maia.ui;
import maia.fs;
import maia.app;

ui.TextArea editor;
string currentPath = "/home/notes.txt";

void saveFile(ui.Event e) {
    fs.writeText(currentPath, editor.text())?;
    ui.notify("Saved: " + currentPath);
}

i32 main() {
    ui.Window win = ui.Window("Maia Notes", 640, 480);
    ui.Column layout = ui.Column(win);
    ui.Row toolbar = ui.Row(layout);
    ui.Button save = ui.Button(toolbar, "Save");
    save.onClick(saveFile);
    editor = ui.TextArea(layout, multiline: true);
    editor.setExpand(true);
    editor.setText(fs.readText(currentPath).orElse(""));
    win.show();
    return app.run();
}
```

**Notice:** `fs.writeText` in an event callback should not block the main rendering thread. Production implementations should use worker threads or asynchronous I/O completion. This example highlights readability and will be adapted to the platform's concurrency model.

## 24. Minimal File Manager

```javascript
import maia.ui;
import maia.fs;
import maia.app;

ui.ListView files;
string directory = "/home";

void refresh() {
    files.clear();
    foreach (fs.Entry entry in fs.list(directory)) {
        files.add(entry.isDirectory ? "[DIR] " + entry.name : entry.name);
    }
}

i32 main() {
    ui.Window win = ui.Window("Files", 620, 440);
    files = ui.ListView(win);
    files.fill();
    refresh();
    win.show();
    return app.run();
}
```

A full application adds path navigation, item selection, change notifications, and asynchronous file operations. This demonstrates how a standalone app utilizes system desktop services without being compiled into the desktop binary.

## 25. Dialogs, Notifications, and Menus

```javascript
ui.Dialog dialog = ui.confirm("Delete selected file?");
dialog.onResult(onConfirmDelete);

void onConfirmDelete(ui.DialogResult r) {
    if (r == ui.DialogResult.Yes) {
        fs.remove(selectedPath)?;
        ui.notify("File deleted");
    }
}
```

A modal dialog must never stall the compositor while awaiting user response. Toast notifications belong to the desktop service and may outlive the process that spawned them.

## 26. Keyboard and Touch Events

```javascript
void onKey(ui.KeyEvent e) {
    if (e.pressed && e.key == ui.Key.Escape) {
        app.quit(0);
    }
}

void onPointer(ui.PointerEvent e) {
    if (e.type == ui.Pointer.Down) {
        console.printf("Touch: %d,%d\n", e.x, e.y);
    }
}

win.onKey(onKey);
win.onPointer(onPointer);
```

The window manager coordinates focus and event dispatching. Applications must not poll the Tab5 I²C keyboard or touch controller directly, except for privileged system diagnostic utilities.

## 27. Custom Drawing and Animation

```javascript
import maia.ui;
import maia.graphics;

void draw(graphics.Canvas c) {
    c.clear(graphics.Color(18, 24, 38));
    c.line(10, 10, 200, 90, graphics.Color(0, 220, 180), 3);
    c.rect(40, 120, 120, 60, graphics.Color(80, 130, 255));
    c.text(20, 220, "Maia graphics", graphics.Color.White);
}

i32 main() {
    ui.Window win = ui.Window("Canvas", 500, 320);
    ui.Canvas canvas = ui.Canvas(win);
    canvas.fill();
    canvas.onDraw(draw);
    win.show();
    return app.run();
}
```

Custom rendering must not run in a busy loop. The backend invalidates damaged rectangles and paints at a disciplined refresh cadence; animations hook into system timers. Graphic buffers and draw contexts are automatically freed upon window closure.

## 28. Direct LVGL: Advanced and Restricted Interface

For desktop components and core system tools, a privileged `maia.lvgl` module can provide safe wrappers:

```javascript
import maia.lvgl;

void buildAdvancedWidget(lvgl.Container root) {
    lvgl.Label label = lvgl.Label(root);
    label.setText("LVGL backend");
    label.align(lvgl.Align.Center);
}
```

**Do not** expose raw `lv_obj_t*` pointers to untrusted third-party apps. LVGL compatibility should exist as a versioned abstraction layer; internal LVGL state belongs strictly to the compositor thread. Trusted system apps may alternatively utilize a dedicated C SDK with direct access.

---

# Part IV — Hardware Services via API

## 29. Wi-Fi: Configuration Utility

```javascript
import maia.ui;
import maia.net.wifi;
import maia.app;

ui.ListView networks;

void scanDone(wifi.ScanResult result) {
    networks.clear();
    foreach (wifi.Network n in result.networks) {
        networks.add(n.ssid + " (" + n.rssi + " dBm)");
    }
}

i32 main() {
    ui.Window win = ui.Window("Wi-Fi", 520, 380);
    networks = ui.ListView(win);
    networks.fill();
    ui.Button scan = ui.Button(win, "Scan networks");
    scan.onClick(() => wifi.scanAsync(scanDone));
    win.show();
    return app.run();
}
```

Network credentials must be stored securely and entered via a dedicated system dialog, never leaked into log files. The Wi-Fi API shown is a **proposal** for API design. Callbacks and lambdas can be replaced by global functions in v1.

## 30. USB: Enumeration, Mounting, and Unmounting

```javascript
import maia.storage;
import maia.ui;

void onUnmount(string deviceId) {
    storage.unmountAsync(deviceId, onUnmountDone);
}

void onUnmountDone(storage.Result result) {
    if (result.ok) ui.notify("Device safely removed");
    else ui.alert("Unmount failed: " + result.message);
}
```

Mounting requires validating device descriptors, filesystem formats, access permissions, active file handles, and flushing dirty buffers. User code never directly accesses USB host controllers or DMA queues.

## 31. Audio and System Sounds

```javascript
import maia.audio;

i32 main() {
    audio.playSystemSound(audio.Sound.Notification)?;
    audio.setVolume(audio.Channel.System, 45)?;
    return 0;
}
```

Applications can stream audio data and adjust their own playback volume, but modifying master volume requires system permissions. The audio service arbitrates mixing channels, sample buffers, and hardware codec synchronization.

## 32. GPIO, I²C, and SPI for Diagnostics

```javascript
import maia.hardware.i2c;

i32 main() {
    i2c.Bus bus = i2c.open(0)?;
    defer bus.close();
    u8[] data = bus.readRegister(0x6D, 0x01, 1)?;
    return data[0];
}
```

**Conceptual example only.** The address `0x6D` corresponds to the Tab5 keyboard controller mentioned in previous discussions; peripheral registers and bus mappings must be verified against actual board schematics. The hardware API requires elevated permissions and arbitrates access to buses shared with the kernel. User applications must not open buses reserved for the keyboard, touch controller, or power management ICs.

## 33. System Information

```javascript
import maia.system;
import maia.console;

i32 main() {
    console.println(system.name());
    console.println(system.version());
    console.println("Free RAM: " + system.memory.freeBytes());
    console.println("Uptime: " + system.uptimeMs());
    return 0;
}
```

---

# Part V — Assembler and On-Device RISC-V Compilation

## 34. MaiaASM: Standalone Assembler

The assembler must generate relocatable RISC-V ELF32 object files, serving both human assembly programmers and the MaiaScript code generator backend. Pseudo-instructions expand into valid machine instructions. Minimum feature set: RV32I plus enabled target extensions; directives `.text`, `.data`, `.bss`, `.rodata`, `.align`, `.byte`, `.word`, `.globl`, `.extern`, `.section`; labels, local symbols, constant expressions, and relocation records.

Example `sum.s`:

```asm
.section .text
.globl sum
sum:
    add a0, a0, a1
    ret
```

**Proposed** toolchain workflow:

```text
maia> maiaasm sum.s -o sum.o
maia> maiald sum.o app.o -o app.elf
maia> ./app.elf
```

Building a functional `maiaasm` requires more than encoding `add`: it demands validating relocation types, external symbol resolution, alignment rules, pseudo-instruction expansions, and ELF structural integrity. A compact implementation can start with a documented subset, avoiding false claims of full GNU as compatibility.

## 35. Calling Assembly from MaiaScript

```javascript
extern i32 sum(i32 a, i32 b) from "sum.o";

i32 main() {
    return sum(7, 9);
}
```

The syntax `extern ... from` is a design proposal. The ABI must clearly define calling conventions, stack alignment (16-byte for RV32), caller/callee-saved register sets, struct return mechanics, variadic arguments, and error states.

## 36. On-Device Compilation

```text
maia> edit /home/demo.maia
maia> maiasc /home/demo.maia -c -o /home/demo.o
maia> maiald /home/demo.o -o /apps/demo.elf
maia> /apps/demo.elf
```

The compiler must emit source-accurate diagnostics with line and column numbers, accept modular source files, and allow incremental builds. Code generation can rely on a simple typed intermediate representation (IR), linear-scan register allocation or stack slots, and an object emitter shared with the assembler. The compiler does not need to write temporary `.s` files to flash; it can emit binary objects directly, retaining an optional `--emit-asm` flag for debugging.

## 37. Graphical App Compiled On the Go

```text
maia> maiasc /home/hello_window.maia -o /apps/hello_window.elf
maia> /apps/hello_window.elf
```

The loader validates the app, registers it with the desktop, creates a window through the UI service, and passes events to the process. This requires the desktop to remain decoupled from the application binary. The modular architecture recently established in MaiaRTOS provides the foundation for this capability.

## 38. Debugging and Diagnostics

```text
maia> maiasc --check hello.maia
maia> maiasc --emit-asm hello.maia -o hello.s
maia> maiaasm --disassemble hello.o
maia> maiaelf --symbols hello.elf
maia> maiaelf --relocs hello.o
```

Proposed diagnostic tools. For application faults: record PC, SP, register dump, trap cause, process ID, symbol map, and serial telemetry. Do not conflate clean `app.quit()` with a hardware exception. Without hardware memory isolation, certain native memory faults can crash the kernel; telemetry must capture as much diagnostic state as possible before restarting.

---

# Part VI — Complete Example Projects

## 39. Graphical Stopwatch

```javascript
import maia.app;
import maia.ui;
import maia.time;

ui.Label elapsedLabel;
u64 startedAt;
time.Timer timer;

void updateTime() {
    u64 elapsed = time.monotonicMs() - startedAt;
    elapsedLabel.setText("Elapsed: " + elapsed + " ms");
}

void stopTimer(ui.Event e) {
    timer.cancel();
    app.quit(0);
}

i32 main() {
    ui.Window win = ui.Window("Stopwatch", 380, 200);
    elapsedLabel = ui.Label(win, "Elapsed: 0 ms");
    elapsedLabel.setPosition(24, 30);
    ui.Button stop = ui.Button(win, "Stop");
    stop.setPosition(24, 100);
    stop.onClick(stopTimer);
    startedAt = time.monotonicMs();
    timer = time.every(100, updateTime);
    win.show();
    return app.run();
}
```

**Required contract:** `time.every` dispatches `updateTime` within the application event loop thread, never invoking LVGL from arbitrary background threads.

## 40. System Resource Monitor

```javascript
import maia.app;
import maia.ui;
import maia.system;
import maia.time;

ui.Label ramLabel;
ui.Label cpuLabel;

void refresh() {
    ramLabel.setText("Free memory: " + system.memory.freeBytes());
    cpuLabel.setText("CPU usage: " + system.cpu.loadPercent() + "%");
}

i32 main() {
    ui.Window win = ui.Window("System Monitor", 420, 240);
    ramLabel = ui.Label(win, "RAM: ...");
    cpuLabel = ui.Label(win, "CPU: ...");
    ramLabel.setPosition(20, 30);
    cpuLabel.setPosition(20, 75);
    time.every(1000, refresh);
    refresh();
    win.show();
    return app.run();
}
```

`cpu.loadPercent` assumes system metrics that the kernel telemetry may provide in future revisions.

## 41. USB Backup Utility

```javascript
import maia.fs;
import maia.storage;
import maia.console;

i32 main() {
    storage.Volume usb = storage.findMountedUsb()?;
    string dest = usb.mountPath + "/backup/notes.txt";
    fs.copy("/home/notes.txt", dest)?;
    fs.sync(dest)?;
    console.println("Backup complete; unmount before removal.");
    return 0;
}
```

This application does not automatically unmount USB drives if other processes hold active references. The storage manager arbitrates state and reference counting.

## 42. Object-Oriented GUI Application

```javascript
import maia.app;
import maia.ui;

class CounterApp {
    private i32 count = 0;
    private ui.Window win;
    private ui.Label label;

    CounterApp() {
        win = ui.Window("Counter", 360, 220);
        label = ui.Label(win, "Count: 0");
        label.setPosition(20, 30);
        ui.Button button = ui.Button(win, "Increment");
        button.setPosition(20, 90);
        button.onClick(onIncrement);
    }

    void onIncrement(ui.Event e) {
        count += 1;
        label.setText("Count: " + count);
    }

    void show() { win.show(); }
}

i32 main() {
    CounterApp counter = CounterApp();
    counter.show();
    return app.run();
}
```

This example highlights a semantic decision required **prior** to EBNF finalization: `button.onClick(onIncrement)` implicitly captures instance `this`. The language must define whether method callbacks are `(function pointer, object handle)` pairs, closures, or event handlers with explicit `userData`. The cleanest approach for v1 is bound method callbacks with stable handles, omitting generalized heap closures.

---

# Part VII — Architectural Decisions Prior to EBNF

## 43. What to Preserve from Original MaiaScript

The existing grammar contains `namespace`, `function`, `plain`, typed functions, `global`, `local`, `if/elseif/else`, `do/while`, `for`, `foreach`, `switch`, `try/catch`, `test`, `import/export/include`, `typeof`, integer/float types (`i32/i64/f32/f64`), arrays, matrices, complex numbers, and diverse operators. **This is a syntactic inventory, not an operational implementation.**

Preferably preserve: curly braces, return-type-on-left function signatures, `namespace`, `if/elseif/else`, `for`, `while`, `foreach`, `import/export`, explicit literals, and static types. Refactor: ambiguous function declarations, variable scopes, power operator precedence, lvalue requirements, array types, string representations, and the object model.

Omit from the v1 core: generalized `async`, stack-unwinding exceptions, built-in matrix/complex mathematics, dynamic metaprogramming, reflection, class inheritance, and arbitrary closures. These can be introduced later as libraries or language extensions.

## 44. Semantic Questions Not Solved by EBNF

1. Exact RV32 ISA and ABI: enabled extensions (M, A, F, C), alignment rules, soft vs. hard float calling conventions.
2. Binary format: ELF32 relocatable (`ET_REL`), executable (`ET_EXEC`), or a custom format? How does the kernel loader perform relocations?
3. Code execution memory: where is generated code stored and executed? Which RAM regions are marked executable? How is I-cache invalidated?
4. Hardware protection: is PMP/MPU utilized, or does code run with kernel privileges?
5. Memory layouts: binary representation of `string`, array, slice, class, enum, and `Result`.
6. Runtime traps: behavior on integer overflow, invalid casts, null pointer dereference, division by zero, and array out-of-bounds indexing.
7. Resource reclamation: how does the kernel track open files, windows, and timers per process, and clean them up after a crash?
8. Threading and UI model: dispatch policy for UI events and LVGL compositor synchronization.
9. Application discovery: how does the desktop discover standalone apps, icons, and manifests on storage?
10. ABI stability: how do system APIs evolve without breaking existing compiled binaries?
11. Symbol resolution: static linkage, a global syscall vector table, or dynamic module linking?
12. Memory management: `new/delete`, compile-time ownership, or strictly scoped local objects and handles?
13. Bound method callbacks: representation, lifetime, and dispatch mechanism.
14. Error classification: recoverable application errors vs. fatal CPU traps.
15. Resource budgets: maximum RAM footprint and acceptable compilation times on the Tab5.

## 45. Recommended Implementation Sequence

**Milestone A — Verifiable Assembler:** RV32I encoding, ELF32 relocatable objects, symbol tables, and relocations; differential validation against GNU as/readelf/objdump on host PC; execution of test binaries on Tab5 hardware.

**Milestone B — Loader and ABI:** standalone native `main`, arguments, return codes, console imports, handle reclamation, and trap diagnostics. Compare against current MaiaRTOS standalone apps.

**Milestone C — MaiaScript CLI:** expression parsing, functions, scalar types, conditionals, loops, constant strings, native function calls; on-device compilation and execution.

**Milestone D — Compound Types:** arrays, slices, structs, bounds checking, managed heap, and modular imports.

**Milestone E — GUI Integration:** `maia.ui`, window creation, event dispatch, labels, buttons, text areas; testing two simultaneous standalone apps, focus handling, window closing, and crash recovery.

**Milestone F — Objects and Standard Library:** inheritance-free classes, methods, constructors, bound callbacks, and module exports.

**Milestone G — Hardware Services:** filesystem, timers, USB storage, Wi-Fi configuration, audio playback, and system diagnostics according to available drivers.

## 46. Acceptance Criteria for an On-Device Compiler

- `maiasc` compiles a self-contained program on the Tab5 without external network or host services.
- Compiled binaries can be written to persistent storage (`/sd`) and executed on demand.
- `maiaasm` assembles `.s` files and the linker resolves real symbol and relocation records.
- Headless CLI applications run without initializing the desktop or LVGL.
- Multiple standalone graphical apps open and manage independent windows.
- Terminating an application cleanly reclaims windows, timers, file descriptors, and memory allocations.
- An application crash does not lock up the desktop compositor; unrecoverable native memory faults are trapped and logged before rebooting.
- Produced machine code is inspectable via disassembler and symbol table dumping tools.
- Output code is cross-verified against a reference RISC-V compiler toolchain.
- The build pipeline profiles compile time, peak RAM utilization, and binary footprint.

## 47. Final Recommendation

MaiaScript Native must be defined **based on real working programs**, particularly existing MaiaRTOS standalone applications. This manual establishes use cases and language surface proposals. The immediate next step is not writing a monolithic EBNF grammar: first, verify the concrete ABI and SDK in the MaiaRTOS codebase, select a cohesive core subset, and formalize the semantics of types, memory, callbacks, and resources. Once that foundation is fixed, specify the grammar and generate the parser using MaiaCC.

---

## Appendix A — Proposed Module Names

`maia.app`, `maia.console`, `maia.serial`, `maia.fs`, `maia.process`, `maia.time`, `maia.ipc`, `maia.ui`, `maia.graphics`, `maia.lvgl`, `maia.net.wifi`, `maia.storage`, `maia.audio`, `maia.hardware.i2c`, `maia.system`.

All are **design names**, not confirmed system endpoints.

## Appendix B — Example Maturity Matrix

| Feature Group | Document Status |
|---|---|
| Scalar syntax and functions | Proposed for MVP |
| Structs and arrays | Proposed post-MVP |
| Classes without inheritance | Proposed after structs |
| `Result`, `?`, `defer` | Under discussion |
| Method callbacks and lambdas | Under discussion; critical for GUI |
| `maia.ui` API | Proposed facade over window manager/LVGL |
| `maia.lvgl` | Proposed advanced interface |
| Wi-Fi / USB / Audio | Target API design examples |
| ELF32, loader, and ABI | Dependent on MaiaRTOS verification |
| Toolchain commands (`maiasc`, `maiaasm`, `maiald`) | Proposed tools |

## Appendix C — References and Information Sources

- `MaiaScript.ebnf`, provided by the author: baseline comparison for original syntax.
- MaiaRTOS architecture, standalone applications, LVGL, and window manager: described by the author.
- Project repository: https://github.com/souzamonteiro/maia-rtos.git
- Public LVGL documentation: https://lvgl.io/docs/open/integration/overview — provides general library characteristics, not specific MaiaRTOS integration details.

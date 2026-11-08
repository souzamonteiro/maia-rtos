# MaiaScript Native — Standard Library and MaiaRTOS API Specification

**Document Version:** 1.0 (Normative Standard)  
**Target Architecture:** MaiaRTOS / ESP32-P4 / RISC-V RV32IMAF (ILP32F ABI)  
**Execution Model:** AOT / Native Compilation to RISC-V ELF Objects (`.mobj`)  
**Specification Language:** English  

---

## 1. Design Goals and Architectural Contracts

MaiaScript Native is a statically typed, deterministic systems and application language designed to execute directly on bare-metal and MaiaRTOS microcontrollers without a JavaScript runtime, WebAssembly VM, or garbage collector.

### 1.1 Core Principles
1. **Zero-Overhead Primitive Types:** Primitive types (`i32`, `f32`, `bool`, `string`) map 1:1 to RISC-V RV32 registers:
   - Integers and pointers reside in `a0..a7` (`x10..x17`).
   - Single-precision floating point numbers reside in hardware FPU registers `fa0..fa7` (`f10..f17`).
2. **Deterministic Memory Ownership:**
   - Stack allocation for local primitives, structs, and arrays with ABI-compliant 16-byte stack frame alignment.
   - Capability Handles for operating system resources (`File`, `ui.Window`, `ui.Button`, `TaskHandle`).
   - Explicit heap allocation with bounded quotas for dynamic structures (`Matrix`, large buffers).
   - No asynchronous garbage collection pauses interrupting real-time control or 60 FPS graphics.
3. **Explicit Error Handling:**
   - Kernel and I/O calls return typed `Result<T, E>` or status integers.
   - No dynamic stack-unwinding exceptions (`try/catch/finally` from JS is deprecated in favor of `Result<T, E>` and `defer`).
4. **Clean Decoupling from Browser/DOM APIs:**
   - Browser-specific legacy APIs (e.g., `openSQLDatabase`, dynamic `eval`, Web Workers via Blob URLs, DOM wrappers) are obsolete and replaced by native MaiaRTOS system services.

---

## 2. Type System Specification

### 2.1 Primitive Value Types
| Type | Bit Width | Alignment | RISC-V Storage | Description |
|---|---|---|---|---|
| `void` | 0 | - | - | Absence of value |
| `bool` | 8 | 1 byte | Integer register | Boolean `true` (1) or `false` (0) |
| `i8` / `u8` | 8 | 1 byte | Integer register | 8-bit signed / unsigned integer |
| `i16` / `u16` | 16 | 2 bytes | Integer register | 16-bit signed / unsigned integer |
| `i32` / `u32` | 32 | 4 bytes | Integer register | 32-bit signed / unsigned integer |
| `i64` / `u64` | 64 | 8 bytes | Register pair / stack | 64-bit integer |
| `f32` | 32 | 4 bytes | FPU register (`f0..f31`) | IEEE 754 single-precision float |
| `f64` | 64 | 8 bytes | Software float emulation | IEEE 754 double-precision float |
| `char` | 32 | 4 bytes | Integer register | Unicode UTF-32 scalar value |
| `string` | 64 | 4 bytes | Register pair (`ptr, len`) | Immutable UTF-8 string view with explicit byte length |

### 2.2 Composite and Collection Types
* **Static Array `T[N]`:** Continuous inline array of $N$ elements allocated directly in the current stack frame.
* **Slice `T[]`:** Fat pointer structure `{ T* data; u32 length; }` with mandatory runtime bounds checking in debug builds.
* **Struct `struct Name { ... }`:** Flat, zero-cost continuous C-ABI layout without hidden pointer indirection.
* **Result `Result<T, E>`:** Discriminated union `{ bool ok; union { T value; E error; }; }`.

---

## 3. Standard Library Modules

### 3.1 Module `maia.system` (or `system`)
Provides console input/output, process lifecycle, memory introspection, and core diagnostics.

```maiascript
namespace system {
    // Console Output
    void print(string msg);
    void println(string msg);
    void printf(string fmt, ...);
    void error(string msg);

    // Console Input
    string readLine();
    i32 readChar();

    // System Status & Memory
    u32 millis();
    u64 micros();
    void sleep(u32 ms);
    u32 freeMemory();
    u32 totalMemory();

    // Process Lifecycle
    void exit(i32 exitCode);
}
```

#### Behavioral Contract
* `system.println` outputs directly to the active MaiaRTOS terminal or serial output device.
* `system.printf` supports standard format specifiers (`%d`, `%i`, `%u`, `%x`, `%f`, `%s`, `%c`). It operates with bounded stack buffers to prevent buffer overflow.
* `system.sleep(ms)` puts the current task into blocked state, yielding CPU execution to other MaiaRTOS tasks.

---

### 3.2 Module `maia.math` (or `math`)
High-performance scalar mathematical operations leveraging hardware FPU instructions (`RV32F`).

```maiascript
namespace math {
    // Constants
    const f32 PI = 3.14159265358979323846f;
    const f32 E  = 2.71828182845904523536f;

    // Trigonometry
    f32 sin(f32 x);
    f32 cos(f32 x);
    f32 tan(f32 x);
    f32 asin(f32 x);
    f32 acos(f32 x);
    f32 atan(f32 x);
    f32 atan2(f32 y, f32 x);
    f32 sinh(f32 x);
    f32 cosh(f32 x);
    f32 tanh(f32 x);

    // Exponential & Logarithmic
    f32 exp(f32 x);
    f32 log(f32 x);
    f32 log2(f32 x);
    f32 log10(f32 x);
    f32 pow(f32 base, f32 exp);
    f32 sqrt(f32 x);        // Emits native 'fsqrt.s' instruction
    f32 cbrt(f32 x);

    // Rounding & Integer Part
    f32 ceil(f32 x);
    f32 floor(f32 x);
    f32 round(f32 x);
    f32 trunc(f32 x);

    // Arithmetic Utilities (Overloaded for i32 and f32)
    f32 abs(f32 x);
    i32 abs(i32 x);
    f32 min(f32 a, f32 b);
    i32 min(i32 a, i32 b);
    f32 max(f32 a, f32 b);
    i32 max(i32 a, i32 b);
    f32 clamp(f32 x, f32 minVal, f32 maxVal);
    i32 clamp(i32 x, i32 minVal, i32 maxVal);
    f32 sign(f32 x);
    i32 sign(i32 x);

    // Angle Conversions
    f32 rad(f32 deg);
    f32 deg(f32 rad);

    // Random Generation
    f32 random();           // Pseudo-random float in [0.0, 1.0)
    i32 randomInt(i32 min, i32 max);
}
```

---

### 3.3 Module `maia.string` (or `string`)
UTF-8 safe string manipulation without garbage collection leaks.

```maiascript
namespace string {
    i32 length(string s);
    char charAt(string s, i32 index);
    i32 indexOf(string s, string needle, i32 fromIndex = 0);
    i32 lastIndexOf(string s, string needle);
    bool includes(string s, string needle);
    bool startsWith(string s, string prefix);
    bool endsWith(string s, string suffix);

    string substr(string s, i32 start, i32 length);
    string slice(string s, i32 start, i32 end);
    string trim(string s);
    string trimLeft(string s);
    string trimRight(string s);
    string toLowerCase(string s);
    string toUpperCase(string s);
    string replace(string s, string pattern, string replacement);

    string sprintf(string fmt, ...);
    i32 toInt(string s, i32 base = 10);
    f32 toFloat(string s);
}
```

---

### 3.4 Module `maia.matrix` (or `matrix`)
Linear algebra and vector mathematics with contiguous memory layout on PSRAM.

```maiascript
struct Matrix {
    i32 rows;
    i32 cols;
    f32* data;      // Contiguous row-major buffer
    bool is_valid;
}

namespace matrix {
    // Allocation & Initialization
    Matrix create(i32 rows, i32 cols, f32 initVal = 0.0f);
    Matrix identity(i32 n);
    Matrix zeros(i32 rows, i32 cols);
    Matrix ones(i32 rows, i32 cols);
    void free(Matrix m);

    // Accessors
    f32 get(Matrix m, i32 r, i32 c);
    void set(Matrix m, i32 r, i32 c, f32 val);
    i32 rows(Matrix m);
    i32 cols(Matrix m);

    // Linear Algebra Operations
    Matrix add(Matrix a, Matrix b);
    Matrix sub(Matrix a, Matrix b);
    Matrix mul(Matrix a, Matrix b);           // Matrix multiplication (dot product)
    Matrix scale(Matrix a, f32 scalar);
    Matrix trans(Matrix a);                   // Transposition
    f32 det(Matrix a);                        // Determinant
    Matrix inv(Matrix a);                     // Inverse

    // Reduction Operations
    f32 sum(Matrix a);
    f32 avg(Matrix a);
    f32 min(Matrix a);
    f32 max(Matrix a);
}
```

---

### 3.5 Module `maia.fs` (or `fs`)
Direct VFS file input/output interfacing with the SD Card and SPI Flash.

```maiascript
enum FileMode {
    READ      = 1,   // "r"
    WRITE     = 2,   // "w"
    APPEND    = 3,   // "a"
    READ_WRITE = 4   // "r+"
}

enum SeekOrigin {
    SET = 0,
    CUR = 1,
    END = 2
}

struct File {
    u32 handle;
    bool valid;

    i32 read(u8[] buffer);
    i32 write(string text);
    i32 writeBytes(u8[] bytes);
    i32 seek(i32 offset, SeekOrigin origin);
    i32 tell();
    void flush();
    void close();
}

namespace fs {
    Result<File, i32> open(string path, FileMode mode);
    bool exists(string path);
    i32 size(string path);
    Result<i32, i32> remove(string path);
    Result<i32, i32> rename(string oldPath, string newPath);
    Result<i32, i32> mkdir(string path);

    // Convenience Helper
    string readAllText(string path, u32 maxBytes = 65536);
    Result<i32, i32> writeAllText(string path, string content);
}
```

---

### 3.6 Module `maia.ui` (or `ui`)
Desktop graphical user interface over LVGL using opaque Capability Handles.

```maiascript
struct Event {
    i32 type;
    u32 targetId;
    i32 x;
    i32 y;
}

typedef void (*EventCallback)(Event event);

class Window {
    u32 handle;
    Window(string title, i32 width, i32 height);
    void show();
    void hide();
    void close();
}

class Label {
    u32 handle;
    Label(Window parent, string text);
    void setText(string text);
    void setPos(i32 x, i32 y);
}

class Button {
    u32 handle;
    Button(Window parent, string text);
    void setText(string text);
    void setPos(i32 x, i32 y);
    void setSize(i32 w, i32 h);
    void onClick(EventCallback cb);
}

class TextArea {
    u32 handle;
    TextArea(Window parent);
    void setText(string text);
    string getText();
    void setPos(i32 x, i32 y);
    void setSize(i32 w, i32 h);
}

namespace ui {
    i32 run();
    void processEvents();
    void showMessageBox(string title, string message);
}
```

---

## 4. Migration Table: Legacy JavaScript vs. MaiaScript Native

| Legacy JavaScript Expression | Native MaiaScript Equivalent | Performance & Memory Impact |
|---|---|---|
| `core.length(arr)` | `string.length(s)` or `arr.length` | Direct integer field load; $O(1)$ zero overhead |
| `core.charAt(str, pos)` | `string.charAt(str, pos)` | Direct byte index fetch without heap allocation |
| `core.matrix(val, r, c)` | `matrix.create(r, c, val)` | Contiguous PSRAM allocation instead of dynamic JS array |
| `core.add(a, b)` | `a + b` | Native machine instruction (`add` / `fadd.s`) |
| `core.push(arr, val)` | `slice[idx] = val` | Bounded slice write; eliminates realloc storms |
| `core.openSQLDatabase(...)` | *Deprecated* | Replaced by `maia.fs` or `maia.kv` persistent storage |
| `core.eval(script)` | *Deprecated* | AOT / on-the-go compiler (`maiasc`) creates native ELFs |
| `system.print(text)` | `system.print(text)` | Direct terminal UART/framebuffer emission |
| `system.println(text)` | `system.println(text)` | Direct terminal emission with newline |
| `system.printf(fmt, ...)` | `system.printf(fmt, ...)` | Safe varargs formatting into stack buffer |
| `math.sin(x)` (untyped) | `f32 math.sin(f32 x)` | Float register pass (`fa0`); zero boxing |
| `math.sqrt(x)` | `f32 math.sqrt(f32 x)` | Generates 1 machine instruction: `fsqrt.s fa0, fa0` |
| `task.new(func)` | `maia.task.spawn(func, arg)` | Native FreeRTOS / MaiaRTOS thread |

---

## 5. Idiomatic Native Examples

### 5.1 Command-Line Native Utility
```maiascript
import maia.system;
import maia.math;

i32 main() {
    system.println("=== MaiaScript Sensor Calibration ===");
    f32 raw_voltage = 3.28f;
    f32 scale_factor = 1.05f;
    f32 calibrated = raw_voltage * scale_factor;

    f32 root = math.sqrt(calibrated);
    system.printf("Voltage: %.2f V | Sqrt: %.3f\n", calibrated, root);
    return 0;
}
```

### 5.2 File Processing on SD Card
```maiascript
import maia.system;
import maia.fs;

i32 main() {
    Result<File, i32> res = fs.open("/sd/data.txt", FileMode.WRITE);
    if (!res.ok) {
        system.println("[ERROR] Failed to open /sd/data.txt");
        return 1;
    }

    File f = res.value;
    defer f.close();

    f.write("MaiaRTOS Sensor Log\n");
    f.write("Sample 1: OK\n");
    system.println("[OK] File written successfully.");
    return 0;
}
```

### 5.3 Standalone Graphical Application
```maiascript
import maia.system;
import maia.ui;

ui.Label statusLabel;

void onButtonClick(ui.Event ev) {
    statusLabel.setText("Button Was Touched!");
    system.println("GUI event received!");
}

i32 main() {
    ui.Window win = ui.Window("Sensor Monitor", 500, 320);
    statusLabel = ui.Label(win, "Press the button below:");
    statusLabel.setPos(40, 60);

    ui.Button btn = ui.Button(win, "CLICK ME");
    btn.setPos(40, 120);
    btn.setSize(180, 50);
    btn.onClick(onButtonClick);

    win.show();
    return ui.run();
}
```

---

## 6. Acceptance and Verification Checklist

* [x] Every function signature uses explicit types (`i32`, `f32`, `bool`, `string`, `Matrix`, `File`).
* [x] No untyped `object` or generic parameters remain in the API specification.
* [x] Operators (`+`, `-`, `*`, `/`, etc.) are recognized as language-level expressions and RISC-V instructions, not runtime library calls.
* [x] Arrays and matrices use explicit dimensions and contiguous memory layouts.
* [x] Memory management is deterministic, eliminating dependency on a JavaScript Garbage Collector.
* [x] Graphical UI uses Capability Handles (`ui.Window`, `ui.Button`) preventing direct pointer exposure.
* [x] Complete alignment between Grammar (`.ebnf`), Conceptual Manual (`.md`), and API Specification (`.md`).

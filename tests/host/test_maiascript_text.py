#!/usr/bin/env python3
"""Exercise the target rasterizer with a pixel sink, without ESP-IDF hardware.

Uses the actual panel strings from the bundled apps. The opaque background
makes every glyph cell observable, independently of the font bitmap.
"""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'platforms/tab5/main/app_main.cpp').read_text()
start = source.index('static void draw_text_px(')
end = source.index('\nstatic void draw_rect_api(', start)
renderer = source[start:end]

cases = []
for name, marker, x, y, box in [
    ('scicalc', b'Parser Descendente', 310, 491, (296, 483, 688, 110)),
    ('alarm', b'Sintetizador Acustico', 320, 415, (306, 405, 668, 176)),
]:
    data = (ROOT / f'examples/modules/{name}/{name}.mobj').read_bytes()
    offset = data.index(marker)
    text = data[offset:data.index(b'\0', offset)].decode('ascii')
    assert '\n' in text, f'{name}: expected multiline panel'
    cases.append((name, text, x, y, box))
# Empty/null input must not write pixels; blank lines must retain spacing.
cases.extend([('empty', '', 10, 10, (0, 0, 100, 150)),
              ('blank line', 'A\n\nB', 10, 10, (0, 0, 100, 150))])

checks = []
for name, text, x, y, (bx, by, bw, bh) in cases:
    checks.append(f'''{{
        pixels.clear(); expected.clear();
        draw_text_px({x}, {y}, {json.dumps(text)}, 0xffff, 1);
        int row = 0;
        std::string text = {json.dumps(text)};
        size_t begin = 0;
        do {{
            size_t end = text.find('\\n', begin);
            if (end == std::string::npos) end = text.size();
            for (int py = {y} + row * 32; py < {y} + (row + 1) * 32; ++py)
                for (int px = {x}; px < {x} + int(end - begin) * 16; ++px) {{
                    assert(px >= {bx} && px < {bx + bw});
                    assert(py >= {by} && py < {by + bh});
                    expected.emplace(px, py);
                }}
            begin = end + 1; ++row;
        }} while (begin <= text.size());
        if (pixels != expected) {{ std::cerr << "FAIL: {name}\\n"; return 1; }}
    }}''')

harness = '''#include <cstdint>
#include <cassert>
#include <set>
#include <string>
#include <iostream>
#define CHAR_H 32
static uint8_t g_font_8x16[95 * 16] = {};
static std::set<std::pair<int, int>> pixels, expected;
static void put_pixel_rot3(int x, int y, uint16_t) { pixels.emplace(x, y); }
''' + renderer + '\nint main() {\n' + '\n'.join(checks) + '''
    pixels.clear(); draw_text_px(10, 10, nullptr, 0xffff, 1);
    assert(pixels.empty());
    std::cout << "PASS: panel pixel bounds, line origins, blank/empty/null text\\n";
}
'''
with tempfile.TemporaryDirectory(prefix='maia-text-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(harness)
    subprocess.run(['c++', '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

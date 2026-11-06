# Accented terminal glyph correction

Status: implemented and validated locally. Font/bitmap APIs and actual platform
RGB565 raster comparisons pass; interactive browser reconfirmation is pending.

2026-10-23. Owner reports ç as a square and ã without the accent. Inspection of
the actual LVGL 9.3 Montserrat-14 source confirms only ASCII plus selected symbols;
the previous UTF-8 tests checked text transport, not glyph coverage. Correct this
rendering defect in the existing LVGL platform, with no new GUI technology,
guest API, process model, compiler change or publication.

Generate a static Montserrat-14 label font covering ASCII, Latin-1, combining
diacritics available in the source, using pinned official lv_font_conv 1.5.3
and the OFL font source shipped with pinned LVGL 9.3.0. Retain source/license,
generated C and reproducible script. Use existing Montserrat-14 as fallback for
its original symbols. Apply the font to actual LVGL guest labels; preserve other
widget/title font choices and existing guest editing/streams. No browser text
overlay, OS font, runtime TTF loader or modification of fetched LVGL sources.

Verify real LVGL glyph lookup/bitmaps, distinct rasterized accented labels and
the existing kernel/application/I/O/shell integration. Rebuild only the LVGL
platform JS/WASM assets with Emscripten; no MaiaOS guest rebuild is needed.
Recheck composition only if evidence of a separate input failure appears.
No claim of arbitrary Unicode fonts, grapheme shaping, IME or browser-interactive
verification from text callback assertions. Preserve all local work; no commits,
push, npm publication or version changes.

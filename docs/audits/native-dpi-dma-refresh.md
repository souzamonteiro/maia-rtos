# Native bounded solid DMA refresh — 2026-11-15

**HARDWARE-TESTED, LIMITED; user confirmed stable red.** One boot-only batch
completed 400 solid RGB565 frames and recovered command access. User explicitly
confirmed readiness before this RAM upload and then reported “Sim, vermelho
estável”. Earlier single-frame non-observation was a readiness/timing problem,
not evidence of a black display. Flash and SD were not written.

## Implementation and ownership

The new `maia_esp32p4_dsi_dma_solid_refresh` composes the existing
[finite-frame transaction](native-dpi-dma-frame.md). It accepts 1..400 frames and
1..100000 polls per frame, boot/core0/interrupts off/exclusive controller ownership,
same immutable8-byte source and fixed destination. No heap, IRQ, list or foreign
runtime. Each frame completes and recovers separately; this is not continuous
streaming. Existing standalone frame remains one-shot.

Between successful frames, fresh private idle/root/configuration/FIFO guards are
required. Only channel0 completion/disabled flags are cleared at +198 with
4000001B (bits0,1,3,4,30); fresh zero status/common/underrun is required before
private rearm. Errors are not cleared or retried. A failed frame/rearm is sticky,
requiring reboot. No caller report can rearm a standalone consumed frame.
Temporary saved/restored GPIO22 illumination spans the whole batch.

Primary source: pinned SDKf5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[DW-GDMA register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h),
`DMAC_CH1_INTCLEAR0_REG` +198 and its five completion/disabled WO fields;
channel named CH1 in documentation is native zero-based channel0. Refer to
[frame evidence](native-dpi-dma-frame.md) for all unchanged DSI/timing/counter
sources, including chip1.3 TRM Pre-release v0.4 §5.5.7 pp337–338 and §5.27 p375.
No new MMIO address/timing or vendor runtime introduced; new composition is MIT.

## Validation

Host: 42/42 CTest PASS, actual driver ASan/UBSan PASS, C++98 header PASS,
invalid refresh-without-frame build rejected. Five compiled mutants rejected:
unbounded frame count, missing completion clear, retry after failure, adoption
of consumed standalone frame, and ignored clear readback. Tests include max400,
invalid0/401/null, duplicate use, timeout and dropped status-clear readback.
Native GCC14.2 RV32IMAC/ILP32 build/runtime gate PASS; project + libgcc only.

Exact command (Python from idf5.4_py3.14_env):

```
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-dma-refresh-20261115/maia_sched_probe_ram.bin --log /tmp/maia-dpi-dma-refresh-hardware.log --dpi-dma-refresh --capture-seconds 25
```

Image SHA256: `48ff157bbb7e68df5a2b65c32468d829371628ae251ae750c0df864b0d561650`.
Runner HOST PASS, 400 frames, 399 guarded rearms, 436369 aggregate completion
polls, 92160000 normalized64-bit items. Total elapsed6109505us is one diagnostic
sample (about6.1s), not a deadline, refresh-rate measurement or WCET.
Last frame: active1/done1, raw383FF/items38400, status12, errors0, FIFO0,
1091 completion/2 recovery polls, recovered1. Source hashE18C2E45/unused list
hash2A42FDB7 unchanged. Output/global/channel off, MODE1/LP0, light restored,
four panel power9C replies. Reset then shell/SciCalc `2+3*4 = 14`/desktop PASS.

[Hardware capture](native-dpi-dma-refresh-hardware.txt),
[desktop capture](native-dpi-dma-refresh-desktop.txt),
[user observation](native-dpi-dma-refresh-observation.txt),
[manifest](native-dpi-dma-refresh-manifest.json).
Recovery: `../maia-rtos-recovery/native-dpi-dma-refresh-20261115/`.

## Limits and next small task

This confirms visible solid color via native DMA in a bounded boot diagnostic.
No framebuffer/PSRAM/native desktop/production rotation-input acceptance yet;
installed complete desktop still uses its preserved original runtime. Existing
bridge discard threshold413 means raw underrun0 is not an all-pixel guarantee.
Interrupts remain disabled for the bounded diagnostic; not a production display
service. WT update has no readable ACK. No unbounded refresh, IRQ scheduler,
160-LLI activation, panel timing/parser or rotation change.
Next small task: design and qualify an owned finite SRAM pixel tile transfer
with source increment and explicit spatial/color expectation, before PSRAM or
full desktop. Preserve native720x1280 and production90-degree rotation mapping.

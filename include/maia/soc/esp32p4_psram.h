/* Original MaiaRTOS MIT. ESP32-P4 PSRAM controller observation only.
 * Pinned primary sources: docs/audits/native-psram-state.md. */
#ifndef MAIA_ESP32P4_PSRAM_H
#define MAIA_ESP32P4_PSRAM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAIA_PSRAM_ROOT_WORDS 9U
#define MAIA_PSRAM_CONTROLLER_WORDS 27U
typedef enum maia_psram_result {
    MAIA_PSRAM_OK=0,MAIA_PSRAM_INVALID=-1,MAIA_PSRAM_CONTEXT=-2,
    MAIA_PSRAM_GATED=-3,MAIA_PSRAM_CHANGED=-4,MAIA_PSRAM_BUSY=-5,
    MAIA_PSRAM_FAILED=-6,MAIA_PSRAM_VERIFY=-7,MAIA_PSRAM_TIMEOUT=-8,MAIA_PSRAM_IO=-9
} maia_psram_result_t;
typedef struct maia_psram_supply_report {
    uint32_t stage,dref,mul,adopted,writes,before[2],after[2];
} maia_psram_supply_report_t;
/* Boot/core0/IRQs-off, CPU1 parked, code/data internal SRAM, exclusive VO2 and
 * all PSRAM/MPLL consumers. Boot-lifetime ownership; call before clock claims.
 * Requires rev1.3/schema1+ and nonzero dedicated1900mV eFuse pair with no RS
 * failure. Adopts only matching enabled SW-owned/non-bypass/ripple-suppressed
 * supply, without writes. For disabled supply requires PSRAM core+PLL gates,
 * MPLL circuit and output clock off, then configures trim/owner/ripple/enable.
 * Preserves other fields. OK means digital programming/readback, NOT measured
 * voltage, stabilization, discharge, device power-on/reset or memory readiness.
 * Caller must qualify electrical/device startup before enabling consumers.
 * No power-off/release/voltage-adjust API. INVALID/CONTEXT/prior FAILED/duplicate
 * refuse without MMIO. Pre-store conflict/change refuses; any store failure
 * sticky until reset with no blind rollback. Fixed bounded work, no waits/heap.
 * Not thread/ISR/SMP-safe; report contains observations, not an ownership key. */
maia_psram_result_t maia_esp32p4_psram_supply1900(maia_psram_supply_report_t *);
typedef struct maia_psram_snapshot {
    uint32_t roots[MAIA_PSRAM_ROOT_WORDS],roots_after[MAIA_PSRAM_ROOT_WORDS];
    uint32_t before[MAIA_PSRAM_CONTROLLER_WORDS],after[MAIA_PSRAM_CONTROLLER_WORDS];
    uint32_t roots_valid,controller_samples;
} maia_psram_snapshot_t;
/* Boot/core0/IRQs-off/exclusive access only, not thread/ISR-safe. Required report
 * initialized before context/MMIO checks. INVALID(null)/CONTEXT cause no MMIO.
 * Read-only bounded whitelist: RCC, VO2/eFuse and PSRAM controllers only; no flash
 * controller, PSRAM array, cache/MMU table/selector, FIFO pop or clear-on-read.
 * Conservative access policy requires PSRAM SYS/PLL/core gates on, dual MSPI
 * resets off. Otherwise GATED with root evidence and zero controller reads.
 * Fresh RCC comparison before each controller read; roots rechecked at end.
 * OK means two matching observations, not atomic snapshot/readiness/ownership/
 * initialization/PSRAM identity, frequency, allocation or coherency proof.
 * CHANGED retains partial evidence and sample count; no retry/repair/store.
 * No polling/waits/heap or changes to controller/power/cache/clock state. */
maia_psram_result_t maia_esp32p4_psram_snapshot(maia_psram_snapshot_t *);
typedef struct maia_psram_mode_report {
    uint32_t reg,bits,stage,setup_attempts,restore_attempts,triggered,polls,saw_busy,command,value,restored;
    uint32_t before[10],after[10];
} maia_psram_mode_report_t;
/* Boot/core0/IRQs-off/exclusive, all code/data internal SRAM, no PSRAM AXI users.
 * INHERITED powered/timed HEX device only: not cold PSRAM initialization.
 * Accept only mode-reg addresses0,2,4,8 and budget1..100000. Read4040/16-bit cmd,
 * 32-bit address,12 dummy bits as preserved200M HEX register protocol; reg0/2 read
 * 16bits, reg4/8 read8. Caller must first qualify device/latency applicability.
 * No PSRAM device write, array, clock, LDO, MMU/cache or IRQ changes. Temporarily
 * configure only PSRAM command port, poison receive word, issue bounded read;
 * compare roots/untouched AXI side and restore saved registers only after idle.
 * MSPI2 AXI requests disabled and both FSMs idle required. No ROM SPI helpers.
 * Triggered means a start-write attempt, not an ACK; response must replace poison.
 * Report initialized; invalid/context and prior FAILED cause no MMIO. BUSY/
 * GATED/CHANGED before claim do not store. Partial write/timeout/lost ownership
 * sticky until reboot; no retry/abort/unsafe active restore. Bad/stale/foreign
 * vendor response returns IO after guarded restore, remains sticky. Successful
 * fully restored calls may repeat. No deadline, cold-init or memory-ready claim. */
maia_psram_result_t maia_esp32p4_psram_read_mode(uint32_t reg,uint32_t poll_budget,maia_psram_mode_report_t *);
typedef struct maia_psram_clock_report {
    uint32_t stage,attempts,restored,before[3],after[3];
} maia_psram_clock_report_t;
/* Boot/core0/IRQs-off/exclusive, internal SRAM only, no external-memory clients.
 * Qualified Tab5 40MHz XTAL precondition; nominal20MHz, not a frequency reading.
 * Warm powered HEX profile only: calibrated VO2 already enabled, AXI off, both
 * FSMs idle, MPLL/core-div1 and both bus-div4 words30103. No PLL programming,
 * voltage/device-mode/array/cache/MMU/reset writes. Private ownership survives
 * successful start; stop requires exact unchanged roots/controllers (temporary
 * read_mode calls must have fully restored). Core gate off before mux change;
 * bus divisors changed after XTAL selected. Stop restores bus divisors before
 * original mux. No controller reads while core gated. Unrelated RCC bits kept.
 * Start duplicate/stop without claim BUSY, invalid/context/prior FAILED no MMIO.
 * Any post-claim failure sticky until reboot: no retry or uncertain rollback.
 * Successful stop releases claim for another cycle. Caller reports never tokens.
 * No cold-init, array/coherency or continuous-display capability is established. */
maia_psram_result_t maia_esp32p4_psram_xtal20_start(maia_psram_clock_report_t *);
maia_psram_result_t maia_esp32p4_psram_xtal20_stop(maia_psram_clock_report_t *);
typedef struct maia_psram_mpll_report {
    uint32_t stage,polls,restored,div,repeat,mhz,power,clock,calibration;
} maia_psram_mpll_report_t;
typedef struct maia_psram_mpll_init_report {
    uint32_t stage,polls,writes,route_restored,saw_not_done,cal_before,cal_after;
    uint32_t before[3],after[3],mhz;
} maia_psram_mpll_init_report_t;
/* Exclusive boot/core0/IRQs-off/internal SRAM, all analog/MPLL clients stopped.
 * Requires genuine native XTAL20 claim, closed AXI/idle PSRAM and powered MPLL/
 * supply/analog clock (not power-on from an unpowered domain). Programs fixed
 *400MHz from40MHz XTAL using pinned vendor DHREF/RSTB/divisor/calibration order.
 * Cycles the MPLL circuit power bit only (PSRAM supply stays on), then
 * recalibrates rather than relying on inherited CAL_END; requires observed END0
 * then END1 and readback. Keeps calibrated analog configuration; restores analog
 * routing, MPLL power and CAL_STOP. No supply voltage/device-mode changes; not full cold
 * PSRAM startup or clock waveform proof. budget1..100000 bounds individual waits.
 * Invalid/context/BUSY before claim do not store; post-claim failure sticky until
 * reset, no unsafe rollback. OK alone does not qualify PSRAM data/DMA margins. */
maia_psram_result_t maia_esp32p4_psram_mpll_initialize400(uint32_t,maia_psram_mpll_init_report_t *);
typedef struct maia_psram_fast_report {
    maia_psram_mpll_report_t mpll;
    maia_psram_clock_report_t clock;
    uint32_t mhz;
} maia_psram_fast_report_t;
/* Same exclusive boot context as xtal20, plus no analog-I2C/calibration clients.
 * Start requires genuine native XTAL20 and X8 claims (known200M device latency),
 * AXI closed/controller restored. Reads existing powered/calibrated MPLL divisor
 * twice through bounded native analog-I2C port, restoring routing. Accepts only
 * MPLL400MHz; stages PSRAM pad drive2/DQS enable before raising clock, restores
 * all20 pads on stop after returning to XTAL. Enables/restores both controller
 * DLL timing-calibration bits as vendor startup does. Selects nominal PSRAM100 or200MHz
 * via divider4 or2, no PLL/power/
 * device timing writes. These are calculated rates, not measured clock signals.
 * Caller must qualify actual data/DQS at the selected rate before using memory.
 * Stop requires all clients stopped/window closed and restores XTAL20. Then
 * xtal20_stop may restore original firmware clocks. Duplicate/no claim BUSY;
 * invalid/context/prior fault no MMIO. Any post-claim failure sticky until reset,
 * no unsafe rollback or retries. Reports are observations, never ownership keys.
 * Analog command port progress is not restored; no analog register write occurs.
 * Bounded polls are not a bus-stall or hard latency guarantee. */
maia_psram_result_t maia_esp32p4_psram_fast_start(uint32_t mhz,uint32_t budget,maia_psram_fast_report_t *);
maia_psram_result_t maia_esp32p4_psram_fast_stop(maia_psram_clock_report_t *);
typedef struct maia_psram_transfer_report {
    uint32_t completed,chunks,polls,command,restored,write_attempted;
} maia_psram_transfer_report_t;
typedef struct maia_psram_profile_report {
    uint32_t stage,write_attempts,polls,restored,before[3],after[3];
} maia_psram_profile_report_t;
/* Boot/core0/IRQs-off/exclusive native XTAL20 claim, closed AXI/idle controllers,
 * powered/readable AP32MiB HEX/X16 device; not a cold power/supply initializer.
 * restore0 saves MR0/1,MR4,MR8 and programs fixed200M latency/drive/burst profile
 * via qualified native command port; accepts differing mode fields, not guessed
 * identity/reserved/interface values. MR0 and MR4 use vendor16bit writes, MR8
 * uses qualified one-byte write (avoids reading past vendor's byte object).
 * Readback preserves all other fields. Existing X8 claim may then be acquired.
 * restore1 requires genuine profile claim, X16 restored, XTAL20, no other clients;
 * restores original device profile after data/window/cache users are stopped.
 * budget1..100000; invalid/context/duplicate/no-claim refuse before I/O. Any
 * post-claim failure sticky until reset, no blind rollback/ACK claim. Reports
 * contain attempts and actual readback, not evidence of a device write ACK. */
maia_psram_result_t maia_esp32p4_psram_profile200(unsigned restore,uint32_t budget,maia_psram_profile_report_t *);
typedef struct maia_psram_mode_switch_report {uint32_t before,after,stage;} maia_psram_mode_switch_report_t;
/* Same boot-exclusive context as clock/array APIs; no clients across switch.
 * restore0 claims exact MR8=4B and writes0B (X8) via C0C0/address8/one-byte data,
 * no dummy, preserving other mode registers. restore1 requires private claim and
 * exact0B then writes4B. Both verify device readback and restore command/FIFO.
 * Invalid/context/duplicate/no-claim/prior FAILED do no MMIO. Any attempted
 * programming failure sticky until reboot. Successful stop releases claim.
 * No array/clock/cache/MMU changes; no arbitrary mode writer/reset is exposed. */
maia_psram_result_t maia_esp32p4_psram_x8_mode(unsigned restore,uint32_t budget,maia_psram_mode_switch_report_t *);
/* Boot-only reserved training aperture: physical bytes[0,4096), not a CPU
 * mapping/allocation API. Caller owns whole PSRAM exclusively, executes with
 * core0/IRQs off/internal SRAM; data/report disjoint and internal SRAM too.
 * Requires successful xtal20_start/x8_mode(0), qualified200M latency profile.
 * offset/length multiples4, length1..4096, read/write exactly0/1, budget1..100000
 * per64-byte command. INVALID/CONTEXT/prior FAILED do no MMIO. Bounded native
 * port of SDK DQS training read/write chunk algorithm; no ROM SPI helpers.
 * No flash/device-mode/clock/voltage/cache/MMU writes. Temporary command/FIFO
 * state restored after each successful chunk before committing read bytes.
 * Completed counts committed read or issued/idle write bytes, NOT device ACK.
 * Writes need independent readback. Partial transfer/error may already have
 * modified array; no rollback/retry after uncertain transfer. Failure sticky
 * until reboot after claim. Read buffers may contain completed prefix on error.
 * Training aperture must stay reserved from all clients until startup finishes;
 * caller saves/restores original contents if preservation is required.
 * This does not establish cold initialization, mapping or framebuffer bandwidth. */
maia_psram_result_t maia_esp32p4_psram_boot_transfer(uint32_t offset,void *data,
    uint32_t length,unsigned write,uint32_t budget,maia_psram_transfer_report_t *);
#define MAIA_PSRAM_WINDOW_BYTES 0x200000U
#define MAIA_PSRAM_WINDOW_VADDR 0x4B800000U
typedef struct maia_psram_cache_report {uint32_t stage,size,line,ways;} maia_psram_cache_report_t;
typedef struct maia_psram_window_report {
    uint32_t stage,vaddr,bytes,physical,restored,polls,mmu_before[32],mmu_after[32];
} maia_psram_window_report_t;
/* Boot/core0/IRQs-off, exclusive ALL cache/external memory clients, code/data in
 * qualified low internal SRAM/ROM only. Cache boot uses documented immutable ROM
 * boot/cache-enable APIs as SDK RAM-app startup does, no vendor runtime/header.
 * Global cache/flash-MMU initialization is a boot-lifetime ownership change,
 * not reversible by window_stop. ROM internal waits have no measured WCET.
 * Reject encryption, invalid context/duplicates/failed initialization. */
maia_psram_result_t maia_esp32p4_psram_cache_boot(maia_psram_cache_report_t *);
/* Same boot exclusivity; cache_boot/xtal20_start/x8_mode(0) claims required.
 * Fixed reserved physical[0,2MiB) -> VADDR[4B800000,4BA00000),32x64KiB pages.
 * All clients must stop using returned address before sync/stop, no ISR/task/SMP
 * use yet. No allocator/full-capacity/cold-init/bandwidth claim. Window setup
 * ports SDK controller/pads/CS/MMU config, enables AXI only after guarded staging.
 * sync writes back/invalidates ONLY owned window and bounds drain polls.
 * stop syncs, closes AXI, restores saved PSRAM config/pads/MMU/index; it does NOT
 * restore overwritten array bytes or global cache boot state. Caller owns data
 * preservation. Invalid/context/prior FAILED and duplicate requests no MMIO;
 * any post-claim error sticky/reboot, no unsafe automatic rollback/retry.
 * MMIO loops bounded; ROM cache internal wait bounds are not established. */
maia_psram_result_t maia_esp32p4_psram_window_start(uint32_t budget,maia_psram_window_report_t *);
maia_psram_result_t maia_esp32p4_psram_window_sync(uint32_t budget,maia_psram_window_report_t *);
maia_psram_result_t maia_esp32p4_psram_window_stop(uint32_t budget,maia_psram_window_report_t *);
#ifdef __cplusplus
}
#endif
#endif

/* Original MaiaRTOS MIT; register references: docs/audits/native-dsi-access.md. */
#ifndef MAIA_ESP32P4_DSI_ACCESS_H
#define MAIA_ESP32P4_DSI_ACCESS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_dsi_access_result {
    MAIA_DSI_ACCESS_OK=0, MAIA_DSI_ACCESS_INVALID=-1, MAIA_DSI_ACCESS_CONTEXT=-2,
    MAIA_DSI_ACCESS_BUSY=-3, MAIA_DSI_ACCESS_VERIFY=-4, MAIA_DSI_ACCESS_FAILED=-5, MAIA_DSI_ACCESS_TIMEOUT=-6, MAIA_DSI_ACCESS_IO=-7
} maia_dsi_access_result_t;
typedef enum maia_dsi_access_stage {
    MAIA_DSI_ACCESS_NONE=0, MAIA_DSI_ACCESS_GUARDED, MAIA_DSI_ACCESS_BUS,
    MAIA_DSI_ACCESS_CONFIG, MAIA_DSI_ACCESS_RELEASED, MAIA_DSI_ACCESS_VERIFIED
} maia_dsi_access_stage_t;
typedef struct maia_dsi_access_report {
    maia_dsi_access_stage_t completed;
    uint32_t attempted_mask; /* Bits0/1/2: DSI bus, config clock, reset release. */
    uint32_t sys_before,sys_after,phy_before,phy_after,reset_before,reset_after;
    uint32_t control,analog; /* Full calibrated active VO3 words, never written. */
    uint32_t version,power,phy_reset,bridge,dpi,fifo,phy_status;
} maia_dsi_access_report_t;
/* Boot-only exclusive register-access preparation, reservation until reset.
 * ESP32-P4 chip1.3, core0/IRQs-off/pre-scheduler; no ISR/task/SMP/concurrent use.
 * Caller owns RCC/VDMA/DSI/CSI/ISP/PMU, all producers quiescent; native VO3
 * acquisition must have succeeded and supply be stable by board policy. Hart/MIE
 * and matching register state are guards, not proof of ownership/analog voltage.
 * Require ALL MIPI SYS/PHY clocks gated, shared resets26..29 held, reset overrides
 * clear, VDMA bus clocks on/global/channels/abort/reset/FIFOs0, supported eFuse
 * calibration and active VO3 exactly matching native2500mV model. No adoption of
 * a running DSI or resetting active hardware. BUSY/VERIFY before writes is retryable.
 * Enable only DSI SYS12, PHY config0, then release ONLY DSI bridge RCC reset26.
 * Full readbacks/fresh RCC, VDMA and supply guards; preserve other fields,
 * CSI/ISP resets and all reference/DPI clocks/source/dividers. Only then read DSI
 * host/bridge and require host power/PHY analog+digital controls/bridge/DPI off,
 * FIFO empty. Version/status are observations, not an analog-ready certificate.
 * No PLL/source/ref-clock/test-port/lane/PHY power/video/DMA/IRQ/panel/PSRAM writes.
 * Finite steps, no waits/heap/locks, no retry/rollback after first write attempt:
 * failed programming is sticky FAILED, may leave clocks/reset changed. Success
 * makes duplicates BUSY without MMIO. Required report is always initialized,
 * last completed stage and conservative attempts reported. No release API.
 * OK means safe configuration access with transmit controls off, not initialized
 * PHY/lanes, settling, functional display or a complete native desktop. */
maia_dsi_access_result_t maia_esp32p4_dsi_open_config(maia_dsi_access_report_t *report);
typedef enum maia_dsi_reference_stage {
    MAIA_DSI_REFERENCE_NONE=0, MAIA_DSI_REFERENCE_GUARDED,
    MAIA_DSI_REFERENCE_PROVIDER, MAIA_DSI_REFERENCE_CONFIG_OFF, MAIA_DSI_REFERENCE_SOURCE,
    MAIA_DSI_REFERENCE_CONFIG,
    MAIA_DSI_REFERENCE_CLOCK, MAIA_DSI_REFERENCE_VERIFIED
} maia_dsi_reference_stage_t;
typedef struct maia_dsi_reference_report {
    maia_dsi_reference_stage_t completed;
    uint32_t attempted_mask; /* Bits0..4: provider, config off, mux, config restore, ref on. */
    uint32_t divider_word,lp_gate,nominal_hz; /* Model480MHz/div24, NOT measured. */
    uint32_t provider_before,provider_after,source_before,source_after,phy_before,phy_after;
} maia_dsi_reference_report_t;
/* Same exclusive boot/supply contract, only after successful native open_config
 * in this boot (internal claim). Requires still-shutdown DSI, same access/reset
 * pattern, inactive VDMA, reference/DPI/CSI clocks off. Caller guarantees stable
 * inherited SPLL480MHz/40MHz XTAL; this is NOT cold PLL startup or frequency
 * measurement. Verify LP XTAL/SPLL gates on and shared REF20 divider exactly24;
 * never reprogram that divider, root PLL/power or LP gates. Refuse other dividers.
 * Enable shared REF20 provider only if off, select DSI source0 only if needed,
 * gate config clock around any mux change, restore config, then enable reference
 * bit1. No DSI MMIO while config clock is off. Preserve other fields; shared gate stays
 * enabled until reset; no general clock manager/refcount/runtime teardown.
 * Fresh RCC/provider/LP/divider/VDMA/eFuse/PMU/DSI-off guards and full readbacks.
 * Finite steps/no waits/rollback; failure after guards sticky FAILED until reset,
 * duplicate successful call BUSY without MMIO. Required report always initialized.
 * No PHY test-port/PLL/analog/lane/panel/video writes; OK is reference routing and
 * nominal software frequency only, not PLL lock, clock measurement or display. */
maia_dsi_access_result_t maia_esp32p4_dsi_reference20(maia_dsi_reference_report_t *report);
typedef struct maia_dsi_phy_report {
    uint32_t completed; /*0:none 1:guarded 2:lanes 3:host 4:analog 5:reset release
                         6:clock 7:force PLL 8:programmed 9:lock 10:stop 11:verified. */
    uint32_t writes,programmed,polls,status;
    uint32_t power,phy_reset,lanes,nominal_mbps;
} maia_dsi_phy_report_t;
/* Fixed two-lane960Mbps PHY, nominal20MHz reference, exclusive boot contract.
 * Only after this module's successful reference20 claim. Require unchanged
 * reference routing/divider/provider, quiet VDMA, calibrated VO3, bridge/DPI off,
 * host/PHY shutdown/reset and no HS clock request; initial lock must be low.
 * Adapted SDK initialization and test-port sequence, attributed Apache-2.0.
 * Mask/check lanes, host power, analog shutdown release, digital reset release,
 * clock lane enable and forcePLL; program five PHY bytes (M48/N1/hsfreq1A).
 * Test-interface readback verifies writable controls only, not hidden PHY bytes.
 * Poll lock bit0 then all lane-stop bits94, finite poll_budget1..100000; no
 * real-time/settling guarantee. TIMEOUT=-6 leaves state as-is/sticky FAILED.
 * Fresh RCC/VDMA/supply/DSI guards between steps/polls; no rollback/retry on any
 * programming failure. Duplicate success BUSY without MMIO, lifetime until reset.
 * No packet/HS request, panel command, video/DMA/PSRAM setup or complete display.
 * Powered PHY may drive LP idle levels; caller exclusively owns attached lanes.
 * Model960Mbps not measured. Report required/always initialized except NULL;
 * writes includes conservative attempted stores, programmed counts complete bytes. */
maia_dsi_access_result_t maia_esp32p4_dsi_phy960(uint32_t poll_budget,maia_dsi_phy_report_t *report);
#define MAIA_DSI_COMMAND_WORDS 14U
typedef struct maia_dsi_command_report {
    uint32_t completed; /*0:none 1:guarded 2..15:verified table steps 16:complete. */
    uint32_t attempted_mask,status,fifo,escape_hz,timeout_hz; /* Frequencies modeled. */
    /* Ordered MODE,CLKMGR,DATA_TMR,CLK_TMR,PCKHDL,TO,HS_RD,LP_RD,HS_WR,LP_WR,
     * BTA,MAX_RD,PHY_IF,LPCLK. Complete before/after words, unchanged on skipped steps. */
    uint32_t before[MAIA_DSI_COMMAND_WORDS],after[MAIA_DSI_COMMAND_WORDS];
} maia_dsi_command_report_t;
/* Same exclusive chip1.3 boot/clock/supply contract, only after native PHY960
 * success. Require powered two-lane PHY locked/all-stopped, no HS clock request,
 * quiet VDMA/bridge/DPI and generic command/write/read FIFOs empty/not busy.
 * Configure command mode, modeled escape120/7MHz/timeout10MHz, SDK switch times,
 * CRC/ECC/HS-EoTp (LP-EoTp off), read-time6000/stop-wait3F. Hardware timeout
 * COUNTS zero (SDK defaults); no transaction API/unbounded wait is introduced.
 * Every future transfer requires separately bounded native completion handling.
 * Finite14 masked/readback steps, skip matching words, preserve unrelated fields;
 * fresh complete RCC/supply/VDMA/PHY/FIFO/table guards before each step and final.
 * Failure after initial guards sticky FAILED until reset; no retry/rollback.
 * Last completed phase/attempts reported; duplicate success BUSY without MMIO.
 * Required report initialized; NULL INVALID/context CONTEXT, incompatible initial
 * state BUSY, unstable fresh initial guard VERIFY without writes. No polling.
 * No header/payload/IRQ acknowledgment/panel/HS/video/DMA/PSRAM writes or actual
 * packet transfer; OK proves checked host setup, not working panel/display. */
maia_dsi_access_result_t maia_esp32p4_dsi_command_config(maia_dsi_command_report_t *report);
typedef struct maia_dsi_short_report {
    uint32_t completed; /*0:none 1:guarded 2:LP configured 3:header attempted
                         4:host drained/PHY stopped observed 5:verified. */
    uint32_t header,header_attempted,command_before,command_after;
    uint32_t polls,status,fifo,error0,error1,saw_busy;
} maia_dsi_short_report_t;
/* Synchronous boot-exclusive DCS short write, only after native command_config.
 * vc0..3, command/parameter0..255, parameter_count0..1; unused parameter must0.
 * poll_budget1..100000 bounds completion observations (NOT elapsed-time limit).
 * Same exclusive clock/supply/producer contract, no ISR/task/SMP/reentrant use.
 * Require stable configured host/PHY, empty generic FIFOs/no read-busy, all lane
 * stops and lock; ACK/TE/BTA disabled. Select only this packet's LP bit16 or17,
 * preserve other CMD_MODE_CFG fields, encode VC in header (no RX VC programming).
 * Publish exactly ONE header; no payload/long/read/BTA/ACK/HS/video operation.
 * Guard clocks before MMIO, preserve table/root/supply; allow lane-stop change
 * during TX while lock stays set. Observe FIFO drained and all-stopped together.
 * Status errors are accumulated, including preexisting flags (IO=-7); status
 * reads may consume diagnostics, so caller exclusively owns them. No IRQ-mask/
 * clear writes. No GEN_HDR readback: store is publication, not a readable config.
 * OK means host drain/stop OBSERVED, not panel ACK or verified wire completion;
 * fast packets may finish without sampled busy (saw_busy records observation).
 * Success permits next exclusive call; LP selection remains set. Timeout/error
 * after guards sticky FAILED until reset, no rollback/retry: packet may already
 * have been accepted. header_attempted set BEFORE store. Initial busy returns
 * BUSY without writes; preexisting error is sticky IO even before publication.
 * Required report initialized; invalid args INVALID/context CONTEXT without I/O.
 * No panel-specific policy, memory/DMA/IRQ or PSRAM access. */
maia_dsi_access_result_t maia_esp32p4_dsi_dcs_short(uint32_t vc,uint32_t command,
    uint32_t parameter_count,uint32_t parameter,uint32_t poll_budget,maia_dsi_short_report_t *report);
#define MAIA_DSI_LONG_MAX_BYTES 64U /* Policy cap, NOT a verified FIFO-depth claim. */
typedef struct maia_dsi_long_report {
    maia_dsi_short_report_t transfer; /* completed0:none 1:guarded 2:LP selected
                                     3:payload attempted 4:header attempted
                                     5:drain/stop observed 6:verified. */
    uint32_t payload_bytes_attempted,payload_words_attempted,space_polls,pre_header_fifo;
} maia_dsi_long_report_t;
/* Same boot-exclusive ownership/supply/context contract and native command claim.
 * Force DCS long39, vc0..3, payload length1..64 INCLUDING command byte. Caller
 * supplies readable immutable ordinary RAM (not MMIO), no alias with report.
 * Bytewise little-endian packing, zero unused last-word bytes, no tail overread.
 * LP bit19, ACK/TE/BTA off; stage all payload before one header with exact WC.
 * Space follows generic-interface full bit3 (SDK LL), not internal full bit19.
 * Initial/final checks require BOTH generic FIFO and GEN_BUFF empty/not full;
 * front FIFO empty alone is not completion. Shared total budget1..100000 covers payload-space/header/completion observations,
 * plus finite preflight/final checks. Length cap is policy; unknown FIFO depth may
 * cause bounded timeout BEFORE header. Do not infer streaming/chunking support.
 * No GEN_PLD or GEN_HDR readback; counters set before stores mean ATTEMPTS, not
 * confirmed bytes. Failure after guards (even partial payload/no header) poisons
 * BOTH short and long transport until reset. No retry/flush/rollback/retransmit.
 * OK allows another exclusive transfer: host drain/PHY stop observed, not panel
 * ACK or proof of payload/wire acceptance. Same status-read/fast-completion limits
 * as short API. Parameter bytes stay caller/panel policy; no automatic DCS command
 * conversion, panel initialization, read/ACK/HS/video/DMA/PSRAM operations. */
maia_dsi_access_result_t maia_esp32p4_dsi_dcs_long(uint32_t vc,const uint8_t *bytes,
    uint32_t length,uint32_t poll_budget,maia_dsi_long_report_t *report);
typedef struct maia_dsi_read_report {
    maia_dsi_short_report_t transfer; /*0:none 1:guarded 2:LP 3:MRPS attempted
       4:MRPS drained 5:BTA configured 6:read attempted 7:RX word popped
       8:RX idle/TX direction 9:host restored 10:verified. */
    uint32_t headers_attempted,words_received,word;
    uint32_t packet_before,vc_before,restore_attempted;
    uint8_t id[3]; /* View of requested3 bytes from one RX word, not RX length proof. */
} maia_dsi_read_report_t;
/* HARDWARE-TESTED bounded responses with escape divider7; see
 * native-dsi-bta-escape audit. No exact RX-length or panel model proof.
 * Fixed boot-exclusive VC0/RDDID04 read with MRPS3, only after native command
 * claim. Same root/supply/producer ownership; initial all FIFOs/buffers empty,
 * PHY locked/stopped/TX direction, TE/ACK/BTA off. Temporary LP read/MRPS bits,
 * BTA and RX VC0; two header attempts, no payload writes. MRPS changes panel's
 * protocol return limit, not image settings. Aggregate budget1..100000 covers
 * MRPS completion/response/idle loops, not elapsed time. At most ONE RX word
 * read, only when data available/readbusy0. Extra data is IO, never blindly drain.
 * Hardware strips response framing; word/low3 bytes do not prove exact response
 * length/model. Success requires no observed errors, RX empty/readbusy0, front+
 * internal buffers empty, PHY stopped and back to TX. Then restore full original
 * packet/RXVC/mode settings with guards/readback (MRPS3 remains protocol state).
 * Failure after guards poisons short/long/read until reset, no retry/rollback/
 * automatic restoration with RX possibly active. headers_attempted flags0:MRPS,
 * 1:DCS read, set before stores. Observed error latches accumulated, may be consumed.
 * Required report initialized, invalid budget/NULL INVALID/context CONTEXT;
 * missing claim/busy initial state BUSY without writes. No panel reset/visual
 * DCS setup, framebuffer/DMA/PSRAM or full display/native desktop claim. */
maia_dsi_access_result_t maia_esp32p4_dsi_read_id(uint32_t poll_budget,maia_dsi_read_report_t *report);
/* Fixed VC0/RDDPM0A with MRPS1. Same boot/context/guard/error/sticky/budget and
 * success-only host restoration contract as read_id; shares the same transport
 * claim. Low byte of word (also id[0]) is the returned power-mode byte view.
 * Other word bytes/id[1..2] are NOT requested payload and must not be decoded.
 * At most one FIFO word, no exact response-framing/length proof; no automatic
 * power-state interpretation. MRPS1 remains the panel's protocol return limit
 * after success. No arbitrary command/length API or visual/state writes. */
maia_dsi_access_result_t maia_esp32p4_dsi_read_power_mode(uint32_t poll_budget,maia_dsi_read_report_t *report);
#define MAIA_DSI_DPI_WORDS 19U
typedef struct maia_dsi_dpi_timing {
    uint32_t width,height,hsync,hback,hfront,vsync,vback,vfront;
} maia_dsi_dpi_timing_t;
typedef struct maia_dsi_dpi_report {
    uint32_t completed,attempted_mask; /*1:guarded 2:clock route 3..21:words 22:verified*/
    uint32_t clock_before,clock_after,nominal_hz;
    maia_dsi_short_report_t observation;
    uint32_t before[MAIA_DSI_DPI_WORDS],after[MAIA_DSI_DPI_WORDS];
} maia_dsi_dpi_report_t;
/* Stage inactive RGB565/VC0/burst+LP timing at fixed nominal240MHz/div3=80MHz,
 * PHY960Mbps/two lanes. Geometry comes from BSP, in pixels/lines. Same exclusive
 * chip1.3/core0/IRQs-off boot ownership and native command claim. Required args
 * immutable/nonaliasing. Range/overflow validation before I/O. Initial all FIFOs
 * idle, locked/stopped TX PHY, bridge/DPI off, DPI clock gated, TE/ACK/BTA/pattern
 * off. Checks fresh cached command/root/supply/VDMA settings around every step.
 * Routes clock while gated; does NOT enable it/provider/PLL or measure frequency.
 * Writes19 host/bridge config words with masked RMW/readback; no bridge update
 * latch, output/video-mode/HS/DMA/framebuffer/IRQ or panel command writes. Frame
 * BTA ACK stays off (activation policy pending). Success reserves stage until
 * reboot; duplicate BUSY without I/O. Failure after guards poisons transfers and
 * stage until reboot, no rollback/retry. Caller must establish panel-format and
 * active clock/latch/DMA ownership separately before video. No pixels claim. */
maia_dsi_access_result_t maia_esp32p4_dsi_dpi_stage(const maia_dsi_dpi_timing_t *,maia_dsi_dpi_report_t *);
#ifdef __cplusplus
}
#endif
#endif

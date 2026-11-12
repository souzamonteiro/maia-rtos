#include <maia/soc/esp32p4_ldo3.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
void maia_ldo3_test_reset(void);
int maia_ldo3_test_context(void);
uint32_t maia_ldo3_test_read(uintptr_t address);
void maia_ldo3_test_write(uintptr_t address,uint32_t value);
static uint32_t trim2,trim3,rs,ctrl,ana,sys,phy,initial_ctrl,initial_ana;
static unsigned context_ok,reads,writes,drop,corrupt_at;
static uint32_t corrupt_mask;
static uintptr_t addresses[64];
static uint32_t write_values[4];
static maia_ldo3_acquire_report_t report;
int maia_ldo3_test_context(void) { return (int)context_ok; }
uint32_t maia_ldo3_test_read(uintptr_t a) {
    CHECK(++reads<64U);addresses[reads]=a;uint32_t value=0U;
    if (a == MAIA_ESP32P4_EFUSE_MAC_SYS2_REG) value=trim2;
    else if (a == MAIA_ESP32P4_EFUSE_MAC_SYS3_REG) value=trim3;
    else if (a == MAIA_ESP32P4_EFUSE_RS_ERR0_REG) value=rs;
    else if (a == MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG) value=ctrl;
    else if (a == MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG) value=ana;
    else if (a == MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) value=sys;
    else if (a == MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG) value=phy;
    else CHECK(0);
    return reads == corrupt_at ? value^corrupt_mask : value;
}
void maia_ldo3_test_write(uintptr_t a,uint32_t value) {
    CHECK(++writes<=3U);write_values[writes]=value;
    if (a == MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG) {
        CHECK(((value^initial_ctrl)&~MAIA_ESP32P4_LDO3_CONTROL_MASK) == 0U);
        if (value&MAIA_ESP32P4_LDO3_XPD) CHECK((ana&MAIA_ESP32P4_LDO3_ANALOG_MASK) == 0xD5800000U);
        if (writes != drop) ctrl=value;
    } else {
        CHECK(a == MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG);
        CHECK(!(ctrl&MAIA_ESP32P4_LDO3_XPD)); /* Never retune a live rail. */
        CHECK(((value^initial_ana)&~MAIA_ESP32P4_LDO3_ANALOG_MASK) == 0U);
        if (writes != drop) ana=value;
    }
}
static void reset(int active) {
    maia_ldo3_test_reset();context_ok=1U;reads=0U;writes=0U;drop=0U;corrupt_at=0U;corrupt_mask=0x10000U;
    trim2=0x313U;trim3=(55U<<6U)|(33U<<14U)|(33U<<20U);rs=0U;sys=0U;phy=0U;
    ctrl=(0x84202020U&~MAIA_ESP32P4_LDO3_CONTROL_MASK) | (active ? 0x180U : 0x4E00U);
    ana=(0x08E54321U&~MAIA_ESP32P4_LDO3_ANALOG_MASK) | (active ? 0xD5800000U : 0xA0000000U);
    initial_ctrl=ctrl;initial_ana=ana;
}
static void unchanged(void) { CHECK(ctrl == initial_ctrl && ana == initial_ana && !writes); }
int main(void) {
    reset(0);CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_OK);
    CHECK(report.completed == MAIA_LDO3_ACQUIRE_VERIFIED && !report.adopted && report.may_have_changed);
    CHECK(report.dref == 13U && report.mul == 3U && writes == 3U && reads == 23U);
    CHECK((ctrl&MAIA_ESP32P4_LDO3_CONTROL_MASK) == 0x180U && (ana&MAIA_ESP32P4_LDO3_ANALOG_MASK) == 0xD5800000U);
    CHECK(!(write_values[2]&MAIA_ESP32P4_LDO3_XPD) && (write_values[3]&MAIA_ESP32P4_LDO3_XPD));
    unsigned saved=reads;CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_BUSY && reads == saved);
    reset(1);CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_OK && report.adopted && !report.may_have_changed);
    CHECK(report.completed == MAIA_LDO3_ACQUIRE_VERIFIED);unchanged();
    /* Every controlled active bit except XPD conflicts; unmasked bits do not. */
    for (unsigned bit=0U;bit<32U;++bit) {
        uint32_t mask=1UL<<bit;
        if (mask&(MAIA_ESP32P4_LDO3_CONTROL_MASK&~MAIA_ESP32P4_LDO3_XPD)) {
            reset(1);ctrl^=mask;initial_ctrl=ctrl;
            CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_BUSY);unchanged();
        }
        if (mask&MAIA_ESP32P4_LDO3_ANALOG_MASK) {
            reset(1);ana^=mask;initial_ana=ana;
            CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_BUSY);unchanged();
        }
    }
    for (unsigned n=1U;n<=3U;++n) {
        reset(0);drop=n;
        CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_VERIFY && writes == n && report.may_have_changed);
        CHECK((unsigned)report.completed == n); /* planned/analog/routed */
        saved=reads;CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_FAILED && reads == saved && writes == n);
    }
    /* Fresh-snapshot mismatch is write-free; programming-phase failure is sticky. */
    for (unsigned n=6U;n<=23U;++n) {
        if (n == 11U || n == 12U) continue; /* clock policy, tested separately */
        reset(0);corrupt_at=n;
        if (n == 6U || n == 8U || n == 19U || n == 21U) corrupt_mask=1U;
        unsigned expected_reads=n<=10U ? 10U : n>=19U ? 23U : n;
        CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_VERIFY && reads == expected_reads);
        if (n<=10U) {
            unchanged();corrupt_at=0U;
            CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_OK);
        } else {
            saved=reads;unsigned written=writes;
            CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_FAILED && reads == saved && writes == written);
        }
    }
    reset(1);sys=MAIA_ESP32P4_HP_MIPI_SYS_CLK_MASK;phy=MAIA_ESP32P4_HP_MIPI_PHY_CLK_MASK;
    CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_OK && report.adopted && !report.may_have_changed);
    unchanged(); /* Running consumers forbid changes, not read-only adoption. */
    reset(0);ana=(ana&~MAIA_ESP32P4_LDO3_ANALOG_MASK)|0xD5800000U;
    ctrl=(ctrl&~MAIA_ESP32P4_LDO3_CONTROL_MASK)|0x80U;initial_ana=ana;initial_ctrl=ctrl;
    CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_OK && writes == 1U); /* only enable */
    const uint32_t clock_bits[]={1U<<10U,1U<<11U,1U<<12U,1U,2U,16U,128U};
    for (unsigned i=0U;i<7U;++i) {
        reset(0);if (i<3U) sys=clock_bits[i];else phy=clock_bits[i];
        CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_BUSY);unchanged();
    }
    reset(0);context_ok=0U;
    CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_CONTEXT && !reads && !writes);
    CHECK(maia_esp32p4_ldo3_acquire2500(0) == MAIA_LDO3_INVALID && !reads);
    context_ok=1U;rs=8U;
    CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_EFUSE_ERROR);unchanged();
    rs=0U;trim2=0x312U;
    CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_UNSUPPORTED);unchanged();
    trim2=0x313U;CHECK(maia_esp32p4_ldo3_acquire2500(&report) == MAIA_LDO3_OK);
    puts("PASS: LDO3 adoption/conflicts, masked off-rail programming, dropped writes, stale snapshots, sticky failures, clocks and lifetime");
    return 0;
}

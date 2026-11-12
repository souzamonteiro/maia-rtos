#include <maia/boards/tab5_panel_reset.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static maia_i2c_lines_t bus;
static maia_tab5_reset_report_t report;
static uint8_t values[5],initial[5];
static unsigned calls,writes,pauses,fail_call,fail_pause,apply_on_error,corrupt_call;
static unsigned stage_at[64],normal_stage[64],low_calls,settle_calls;
static const uint8_t regs[]={3U,5U,7U,0x0BU,0x0DU};
static const uint8_t masks[]={0x30U,0x30U,0x30U,0x10U,0x10U};
static unsigned index_of(uint8_t reg) {
    for (unsigned i=0U;i<5U;++i) if (regs[i] == reg) return i;
    CHECK(0);return 0U;
}
static void safe(void) {
    CHECK(!((values[0]&0x10U) && !(values[2]&0x10U) && (values[1]&0x10U)));
    for (unsigned i=0U;i<5U;++i) CHECK(((values[i]^initial[i]) & (uint8_t)~masks[i]) == 0U);
}
maia_i2c_result_t maia_i2c_read_register8(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t *out) {
    CHECK(b == &bus && address == 0x43U && out);CHECK(++calls<64U);
    stage_at[calls]=(unsigned)report.completed;
    if (calls == fail_call) return MAIA_I2C_NACK;
    *out=(uint8_t)(values[index_of(reg)] ^ (calls == corrupt_call ? 0x80U : 0U));
    return MAIA_I2C_OK;
}
maia_i2c_result_t maia_i2c_write_register8(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t value) {
    CHECK(b == &bus && address == 0x43U);CHECK(++calls<64U);++writes;
    stage_at[calls]=(unsigned)report.completed;
    if (calls != fail_call || apply_on_error) { values[index_of(reg)]=value;safe(); }
    return calls == fail_call ? MAIA_I2C_NACK : MAIA_I2C_OK;
}
static void drive(void *context,unsigned mask,unsigned release) { (void)context;(void)mask;(void)release;CHECK(0); }
static unsigned sample(void *context) { (void)context;CHECK(0);return 0U; }
static maia_i2c_result_t pause_step(void *context) {
    (void)context;++pauses;safe();
    if (pauses<=20000U) {
        CHECK((values[0]&0x30U) == 0x30U && !(values[1]&0x30U) && !(values[2]&0x30U));
        CHECK(report.completed == MAIA_TAB5_RESET_DRIVE_ENABLED);
        low_calls=calls;
    } else {
        CHECK((values[0]&0x30U) == 0x20U && (values[1]&0x30U) == 0x20U && !(values[2]&0x30U));
        CHECK((values[3]&0x10U) && (values[4]&0x10U));
        CHECK(report.completed == MAIA_TAB5_RESET_LCD_RELEASED);settle_calls=calls;
    }
    return pauses == fail_pause ? MAIA_I2C_TIMEOUT : MAIA_I2C_OK;
}
static void reset(void) {
    const uint8_t start[]={0x4FU,0xDFU,0xF3U,0xA5U,0x42U};
    memcpy(values,start,5);memcpy(initial,start,5);
    calls=0U;writes=0U;pauses=0U;fail_call=0U;fail_pause=0U;apply_on_error=0U;corrupt_call=0U;
    bus.context=0;bus.drive=drive;bus.sample=sample;bus.pause=pause_step;
    report.completed=MAIA_TAB5_RESET_VERIFIED;report.may_have_changed=9U;
}
int main(void) {
    reset();CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_OK);
    CHECK(report.completed == MAIA_TAB5_RESET_VERIFIED && report.may_have_changed == 1U);
    CHECK(calls == 31U && writes == 7U && pauses == 40000U);
    unsigned total=calls,low=low_calls,settle=settle_calls;
    memcpy(normal_stage,stage_at,sizeof normal_stage);
    for (unsigned applied=0U;applied<2U;++applied) for (unsigned n=1U;n<=total;++n) {
        reset();fail_call=n;apply_on_error=applied;
        CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_NACK && calls == n);
        CHECK((unsigned)report.completed == normal_stage[n]);
        CHECK(report.may_have_changed == (n>5U ? 1U : 0U));
        safe();
    }
    const unsigned delays[]={1U,1000U,20000U,20001U,30000U,40000U};
    for (unsigned i=0U;i<sizeof delays/sizeof delays[0];++i) {
        reset();fail_pause=delays[i];
        CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_TIMEOUT && pauses == delays[i]);
        CHECK(calls == (delays[i]<=20000U ? low : settle));
        CHECK(report.completed == (delays[i]<=20000U ? MAIA_TAB5_RESET_DRIVE_ENABLED : MAIA_TAB5_RESET_LCD_RELEASED));
    }
    /* Seven RMW verification reads and final snapshot: mismatch must stop. */
    const unsigned corruptions[]={8U,11U,14U,17U,20U,23U,26U,27U,28U,29U,30U,31U};
    for (unsigned i=0U;i<sizeof corruptions/sizeof corruptions[0];++i) {
        reset();corrupt_call=corruptions[i];
        CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_VERIFY && calls == corrupt_call);
        CHECK((unsigned)report.completed == normal_stage[corrupt_call]);
    }
    reset();values[0]|=0x10U;values[2]&=(uint8_t)~0x10U; /* Already driven high. */
    CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_BUSY && !writes && !pauses && calls == 5U);
    CHECK(report.completed == MAIA_TAB5_RESET_NONE && !report.may_have_changed);
    reset();CHECK(maia_tab5_panel_reset(0,&report) == MAIA_I2C_INVALID && !calls);
    CHECK(report.completed == MAIA_TAB5_RESET_NONE && !report.may_have_changed);
    CHECK(maia_tab5_panel_reset(&bus,0) == MAIA_I2C_INVALID && !calls);
    bus.pause=0;CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_INVALID && !calls);
    uint32_t seed=0x51AC3D19U;
    for (unsigned run=0U;run<64U;++run) {
        reset();for (unsigned i=0U;i<5U;++i) {
            seed^=seed<<13U;seed^=seed>>17U;seed^=seed<<5U;values[i]=(uint8_t)seed;
        }
        if ((values[0]&0x10U) && !(values[2]&0x10U)) values[1]&=(uint8_t)~0x10U;
        memcpy(initial,values,5);
        CHECK(maia_tab5_panel_reset(&bus,&report) == MAIA_I2C_OK && pauses == 40000U);
        safe();CHECK(report.completed == MAIA_TAB5_RESET_VERIFIED);
    }
    puts("PASS: reset ordering, 64 initial states, 62 bus failures, 6 delay failures, readback mismatches and no driven-high LCD");
    return 0;
}

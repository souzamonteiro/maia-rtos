#include <maia/drivers/pi4ioe5v6408.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static maia_i2c_lines_t bus;
static unsigned reads,writes,fail_at,corrupt;
static uint8_t current,written;
static maia_i2c_result_t error;
static void reset(uint8_t old) { reads=0U;writes=0U;fail_at=0U;corrupt=0U;current=old;error=MAIA_I2C_NACK; }
maia_i2c_result_t maia_i2c_read_register8(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t *out) {
    CHECK(b == &bus && (address == 0x43U || address == 0x44U));(void)reg;
    ++reads;
    if (!out) return MAIA_I2C_INVALID;
    if (reads+writes == fail_at) return error;
    *out=(uint8_t)(current ^ (writes ? corrupt : 0U));return MAIA_I2C_OK;
}
maia_i2c_result_t maia_i2c_write_register8(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t value) {
    CHECK(b == &bus && (address == 0x43U || address == 0x44U));(void)reg;
    ++writes;written=value;current=value; /* An error may follow application. */
    return reads+writes == fail_at ? error : MAIA_I2C_OK;
}
int main(void) {
    for (unsigned old=0U;old<256U;++old) for (unsigned mask=0U;mask<256U;++mask) {
        reset((uint8_t)old);uint8_t value=(uint8_t)(0xA5U & mask);
        uint8_t expected=(uint8_t)((old & ~mask) | value);
        CHECK(maia_pi4io_update_config(&bus,0x43U,MAIA_PI4IO_OUTPUT,(uint8_t)mask,value) == MAIA_I2C_OK);
        CHECK(current == expected && ((current ^ old) & ~mask) == 0U);
        CHECK(writes == (expected != old ? 1U : 0U) && reads == (writes ? 2U : 1U));
    }
    for (unsigned stage=1U;stage<=3U;++stage) {
        reset(0xA5U);fail_at=stage;error=stage == 2U ? MAIA_I2C_TIMEOUT : MAIA_I2C_NACK;
        CHECK(maia_pi4io_update_config(&bus,0x44U,MAIA_PI4IO_OUTPUT,0x10U,0x10U) == error);
        CHECK(reads+writes == stage && writes == (stage>1U ? 1U : 0U));
        if (stage>1U) CHECK(current == 0xB5U); /* No rollback or hidden retry. */
    }
    for (unsigned bit=1U;bit<=128U;bit<<=1U) {
        reset(0xA5U);corrupt=bit;
        CHECK(maia_pi4io_update_config(&bus,0x43U,MAIA_PI4IO_OUTPUT,0x10U,0x10U) == MAIA_I2C_VERIFY);
        CHECK(written == 0xB5U && reads == 2U && writes == 1U);
    }
    const uint8_t regs[]={3U,5U,7U,0x0BU,0x0DU};
    for (unsigned addr=0U;addr<128U;++addr) for (unsigned reg=0U;reg<256U;++reg) {
        int allowed=0;for (unsigned i=0U;i<sizeof regs;++i) if (regs[i] == reg) allowed=1;
        allowed=allowed && (addr == 0x43U || addr == 0x44U);
        reset(0);uint8_t out=0xCCU;
        maia_i2c_result_t r=maia_pi4io_read_config(&bus,(uint8_t)addr,(uint8_t)reg,&out);
        CHECK(r == (allowed ? MAIA_I2C_OK : MAIA_I2C_INVALID));
        CHECK(reads == (allowed ? 1U : 0U) && !writes && out == (allowed ? 0U : 0xCCU));
        reset(0);
        CHECK(maia_pi4io_update_config(&bus,(uint8_t)addr,(uint8_t)reg,0xFFU,0) == (allowed ? MAIA_I2C_OK : MAIA_I2C_INVALID));
        CHECK(reads == (allowed ? 1U : 0U) && !writes);
    }
    reset(0);
    CHECK(maia_pi4io_update_config(&bus,0x43U,5U,0x10U,0x01U) == MAIA_I2C_INVALID && !reads && !writes);
    puts("PASS: 65536 bit-preserving updates, no-op, stage failures, full-byte verify and register/address policy");
    return 0;
}

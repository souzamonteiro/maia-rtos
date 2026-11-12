/* Virtual slave checks observed wire bytes and failure cleanup, not MMIO. */
#include <maia/boards/tab5_touch.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
typedef struct slave {
    unsigned master, slave_low, active, bit, reading, raised;
    unsigned starts, stops, bytes, pauses, fail_pause, nack_byte;
    unsigned stuck_scl, stuck_sda, stretch, remaining_stretch, fail_stop;
    unsigned master_nacks, high_since, address_phase;
    uint8_t accumulator, wire[64], reply;
} slave_t;
static unsigned sample(void *context) {
    slave_t *s = context;
    unsigned levels = s->master;
    if (s->slave_low || s->stuck_sda || (s->fail_stop && s->stops >= s->fail_stop)) levels &= ~MAIA_I2C_SDA;
    if (s->stuck_scl || s->remaining_stretch) {
        levels &= ~MAIA_I2C_SCL;
        if (s->remaining_stretch) --s->remaining_stretch;
    }
    if ((levels & MAIA_I2C_SCL) && s->active && s->raised && !s->high_since)
        s->high_since=s->pauses;
    return levels;
}
static void drive(void *context, unsigned mask, unsigned release) {
    slave_t *s = context;
    unsigned old = s->master;
    s->master = release ? old | mask : old & ~mask;
    if ((old & MAIA_I2C_SCL) && (s->master & MAIA_I2C_SCL) && ((old ^ s->master) & MAIA_I2C_SDA)) {
        if (!(s->master & MAIA_I2C_SDA)) {
            ++s->starts;s->active=1U;s->bit=0U;s->reading=0U;s->raised=0U;s->accumulator=0U;s->address_phase=1U;
        } else { ++s->stops;s->active=0U;s->slave_low=0U; }
    }
    if (!(old & MAIA_I2C_SCL) && (s->master & MAIA_I2C_SCL) && s->active) {
        s->raised=1U;s->remaining_stretch=s->stretch;
        if (s->bit < 8U) {
            if (s->reading) s->slave_low=!(s->reply & (0x80U >> s->bit));
            else s->accumulator=(uint8_t)((s->accumulator << 1U) | ((s->master & MAIA_I2C_SDA) != 0U));
        } else if (s->reading) {
            CHECK(s->master & MAIA_I2C_SDA);++s->master_nacks;s->slave_low=0U;
        } else {
            CHECK(s->bytes < sizeof s->wire);s->wire[s->bytes++]=s->accumulator;
            s->slave_low=s->nack_byte != s->bytes;
        }
    }
    if ((old & MAIA_I2C_SCL) && !(s->master & MAIA_I2C_SCL) && s->active && s->raised) {
        if (s->high_since) CHECK(s->pauses > s->high_since);
        s->high_since=0U;s->raised=0U;s->slave_low=0U;
        if (++s->bit == 9U) {
            s->bit=0U;
            /* Decode only the address byte; odd data is not an address. */
            if (s->address_phase) {
                s->reading=s->wire[s->bytes-1U] & 1U;
                s->address_phase=0U;
            }
            s->accumulator=0U;
        }
    }
}
static maia_i2c_result_t pause_step(void *context) {
    slave_t *s=context;
    return ++s->pauses == s->fail_pause ? MAIA_I2C_TIMEOUT : MAIA_I2C_OK;
}
static maia_i2c_lines_t setup(slave_t *s) {
    memset(s,0,sizeof *s);s->master=MAIA_I2C_BOTH;s->reply=0x81U;
    maia_i2c_lines_t b={s,drive,sample,pause_step};return b;
}
int main(void) {
    slave_t s;maia_i2c_lines_t b=setup(&s);uint8_t out=0xCCU;
    CHECK(maia_i2c_read_register16(&b,0x55U,0x1234U,&out) == MAIA_I2C_OK);
    CHECK(out == 0x81U && s.bytes == 4U && s.starts == 2U && s.stops == 2U && s.master_nacks == 1U);
    const uint8_t expected[]={0xAAU,0x12U,0x34U,0xABU};CHECK(!memcmp(expected,s.wire,4));
    CHECK(s.master == MAIA_I2C_BOTH);
    unsigned full_pauses=s.pauses;
    /* Every pause in a complete transaction can fail, including final STOP. */
    for (unsigned n=1U;n<=full_pauses;++n) {
        b=setup(&s);s.fail_pause=n;out=0xCCU;
        CHECK(maia_i2c_read_register16(&b,0x55U,0x1234U,&out) == MAIA_I2C_TIMEOUT);
        CHECK(out == 0xCCU && s.master == MAIA_I2C_BOTH && s.pauses <= full_pauses);
    }
    for (unsigned n=1U;n<=4U;++n) {
        b=setup(&s);s.nack_byte=n;out=0xCCU;
        CHECK(maia_i2c_read_register16(&b,0x55U,0x1234U,&out) == MAIA_I2C_NACK);
        CHECK(out == 0xCCU && s.bytes == n && s.master == MAIA_I2C_BOTH && s.stops>0U);
    }
    for (unsigned n=1U;n<=2U;++n) {
        b=setup(&s);s.fail_stop=n;out=0xCCU;
        CHECK(maia_i2c_read_register16(&b,0x55U,0x1234U,&out) == MAIA_I2C_BUSY);
        CHECK(out == 0xCCU && s.master == MAIA_I2C_BOTH && s.stops == n);
    }
    b=setup(&s);s.nack_byte=1U;
    CHECK(maia_i2c_probe_address(&b,0x55U) == MAIA_I2C_NACK);
    unsigned nack_pauses=s.pauses;
    b=setup(&s);s.nack_byte=1U;s.fail_pause=nack_pauses-1U;out=0xCCU;
    CHECK(maia_i2c_read_register16(&b,0x55U,0,&out) == MAIA_I2C_TIMEOUT);
    CHECK(out == 0xCCU && s.master == MAIA_I2C_BOTH);
    b=setup(&s);s.stuck_scl=1U;
    CHECK(maia_i2c_read_register16(&b,0x55U,0,&out) == MAIA_I2C_TIMEOUT);
    CHECK(s.pauses == 40U && s.master == MAIA_I2C_BOTH);
    b=setup(&s);s.stuck_sda=1U;
    CHECK(maia_i2c_probe_address(&b,0x55U) == MAIA_I2C_BUSY);
    CHECK(s.bytes == 0U && s.master == MAIA_I2C_BOTH);
    b=setup(&s);s.stretch=3U;
    CHECK(maia_i2c_read_register16(&b,0x55U,0x1234U,&out) == MAIA_I2C_OK && out == 0x81U);
    b=setup(&s);s.stretch=40U;out=0xCCU;
    CHECK(maia_i2c_read_register16(&b,0x55U,0x1234U,&out) == MAIA_I2C_TIMEOUT && out == 0xCCU);
    b=setup(&s);
    CHECK(maia_i2c_read_register16(&b,0x55U,0,0) == MAIA_I2C_INVALID);
    CHECK(maia_i2c_probe_address(&b,0) == MAIA_I2C_INVALID);
    CHECK(maia_i2c_probe_address(&b,0x78U) == MAIA_I2C_INVALID);
    CHECK(maia_i2c_probe_address(0,0x55U) == MAIA_I2C_INVALID);
    CHECK(!s.pauses && !s.starts);
    b.pause=0;
    CHECK(maia_i2c_probe_address(&b,0x55U) == MAIA_I2C_INVALID && !s.starts);
    b=setup(&s);out=0xCCU;
    CHECK(maia_i2c_read_register8(&b,0x43U,0x05U,&out) == MAIA_I2C_OK && out == 0x81U);
    const uint8_t read8[]={0x86U,0x05U,0x87U};
    CHECK(s.bytes == 3U && !memcmp(s.wire,read8,3) && s.starts == 2U && s.stops == 1U);
    unsigned read8_pauses=s.pauses;
    b=setup(&s);
    CHECK(maia_i2c_write_register8(&b,0x44U,0x05U,0xABU) == MAIA_I2C_OK);
    const uint8_t write8[]={0x88U,0x05U,0xABU};
    CHECK(s.bytes == 3U && !memcmp(s.wire,write8,3) && s.starts == 1U && s.stops == 1U);
    unsigned write8_pauses=s.pauses;
    for (unsigned write=0U;write<2U;++write) {
        for (unsigned n=1U;n<=(write ? write8_pauses : read8_pauses);++n) {
            b=setup(&s);s.fail_pause=n;out=0xCCU;
            maia_i2c_result_t r=write ? maia_i2c_write_register8(&b,0x43U,5U,0xABU) :
                maia_i2c_read_register8(&b,0x43U,5U,&out);
            CHECK(r == MAIA_I2C_TIMEOUT && out == 0xCCU && s.master == MAIA_I2C_BOTH);
        }
        for (unsigned n=1U;n<=3U;++n) {
            b=setup(&s);s.nack_byte=n;out=0xCCU;
            maia_i2c_result_t r=write ? maia_i2c_write_register8(&b,0x43U,5U,0xABU) :
                maia_i2c_read_register8(&b,0x43U,5U,&out);
            CHECK(r == MAIA_I2C_NACK && out == 0xCCU && s.bytes == n && s.master == MAIA_I2C_BOTH);
        }
    }
    printf("PASS: read8/write8 wire order, repeated START, %u/%u pause failures and all byte NACK stages\n",read8_pauses,write8_pauses);
    b=setup(&s);s.nack_byte=1U;
    CHECK(maia_i2c_probe_address(&b,0x56U) == MAIA_I2C_NACK && s.wire[0] == 0xACU);
    b=setup(&s);s.reply=0U;
    CHECK(maia_tab5_touch_identify(&b,&out) == MAIA_I2C_OK && out == 0U); /* Unknown preserved. */
    CHECK(s.bytes == 8U && s.wire[1] == 0U && s.wire[2] == 1U && s.wire[5] == 0U && s.wire[6] == 0U);
    b=setup(&s);s.reply=1U;out=0xCCU;
    CHECK(maia_tab5_touch_identify(&b,&out) == MAIA_I2C_BUSY && out == 0xCCU && s.bytes == 4U);
    for (unsigned state=1U;state<256U;++state) {
        b=setup(&s);s.reply=(uint8_t)state;out=0xCCU;
        maia_i2c_result_t r=maia_tab5_touch_identify(&b,&out);
        if (state == 4U) CHECK(r == MAIA_I2C_OK && out == 4U && s.bytes == 8U);
        else CHECK(r == MAIA_I2C_BUSY && out == 0xCCU && s.bytes == 4U);
    }
    CHECK(maia_tab5_touch_identify(&b,0) == MAIA_I2C_INVALID);
    printf("PASS: wire order, master NACK, %u pause-failure positions, slave NACKs, stretching, stuck bus, status gating\n",full_pauses);
    return 0;
}

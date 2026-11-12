#include <maia/soc/esp32p4_ldo3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static maia_ldo3_snapshot_t snapshot(unsigned k,unsigned v,unsigned c) {
    maia_ldo3_snapshot_t s={0};s.mac_sys2=0x113U;s.mac_sys3=(k<<6U)|(v<<14U)|(c<<20U);return s;
}
static int signed_magnitude(unsigned x,unsigned sign) {
    return (x&sign) ? -(int)(x-sign) : (int)x;
}
static int64_t model(const maia_ldo3_plan_t *p,unsigned d,unsigned m) {
    /* Independent polynomial in regulator gain; same exact physical units. */
    int64_t ref=d<9U ? (int64_t)d+10 : 2*(int64_t)d+2;
    int64_t a=ref*p->k1000+20*p->vos1000;
    return 4000*a+(int64_t)m*p->c1000*a;
}
static void verify(unsigned k,unsigned v,unsigned c) {
    maia_ldo3_snapshot_t s=snapshot(k,v,c);maia_ldo3_plan_t p;
    CHECK(maia_esp32p4_ldo3_plan2500(&s,&p) == MAIA_LDO3_OK);
    CHECK(p.k1000 == (k ? 975+signed_magnitude(k,128U) : 1000));
    CHECK(p.vos1000 == (v ? -3+signed_magnitude(v,32U) : 0));
    CHECK(p.c1000 == (c ? 990+signed_magnitude(c,32U) : 1000));
    CHECK(p.default_mask == ((!k ? 1U : 0U)|(!v ? 2U : 0U)|(!c ? 4U : 0U)));
    CHECK(p.dref<16U && p.mul<8U);
    int64_t chosen=model(&p,p.dref,p.mul),error=llabs(INT64_C(200000000)-chosen);
    for (unsigned m=8U;m--;) for (unsigned d=16U;d--;) {
        int64_t difference=llabs(INT64_C(200000000)-model(&p,d,m));
        CHECK(difference>=error);
        if (difference == error) CHECK(d*8U+m >= (unsigned)p.dref*8U+p.mul);
    }
    CHECK(p.estimated_uv == (uint32_t)((chosen+40)/80) && p.error_uv == (uint32_t)((error+40)/80));
}
int main(void) {
    const unsigned c_values[]={0U,1U,10U,31U,32U,33U,63U};
    for (unsigned k=0U;k<256U;++k) for (unsigned v=0U;v<64U;++v)
        for (unsigned i=0U;i<sizeof c_values/sizeof c_values[0];++i) verify(k,v,c_values[i]);
    for (unsigned c=0U;c<64U;++c) verify(255U,63U,c);
    maia_ldo3_snapshot_t s=snapshot(0,0,0);maia_ldo3_plan_t p;
    CHECK(maia_esp32p4_ldo3_plan2500(&s,&p) == MAIA_LDO3_OK && p.dref == 9U && p.mul == 6U && p.estimated_uv == 2500000U && !p.error_uv && p.default_mask == 7U);
    s=snapshot(25U,3U,10U);
    CHECK(maia_esp32p4_ldo3_plan2500(&s,&p) == MAIA_LDO3_OK && p.dref == 9U && p.mul == 6U && !p.default_mask);
    maia_ldo3_plan_t sentinel;memset(&sentinel,0xA5,sizeof sentinel);
    const uint32_t unsupported[]={0x13U,0x213U|0x40U,0x113U|0x80U,0x913U,0x112U,0x114U,0x103U,0x123U};
    for (unsigned i=0U;i<sizeof unsupported/sizeof unsupported[0];++i) {
        s.mac_sys2=unsupported[i];memcpy(&p,&sentinel,sizeof p);
        CHECK(maia_esp32p4_ldo3_plan2500(&s,&p) == MAIA_LDO3_UNSUPPORTED && !memcmp(&p,&sentinel,sizeof p));
    }
    s=snapshot(255U,63U,63U);s.rs_error0=8U;memcpy(&p,&sentinel,sizeof p);
    CHECK(maia_esp32p4_ldo3_plan2500(&s,&p) == MAIA_LDO3_EFUSE_ERROR && !memcmp(&p,&sentinel,sizeof p));
    s.rs_error0=7U;CHECK(maia_esp32p4_ldo3_plan2500(&s,&p) == MAIA_LDO3_OK);
    CHECK(maia_esp32p4_ldo3_plan2500(0,&p) == MAIA_LDO3_INVALID);
    CHECK(maia_esp32p4_ldo3_plan2500(&s,0) == MAIA_LDO3_INVALID);
    puts("PASS:114752 trim sets, signed magnitude/zero defaults, all128 candidates, tie order, invalid revision/schema and eFuse failure");
    return 0;
}

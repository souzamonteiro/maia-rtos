/* SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native C port of ldo_ll_voltage_to_dref_mul, ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0.
 * Modified: VO3/2500mV scope, fail-closed schema/revision/error validation,
 * int64 arithmetic and modeled-voltage/default diagnostics. No SDK dependencies.
 * License: LICENSES/Apache-2.0.txt. */
#include <maia/soc/esp32p4_ldo3.h>
maia_ldo3_result_t maia_esp32p4_ldo3_plan2500(const maia_ldo3_snapshot_t *s,
                                           maia_ldo3_plan_t *out) {
    if (!s || !out) return MAIA_LDO3_INVALID;
    unsigned chip_major=(s->mac_sys2>>4U)&3U,chip_minor=s->mac_sys2&15U;
    unsigned block_major=(s->mac_sys2>>11U)&3U,block_minor=(s->mac_sys2>>8U)&7U;
    if (chip_major != 1U || chip_minor != 3U || (s->mac_sys2&0xC0U) || block_major || !block_minor)
        return MAIA_LDO3_UNSUPPORTED;
    if (s->rs_error0&8U) return MAIA_LDO3_EFUSE_ERROR;
    unsigned k=(s->mac_sys3>>6U)&255U,vos=(s->mac_sys3>>14U)&63U,c=(s->mac_sys3>>20U)&63U;
    maia_ldo3_plan_t p={0};
    p.k1000=k ? (k&128U ? 975-(int32_t)(k&127U) : 975+(int32_t)k) : 1000;
    p.vos1000=vos ? (vos&32U ? -3-(int32_t)(vos&31U) : (int32_t)vos-3) : 0;
    p.c1000=c ? (c&32U ? 990-(int32_t)(c&31U) : 990+(int32_t)c) : 1000;
    p.default_mask=(uint8_t)((!k ? 1U : 0U) | (!vos ? 2U : 0U) | (!c ? 4U : 0U));
    int64_t best=INT64_MAX,predicted=0;
    for (unsigned d=0U;d<16U;++d) {
        int32_t vref20=d<9U ? 10+(int32_t)d : 20+((int32_t)d-9)*2;
        for (unsigned m=0U;m<8U;++m) {
            int64_t v=((int64_t)vref20*p.k1000+20*p.vos1000)*(4000+(int32_t)m*p.c1000);
            int64_t diff=INT64_C(200000000)-v; /*2500mV *80000*/
            if (diff<0) diff=-diff;
            if (diff<best) { best=diff;predicted=v;p.dref=(uint8_t)d;p.mul=(uint8_t)m; }
        }
    }
    p.estimated_uv=(uint32_t)((predicted+40)/80);
    p.error_uv=(uint32_t)((best+40)/80);
    *out=p;
    return MAIA_LDO3_OK;
}

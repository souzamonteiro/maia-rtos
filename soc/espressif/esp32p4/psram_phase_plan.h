/* SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native adaptation of ESP-IDF f5c3654 mspi_timing_by_dqs.c phase selection:
 * first phase of longest consecutive passing run,4 phases67.5..101.25deg.
 * Modified: no passing phase is an error, never a silent fallback. */
#ifndef MAIA_PSRAM_PHASE_PLAN_H
#define MAIA_PSRAM_PHASE_PLAN_H
static inline int maia_psram_phase_best(unsigned mask){
    if(mask>15U||!mask)return -1;
    unsigned length=0,best_length=0,best=0;
    for(unsigned i=0;i<4U;++i){
        if(mask&(1U<<i)){++length;if(length>best_length){best_length=length;best=i+1U-length;}}
        else length=0;
    }
    return (int)best;
}
#endif

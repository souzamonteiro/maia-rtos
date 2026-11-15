/* Original MIT native HEX MR-read policy. Field facts from pinned P4 register
 * headers and HEX s_get_psram_mode_reg, docs/audits/native-psram-identity.md.
 * No SDK/ROM driver calls. Mutable command port only, no PSRAM mode/array write. */
#ifndef MAIA_PSRAM_MODE_INTERNAL_H
#define MAIA_PSRAM_MODE_INTERNAL_H
#include "psram_internal.h"
#define PSRAM_MODE_WORDS 10U
#define PSRAM_MODE_POISON 0xA55A5AA5U
static const uint16_t psram_mode_offsets[PSRAM_MODE_WORDS]={8,0x18,0x1C,0x20,4,0x28,0x34,0x3C,0xD4,0x58};
static const uint32_t psram_mode_masks[PSRAM_MODE_WORDS]={0x2270U,0xFF00F000U,0xFC00003FU,0xF000FFFFU,0xFFFFFFFFU,0x3FFU,0x403U,2U,0x11U,0xFFFFFFFFU};
static inline uintptr_t psram_mode_address(unsigned i){return PSRAM_MSPI1+psram_mode_offsets[i];}
static inline uint32_t psram_mode_value(unsigned i,uint32_t original,uint32_t reg){
    const uint32_t value[PSRAM_MODE_WORDS]={0x2270U,0xF0000000U,0x7C00000BU,0xF0004040U,reg,reg<4U?15U:7U,1U,2U,1U,PSRAM_MODE_POISON};
    return (original&~psram_mode_masks[i])|value[i];
}
#endif

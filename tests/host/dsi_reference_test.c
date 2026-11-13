#include <assert.h>
#include <stdio.h>
#include "dsi_access_internal.h"
#include <maia/soc/esp32p4_dsi_access.h>
void maia_dsi_access_test_reset(void);
static const uintptr_t addresses[]={DSI_CPU,DSI_SYS,DSI_PHY,DSI_RESET,DSI_ISP,DSI_NORST,
 0x50081010,0x50081018,0x5008101C,0x50081058,0x50081134,0x50081234,0x50081334,0x50081434,
 MAIA_ESP32P4_EFUSE_MAC_SYS2_REG,MAIA_ESP32P4_EFUSE_MAC_SYS3_REG,MAIA_ESP32P4_EFUSE_RS_ERR0_REG,
 MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG,MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG,
 MAIA_ESP32P4_DSI_HOST_BASE,MAIA_ESP32P4_DSI_HOST_BASE+4U,MAIA_ESP32P4_DSI_HOST_BASE+0xA0U,
 MAIA_ESP32P4_DSI_HOST_BASE+0xB0U,MAIA_ESP32P4_DSI_BRG_BASE+4U,
 MAIA_ESP32P4_DSI_BRG_BASE+0x40U,MAIA_ESP32P4_DSI_BRG_BASE+0x14U,DSI_REF_DIV,DSI_LP_GATE,DSI_REF_GATE,DSI_REF_SOURCE};
static uint32_t regs[30];static unsigned reads,writes,drop,context,counts[30],inject_index,inject_n,change_after,change_index;
static maia_dsi_access_report_t report;
static unsigned reference_mode;
static maia_dsi_reference_report_t ref;
int maia_dsi_access_test_context(void) { return (int)context; }
uint32_t maia_dsi_access_test_read(uintptr_t address) {
 ++reads;
 for (unsigned i=0;i<30U;++i) if (addresses[i] == address) {
  if (i>=6U && i<=13U) assert((regs[0]&0x2000U) && (regs[1]&0x20U));
  if (i>=19U && i<26U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U) && (writes == 3U || reference_mode));
  ++counts[i];return regs[i]^((i == inject_index && counts[i] == inject_n) ? 1U : 0U);
 }
 assert(!"unexpected MMIO");return 0;
}
int maia_ldo3_test_context(void) { return (int)context; }
uint32_t maia_ldo3_test_read(uintptr_t address) { return maia_dsi_access_test_read(address); }
void maia_dsi_access_test_write(uintptr_t address,uint32_t value) {
 ++writes;assert(writes<=5U);
 if (reference_mode) {
  unsigned index=reference_mode == 2U ? 2U : writes == 1U ? 28U : writes == 3U ? 29U : 2U;
  assert(address == addresses[index]);
  assert(value == (index == 28U ? regs[28]|0x100U : index == 29U ? regs[29]&~0xC0000000U : reference_mode == 2U || writes == 5U ? regs[2]|2U : writes == 2U ? regs[2]&~1U : regs[2]|1U));
  if (index == 29U) assert((regs[28]&0x100U) && !(regs[2]&3U));
  if (index == 2U && writes != 2U) assert((regs[28]&0x100U) && !(regs[29]&0xC0000000U));
  if (writes != drop) regs[index]=value;
  if (writes == change_after) regs[change_index]^=1U;
  return;
 }
 unsigned index=writes == 1U ? 1U : writes == 2U ? 2U : 3U;
 assert(address == addresses[index]);
 assert(value == (index == 1U ? regs[1]|0x1000U : index == 2U ? regs[2]|1U : regs[3]&~0x04000000U));
 if (writes<=2U) assert((regs[3]&0x3C000000U) == 0x3C000000U);
 if (writes == 3U) assert((regs[1]&0x1000U) && (regs[2]&1U));
 if (writes != drop) regs[index]=value;
 if (writes == change_after) regs[change_index]^=1U;
}
static void setup(void) {
 reference_mode=0;maia_dsi_access_test_reset();reads=writes=drop=0;context=1;inject_index=99;inject_n=0;change_after=0;change_index=0;
 for (unsigned i=0;i<30U;++i) { regs[i]=0;counts[i]=0; }
 regs[0]=0xE6DFB7FF;regs[1]=0x0C0D4278;regs[2]=0x220;regs[3]=0xBC000041;regs[4]=0x02E82000;
 regs[14]=0x313;regs[15]=(55U<<6U)|(33U<<14U)|(33U<<20U);regs[17]=0x40200180;regs[18]=0xDD812345;
 regs[19]=0x3134312A;
}
static maia_dsi_access_result_t open_config(void) { return maia_esp32p4_dsi_open_config(&report); }
static void setup_reference(void) {
 setup();assert(open_config() == MAIA_DSI_ACCESS_OK);
 reads=writes=0;reference_mode=1;
 for (unsigned i=0;i<30U;++i) counts[i]=0;
 regs[26]=0xA51705F1;regs[27]=0x0A12ABCD;regs[28]=0xA0FF00FF;regs[29]=0xA1234567;
}
static maia_dsi_access_result_t reference(void) { return maia_esp32p4_dsi_reference20(&ref); }
static void sticky(unsigned expected) {
 unsigned saved=reads;assert(reference() == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == expected);
}
int main(void) {
 setup();assert(reference() == MAIA_DSI_ACCESS_BUSY && !reads && !writes); /* No fake report/token. */
 assert(maia_esp32p4_dsi_reference20(NULL) == MAIA_DSI_ACCESS_INVALID && !reads);
 setup_reference();context=0;assert(reference() == MAIA_DSI_ACCESS_CONTEXT && !reads && !writes);
 setup_reference();assert(reference() == MAIA_DSI_ACCESS_OK && writes == 5U);
 assert(ref.completed == MAIA_DSI_REFERENCE_VERIFIED && ref.attempted_mask == 31U && ref.nominal_hz == 20000000U);
 assert(regs[26] == 0xA51705F1 && regs[27] == 0x0A12ABCD && regs[28] == 0xA0FF01FF && regs[29] == 0x21234567 && regs[2] == 0x223);
 unsigned saved=reads;assert(reference() == MAIA_DSI_ACCESS_BUSY && reads == saved && writes == 5U);
 /* Matching provider/mux steps skip writes, retain all unrelated bits. */
 setup_reference();regs[28]|=0x100U;regs[29]&=~0xC0000000U;
 reference_mode=2;
 assert(reference() == MAIA_DSI_ACCESS_OK && writes == 1U && ref.attempted_mask == 16U);
 for (unsigned step=1;step<=5U;++step) {
  setup_reference();drop=step;assert(reference() == MAIA_DSI_ACCESS_VERIFY && writes == step);
  assert((unsigned)ref.completed == step && ref.attempted_mask == ((1U<<step)-1U));sticky(step);
 }
 const unsigned idx[]={0,1,1,2,2,2,2,3,3,5,6,7,8,9,10,11,12,13,17,18,14,16,26,27,27};
 const uint32_t masks[]={0x2000,0x20,0x400,1,2,16,128,0x04000000,0x20000000,0x4000,
  1,1,1,1,1,1,1,1,0x100,0x10000000,1,8,0x10000,0x02000000,0x08000000};
 for (unsigned i=0;i<sizeof(idx)/sizeof(idx[0]);++i) {
  setup_reference();regs[idx[i]]^=masks[i];assert(reference() == MAIA_DSI_ACCESS_BUSY && !writes);
 }
 for (unsigned i=0;i<30U;++i) if (i != 19U && i != 22U) {
  setup_reference();inject_index=i;inject_n= i>=19U && i<26U ? 1U : 2U;
  assert(reference() == MAIA_DSI_ACCESS_VERIFY && !writes);
  inject_index=99;assert(reference() == MAIA_DSI_ACCESS_OK);
 }
 for (unsigned step=1;step<=5U;++step) for (unsigned i=0;i<30U;++i) if (i != 19U && i != 22U && !((step == 2U || step == 3U) && i>=20U && i<=25U)) {
  setup_reference();change_after=step;change_index=i;
  assert(reference() == MAIA_DSI_ACCESS_VERIFY && writes == step);sticky(step);
 }
 puts("PASS: reference20 internal access claim, divider/provider/mux/clock order, preserved guards, skipped writes and sticky failures");
}

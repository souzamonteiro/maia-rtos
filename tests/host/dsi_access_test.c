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
 MAIA_ESP32P4_DSI_BRG_BASE+0x40U,MAIA_ESP32P4_DSI_BRG_BASE+0x14U};
static uint32_t regs[26];static unsigned reads,writes,drop,context,counts[26],inject_index,inject_n,change_after,change_index;
static maia_dsi_access_report_t report;
int maia_dsi_access_test_context(void) { return (int)context; }
uint32_t maia_dsi_access_test_read(uintptr_t address) {
 ++reads;
 for (unsigned i=0;i<26U;++i) if (addresses[i] == address) {
  if (i>=6U && i<=13U) assert((regs[0]&0x2000U) && (regs[1]&0x20U));
  if (i>=19U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U) && writes == 3U);
  ++counts[i];return regs[i]^((i == inject_index && counts[i] == inject_n) ? 1U : 0U);
 }
 assert(!"unexpected MMIO");return 0;
}
int maia_ldo3_test_context(void) { return (int)context; }
uint32_t maia_ldo3_test_read(uintptr_t address) { return maia_dsi_access_test_read(address); }
void maia_dsi_access_test_write(uintptr_t address,uint32_t value) {
 ++writes;assert(writes<=3U);
 unsigned index=writes == 1U ? 1U : writes == 2U ? 2U : 3U;
 assert(address == addresses[index]);
 assert(value == (index == 1U ? regs[1]|0x1000U : index == 2U ? regs[2]|1U : regs[3]&~0x04000000U));
 if (writes<=2U) assert((regs[3]&0x3C000000U) == 0x3C000000U);
 if (writes == 3U) assert((regs[1]&0x1000U) && (regs[2]&1U));
 if (writes != drop) regs[index]=value;
 if (writes == change_after) regs[change_index]^=1U;
}
static void setup(void) {
 maia_dsi_access_test_reset();reads=writes=drop=0;context=1;inject_index=99;inject_n=0;change_after=0;change_index=0;
 for (unsigned i=0;i<26U;++i) { regs[i]=0;counts[i]=0; }
 regs[0]=0xE6DFB7FF;regs[1]=0x0C0D4278;regs[2]=0x220;regs[3]=0xBC000041;regs[4]=0x02E82000;
 regs[14]=0x313;regs[15]=(55U<<6U)|(33U<<14U)|(33U<<20U);regs[17]=0x40200180;regs[18]=0xDD812345;
 regs[19]=0x3134312A;
}
static maia_dsi_access_result_t open_config(void) { return maia_esp32p4_dsi_open_config(&report); }
static void sticky(unsigned expected_writes) {
 unsigned saved=reads;assert(open_config() == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == expected_writes);
}
int main(void) {
 setup();assert(open_config() == MAIA_DSI_ACCESS_OK && writes == 3U);
 assert(report.completed == MAIA_DSI_ACCESS_VERIFIED && report.attempted_mask == 7U);
 assert(regs[1] == 0x0C0D5278 && regs[2] == 0x221 && regs[3] == 0xB8000041);
 assert(regs[17] == 0x40200180 && regs[18] == 0xDD812345 && report.version == regs[19]);
 unsigned saved=reads;assert(open_config() == MAIA_DSI_ACCESS_BUSY && reads == saved && writes == 3U);
 setup();assert(maia_esp32p4_dsi_open_config(NULL) == MAIA_DSI_ACCESS_INVALID && !reads && !writes);
 setup();context=0;assert(open_config() == MAIA_DSI_ACCESS_CONTEXT && !reads && !writes);
 for (unsigned step=1;step<=3U;++step) {
  setup();drop=step;assert(open_config() == MAIA_DSI_ACCESS_VERIFY && writes == step);
  assert((unsigned)report.completed == step && report.attempted_mask == ((1U<<step)-1U));sticky(step);
 }
 const unsigned idx[]={0,1,1,2,2,2,2,3,3,3,3,5,6,7,8,9,10,11,12,13,17,17,18,14,16};
 const uint32_t masks[]={0x2000,0x20,0x400,1,2,16,128,0x04000000,0x08000000,0x10000000,0x20000000,
  0x4000,1,1,1,1,1,1,1,1,0x100,0x4000,0x10000000,1,8};
 for (unsigned i=0;i<sizeof(idx)/sizeof(idx[0]);++i) {
  setup();regs[idx[i]]^=masks[i];assert(open_config() == MAIA_DSI_ACCESS_BUSY && !writes);
  for (unsigned n=19;n<26U;++n) assert(!counts[n]);
 }
 /* Mutate each RCC/eFuse/PMU guard on its fresh pre-write observation. */
 for (unsigned i=0;i<19U;++i) {
  setup();inject_index=i;inject_n=2;
  assert(open_config() == MAIA_DSI_ACCESS_VERIFY && !writes);
  inject_index=99;assert(open_config() == MAIA_DSI_ACCESS_OK); /* pre-write refusal not sticky */
 }
 /* Fresh guards reject changes after each write, including supply/eFuse drift. */
 for (unsigned step=1;step<=3U;++step) for (unsigned i=0;i<19U;++i) {
  setup();change_after=step;change_index=i;
  assert(open_config() == MAIA_DSI_ACCESS_VERIFY && writes == step);sticky(step);
 }
 /* Persistent transmit/bridge/FIFO state after reset-release is fatal, no rollback. */
 for (unsigned i=20;i<26U;++i) if (i != 22U) {
  setup();regs[i]=1;assert(open_config() == MAIA_DSI_ACCESS_VERIFY && writes == 3U);sticky(3);
 }
 setup();regs[21]=8;assert(open_config() == MAIA_DSI_ACCESS_VERIFY);sticky(3);
 puts("PASS: DSI access ordering, calibration/RCC/DMA guards, preserved fields, readback failures and sticky claims");
}

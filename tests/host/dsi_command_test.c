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
 MAIA_ESP32P4_DSI_BRG_BASE+0x40U,MAIA_ESP32P4_DSI_BRG_BASE+0x14U,DSI_REF_DIV,DSI_LP_GATE,DSI_REF_GATE,DSI_REF_SOURCE,MAIA_ESP32P4_DSI_HOST_BASE+0xA4U,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,MAIA_ESP32P4_DSI_HOST_BASE+0xB4U,MAIA_ESP32P4_DSI_HOST_BASE+0xB8U,
 MAIA_ESP32P4_DSI_HOST_BASE+0x34U,MAIA_ESP32P4_DSI_HOST_BASE+8U,MAIA_ESP32P4_DSI_HOST_BASE+0x9CU,MAIA_ESP32P4_DSI_HOST_BASE+0x98U,MAIA_ESP32P4_DSI_HOST_BASE+0x2CU,MAIA_ESP32P4_DSI_HOST_BASE+0x78U,MAIA_ESP32P4_DSI_HOST_BASE+0x7CU,MAIA_ESP32P4_DSI_HOST_BASE+0x80U,MAIA_ESP32P4_DSI_HOST_BASE+0x84U,MAIA_ESP32P4_DSI_HOST_BASE+0x88U,MAIA_ESP32P4_DSI_HOST_BASE+0x8CU,MAIA_ESP32P4_DSI_HOST_BASE+0xF4U,MAIA_ESP32P4_DSI_HOST_BASE+0x74U};
static uint32_t regs[47];static unsigned reads,writes,drop,context,counts[47],inject_index,inject_n,change_after,change_index;
static maia_dsi_access_report_t report;
static unsigned reference_mode,test_writes,poll_reads,lock_after,stop_after;
static maia_dsi_reference_report_t ref;
static const unsigned table_index[14]={34,35,36,37,38,39,40,41,42,43,44,45,30,31};
static const uint32_t masks[14]={1,0xFFFF,0x03FF03FF,0x03FF03FF,0x39,0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x7FFF,0xFF00,3};
static const uint32_t values[14]={1,0x0C06,0x00320068,0x002E0080,0x19,0,0,0,0,0,0,6000,0x3F00,0};
int maia_dsi_access_test_context(void) { return (int)context; }
uint32_t maia_dsi_access_test_read(uintptr_t address) {
 ++reads;
 for (unsigned i=0;i<47U;++i) if (addresses[i] == address) {
  if (i>=6U && i<=13U) assert((regs[0]&0x2000U) && (regs[1]&0x20U));
  if (i>=19U && i<26U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U) && (writes == 3U || reference_mode));
  if (reference_mode == 3U && i == 22U && test_writes == 35U) {
   ++poll_reads;return 0x1500U | (poll_reads>=lock_after ? 1U : 0U) | (poll_reads>=stop_after ? 0x94U : 0U);
  }
  ++counts[i];return regs[i]^((i == inject_index && counts[i] == inject_n) ? 1U : 0U);
 }
 assert(!"unexpected MMIO");return 0;
}
int maia_ldo3_test_context(void) { return (int)context; }
uint32_t maia_ldo3_test_read(uintptr_t address) { return maia_dsi_access_test_read(address); }
void maia_dsi_access_test_write(uintptr_t address,uint32_t value) {
 ++writes;assert(writes<=41U);
 if (reference_mode == 4U) {
  assert(writes<=13U);unsigned i=writes-1U,index=table_index[i];
  assert(address == addresses[index] && value == ((regs[index]&~masks[i])|values[i]));
  if (writes != drop) regs[index]=value;
  if (writes == change_after) regs[change_index]^=1U;
  return;
 }
 if (reference_mode == 3U) {
  unsigned index;
  if (writes<=6U) {
   static const unsigned indices[6]={30,20,21,21,21,21};
   static const uint32_t expected[6]={0xABCD1201,1,1,3,7,15};
   index=indices[writes-1U];assert(address == addresses[index] && value == expected[writes-1U]);
  } else {
   static const uint8_t addr[5]={0x44,0x19,0x17,0x18,0x18},data[5]={0x34,0x30,0,0x0F,0x81};
   unsigned b=test_writes/7U,p=test_writes%7U;assert(b<5U);
   index=(p == 1U || p == 4U) ? 33U : 32U;
   uint32_t mask=index == 32U ? 3U : 0x100FFU;
   uint32_t expected=p == 1U ? 0x10000U|addr[b] : p == 4U ? data[b] : (p == 2U || p == 5U) ? 2U : 0U;
   assert(address == addresses[index] && (value&mask) == expected && ((value^regs[index])&~mask) == 0U);
   ++test_writes;
  }
  if (writes != drop) regs[index]=value;
  if (writes == change_after) regs[change_index]^=1U;
  return;
 }
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
 for (unsigned i=0;i<47U;++i) { regs[i]=0;counts[i]=0; }
 regs[0]=0xE6DFB7FF;regs[1]=0x0C0D4278;regs[2]=0x220;regs[3]=0xBC000041;regs[4]=0x02E82000;
 regs[14]=0x313;regs[15]=(55U<<6U)|(33U<<14U)|(33U<<20U);regs[17]=0x40200180;regs[18]=0xDD812345;
 regs[19]=0x3134312A;
}
static maia_dsi_access_result_t open_config(void) { return maia_esp32p4_dsi_open_config(&report); }
static void setup_reference(void) {
 setup();assert(open_config() == MAIA_DSI_ACCESS_OK);
 reads=writes=0;reference_mode=1;
 for (unsigned i=0;i<47U;++i) counts[i]=0;
 regs[26]=0xA51705F1;regs[27]=0x0A12ABCD;regs[28]=0xA0FF00FF;regs[29]=0xA1234567;
}
static maia_dsi_access_result_t reference(void) { return maia_esp32p4_dsi_reference20(&ref); }
static maia_dsi_phy_report_t phy_report;
static void setup_phy(void) {
 setup_reference();assert(reference() == MAIA_DSI_ACCESS_OK);
 reference_mode=3;reads=writes=test_writes=poll_reads=0;lock_after=2;stop_after=4;
 for (unsigned i=0;i<47U;++i) counts[i]=0;
 regs[30]=0xABCD1200;regs[31]=0x40000000;regs[32]=0x40000001;regs[33]=0x8000EE00;
}
static maia_dsi_access_result_t phy(unsigned budget) { return maia_esp32p4_dsi_phy960(budget,&phy_report); }
static maia_dsi_command_report_t command_report;
static void setup_command(int matching) {
 setup_phy();assert(phy(8) == MAIA_DSI_ACCESS_OK);
 reference_mode=4;reads=writes=0;
 for (unsigned i=0;i<47U;++i) counts[i]=0;
 regs[22]=0x1595;regs[46]=0x15;
 for (unsigned i=0;i<12U;++i) regs[table_index[i]]=(0xA5000000U&~masks[i])|((matching ? values[i] : ~values[i])&masks[i]);
 if (matching) regs[30]=(regs[30]&~0xFF00U)|0x3F00U;
}
static maia_dsi_access_result_t configure(void) { return maia_esp32p4_dsi_command_config(&command_report); }
static void sticky(void) {
 unsigned saved=reads,written=writes;assert(configure() == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == written);
}
int main(void) {
 setup();assert(configure() == MAIA_DSI_ACCESS_BUSY && !reads && !writes);
 assert(maia_esp32p4_dsi_command_config(NULL) == MAIA_DSI_ACCESS_INVALID);
 setup_command(0);context=0;assert(configure() == MAIA_DSI_ACCESS_CONTEXT && !reads && !writes);
 setup_command(0);assert(configure() == MAIA_DSI_ACCESS_OK && writes == 13U);
 assert(command_report.completed == 16 && command_report.attempted_mask == 0x1FFFU);
 assert(command_report.escape_hz == 20000000 && command_report.timeout_hz == 10000000);
 for (unsigned i=0;i<14U;++i) {
  assert(command_report.after[i] == ((command_report.before[i]&~masks[i])|values[i]));
  assert(regs[table_index[i]] == command_report.after[i]);
 }
 unsigned saved=reads;assert(configure() == MAIA_DSI_ACCESS_BUSY && reads == saved && writes == 13U);
 setup_command(1);assert(configure() == MAIA_DSI_ACCESS_OK && !writes && !command_report.attempted_mask);
 for (unsigned n=1;n<=13U;++n) {
  setup_command(0);drop=n;assert(configure() == MAIA_DSI_ACCESS_VERIFY && writes == n);
  assert(command_report.completed == n && command_report.attempted_mask == ((1U<<n)-1U));sticky();
 }
 const unsigned indices[]={0,1,2,3,5,6,7,8,9,10,11,12,13,17,18,26,27,28,29,20,21,30,31,24,22,46,46,46,46,46,46,46};
 const uint32_t bad[]={0x2000,0x400,2,0x04000000,0x4000,1,1,1,1,1,1,1,1,0x100,0x10000000,
  0x10000,0x08000000,0x100,0x40000000,1,1,1,1,1,1,1,2,4,8,16,32,64};
 for (unsigned i=0;i<sizeof(indices)/sizeof(indices[0]);++i) {
  setup_command(0);regs[indices[i]]^=bad[i];assert(configure() == MAIA_DSI_ACCESS_BUSY && !writes);
 }
 for (unsigned i=0;i<47U;++i) if (i != 19 && i != 32 && i != 33) {
  setup_command(0);change_after=1;change_index=i;
  assert(configure() == MAIA_DSI_ACCESS_VERIFY && writes == 1);sticky();
 }
 puts("PASS: command config exact masks/order/dividers, all13 write failures, FIFO/PLL/root/supply guards and no-op/sticky claims");
}

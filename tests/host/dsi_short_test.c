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
 MAIA_ESP32P4_DSI_HOST_BASE+0x34U,MAIA_ESP32P4_DSI_HOST_BASE+8U,MAIA_ESP32P4_DSI_HOST_BASE+0x9CU,MAIA_ESP32P4_DSI_HOST_BASE+0x98U,MAIA_ESP32P4_DSI_HOST_BASE+0x2CU,MAIA_ESP32P4_DSI_HOST_BASE+0x78U,MAIA_ESP32P4_DSI_HOST_BASE+0x7CU,MAIA_ESP32P4_DSI_HOST_BASE+0x80U,MAIA_ESP32P4_DSI_HOST_BASE+0x84U,MAIA_ESP32P4_DSI_HOST_BASE+0x88U,MAIA_ESP32P4_DSI_HOST_BASE+0x8CU,MAIA_ESP32P4_DSI_HOST_BASE+0xF4U,MAIA_ESP32P4_DSI_HOST_BASE+0x74U,MAIA_ESP32P4_DSI_HOST_BASE+0x68U,MAIA_ESP32P4_DSI_HOST_BASE+0x6CU,MAIA_ESP32P4_DSI_HOST_BASE+0xBCU,MAIA_ESP32P4_DSI_HOST_BASE+0xC0U};
static uint32_t regs[51];static unsigned reads,writes,drop,context,counts[51],inject_index,inject_n,change_after,change_index;
static maia_dsi_access_report_t report;
static unsigned reference_mode,test_writes,poll_reads,lock_after,stop_after;
static maia_dsi_reference_report_t ref;
static unsigned tx_active,post_polls,drain_after,stopped_after,header_writes,accepted,drop_header,drop_lp,error_at,error_word;
static uint32_t expected_header,selected_lp,last_header,drift_mask;
static const uint32_t ERROR0_MASK=0x1FFFFF,ERROR1_MASK=0x81FFF;
static const unsigned table_index[14]={34,35,36,37,38,39,40,41,42,43,44,45,30,31};
static const uint32_t masks[14]={1,0xFFFF,0x03FF03FF,0x03FF03FF,0x39,0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x7FFF,0xFF00,3};
static const uint32_t values[14]={1,0x0C06,0x00320068,0x002E0080,0x19,0,0,0,0,0,0,6000,0x3F00,0};
int maia_dsi_access_test_context(void) { return (int)context; }
uint32_t maia_dsi_access_test_read(uintptr_t address) {
 ++reads;
 for (unsigned i=0;i<51U;++i) if (addresses[i] == address) {
  if (i>=6U && i<=13U) assert((regs[0]&0x2000U) && (regs[1]&0x20U));
  if (i>=19U && i<26U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U) && (writes == 3U || reference_mode));
  if (reference_mode == 3U && i == 22U && test_writes == 35U) {
   ++poll_reads;return 0x1500U | (poll_reads>=lock_after ? 1U : 0U) | (poll_reads>=stop_after ? 0x94U : 0U);
  }
  if (reference_mode == 5U) {
   if (i>=30U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U));
   assert(i != 48U); /* GEN_HDR is publication, never safe readback. */
   if (i == 46U && tx_active) { ++post_polls;return regs[i] | (post_polls>=drain_after ? 1U : 0U); }
   if (i == 22U && tx_active) return regs[i] | (post_polls>=stopped_after ? 0x94U : 0x84U);
   if (i == 49U || i == 50U) {
    uint32_t result=regs[i];regs[i]=0; /* Potential read-to-clear semantics modeled. */
    if (tx_active && error_at && post_polls == error_at && i == (error_word ? 50U : 49U)) result|=1U;
    return result;
   }
  }
  ++counts[i];return regs[i]^((i == inject_index && counts[i] == inject_n) ? 1U : 0U);
 }
 assert(!"unexpected MMIO");return 0;
}
int maia_ldo3_test_context(void) { return (int)context; }
uint32_t maia_ldo3_test_read(uintptr_t address) { return maia_dsi_access_test_read(address); }
void maia_dsi_access_test_write(uintptr_t address,uint32_t value) {
 ++writes;assert(writes<=41U);
 if (reference_mode == 5U) {
  if (address == addresses[47]) {
   assert(value == (regs[47]|selected_lp));if (!drop_lp) regs[47]=value;
  } else {
   assert(address == addresses[48] && value == expected_header);
   ++header_writes;last_header=value;
   if (!drop_header) { ++accepted;tx_active=1;post_polls=0;regs[46]=0x14;regs[22]=0x1501; }
  }
  if (writes == change_after) regs[change_index]^=drift_mask;
  return;
 }
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
 for (unsigned i=0;i<51U;++i) { regs[i]=0;counts[i]=0; }
 regs[0]=0xE6DFB7FF;regs[1]=0x0C0D4278;regs[2]=0x220;regs[3]=0xBC000041;regs[4]=0x02E82000;
 regs[14]=0x313;regs[15]=(55U<<6U)|(33U<<14U)|(33U<<20U);regs[17]=0x40200180;regs[18]=0xDD812345;
 regs[19]=0x3134312A;
}
static maia_dsi_access_result_t open_config(void) { return maia_esp32p4_dsi_open_config(&report); }
static void setup_reference(void) {
 setup();assert(open_config() == MAIA_DSI_ACCESS_OK);
 reads=writes=0;reference_mode=1;
 for (unsigned i=0;i<51U;++i) counts[i]=0;
 regs[26]=0xA51705F1;regs[27]=0x0A12ABCD;regs[28]=0xA0FF00FF;regs[29]=0xA1234567;
}
static maia_dsi_access_result_t reference(void) { return maia_esp32p4_dsi_reference20(&ref); }
static maia_dsi_phy_report_t phy_report;
static void setup_phy(void) {
 setup_reference();assert(reference() == MAIA_DSI_ACCESS_OK);
 reference_mode=3;reads=writes=test_writes=poll_reads=0;lock_after=2;stop_after=4;
 for (unsigned i=0;i<51U;++i) counts[i]=0;
 regs[30]=0xABCD1200;regs[31]=0x40000000;regs[32]=0x40000001;regs[33]=0x8000EE00;
}
static maia_dsi_access_result_t phy(unsigned budget) { return maia_esp32p4_dsi_phy960(budget,&phy_report); }
static maia_dsi_command_report_t command_report;
static void setup_command(int matching) {
 setup_phy();assert(phy(8) == MAIA_DSI_ACCESS_OK);
 reference_mode=4;reads=writes=0;
 for (unsigned i=0;i<51U;++i) counts[i]=0;
 regs[22]=0x1595;regs[46]=0x15;
 for (unsigned i=0;i<12U;++i) regs[table_index[i]]=(0xA5000000U&~masks[i])|((matching ? values[i] : ~values[i])&masks[i]);
 if (matching) regs[30]=(regs[30]&~0xFF00U)|0x3F00U;
}
static maia_dsi_access_result_t configure(void) { return maia_esp32p4_dsi_command_config(&command_report); }
static maia_dsi_short_report_t short_report;
static void setup_short(void) {
 setup_command(0);assert(configure() == MAIA_DSI_ACCESS_OK);
 reference_mode=5;reads=writes=0;header_writes=accepted=tx_active=post_polls=drop_header=drop_lp=error_at=error_word=0;
 drift_mask=1;drain_after=2;stopped_after=4;regs[47]=0xA5500000;regs[49]=regs[50]=0;
 for (unsigned i=0;i<51U;++i) counts[i]=0;
}
static maia_dsi_access_result_t send(uint32_t vc,uint32_t command,uint32_t count,uint32_t parameter,uint32_t budget) {
 expected_header=(parameter<<16)|(command<<8)|(vc<<6)|(count ? 0x15U : 5U);selected_lp=1U<<(16+(count&1));
 return maia_esp32p4_dsi_dcs_short(vc,command,count,parameter,budget,&short_report);
}
static void sticky(void) {
 unsigned saved=reads,written=writes,headers=header_writes;
 assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == written && header_writes == headers);
}
int main(void) {
 setup();assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_BUSY && !reads && !writes);
 assert(maia_esp32p4_dsi_dcs_short(0,0,0,0,8,NULL) == MAIA_DSI_ACCESS_INVALID);
 setup_short();context=0;assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_CONTEXT && !reads && !writes);
 const uint32_t invalid[7][5]={{4,0,0,0,8},{0,256,0,0,8},{0,0,2,0,8},{0,0,1,256,8},{0,0,0,1,8},{0,0,0,0,0},{0,0,0,0,100001}};
 for (unsigned i=0;i<7;++i) { setup_short();assert(send(invalid[i][0],invalid[i][1],invalid[i][2],invalid[i][3],invalid[i][4]) == MAIA_DSI_ACCESS_INVALID && !reads && !writes); }
 for (unsigned vc=0;vc<4U;++vc) for (unsigned count=0;count<2U;++count) {
  setup_short();assert(send(vc,count ? 0x51 : 0,count,count ? 0x7F : 0,8) == MAIA_DSI_ACCESS_OK);
  assert(short_report.completed == 5 && short_report.header_attempted == 1 && short_report.header == expected_header);
  assert(last_header == expected_header && header_writes == 1 && accepted == 1 && writes == 2 && short_report.polls == 4 && short_report.saw_busy);
  assert(regs[47] == (0xA5500000|selected_lp) && !short_report.error0 && !short_report.error1);
 }
 setup_short();assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_OK);writes=0;
 assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_OK && writes == 1 && header_writes == 2); /* No repeated mode write. */
 setup_short();drain_after=stopped_after=1;assert(send(0,0,0,0,1) == MAIA_DSI_ACCESS_OK && !short_report.saw_busy);
 setup_short();drain_after=99;assert(send(0,0,0,0,3) == MAIA_DSI_ACCESS_TIMEOUT && short_report.polls == 3 && header_writes == 1);sticky();
 setup_short();stopped_after=99;assert(send(0,0,0,0,3) == MAIA_DSI_ACCESS_TIMEOUT);sticky();
 setup_short();drop_lp=1;assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_VERIFY && !header_writes && !short_report.header_attempted);sticky();
 /* A silently ignored publication is indistinguishable from a fast packet.
  * API promises attempted store + observed host idle, never verified panel delivery. */
 setup_short();drop_header=1;assert(send(0,0,0,0,1) == MAIA_DSI_ACCESS_OK && short_report.header_attempted && !accepted && !short_report.saw_busy);
 for (unsigned word=0;word<2;++word) for (unsigned bit=0;bit<21U;++bit) {
  uint32_t mask=1U<<bit;if (!(mask&(word ? ERROR1_MASK : ERROR0_MASK))) continue;
  setup_short();regs[49+word]=mask;
  assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_IO && !writes && !header_writes);
  assert((word ? short_report.error1 : short_report.error0) == mask && !regs[49+word]);sticky();
 }
 for (unsigned word=0;word<2;++word) {
  setup_short();error_at=1;error_word=word;assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_IO && header_writes == 1);
  assert((word ? short_report.error1 : short_report.error0) == 1);sticky();
 }
 setup_short();change_after=2;change_index=1;drift_mask=0x1000;
 assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_VERIFY && header_writes == 1);sticky();
 setup_short();change_after=2;change_index=22;
 assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_IO && header_writes == 1);sticky();
 for (unsigned bit=0;bit<7U;++bit) {
  setup_short();regs[46]^=1U<<bit;assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_BUSY && !writes);
 }
 setup_short();regs[47]|=2;assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_BUSY && !writes);
 for (unsigned i=0;i<51U;++i) if (i != 19 && i != 32 && i != 33 && i != 48 && i != 46 && i != 22 && i != 49 && i != 50) {
  setup_short();change_after=1;change_index=i;
  assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_VERIFY && !header_writes);sticky();
 }
 puts("PASS: bounded DCS0/1 all VCs, LP masks, one publication, read-clear errors/sticky failure, fast/unobservable completion limits");
}

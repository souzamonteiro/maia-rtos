#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
 MAIA_ESP32P4_DSI_HOST_BASE+0x34U,MAIA_ESP32P4_DSI_HOST_BASE+8U,MAIA_ESP32P4_DSI_HOST_BASE+0x9CU,MAIA_ESP32P4_DSI_HOST_BASE+0x98U,MAIA_ESP32P4_DSI_HOST_BASE+0x2CU,MAIA_ESP32P4_DSI_HOST_BASE+0x78U,MAIA_ESP32P4_DSI_HOST_BASE+0x7CU,MAIA_ESP32P4_DSI_HOST_BASE+0x80U,MAIA_ESP32P4_DSI_HOST_BASE+0x84U,MAIA_ESP32P4_DSI_HOST_BASE+0x88U,MAIA_ESP32P4_DSI_HOST_BASE+0x8CU,MAIA_ESP32P4_DSI_HOST_BASE+0xF4U,MAIA_ESP32P4_DSI_HOST_BASE+0x74U,MAIA_ESP32P4_DSI_HOST_BASE+0x68U,MAIA_ESP32P4_DSI_HOST_BASE+0x6CU,MAIA_ESP32P4_DSI_HOST_BASE+0xBCU,MAIA_ESP32P4_DSI_HOST_BASE+0xC0U,MAIA_ESP32P4_DSI_HOST_BASE+0x70U};
static uint32_t regs[52];static unsigned reads,writes,drop,context,counts[52],inject_index,inject_n,change_after,change_index;
static maia_dsi_access_report_t report;
static unsigned reference_mode,test_writes,poll_reads,lock_after,stop_after;
static maia_dsi_reference_report_t ref;
static unsigned tx_active,post_polls,drain_after,stopped_after,header_writes,accepted,drop_header,drop_lp,error_at,error_word;
static uint32_t expected_header,selected_lp,last_header,drift_mask;
static const uint8_t *expected_bytes;
static unsigned expected_length,long_request,staged_words,payload_writes,capacity,space_stalls,lp_changed,partial_error,buffer_drain_after;
static uint32_t payload_trace[16];

static const unsigned table_index[14]={34,35,36,37,38,39,40,41,42,43,44,45,30,31};
static const uint32_t masks[14]={1,0xFFFF,0x03FF03FF,0x03FF03FF,0x39,0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x7FFF,0xFF00,3};
static const uint32_t values[14]={1,0x0C06,0x00320068,0x002E0080,0x19,0,0,0,0,0,0,6000,0x3F00,0};
int maia_dsi_access_test_context(void) { return (int)context; }
uint32_t maia_dsi_access_test_read(uintptr_t address) {
 ++reads;
 for (unsigned i=0;i<52U;++i) if (addresses[i] == address) {
  if (i>=6U && i<=13U) assert((regs[0]&0x2000U) && (regs[1]&0x20U));
  if (i>=19U && i<26U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U) && (writes == 3U || reference_mode));
  if (reference_mode == 3U && i == 22U && test_writes == 35U) {
   ++poll_reads;return 0x1500U | (poll_reads>=lock_after ? 1U : 0U) | (poll_reads>=stop_after ? 0x94U : 0U);
  }
  if (reference_mode == 5U) {
   if (i>=30U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U));
   assert(i != 48U && i != 51U); /* GEN_HDR is publication, never safe readback. */
   if (i == 46U && tx_active) {
    ++post_polls;if (post_polls>=drain_after) {
     if (post_polls>=buffer_drain_after) staged_words=0;
     return 0x10015U|(post_polls>=buffer_drain_after ? 0x40000U : 0U);
    }return long_request ? 0x10 : 0x14;
   }
   if (i == 46U && long_request) {
    if (lp_changed && space_stalls) { --space_stalls;return 0x90019; }
    return staged_words>=capacity ? 0x90019 : staged_words ? 0x10011 : 0x50015;
   }
   if (i == 22U && tx_active) return regs[i] | (post_polls>=stopped_after ? 0x94U : 0x84U);
   if (i == 49U || i == 50U) {
    uint32_t result=regs[i];regs[i]=0;
    if (!tx_active && partial_error && staged_words) result|=1; /* Potential read-to-clear semantics modeled. */
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
   assert(value == (regs[47]|selected_lp));if (!drop_lp) { regs[47]=value;lp_changed=1; }
  } else if (address == addresses[51]) {
   assert(long_request && staged_words<capacity && staged_words<16U);
   unsigned offset=staged_words*4U,count=expected_length-offset;if (count>4) count=4;
   uint32_t expected=0;for (unsigned n=0;n<count;++n) expected|=(uint32_t)expected_bytes[offset+n]<<(8U*n);
   assert(offset<expected_length && value == expected);payload_trace[staged_words]=value;++staged_words;++payload_writes;
  } else {
   assert(address == addresses[48] && value == expected_header);
   if (long_request) assert(staged_words == (expected_length+3U)/4U);else assert(!staged_words);
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
 for (unsigned i=0;i<52U;++i) { regs[i]=0;counts[i]=0; }
 regs[0]=0xE6DFB7FF;regs[1]=0x0C0D4278;regs[2]=0x220;regs[3]=0xBC000041;regs[4]=0x02E82000;
 regs[14]=0x313;regs[15]=(55U<<6U)|(33U<<14U)|(33U<<20U);regs[17]=0x40200180;regs[18]=0xDD812345;
 regs[19]=0x3134312A;
}
static maia_dsi_access_result_t open_config(void) { return maia_esp32p4_dsi_open_config(&report); }
static void setup_reference(void) {
 setup();assert(open_config() == MAIA_DSI_ACCESS_OK);
 reads=writes=0;reference_mode=1;
 for (unsigned i=0;i<52U;++i) counts[i]=0;
 regs[26]=0xA51705F1;regs[27]=0x0A12ABCD;regs[28]=0xA0FF00FF;regs[29]=0xA1234567;
}
static maia_dsi_access_result_t reference(void) { return maia_esp32p4_dsi_reference20(&ref); }
static maia_dsi_phy_report_t phy_report;
static void setup_phy(void) {
 setup_reference();assert(reference() == MAIA_DSI_ACCESS_OK);
 reference_mode=3;reads=writes=test_writes=poll_reads=0;lock_after=2;stop_after=4;
 for (unsigned i=0;i<52U;++i) counts[i]=0;
 regs[30]=0xABCD1200;regs[31]=0x40000000;regs[32]=0x40000001;regs[33]=0x8000EE00;
}
static maia_dsi_access_result_t phy(unsigned budget) { return maia_esp32p4_dsi_phy960(budget,&phy_report); }
static maia_dsi_command_report_t command_report;
static void setup_command(int matching) {
 setup_phy();assert(phy(8) == MAIA_DSI_ACCESS_OK);
 reference_mode=4;reads=writes=0;
 for (unsigned i=0;i<52U;++i) counts[i]=0;
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
 for (unsigned i=0;i<52U;++i) counts[i]=0;
}
static maia_dsi_access_result_t send(uint32_t vc,uint32_t command,uint32_t count,uint32_t parameter,uint32_t budget) {
 long_request=0;
 expected_header=(parameter<<16)|(command<<8)|(vc<<6)|(count ? 0x15U : 5U);selected_lp=1U<<(16+(count&1));
 return maia_esp32p4_dsi_dcs_short(vc,command,count,parameter,budget,&short_report);
}
static maia_dsi_long_report_t long_report;
static void setup_long(void) {
 setup_short();long_request=1;staged_words=payload_writes=space_stalls=lp_changed=partial_error=0;capacity=16;buffer_drain_after=2;
 for (unsigned i=0;i<16;++i) payload_trace[i]=0;
}
static maia_dsi_access_result_t transmit(uint32_t vc,const uint8_t *bytes,unsigned length,unsigned budget) {
 long_request=1;expected_header=(length<<8U)|(vc<<6U)|0x39U;selected_lp=1U<<19;
 expected_bytes=bytes;expected_length=length;
 return maia_esp32p4_dsi_dcs_long(vc,bytes,length,budget,&long_report);
}
static void sticky(void) {
 unsigned saved=reads,written=writes,headers=header_writes;
 uint8_t nop=0;
 assert(transmit(0,&nop,1,100) == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == written && header_writes == headers);
 assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == written && header_writes == headers);
}
int main(void) {
 uint8_t nop=0;
 setup();assert(transmit(0,&nop,1,8) == MAIA_DSI_ACCESS_BUSY && !reads && !writes);
 assert(maia_esp32p4_dsi_dcs_long(0,&nop,1,8,NULL) == MAIA_DSI_ACCESS_INVALID);
 setup_long();context=0;assert(transmit(0,&nop,1,8) == MAIA_DSI_ACCESS_CONTEXT && !reads && !writes);
 setup_long();assert(transmit(4,&nop,1,8) == MAIA_DSI_ACCESS_INVALID && !reads);
 assert(transmit(0,NULL,1,8) == MAIA_DSI_ACCESS_INVALID && !reads);
 assert(transmit(0,&nop,0,8) == MAIA_DSI_ACCESS_INVALID && !reads);
 assert(transmit(0,&nop,65,8) == MAIA_DSI_ACCESS_INVALID && !reads);
 assert(transmit(0,&nop,1,0) == MAIA_DSI_ACCESS_INVALID && !reads);
 assert(transmit(0,&nop,1,100001) == MAIA_DSI_ACCESS_INVALID && !reads);
 /* Exact end-of-allocation and unaligned buffers under ASan: no fourth-byte read
  * when remaining1..3. Every length1..64 exercises complete/tail groups. */
 for (unsigned length=1;length<=64;++length) for (unsigned vc=0;vc<4;++vc) {
  uint8_t *allocation=malloc(length+1U);assert(allocation);uint8_t *bytes=allocation+1U;
  for (unsigned i=0;i<length;++i) bytes[i]=(uint8_t)(i*37U+0x51U);
  setup_long();assert(transmit(vc,bytes,length,100) == MAIA_DSI_ACCESS_OK);
  assert(long_report.transfer.completed == 6 && long_report.transfer.header == expected_header && header_writes == 1);
  assert(long_report.payload_bytes_attempted == length && long_report.payload_words_attempted == (length+3U)/4U && payload_writes == (length+3U)/4U);
  assert(long_report.space_polls == payload_writes+1 && long_report.transfer.polls == long_report.space_polls+4U);
  if (length%4U) assert((payload_trace[payload_writes-1]>>(8U*(length%4U))) == 0U);
  free(allocation);
 }
 setup_long();capacity=1;assert(transmit(0,&nop,1,20) == MAIA_DSI_ACCESS_OK); /* Full payload FIFO still permits header. */
 setup_long();drain_after=stopped_after=1;buffer_drain_after=4;
 assert(transmit(0,&nop,1,20) == MAIA_DSI_ACCESS_OK && long_report.transfer.polls == 6);
 setup_long();drain_after=stopped_after=1;buffer_drain_after=99;
 assert(transmit(0,&nop,1,4) == MAIA_DSI_ACCESS_TIMEOUT && header_writes == 1);sticky();
 setup_long();space_stalls=2;assert(transmit(0,&nop,1,20) == MAIA_DSI_ACCESS_OK && long_report.space_polls == 4);
 setup_long();space_stalls=100;assert(transmit(0,&nop,1,3) == MAIA_DSI_ACCESS_TIMEOUT && !payload_writes && !header_writes);sticky();
 uint8_t bytes[8]={0xFF,1,2,3,4,5,6,7};
 setup_long();capacity=1;assert(transmit(0,bytes,5,8) == MAIA_DSI_ACCESS_TIMEOUT && payload_writes == 1 && !header_writes && long_report.payload_bytes_attempted == 4);sticky();
 setup_long();assert(transmit(0,&nop,1,1) == MAIA_DSI_ACCESS_TIMEOUT && payload_writes == 1 && !header_writes);sticky();
 setup_long();drain_after=99;assert(transmit(0,&nop,1,4) == MAIA_DSI_ACCESS_TIMEOUT && header_writes == 1 && long_report.transfer.polls == 4);sticky();
 setup_long();partial_error=1;assert(transmit(0,bytes,5,8) == MAIA_DSI_ACCESS_IO && payload_writes == 1 && !header_writes);sticky();
 setup_long();drop_lp=1;assert(transmit(0,&nop,1,8) == MAIA_DSI_ACCESS_VERIFY && !payload_writes && !header_writes);sticky();
 setup_long();change_after=2;change_index=1;drift_mask=0x1000;
 assert(transmit(0,bytes,5,8) == MAIA_DSI_ACCESS_VERIFY && payload_writes == 1 && !header_writes);sticky();
 setup_long();assert(transmit(0,&nop,1,20) == MAIA_DSI_ACCESS_OK);
 assert(send(0,0,0,0,20) == MAIA_DSI_ACCESS_OK && header_writes == 2 && !staged_words);
 setup_long();drain_after=99;assert(send(0,0,0,0,2) == MAIA_DSI_ACCESS_TIMEOUT);sticky(); /* Shared failure in opposite direction. */
 puts("PASS: long lengths1..64/allVC/unaligned exact tails, aggregate budget/FIFO cap, full-final FIFO, partial/failure claims and short interop");
}

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
 MAIA_ESP32P4_DSI_HOST_BASE+0x34U,MAIA_ESP32P4_DSI_HOST_BASE+8U,MAIA_ESP32P4_DSI_HOST_BASE+0x9CU,MAIA_ESP32P4_DSI_HOST_BASE+0x98U,MAIA_ESP32P4_DSI_HOST_BASE+0x2CU,MAIA_ESP32P4_DSI_HOST_BASE+0x78U,MAIA_ESP32P4_DSI_HOST_BASE+0x7CU,MAIA_ESP32P4_DSI_HOST_BASE+0x80U,MAIA_ESP32P4_DSI_HOST_BASE+0x84U,MAIA_ESP32P4_DSI_HOST_BASE+0x88U,MAIA_ESP32P4_DSI_HOST_BASE+0x8CU,MAIA_ESP32P4_DSI_HOST_BASE+0xF4U,MAIA_ESP32P4_DSI_HOST_BASE+0x74U,MAIA_ESP32P4_DSI_HOST_BASE+0x68U,MAIA_ESP32P4_DSI_HOST_BASE+0x6CU,MAIA_ESP32P4_DSI_HOST_BASE+0xBCU,MAIA_ESP32P4_DSI_HOST_BASE+0xC0U,MAIA_ESP32P4_DSI_HOST_BASE+0x30U,MAIA_ESP32P4_DSI_HOST_BASE+0x70U};
static uint32_t regs[53];static unsigned reads,writes,drop,context,counts[53],inject_index,inject_n,change_after,change_index;
static const uintptr_t dpi_extra_addresses[]={
 MAIA_ESP32P4_DSI_HOST_BASE+0x0C,MAIA_ESP32P4_DSI_HOST_BASE+0x10,MAIA_ESP32P4_DSI_HOST_BASE+0x14,
 MAIA_ESP32P4_DSI_HOST_BASE+0x38,MAIA_ESP32P4_DSI_HOST_BASE+0x3C,MAIA_ESP32P4_DSI_HOST_BASE+0x40,
 MAIA_ESP32P4_DSI_HOST_BASE+0x44,MAIA_ESP32P4_DSI_HOST_BASE+0x48,MAIA_ESP32P4_DSI_HOST_BASE+0x4C,
 MAIA_ESP32P4_DSI_HOST_BASE+0x50,MAIA_ESP32P4_DSI_HOST_BASE+0x54,MAIA_ESP32P4_DSI_HOST_BASE+0x58,
 MAIA_ESP32P4_DSI_HOST_BASE+0x5C,MAIA_ESP32P4_DSI_HOST_BASE+0x60,
 MAIA_ESP32P4_DSI_BRG_BASE+0x18,MAIA_ESP32P4_DSI_BRG_BASE+0x30,MAIA_ESP32P4_DSI_BRG_BASE+0x34,
 MAIA_ESP32P4_DSI_BRG_BASE+0x38,MAIA_ESP32P4_DSI_BRG_BASE+0x3C};
static const uint32_t dpi_test_masks[]={3,0x10F,0x1F,0xBF03,0x3FFF,0x1FFF,0x1FFF,0xFFF,0xFFF,0x7FFF,0x3FF,0x3FF,0x3FF,0x3FFF,0x7F,0x0FFF0FFF,0x0FFF0FFF,0x0FFF0FFF,0x0FFF0FFF};
static const uint32_t dpi_test_values[]={0,0,0,0xBF02,720,0,0,3,60,1203,20,24,200,1280,2,0x050005F4,0x00140018,0x02D00322,0x00020028};
static uint32_t dpi_regs[19],dpi_old[19],dpi_root;
static unsigned dpi_drop,dpi_drift,dpi_fault_after,clock_writes,clock_drop,clock_drift,clock_fault;
static unsigned pattern_writes,pattern_drop,pattern_drift,pattern_fault,pattern_idle_reads,pattern_timeout;
static uint32_t pattern_shadow,bridge_irq_enable,bridge_irq_raw;
static unsigned bridge_writes,bridge_drop,bridge_drift,bridge_fault,bridge_underflow,bridge_update_reads;
static maia_dsi_command_report_t command_report;
static maia_dsi_access_report_t report;
static unsigned reference_mode,test_writes,poll_reads,lock_after,stop_after;
static maia_dsi_reference_report_t ref;
static unsigned tx_active,post_polls,drain_after,stopped_after,header_writes,accepted,drop_header,drop_lp,error_at,error_word;
static uint32_t expected_header,selected_lp,last_header,drift_mask;
static unsigned read_phase,phase_polls,mrps_delay,reply_delay,idle_delay,busy_clear_at,pops,extra,read_error_phase,read_error_word,drop_read_write;
static uint32_t reply_word,original_mode,original_packet,original_vc;
static uint32_t read_error_mask,requested_mrps,requested_command;
static unsigned error_when_ready;
static const unsigned table_index[14]={34,35,36,37,38,39,40,41,42,43,44,45,30,31};
static const uint32_t masks[14]={1,0xFFFF,0x03FF03FF,0x03FF03FF,0x39,0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x7FFF,0xFF00,3};
static const uint32_t values[14]={1,0x0C07,0x00320068,0x002E0080,0x19,0,0,0,0,0,0,6000,0x3F00,0};
int maia_dsi_access_test_context(void) { return (int)context; }
uint32_t maia_dsi_access_test_read(uintptr_t address) {
 ++reads;
 if(reference_mode == 10U) {
  assert((regs[1]&0x1000U) || address == DSI_CPU || address == DSI_SYS || address == DSI_PHY || address == DSI_RESET || address == DSI_ISP || address == DSI_NORST || address == DSI_REF_DIV || address == DSI_LP_GATE || address == DSI_REF_GATE || address == DSI_REF_SOURCE);
  if(address == MAIA_ESP32P4_DSI_HOST_BASE+0x100U)return pattern_shadow;
  if(address == MAIA_ESP32P4_DSI_BRG_BASE+0x50U)return bridge_irq_enable;
  if(address == MAIA_ESP32P4_DSI_BRG_BASE+0x58U)return bridge_irq_raw;
  if(address == MAIA_ESP32P4_DSI_BRG_BASE+0x44U){++bridge_update_reads;assert(!"WT is never read as acknowledgment");}
 }
 if (reference_mode == 9U && address == MAIA_ESP32P4_DSI_HOST_BASE+0x100U) return pattern_shadow;
 if (reference_mode == 9U && address == MAIA_ESP32P4_DSI_HOST_BASE+0xB0U) {
  if (!(regs[34]&1U) || (regs[31]&3U)) return 0x1591;
  if (pattern_writes>=5U && (pattern_timeout || pattern_idle_reads++<2U)) return 0x1591;
  return 0x1595;
 }
 if (reference_mode == 7U || reference_mode == 8U || reference_mode == 9U || reference_mode == 10U) {
  for (unsigned i=0;i<19U;++i) if (address == dpi_extra_addresses[i]) {
   assert((regs[2]&0x13U)==3 && (regs[1]&0x1000U));return dpi_regs[i];
  }
 }
 for (unsigned i=0;i<53U;++i) if (addresses[i] == address) {
  if (i>=6U && i<=13U) assert((regs[0]&0x2000U) && (regs[1]&0x20U));
  if (i>=19U && i<26U) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U) && (writes == 3U || reference_mode));
  if (reference_mode == 3U && i == 22U && test_writes == 35U) {
   ++poll_reads;return 0x1500U | (poll_reads>=lock_after ? 1U : 0U) | (poll_reads>=stop_after ? 0x94U : 0U);
  }
  if (reference_mode == 6U) {
   if (i>=19U && (i<26U || i>=30U)) assert((regs[1]&0x1000U) && (regs[2]&1U) && !(regs[3]&0x04000000U));
   assert(i != 48U);
   if (i == 46U) {
    ++phase_polls;
    if (read_phase == 1U) return phase_polls>=mrps_delay ? 0x50015 : 0x10014;
    if (read_phase == 2U) return phase_polls>=reply_delay ? 0x50005 : phase_polls>=busy_clear_at ? 0x50015 : 0x50055;
    if (read_phase == 3U && extra) return 0x50005;
    return regs[i];
   }
   if (i == 22U && read_phase) {
    if (read_phase == 2U || (read_phase == 3U && phase_polls<idle_delay)) return 0x1597;
    return 0x1595;
   }
   if (i == 49U || i == 50U) {
    uint32_t value=regs[i];regs[i]=0;
    if (read_phase == read_error_phase && read_phase && i == 49U+read_error_word &&
        (!error_when_ready || phase_polls>=reply_delay)) value|=read_error_mask;
    return value;
   }
   if (i == 52U) {
    assert(read_phase == 2U && phase_polls>=reply_delay && !pops);
    ++pops;read_phase=3;phase_polls=0;return reply_word;
   }
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
 if(reference_mode == 10U) {
  ++bridge_writes;
  if(bridge_writes == 1U) {
   assert(address == MAIA_ESP32P4_DSI_BRG_BASE+4U && value == (regs[23]|1U));
   if(bridge_writes != bridge_drop)regs[23]=value;
  } else {
   assert(bridge_writes == 2U && address == MAIA_ESP32P4_DSI_BRG_BASE+0x44U && value == 1U);
   /* A dropped WT publication has no readable ACK. Its nondetection is explicit. */
  }
  if(bridge_writes == bridge_drift)regs[1]^=0x1000U;
  if(bridge_writes == bridge_fault)regs[49]=0x20U;
  if(bridge_writes == bridge_underflow)bridge_irq_raw|=1U;
  return;
 }
 if (reference_mode == 9U) {
  static const uintptr_t order[]={MAIA_ESP32P4_DSI_HOST_BASE+0x38U,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,
   MAIA_ESP32P4_DSI_HOST_BASE+0x34U,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,
   MAIA_ESP32P4_DSI_HOST_BASE+0x94U,MAIA_ESP32P4_DSI_HOST_BASE+0x38U};
  uint32_t expected[]={ (dpi_old[3]&~0x1110000U)|0x10000U,values[13]|3U,
   values[0]&~1U,values[0],values[13],dpi_old[3] };
  unsigned n=pattern_writes++;assert(n<6 && address == order[n]);
  /* Full upper bits of inherited LP/mode values are preserved. */
  uint32_t mode=command_report.after[0],lp=command_report.after[13];
  expected[1]=lp|3U;expected[2]=mode&~1U;expected[3]=mode;expected[4]=lp;
  assert(value == expected[n]);
  if (pattern_writes != pattern_drop) {
   if (n == 0U || n == 5U) dpi_regs[3]=value;
   else regs[n == 1U || n == 4U ? 31U : 34U]=value;
  }
  if (pattern_writes == pattern_drift) regs[1]^=0x1000U;
  if (pattern_writes == pattern_fault) regs[49]=0x20U;
  return;
 }
 if (reference_mode == 8U) {
  ++clock_writes;assert(address == DSI_REF_DIV || address == DSI_PHY);
  unsigned index=address == DSI_REF_DIV ? 26 : 2;
  assert(value == (regs[index]|(index == 26 ? 0x40000000U : 0x80U)));
  if (clock_writes != clock_drop) regs[index]=value;
  if (clock_writes == clock_drift) regs[1]^=0x1000;
  if (clock_writes == clock_fault) regs[49]=0x100000;
  return;
 }
 if (reference_mode == 7U) {
  unsigned index=writes-1;
  if (index == 0) {
   assert(address == DSI_PHY && value == ((dpi_root&~0xFF60U)|0x220U));
   if (writes != dpi_drop) regs[2]=value;
  } else {
   --index;assert(index<19 && address == dpi_extra_addresses[index]);
   assert(value == ((dpi_old[index]&~dpi_test_masks[index])|dpi_test_values[index]));
   if (writes != dpi_drop) dpi_regs[index]=value;
  }
  if (writes == dpi_drift) regs[1]^=0x1000;
  if (writes == dpi_fault_after) regs[49]=0x100000;
  return;
 }
 if (reference_mode == 6U) {
  static const unsigned indices[8]={47,48,38,51,48,38,51,47};
  uint32_t values[8]={original_mode|0x01040000U,requested_mrps,original_packet|4U,original_vc&~3U,requested_command,original_packet,original_vc,original_mode};
  assert(writes<=8U && address == addresses[indices[writes-1U]] && value == values[writes-1U]);
  if (writes>=6U) assert(pops == 1U && read_phase == 3U && phase_polls>=idle_delay);
  if (writes != drop_read_write) {
   if (writes == 2U) { read_phase=1;phase_polls=0; }
   else if (writes == 5U) { read_phase=2;phase_polls=0; }
   else regs[indices[writes-1U]]=value;
  }
  return;
 }
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
 for (unsigned i=0;i<53U;++i) { regs[i]=0;counts[i]=0; }
 regs[0]=0xE6DFB7FF;regs[1]=0x0C0D4278;regs[2]=0x220;regs[3]=0xBC000041;regs[4]=0x02E82000;
 regs[14]=0x313;regs[15]=(55U<<6U)|(33U<<14U)|(33U<<20U);regs[17]=0x40200180;regs[18]=0xDD812345;
 regs[19]=0x3134312A;
}
static maia_dsi_access_result_t open_config(void) { return maia_esp32p4_dsi_open_config(&report); }
static void setup_reference(void) {
 setup();assert(open_config() == MAIA_DSI_ACCESS_OK);
 reads=writes=0;reference_mode=1;
 for (unsigned i=0;i<53U;++i) counts[i]=0;
 regs[26]=0xA51705F1;regs[27]=0x0A12ABCD;regs[28]=0xA0FF00FF;regs[29]=0xA1234567;
}
static maia_dsi_access_result_t reference(void) { return maia_esp32p4_dsi_reference20(&ref); }
static maia_dsi_phy_report_t phy_report;
static void setup_phy(void) {
 setup_reference();assert(reference() == MAIA_DSI_ACCESS_OK);
 reference_mode=3;reads=writes=test_writes=poll_reads=0;lock_after=2;stop_after=4;
 for (unsigned i=0;i<53U;++i) counts[i]=0;
 regs[30]=0xABCD1200;regs[31]=0x40000000;regs[32]=0x40000001;regs[33]=0x8000EE00;
}
static maia_dsi_access_result_t phy(unsigned budget) { return maia_esp32p4_dsi_phy960(budget,&phy_report); }
static void setup_command(int matching) {
 setup_phy();assert(phy(8) == MAIA_DSI_ACCESS_OK);
 reference_mode=4;reads=writes=0;
 for (unsigned i=0;i<53U;++i) counts[i]=0;
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
 for (unsigned i=0;i<53U;++i) counts[i]=0;
}
static maia_dsi_access_result_t send(uint32_t vc,uint32_t command,uint32_t count,uint32_t parameter,uint32_t budget) {
 expected_header=(parameter<<16)|(command<<8)|(vc<<6)|(count ? 0x15U : 5U);selected_lp=1U<<(16+(count&1));
 return maia_esp32p4_dsi_dcs_short(vc,command,count,parameter,budget,&short_report);
}
static maia_dsi_read_report_t received;
static void setup_read(void) {
 setup_short();reference_mode=6;reads=writes=0;
 for (unsigned i=0;i<53U;++i) counts[i]=0;
 regs[46]=0x50015;regs[47]=original_mode=0x80500000;original_packet=regs[38];regs[51]=original_vc=0xA5040003;
 read_phase=phase_polls=pops=extra=read_error_phase=read_error_word=drop_read_write=0;
 read_error_mask=1;error_when_ready=0;requested_mrps=0x337;requested_command=0x406;
 mrps_delay=2;reply_delay=3;idle_delay=2;busy_clear_at=2;reply_word=0xAABBCCDD;
}
static maia_dsi_access_result_t read_id(unsigned budget) { return maia_esp32p4_dsi_read_id(budget,&received); }
static void sticky(void) {
 unsigned saved=reads,written=writes;
 assert(read_id(20) == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == written);
 assert(send(0,0,0,0,8) == MAIA_DSI_ACCESS_FAILED && reads == saved && writes == written);
}

static maia_dsi_dpi_timing_t timing={720,1280,2,40,40,20,24,200};
static maia_dsi_dpi_report_t staged;
static void setup_dpi(void) {
 setup_read();reference_mode=7;writes=reads=dpi_drop=dpi_drift=dpi_fault_after=0;
 regs[2]&=~0xFF60U;dpi_root=regs[2];
 for (unsigned i=0;i<19;++i) dpi_regs[i]=dpi_old[i]=0xA4000000U|((~dpi_test_values[i])&dpi_test_masks[i]);
}
static void fail_sticky(void) {
 unsigned old_reads=reads,old_writes=writes;
 assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_FAILED);
 assert(read_id(20) == MAIA_DSI_ACCESS_FAILED);
 assert(maia_esp32p4_dsi_read_power_mode(20,&received) == MAIA_DSI_ACCESS_FAILED);
 assert(send(0,0,0,0,20) == MAIA_DSI_ACCESS_FAILED);
 assert(reads == old_reads && writes == old_writes);sticky();
}
static maia_dsi_dpi_clock_report_t enabled;
static void setup_clock(unsigned provider_on) {
 setup_dpi();if (provider_on) regs[26]|=0x40000000U;else regs[26]&=~0x40000000U;
 assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_OK);
 reference_mode=8;writes=reads=clock_writes=clock_drop=clock_drift=clock_fault=0;
}
static maia_dsi_dpi_pattern_report_t patterned;
static void setup_pattern(void) {
 setup_clock(1);assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_OK);
 reference_mode=9;reads=writes=pattern_writes=pattern_drop=pattern_drift=pattern_fault=pattern_idle_reads=pattern_timeout=0;
 pattern_shadow=0;dpi_old[3]=dpi_regs[3];
}
static maia_dsi_access_result_t pattern_start(void) { return maia_esp32p4_dsi_dpi_pattern_start(&patterned); }
static maia_dsi_access_result_t pattern_stop(unsigned n) { return maia_esp32p4_dsi_dpi_pattern_stop(n,&patterned); }
static void pattern_failed(void) {
 unsigned a=reads,b=writes;
 assert(pattern_start() == MAIA_DSI_ACCESS_FAILED && pattern_stop(20) == MAIA_DSI_ACCESS_FAILED);
 fail_sticky();assert(a == reads && b == writes);
}
static maia_dsi_dpi_bridge_report_t prepared;
static void setup_bridge(void) {
 setup_clock(1);assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_OK);
 reference_mode=10;reads=writes=bridge_writes=bridge_drop=bridge_drift=bridge_fault=bridge_underflow=bridge_update_reads=0;
 bridge_irq_enable=bridge_irq_raw=pattern_shadow=0;regs[23]=0xA4000000;
}
static maia_dsi_access_result_t prepare_bridge(void) { return maia_esp32p4_dsi_dpi_bridge_prepare(&prepared); }
static void bridge_failed(void) {
 unsigned a=reads,b=writes;
 assert(prepare_bridge() == MAIA_DSI_ACCESS_FAILED && pattern_start() == MAIA_DSI_ACCESS_FAILED);
 fail_sticky();assert(a == reads && b == writes);
}
int main(void) {
 setup_dpi();assert(maia_esp32p4_dsi_dpi_stage(NULL,&staged) == MAIA_DSI_ACCESS_INVALID && !reads && !writes);
 assert(maia_esp32p4_dsi_dpi_stage(&timing,NULL) == MAIA_DSI_ACCESS_INVALID);
 maia_dsi_dpi_timing_t bad=timing;bad.width=0xFFFFFFFF;
 assert(maia_esp32p4_dsi_dpi_stage(&bad,&staged) == MAIA_DSI_ACCESS_INVALID && !reads);
 bad=timing;bad.height=4095;assert(maia_esp32p4_dsi_dpi_stage(&bad,&staged) == MAIA_DSI_ACCESS_INVALID && !reads);
 bad=timing;bad.hsync=3000;assert(maia_esp32p4_dsi_dpi_stage(&bad,&staged) == MAIA_DSI_ACCESS_INVALID && !reads);
 context=0;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_CONTEXT && !reads);
 setup_dpi();assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_OK);
 assert(staged.completed == 22 && staged.attempted_mask == 0xFFFFF && writes == 20 && staged.nominal_hz == 80000000);
 for (unsigned i=0;i<19;++i) assert((staged.after[i]&dpi_test_masks[i]) == dpi_test_values[i] && staged.before[i] == dpi_old[i]);
 unsigned saved=reads;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_BUSY && reads == saved && writes == 20);
 reference_mode=6;writes=reads=read_phase=phase_polls=pops=0;
 assert(read_id(20) == MAIA_DSI_ACCESS_OK); /* Staged/gated DPI does not break command reads. */
 setup_dpi();regs[2]=(regs[2]&~0xFF60U)|0x220U;
 for (unsigned i=0;i<19;++i) dpi_regs[i]=(dpi_regs[i]&~dpi_test_masks[i])|dpi_test_values[i];
 assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_OK && !writes && !staged.attempted_mask);
 for (unsigned i=1;i<=20;++i) {
  setup_dpi();dpi_drop=i;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_VERIFY && writes == i);fail_sticky();
 }
 setup_dpi();dpi_drift=3;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_VERIFY && writes == 3);fail_sticky();
 setup_dpi();dpi_fault_after=5;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_IO && writes == 5);fail_sticky();
 setup_dpi();regs[2]|=0x80;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_dpi();regs[24]|=1;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_dpi();regs[25]|=1;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_dpi();dpi_regs[3]|=0x10000;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_dpi();regs[20]=0;assert(maia_esp32p4_dsi_dpi_stage(&timing,&staged) == MAIA_DSI_ACCESS_BUSY && !writes);
 /* Real private staging claim, cached config/root and owned clock lifecycle. */
 setup_read();assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_BUSY);
 assert(maia_esp32p4_dsi_dpi_clock_enable(NULL) == MAIA_DSI_ACCESS_INVALID);
 for (unsigned provider=0;provider<2;++provider) {
  setup_clock(provider);assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_OK);
  assert(enabled.completed == 4 && enabled.attempted_mask == (provider ? 2U : 3U) && clock_writes == (provider ? 1U : 2U));
  assert((enabled.clock_after&0xFFF3U) == 0x2A3U && (enabled.provider_after&0x40000000U));
  unsigned n=reads;assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_BUSY && reads == n);
  reference_mode=6;writes=reads=pops=read_phase=phase_polls=0;
  assert(read_id(20) == MAIA_DSI_ACCESS_OK); /* LP still works with owned DPI gate. */
 }
 for (unsigned dropped=1;dropped<=2;++dropped) {
  setup_clock(0);clock_drop=dropped;
  assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_VERIFY && clock_writes == dropped);
  assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_FAILED);fail_sticky();
 }
 setup_clock(1);context=0;assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_CONTEXT && !reads && !clock_writes);
 setup_clock(0);clock_fault=1;assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_IO && clock_writes == 1);fail_sticky();
 setup_clock(0);clock_drift=1;assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_VERIFY && clock_writes == 1);fail_sticky();
 setup_clock(1);dpi_regs[17]^=1;assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_VERIFY && !clock_writes);
 setup_clock(1);regs[2]|=0x80;assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_VERIFY && !clock_writes);
 reference_mode=6;writes=0;assert(read_id(20) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_clock(1);assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_OK);
 regs[26]&=~0x40000000U;reference_mode=6;writes=0;assert(read_id(20) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_clock(1);assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_OK);
 regs[2]^=0x100;reference_mode=6;writes=0;assert(read_id(20) == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_clock(1);assert(maia_esp32p4_dsi_dpi_clock_enable(&enabled) == MAIA_DSI_ACCESS_OK);
 regs[2]&=~0x80U;reference_mode=6;writes=reads=0;
 assert(read_id(20) == MAIA_DSI_ACCESS_BUSY && !writes); /* Clock drift is not adopted. */
 /* Actual driver VPG cycle, start/stop ordering, six dropped stores, exclusive
  * transport, shadow/config/source failures, bounded stop and restored reads. */
 setup_clock(1);assert(pattern_start() == MAIA_DSI_ACCESS_BUSY);
 setup_pattern();assert(maia_esp32p4_dsi_dpi_pattern_start(NULL) == MAIA_DSI_ACCESS_INVALID && !reads && !writes);
 assert(pattern_stop(0) == MAIA_DSI_ACCESS_INVALID && !reads);
 assert(maia_esp32p4_dsi_dpi_pattern_stop(1,NULL) == MAIA_DSI_ACCESS_INVALID);
 context=0;assert(pattern_start() == MAIA_DSI_ACCESS_CONTEXT && !reads);
 setup_pattern();assert(pattern_start() == MAIA_DSI_ACCESS_OK && patterned.completed == 4 && patterned.attempted_mask == 7);
 assert(patterned.pattern == ((dpi_old[3]&~0x1110000U)|0x10000U) && !(patterned.mode&1U) && (patterned.lp_clock&3U) == 3U);
 saved=reads;assert(pattern_start() == MAIA_DSI_ACCESS_BUSY && reads == saved);
 assert(read_id(20) == MAIA_DSI_ACCESS_BUSY && reads == saved);
 assert(send(0,0,0,0,20) == MAIA_DSI_ACCESS_BUSY && reads == saved);
 assert(prepare_bridge() == MAIA_DSI_ACCESS_BUSY && reads == saved);
 assert(pattern_stop(100001) == MAIA_DSI_ACCESS_INVALID && reads == saved);
 assert(pattern_stop(10) == MAIA_DSI_ACCESS_OK && patterned.completed == 5 && patterned.attempted_mask == 7 && patterned.observation.polls == 2);
 assert(patterned.pattern == dpi_old[3] && patterned.mode == command_report.after[0] && patterned.lp_clock == command_report.after[13]);
 saved=reads;assert(pattern_stop(10) == MAIA_DSI_ACCESS_BUSY && pattern_start() == MAIA_DSI_ACCESS_BUSY && reads == saved);
 reference_mode=6;writes=reads=pops=read_phase=phase_polls=0;assert(read_id(20) == MAIA_DSI_ACCESS_OK);
 for (unsigned n=1;n<=6;++n) {
  setup_pattern();pattern_drop=n;
  if (n<=3) assert(pattern_start() == MAIA_DSI_ACCESS_VERIFY);
  else { assert(pattern_start() == MAIA_DSI_ACCESS_OK);assert(pattern_stop(10) == MAIA_DSI_ACCESS_VERIFY); }
  assert(pattern_writes == n);pattern_failed();
 }
 setup_pattern();pattern_shadow=1;assert(pattern_start() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_pattern();dpi_regs[10]^=1;assert(pattern_start() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_pattern();regs[26]&=~0x40000000U;assert(pattern_start() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_pattern();regs[49]=0x20;assert(pattern_start() == MAIA_DSI_ACCESS_IO && !writes);pattern_failed();
 for (unsigned n=1;n<=6;++n) {
  setup_pattern();pattern_drift=n;
  if (n<=3) assert(pattern_start() == MAIA_DSI_ACCESS_VERIFY);
  else { assert(pattern_start() == MAIA_DSI_ACCESS_OK);assert(pattern_stop(10) == MAIA_DSI_ACCESS_VERIFY); }
  assert(pattern_writes == n);pattern_failed();
 }
 setup_pattern();assert(pattern_start() == MAIA_DSI_ACCESS_OK);pattern_timeout=1;
 assert(pattern_stop(2) == MAIA_DSI_ACCESS_TIMEOUT && patterned.observation.polls == 2 && pattern_writes == 5);pattern_failed();
 setup_pattern();assert(pattern_start() == MAIA_DSI_ACCESS_OK);dpi_regs[4]^=1;
 assert(pattern_stop(10) == MAIA_DSI_ACCESS_VERIFY && pattern_writes == 3);pattern_failed();
 setup_pattern();pattern_fault=3;assert(pattern_start() == MAIA_DSI_ACCESS_IO);pattern_failed();
 /* Actual native bridge enable and write-trigger publication. */
 setup_clock(1);assert(prepare_bridge() == MAIA_DSI_ACCESS_BUSY);
 setup_bridge();assert(maia_esp32p4_dsi_dpi_bridge_prepare(NULL) == MAIA_DSI_ACCESS_INVALID && !reads && !writes);
 context=0;assert(prepare_bridge() == MAIA_DSI_ACCESS_CONTEXT && !reads);
 setup_bridge();assert(prepare_bridge() == MAIA_DSI_ACCESS_OK);
 assert(prepared.completed == 4 && prepared.attempted_mask == 3 && prepared.bridge_before == 0xA4000000 && prepared.bridge_after == 0xA4000001 && prepared.update_value == 1 && bridge_writes == 2 && !bridge_update_reads);
 saved=reads;assert(prepare_bridge() == MAIA_DSI_ACCESS_BUSY && pattern_start() == MAIA_DSI_ACCESS_BUSY && reads == saved);
 reference_mode=6;writes=reads=pops=read_phase=phase_polls=0;assert(read_id(20) == MAIA_DSI_ACCESS_OK);
 setup_bridge();bridge_drop=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && bridge_writes == 1);bridge_failed();
 setup_bridge();bridge_drop=2;assert(prepare_bridge() == MAIA_DSI_ACCESS_OK && prepared.update_value == 1 && !bridge_update_reads);
 for(unsigned n=1;n<=2;++n){
  setup_bridge();bridge_drift=n;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && bridge_writes == n);bridge_failed();
  setup_bridge();bridge_fault=n;assert(prepare_bridge() == MAIA_DSI_ACCESS_IO && bridge_writes == n);bridge_failed();
  setup_bridge();bridge_underflow=n;assert(prepare_bridge() == MAIA_DSI_ACCESS_IO && bridge_writes == n);bridge_failed();
 }
 setup_bridge();regs[23]|=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && !writes);
 reference_mode=6;writes=0;assert(read_id(20) == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_bridge();pattern_shadow=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_bridge();bridge_irq_enable=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_bridge();bridge_irq_raw=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_IO && !writes);bridge_failed();
 setup_bridge();dpi_regs[17]^=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_bridge();regs[24]|=1;assert(prepare_bridge() == MAIA_DSI_ACCESS_VERIFY && !writes);
 setup_bridge();regs[22]&=~0x94U;assert(prepare_bridge() == MAIA_DSI_ACCESS_BUSY && !writes);
 setup_bridge();assert(prepare_bridge() == MAIA_DSI_ACCESS_OK);regs[23]&=~1U;
 reference_mode=6;writes=0;assert(read_id(20) == MAIA_DSI_ACCESS_VERIFY && !writes);
 puts("PASS: actual native DPI staging, exact80MHz route/RGB565/timing,20 dropped stores, drift/errors and sticky guarded refusal");
}

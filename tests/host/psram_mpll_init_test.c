#include <assert.h>
#include <string.h>
#include "psram_mpll_internal.h"
void maia_psram_mpll_test_reset(void);
static uint32_t root[5],route[2],control,regs[4];
static unsigned power_transitions;
static unsigned context,rate,writes,commands,busy,drop,stuck,cal_stuck,stale_end,wrong_mux;
int maia_psram_test_context(void){return (int)context;}
unsigned maia_psram_clock_mhz(void){return rate;}
maia_psram_result_t maia_esp32p4_psram_snapshot(maia_psram_snapshot_t *s){memset(s,0,sizeof *s);s->roots[1]=wrong_mux?0x9000:0x8000;s->before[7]=s->before[20]=0x10001;return MAIA_PSRAM_OK;}
static uint32_t *ptr(uintptr_t p){
 const uintptr_t a[]={MPLL_LPCLK,MPLL_LPRESET,MPLL_POWER,MPLL_CLOCK,MPLL_CAL};
 for(unsigned i=0;i<5;++i)if(p==a[i])return &root[i];
 if(p==MPLL_CONF1)return &route[0];if(p==MPLL_CONF2)return &route[1];
 assert(p==MPLL_ANA||p==MPLL_ANA+4U);return &control;
}
uint32_t maia_psram_test_read(uintptr_t p){
 if(p==PSRAM_MSPI0+0x3CU)return 0xC0000000U;
 if(p==MPLL_ANA&&busy&&!stuck){busy=0;unsigned reg=(control>>8U)&255U;
  if(control&0x1000000U){regs[reg]=(control>>16U)&255U;if(reg==2&&!cal_stuck)root[4]|=0x100;}
  control=(control&0x1ffffffU)|(regs[reg]<<16U);control&=~MPLL_BUSY;}
 return *ptr(p);
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
 ++writes;if(writes==drop)return;
 if(p==MPLL_ANA){unsigned reg=(v>>8U)&255U;assert(reg>=1&&reg<=3&&(v&255U)==0x63&&route[0]==0xA5000000&&route[1]==0x5A000200);++commands;control=v|MPLL_BUSY;busy=1;}
 else if(p==MPLL_POWER){assert(!((v^root[2])&~0x1000000U));if((v^root[2])&0x1000000U)++power_transitions;root[2]=v;if(!(v&0x1000000U)&&!stale_end)root[4]&=~0x100U;}
 else if(p==MPLL_CAL){assert(!((v^root[4])&~0x300U));root[4]=(v&~0x100U)|(root[4]&0x100U);if(!(v&0x200U)&&!stale_end)root[4]&=~0x100U;}
 else {*ptr(p)=v;assert(p==MPLL_CONF1||p==MPLL_CONF2);}
}
static void setup(void){maia_psram_mpll_test_reset();context=1;rate=20;power_transitions=0;writes=commands=busy=drop=stuck=cal_stuck=stale_end=wrong_mux=0;
 root[0]=0x8000000;root[1]=0;root[2]=0x1000000;root[3]=0x10000000;root[4]=0x34C;
 route[0]=0xA5123456;route[1]=0x5A654321;control=0;regs[1]=0x20;regs[2]=0x99;regs[3]=0x0C;
}
int main(void){maia_psram_mpll_init_report_t r;
 setup();assert(maia_esp32p4_psram_mpll_initialize400(0,&r)==MAIA_PSRAM_INVALID&&!writes);
 context=0;assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_CONTEXT&&!writes);
 setup();rate=200;assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_BUSY&&!writes);
 setup();wrong_mux=1;assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_BUSY&&!writes);
 for(unsigned end=0;end<2;++end){setup();if(!end)root[4]&=~0x100U;
  assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_OK&&r.stage==5&&r.route_restored&&r.saw_not_done&&r.mhz==400&&r.writes==4&&commands==14&&power_transitions==2);
  assert(regs[1]==0x20&&regs[2]==0x99&&regs[3]==0x3C&&root[4]==0x34C&&route[0]==0xA5123456&&route[1]==0x5A654321);
 }
 setup();stale_end=1;assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_VERIFY&&!r.route_restored);
 setup();cal_stuck=1;assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_TIMEOUT&&!r.route_restored);
 setup();stuck=1;assert(maia_esp32p4_psram_mpll_initialize400(3,&r)==MAIA_PSRAM_TIMEOUT);
 unsigned n=writes;assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_FAILED&&writes==n);
 maia_psram_mpll_report_t read;assert(maia_psram_mpll_read(10,&read)==MAIA_PSRAM_FAILED&&writes==n);
 setup();assert(maia_esp32p4_psram_mpll_initialize400(10,&r)==MAIA_PSRAM_OK);unsigned total=writes;
 for(unsigned i=1;i<=total;++i){setup();drop=i;assert(maia_esp32p4_psram_mpll_initialize400(4,&r)!=MAIA_PSRAM_OK);}
}

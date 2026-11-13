#include <assert.h>
#include <stdio.h>
#include "vo3_cycle_fixture.h"
static const uintptr_t addr[]={0x500E6018,0x500E603C,0x500E6014,0x50081010,0x50081018,0x5008101C,0x50081058,
 0x50081134,0x50081234,0x50081334,0x50081434,0x500E60A8,0x500E60C0,0x500E60CC,0x500A1008,0x500A1114,0x5009F008,0x5009F040,0x5009F044};
static uint32_t regs[19];static unsigned writes,drop;
static unsigned find(uintptr_t p) { for (unsigned i=0;i<19U;++i) if (addr[i] == p) return i;assert(!"unexpected read");return 0; }
static uint32_t rd(uintptr_t p) {
 unsigned i=find(p);
 if (i>=3U && i<=10U) assert((regs[0]&0x20U) && (regs[2]&0x2000U));
 if (i>=16U) assert((regs[0]&0xC00U) == 0xC00U && (regs[1]&0x10U) && !(regs[12]&0x18000000U));
 if (i == 14U || i == 15U) assert((regs[11]&0x02000000U) && !(regs[12]&0x20000000U));
 return regs[i];
}
static void wr(uintptr_t p,uint32_t v) {
 ++writes;assert(p == 0x500E6018 || p == 0x500E603C || p == 0x500E60C0);
 if (p == 0x500E60C0) {
   assert(v == ((v&0x18000000U) ? regs[12]|0x18000000U : regs[12]&~0x18000000U));
   if (!(v&0x18000000U)) assert((regs[0]&0xC00U) == 0xC00U && (regs[1]&0x10U));
 }
 else if (p == 0x500E6018) assert(v == (regs[0]|0xC00U) && (regs[12]&0x18000000U) == 0x18000000U);
 else assert(v == (regs[1]|0x10U) && (regs[12]&0x18000000U) == 0x18000000U);
 if (writes != drop) regs[find(p)]=v;
}
static void setup(void) {
 for (unsigned i=0;i<19U;++i) regs[i]=0;
 regs[0]=0x0C0D5278;regs[1]=0x2A3;regs[2]=0xE6DFB7FF;
 regs[11]=0x02E82000;regs[12]=0x40;regs[14]=0x40002442;regs[15]=4;writes=drop=0;
}
int main(void) {
 setup();assert(vo3_prepare_csi(rd,wr));assert(writes == 4U && regs[0] == 0x0C0D5E78 && regs[1] == 0x2B3);
 setup();regs[12]=0x08000040;assert(vo3_prepare_csi(rd,wr));assert(writes == 4U && regs[12] == 0x40U);
 setup();regs[12]=0x38000040;assert(vo3_prepare_csi(rd,wr));assert(writes == 3U && regs[12] == 0x20000040);
 for (unsigned d=1;d<=4U;++d) { setup();drop=d;assert(!vo3_prepare_csi(rd,wr));assert(writes == d); }
 for (unsigned i=3;i<=10U;++i) { setup();regs[i]=1;assert(!vo3_prepare_csi(rd,wr));assert(!writes); }
 setup();regs[0]&=~0x20U;assert(!vo3_prepare_csi(rd,wr) && !writes);
 setup();regs[2]&=~0x2000U;assert(!vo3_prepare_csi(rd,wr) && !writes);
 setup();regs[14]|=1;assert(!vo3_prepare_csi(rd,wr) && !writes);
 setup();regs[15]|=1;assert(!vo3_prepare_csi(rd,wr) && !writes);
 for (unsigned i=16;i<=18U;++i) { setup();regs[i]=1;assert(!vo3_prepare_csi(rd,wr));assert(writes == 4U); }
 puts("PASS: CSI access-clock preparation, source/DMA refusals, no gated reads and checked writes");return 0;
}

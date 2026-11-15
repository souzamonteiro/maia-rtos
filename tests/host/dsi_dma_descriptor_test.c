#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "dsi_dma_descriptor.h"
static dsi_dma_tile_request_t valid={720,1280,8,0x4FF10000,11520,0x4FF13000,10240};
static void reject(dsi_dma_tile_request_t r) {
 dsi_dma_tile_plan_t p;uint32_t words[16];for(unsigned i=0;i<16;++i)words[i]=0xA5A5A5A5;
 assert(!dsi_dma_tile_plan(&r,&p));assert(!p.source_bytes&&!p.count&&!p.list_bytes&&!p.frame_bytes);
 assert(!dsi_dma_tile_encode(&r,0,words));for(unsigned i=0;i<16;++i)assert(words[i]==0xA5A5A5A5);
}
int main(void) {
 dsi_dma_tile_plan_t p;assert(dsi_dma_tile_plan(&valid,&p));
 assert(p.source_bytes==11520&&p.count==160&&p.list_bytes==10240&&p.frame_bytes==1843200);
 uint32_t words[16],sum=0;
 for(unsigned n=0;n<p.count;++n){
  assert(dsi_dma_tile_encode(&valid,n,words));
  assert(words[0]==valid.source && !words[1] && words[2]==0x50105000 && !words[3] && words[4]==1439 && !words[5]);
  assert(words[6]==(n==159?0U:(valid.list+(n+1)*64U)|1U)&&!words[7]);
  /* Independent decoded fields, including every high/reserved word. */
  assert((words[8]&1)==1&&!(words[8]&4)&&!(words[8]&0x10)&&(words[8]&0x40));
  assert(((words[8]>>8)&7)==3&&((words[8]>>11)&7)==3&&((words[8]>>14)&15)==8&&((words[8]>>18)&15)==7);
  assert((words[8]&~0x003F7F55U)==0);
  assert((words[9]&0x80000000)&&!!(words[9]&0x40000000)==(n==159));
  assert((words[9]&0x7FFFFFFFU)==(n==159?0x40108840U:0x00108840U));
  for(unsigned i=10;i<16;++i)assert(!words[i]);sum+=(words[4]+1)*8;
 }
 assert(sum==p.frame_bytes);
 dsi_dma_tile_request_t r=valid;r.height=8;assert(dsi_dma_tile_encode(&r,0,words)&&words[6]==0&&(words[9]&0xC0000000U)==0xC0000000U);
 assert(!dsi_dma_tile_plan(NULL,&p)&&!dsi_dma_tile_plan(&valid,NULL)&&!dsi_dma_tile_encode(&valid,160,words)&&!dsi_dma_tile_encode(&valid,0,NULL));
 r=valid;r.width=0;reject(r);r=valid;r.width=4096;reject(r);r=valid;r.width=721;reject(r);
 r=valid;r.height=0;reject(r);r=valid;r.height=0xFFFFFFFF;reject(r);r=valid;r.tile_rows=0;reject(r);
 r=valid;r.tile_rows=1281;reject(r);r=valid;r.tile_rows=3;reject(r);
 r=valid;r.source++;reject(r);r=valid;r.list+=64;reject(r);r=valid;r.source=0x8FF10000;reject(r);
 r=valid;r.source=0x4FFFFF80;reject(r);r=valid;r.source_capacity=0xFFFFFFFF;reject(r);
 r=valid;r.source_capacity=11519;reject(r);r=valid;r.list_capacity=10239;reject(r);
 r=valid;r.list=r.source+128;reject(r);r=valid;r.list_capacity=0xFFFFFFFF;reject(r);
 r=valid;r.source=0x4FF27F80;r.source_capacity=129;reject(r);
 puts("PASS: bounded finite RGB565 tile LLI plan, decoded fields, exact frame span and invalid geometry/ranges/overlap");
}

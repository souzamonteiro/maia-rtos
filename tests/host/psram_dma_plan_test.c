#include <assert.h>
#include "psram_dma_plan.h"
int main(void){uint32_t p[12];
 for(unsigned b=0;b<3;++b){unsigned burst=b==0?1:b==1?4:16;
 assert(psram_dma_plan(0x4B800000,0x4FF10080,burst,p));
 assert(p[0]==0x4B800000&&p[2]==0x4FF10080&&p[4]==511&&p[7]==0&&p[8]==24&&p[9]==0&&p[10]==0&&p[11]==0);
 assert((p[5]&0x700U)==(3U<<8U));
 assert((p[5]&0x3800U)==(3U<<11U));
 assert((p[5]&0x3C000U)==((b==0?0U:b==1?1U:3U)<<14U));
 assert((p[5]&0x3C0000U)==0);
 assert((p[5]&0xFFU)==5U);
 assert((p[6]&0x40U)&&((p[6]&0x7F80U)>>7U)==burst-1U);
 }
 assert(psram_dma_plan(0x4B9FF000,0x4FF26F80,16,p));
 uint32_t before=p[0];
 assert(!psram_dma_plan(0x4BBFF008,0x4FF10080,16,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B7FFFF8,0x4FF10080,16,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800004,0x4FF10080,16,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800000,0x4FF10088,16,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800000,0x4FF27000,16,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800000,0x4FF00000,16,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800000,0x4FF10080,8,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800000,0x4FF10080,0,p)&&p[0]==before);
 assert(!psram_dma_plan(0x4B800000,0x4FF10080,16,0));
}

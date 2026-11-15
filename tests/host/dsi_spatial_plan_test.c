#include <assert.h>
#include "dsi_dma_spatial_probe.h"
int main(void){
    assert(dsi_spatial_layout(0x4FF10000,0x4FF14000));
    assert(!dsi_spatial_layout(0x4FF10001,0x4FF14000));
    assert(!dsi_spatial_layout(0x4FF10000,0x4FF10080));
    assert(!dsi_spatial_layout(0x8FF10000,0x4FF14000));
    assert(!dsi_spatial_layout(0x4FF28000-128,0x4FF14000));
    unsigned rows[5]={8,312,320,320,320},total=0;
    for(unsigned n=0;n<5;++n){
        unsigned items=dsi_spatial_word(0x4FF10000,0x4FF14000,n,4)+1;assert(items*8==rows[n]*720*2);total+=items*8;
        assert(dsi_spatial_word(0x4FF10000,0x4FF14000,n,0)==0x4FF10000+(n?(n-1)*360:0));
        assert(dsi_spatial_word(0x4FF10000,0x4FF14000,n,2)==0x50105000);
        assert(dsi_spatial_word(0x4FF10000,0x4FF14000,n,6)==(n==4?0:((0x4FF14000+(n+1)*64)|1)));
        uint32_t ctl=dsi_spatial_word(0x4FF10000,0x4FF14000,n,8),hi=dsi_spatial_word(0x4FF10000,0x4FF14000,n,9);
        assert((ctl&1)&&!(ctl&4)&&!!(ctl&16)==!!n&&(ctl&64));
        assert(((ctl>>8)&7)==3&&((ctl>>11)&7)==3&&((ctl>>14)&15)==0&&((ctl>>18)&15)==7);
        assert((hi&0x40)&&!(hi&0x7F80)&&((hi>>16)&255)==15&&!(hi&0x07000000));
        assert(hi&0x80000000);assert(!!(hi&0x40000000)==(n==4));
        assert(dsi_spatial_done_word(0x4FF10000,0x4FF14000,n,9)==(hi&0x7FFFFFFF));
        assert(dsi_spatial_done_word(0x4FF10000,0x4FF14000,n,12)==items-1);
        assert(dsi_spatial_done_word(0x4FF10000,0x4FF14000,n,13)==(n==4?0xC0000000:0x40000000));
    }
    assert(total==1843200);return 0;
}

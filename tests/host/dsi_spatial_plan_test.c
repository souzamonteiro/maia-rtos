#include <assert.h>
#include "dsi_dma_spatial_probe.h"
int main(void){
#ifdef MAIA_DSI_SPATIAL_CENTER
    const uint32_t source=0x4FF02000U,list=0x4FF10000U;
    unsigned rows[5]={320,304,32,304,320},offsets[5]={0,360,0,720,1080},increment=2;
#else
    const uint32_t source=0x4FF10000U,list=0x4FF14000U;
    unsigned rows[5]={8,312,320,320,320},offsets[5]={0,0,360,720,1080},increment=0;
#endif
    assert(dsi_spatial_layout(source,list));
    assert(!dsi_spatial_layout(source+1,list));
    assert(!dsi_spatial_layout(source,source+128));
    assert(!dsi_spatial_layout(source+0x40000000U,list));
    assert(!dsi_spatial_layout(0x4FF28000-128,list));
#ifdef MAIA_DSI_SPATIAL_CENTER
    assert(!dsi_spatial_layout(0x4FF25000U,0x4FF00000U));
#else
    assert(dsi_spatial_layout(0x4FF25000U,0x4FF00000U));
#endif
    unsigned total=0,native_y=0;
    for(unsigned n=0;n<5;++n){
        unsigned items=dsi_spatial_word(source,list,n,4)+1;assert(items*8==rows[n]*720*2);total+=items*8;
        assert(dsi_spatial_word(source,list,n,0)==source+offsets[n]);
        assert(dsi_spatial_word(source,list,n,2)==0x50105000);
        assert(dsi_spatial_word(source,list,n,6)==(n==4?0:((list+(n+1)*64)|1)));
        uint32_t ctl=dsi_spatial_word(source,list,n,8),hi=dsi_spatial_word(source,list,n,9);
#ifdef MAIA_DSI_SPATIAL_CENTER
        if(n==increment)assert(native_y==624U&&native_y+rows[n]==656U);
#endif
        native_y+=rows[n];
        assert(offsets[n]+(n==increment?items*8U:8U)<=DSI_SPATIAL_SOURCE_BYTES);
        assert((ctl&1)&&!(ctl&4)&&!!(ctl&16)==(n!=increment)&&(ctl&64));
        assert(((ctl>>8)&7)==3&&((ctl>>11)&7)==3&&((ctl>>14)&15)==0&&((ctl>>18)&15)==7);
        assert((hi&0x40)&&!(hi&0x7F80)&&((hi>>16)&255)==15&&!(hi&0x07000000));
        assert(hi&0x80000000);assert(!!(hi&0x40000000)==(n==4));
        assert(dsi_spatial_done_word(source,list,n,9)==(hi&0x7FFFFFFF));
        assert(dsi_spatial_done_word(source,list,n,12)==items-1);
        assert(dsi_spatial_done_word(source,list,n,13)==(n==4?0xC0000000:0x40000000));
    }
    assert(total==1843200&&native_y==1280);return 0;
}

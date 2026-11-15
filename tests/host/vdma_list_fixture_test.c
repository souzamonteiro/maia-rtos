#include <assert.h>
#include <stdint.h>
#include "vdma_list_fixture.h"
int main(void){
    uint32_t words[32];for(unsigned i=0;i<32;++i)words[i]=0xDEADBEEFU;
    assert(!dma_list_encode(0x4FF00001U,words));assert(!dma_list_encode(0x4FEFFF80U,words));
    assert(!dma_list_encode(0x4FF28000U-DMA_FIXTURE_TOTAL+128U,words));assert(!dma_list_encode(0x4FF00000U,0));
    for(unsigned i=0;i<32;++i)assert(words[i]==0xDEADBEEFU);
    assert(dma_list_encode(0x4FF00000U,words));
    unsigned total=0;for(unsigned b=0;b<2;++b){
        uint32_t *w=words+b*16U;unsigned bytes=(w[4]+1U)*8U;total+=bytes;
        assert((w[8]&5U)==5U);assert(((w[8]>>8)&7U)==3U);assert(((w[8]>>11)&7U)==3U);
        assert(((w[8]>>4)&1U)==b);assert(!(w[8]&0x40U));
        assert((w[9]&0x80000000U)&&!!(w[9]&0x40000000U)==!!b);assert((w[9]&0x3FFFFFFFU)==(b?64U:0U));
        assert(w[0]==0x4FF00080U+(b?12288U:0U));assert(w[2]==0x4FF08180U+(b?11520U:0U));
        assert(w[6]==(b?0U:0x4FF040C1U));
        for(unsigned i=0;i<16;++i)if(i!=0&&i!=2&&i!=4&&i!=6&&i!=8&&i!=9)assert(!w[i]);
    }
    assert(total==32768U);
    for(unsigned i=0;i<32U;++i){
        uint32_t v=dma_list_completed_word(i,words[i]);
        if(i==9U||i==25U)assert(v==(words[i]&0x7FFFFFFFU));
        else if(i==12U||i==28U)assert(v==(i==12U?1439U:2655U));
        else if(i==13U||i==29U)assert(v==(i==13U?0x40000000U:0xC0000000U));
        else assert(v==words[i]);
    }
    assert(dma_list_pixel_byte(0)==0&&dma_list_pixel_byte(1)==0xF8);
    assert(dma_list_pixel_byte(360)==0xE0&&dma_list_pixel_byte(361)==7);
    assert(dma_list_pixel_byte(720)==0x1F&&dma_list_pixel_byte(721)==0);
    assert(dma_list_pixel_byte(1080)==0xE0&&dma_list_pixel_byte(1081)==0xFF);
    assert(dma_list_pixel_byte(11520)==0x10&&dma_list_pixel_byte(11521)==0x84);
    assert(dma_list_encode(0x4FF28000U-DMA_FIXTURE_TOTAL,words));return 0;
}

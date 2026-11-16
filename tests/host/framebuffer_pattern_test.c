#include <assert.h>
#include <stdio.h>
#include "framebuffer_pattern.h"
int main(void){
 assert(FB_BYTES==720U*1280U*2U);
 assert(fb_native_pixel(20,1259)==0xF800U);
 assert(fb_native_pixel(20,19)==0x07E0U);
 assert(fb_native_pixel(700,1259)==0x001FU);
 assert(fb_native_pixel(700,19)==0xFFE0U);
 assert(fb_native_pixel(46,1181)==0xFFFFU);
 assert(!fb_native_pixel(720,0)&&!fb_native_pixel(0,1280));
 uint32_t hash=2166136261U;
 for(unsigned y=0;y<1280U;++y)for(unsigned x=0;x<720U;++x){uint16_t p=fb_native_pixel(x,y);hash=(hash^(p&255U))*16777619U;hash=(hash^(p>>8U))*16777619U;}
 printf("Full rotated framebuffer FNV32=%08x\n",hash);
}

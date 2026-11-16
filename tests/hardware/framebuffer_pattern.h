/* Original MIT. Fixed diagnostic picture, logical1280x720 -> native720x1280.
 * Mapping matches vendored LVGL9 rotate90_rgb565, no LVGL runtime dependency. */
#ifndef MAIA_FRAMEBUFFER_PATTERN_H
#define MAIA_FRAMEBUFFER_PATTERN_H
#include <stdint.h>
#define FB_BYTES 1843200U
static inline int fb_digit(unsigned x,unsigned y,unsigned digit){
    static const unsigned bits[4]={0x06U,0x5BU,0x4FU,0x66U};
    if(digit<1U||digit>4U||x>=44U||y>=76U)return 0;
    unsigned mask=bits[digit-1U];
    return ((mask&1U)&&y<8U&&x>=8U&&x<36U)||((mask&2U)&&x>=36U&&y>=8U&&y<34U)||
        ((mask&4U)&&x>=36U&&y>=42U&&y<68U)||((mask&8U)&&y>=68U&&x>=8U&&x<36U)||
        ((mask&16U)&&x<8U&&y>=42U&&y<68U)||((mask&32U)&&x<8U&&y>=8U&&y<34U)||
        ((mask&64U)&&y>=34U&&y<42U&&x>=8U&&x<36U);
}
static inline uint16_t fb_logical_pixel(unsigned x,unsigned y){
    if(x>=1280U||y>=720U)return 0;
    static const uint16_t corners[4]={0xF800U,0x07E0U,0x001FU,0xFFE0U};
    if((x<160U||x>=1120U)&&(y<128U||y>=592U)){
        unsigned side=x>=1120U,bottom=y>=592U,k=bottom*2U+side;
        unsigned u=x-(side?1120U:0U),v=y-(bottom?592U:0U);
        if(u>=58U&&v>=26U&&fb_digit(u-58U,v-26U,k+1U))return (k==1U||k==3U)?0U:0xFFFFU;
        return corners[k];
    }
    if(x<8U||y<8U||x>=1272U||y>=712U)return 0xFFFFU;
    if(x>=256U&&x<1024U&&y>=224U&&y<496U){
        if(x<264U||x>=1016U||y<232U||y>=488U)return 0xFFFFU;
        static const uint16_t c[6]={0xF800U,0xFFE0U,0x07E0U,0x07FFU,0x001FU,0xF81FU};
        return c[(x-256U)/128U];
    }
    if(!(x%128U)||!(y%80U))return 0xFFFFU;
    return (uint16_t)(((x*31U/1279U)<<11U)|((y*63U/719U)<<5U)|((x+y)%32U));
}
static inline uint16_t fb_native_pixel(unsigned x,unsigned y){
    if(x>=720U||y>=1280U)return 0;
    return fb_logical_pixel(1279U-y,x);
}
#endif

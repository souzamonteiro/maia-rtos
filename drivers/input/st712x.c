/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native adaptation of esp_lcd_touch_st7123.c read_data/read_fw_info,
 * esp-iot-solution 1f855ac00e3fdb33773c8ba5893d4f98f1d9b576.
 * Modified: injected bounded reader, explicit byte masks (no bitfield ABI),
 * fixed storage/slot bounds, status/error validation, transactional output;
 * no allocation, SDK, locks, GPIO setup, delays or FreeRTOS dependencies.
 * LICENSES/Apache-2.0.txt. ST7123 protocol v01.11 pp12-13/18/20-21. */
#include <maia/drivers/st712x.h>
static int supported(const maia_st712x_info_t *i){
    return i&&(i->firmware==1U||i->firmware==3U)&&i->slots&&i->slots<=10U&&i->max_x&&i->max_x<=0x3FFFU&&i->max_y&&i->max_y<=0x3FFFU;
}
maia_i2c_result_t maia_st712x_identify(maia_touch_read_fn read,void *ctx,maia_st712x_info_t *out){
    if(!read||!out)return MAIA_I2C_INVALID;
    uint8_t status;maia_i2c_result_t r=read(ctx,1,&status,1);if(r!=MAIA_I2C_OK)return r;
    if(status!=0U&&status!=4U)return MAIA_I2C_BUSY;
    uint8_t b[10];r=read(ctx,0,b,sizeof b);if(r!=MAIA_I2C_OK)return r;
    if(b[1]!=0U&&b[1]!=4U)return MAIA_I2C_BUSY;
    maia_st712x_info_t info={(uint16_t)(((b[5]&0x3FU)<<8U)|b[6]),(uint16_t)(((b[7]&0x3FU)<<8U)|b[8]),b[0],b[9]};
    if(!supported(&info))return MAIA_I2C_VERIFY;
    *out=info;return MAIA_I2C_OK;
}
maia_i2c_result_t maia_st712x_read(maia_touch_read_fn read,void *ctx,const maia_st712x_info_t *info,maia_touch_frame_t *out){
    if(!read||!out||!supported(info))return MAIA_I2C_INVALID;
    uint8_t adv;maia_i2c_result_t r=read(ctx,0x10,&adv,1);if(r!=MAIA_I2C_OK)return r;
    if(adv&0x84U)return MAIA_I2C_VERIFY; /* reset request/proximity data unsupported */
    maia_touch_frame_t frame;
    for(unsigned i=0;i<sizeof frame;++i)((volatile uint8_t *)&frame)[i]=0;
    if(adv&8U){
        uint8_t bytes[70];r=read(ctx,0x14,bytes,7U*info->slots);if(r!=MAIA_I2C_OK)return r;
        frame.fresh=1;
        for(unsigned i=0;i<info->slots;++i){const uint8_t *p=bytes+7U*i;
            if(!(p[0]&0x80U))continue;
            uint16_t x=(uint16_t)(((p[0]&0x3FU)<<8U)|p[1]),y=(uint16_t)(((p[2]&0x3FU)<<8U)|p[3]);
            if(x>info->max_x||y>info->max_y)return MAIA_I2C_VERIFY;
            maia_touch_point_t *point=&frame.points[frame.count++];point->x=x;point->y=y;point->id=(uint8_t)i;point->area=p[4];
        }
    }
    for(unsigned i=0;i<sizeof frame;++i)((volatile uint8_t *)out)[i]=((const uint8_t *)&frame)[i];
    return MAIA_I2C_OK;
}

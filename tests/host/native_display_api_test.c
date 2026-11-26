#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <maia/services/lcd_output.h>
#include <maia/services/native_display.h>

void maia_native_display_test_reset(void);

static uint16_t frame[720U * 1280U];
static unsigned lease_active,lease_begins,lease_publishes,exclusive_calls;
static int exclusive_value,begin_fails,publish_fails;
static char console_log[128];

maia_status_t maia_lcd_output_edit_begin(uint16_t **pixels){
    ++lease_begins;
    if(begin_fails)return MAIA_ERR_BUSY;
    assert(!lease_active);
    lease_active=1;*pixels=frame;return MAIA_OK;
}

maia_status_t maia_lcd_output_edit_publish(void){
    ++lease_publishes;
    if(publish_fails)return MAIA_ERR_BUSY;
    assert(lease_active);lease_active=0;return MAIA_OK;
}

void maia_display_set_exclusive(int value){
    ++exclusive_calls;exclusive_value=value;
}

void maia_console_puts(const char *text){
    size_t used=strlen(console_log),len=strlen(text);
    assert(used+len<sizeof console_log);
    memcpy(console_log+used,text,len+1U);
}

int main(void){
    const maia_display_api_t *api=maia_native_display_api();
    assert(api&&api->width==1280&&api->height==720);
    assert(api->put_pixel&&api->fill_rect&&api->sync&&api->acquire_exclusive&&api->release_exclusive);
    assert(api->get_framebuffer&&api->get_framebuffer()==0);
    maia_native_display_test_reset();
    memset(frame,0,sizeof frame);

    assert(maia_native_display_scope_begin()==0);
    assert(api->acquire_exclusive()==0&&exclusive_value==1);
    assert(maia_native_display_scope_begin()==0);
    assert(api->acquire_exclusive()==0&&api->acquire_exclusive()==0);
    assert(api->fill_rect(0,0,1,1,0xF800U)==0);
    assert(api->release_exclusive()==0);
    assert(maia_native_display_scope_end()==0&&exclusive_value==1);
    assert(api->release_exclusive()==0&&exclusive_value==0);
    assert(maia_native_display_scope_end()==0);
    lease_active=lease_begins=lease_publishes=exclusive_calls=0;
    exclusive_value=begin_fails=publish_fails=0;
    console_log[0]='\0';

    assert(api->put_pixel(0,0,0xFFFFU)<0&&lease_begins==0);
    assert(api->acquire_exclusive()==0&&exclusive_calls==1&&exclusive_value==1);
    assert(api->fill_rect(4,4,0,2,0x1111U)==0&&lease_begins==0);
    assert(api->fill_rect(-1,-1,3,3,0x1234U)==0&&lease_begins==1);
    assert(frame[1279U*720U]==0x1234U&&frame[1279U*720U+1U]==0x1234U);
    assert(frame[1278U*720U]==0x1234U&&frame[1278U*720U+1U]==0x1234U);
    assert(api->put_pixel(1279,719,0x07E0U)==0&&frame[719U]==0x07E0U);
    assert(api->put_pixel(-1,0,0)<0&&api->put_pixel(0,720,0)<0);
    assert(api->fill_rect(0,0,-1,2,0)<0);
    api->sync();assert(lease_publishes==1&&!lease_active);

    assert(api->acquire_exclusive()==0&&exclusive_calls==1);
    assert(api->fill_rect(10,10,2,2,0xF800U)==0);
    assert(api->release_exclusive()==0&&exclusive_calls==1&&exclusive_value==1);
    assert(lease_active&&lease_publishes==1);
    api->sync();assert(!lease_active&&lease_publishes==2);
    assert(api->fill_rect(20,20,1,1,0xFFFFU)==0);
    assert(api->release_exclusive()==0&&exclusive_calls==2&&exclusive_value==0);
    assert(!lease_active&&lease_publishes==3);
    assert(api->release_exclusive()<0);

    assert(api->acquire_exclusive()==0);
    begin_fails=1;
    assert(api->put_pixel(1,1,1)<0);
    api->sync();assert(strstr(console_log,"transaction failed")!=0);
    assert(api->release_exclusive()<0&&exclusive_value==1);
    begin_fails=0;
    assert(api->put_pixel(1,1,1)==0);
    publish_fails=1;
    api->sync();
    assert(api->release_exclusive()<0&&exclusive_value==1);
    return 0;
}

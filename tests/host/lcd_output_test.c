#include <assert.h>
#include <maia/services/lcd_output.h>
#include <maia/arch/irq.h>
#include "lcd_async.h"
void maia_lcd_output_test_reset(void);
static uintptr_t mask;static maia_task_id_t current;
static uint64_t ticks;static unsigned prepared,active,paused,pause_requested,pending,notify_bit,frames,waits,cancels,stops,edits,publishes,edit_fail,publish_fail,drawn,timeout_mode,early_stop,drain_once,delays,stop_fails,update_during_run,update_queued,stop_after_frames,swap_pending,first_frame_pending,presented;
static maia_lcd_wake_t wake_fn;static void *wake_arg;
static void draw(uint16_t *,uint32_t,uint32_t,void *);
uintptr_t maia_arch_irq_save(void){uintptr_t previous=mask;mask=0;return previous;}
void maia_arch_irq_restore(uintptr_t saved){mask=saved;}
maia_task_id_t maia_task_current_id(void){return current;}
maia_status_t maia_task_get_info(uint32_t id,maia_task_info_t *r){if(id!=7)return MAIA_ERR_INVALID;r->state=MAIA_TASK_READY;return MAIA_OK;}
uint64_t maia_tick_get(void){return ticks;}
maia_status_t maia_task_notify(maia_task_id_t id){assert(id==7);notify_bit=1;return MAIA_OK;}
maia_status_t maia_task_wait_notification(uint32_t timeout){
 assert(mask==8&&timeout>0&&timeout<=100);++waits;
 if(notify_bit){notify_bit=0;return MAIA_OK;}
 if(early_stop&&waits==2){assert(maia_lcd_output_request_stop()==MAIA_OK);notify_bit=0;return MAIA_OK;}
 if(update_during_run&&!update_queued&&frames==1){update_queued=1;assert(maia_lcd_output_request_update(draw,&drawn)==MAIA_OK);notify_bit=0;return MAIA_OK;}
 if(timeout_mode){ticks+=timeout;return MAIA_ERR_TIMEOUT;}
 ticks+=3;assert(active);pending=1;if(pause_requested){active=0;paused=1;pause_requested=0;}uintptr_t saved=maia_arch_irq_save();wake_fn(wake_arg);maia_arch_irq_restore(saved);notify_bit=0;return MAIA_OK;
}
void maia_task_delay_ticks(uint32_t n){assert(mask==8&&n==1);ticks+=n;++delays;}
maia_dsi_access_result_t maia_lcd_async_prepare(maia_lcd_wake_t f,void *a){assert(!mask&&!prepared);prepared=1;wake_fn=f;wake_arg=a;swap_pending=first_frame_pending=presented=0;return MAIA_DSI_ACCESS_OK;}
maia_dsi_access_result_t maia_lcd_async_start(void){assert(!mask&&prepared&&!active);active=1;paused=pause_requested=pending=0;return MAIA_DSI_ACCESS_OK;}
maia_dsi_access_result_t maia_lcd_async_request_pause(void){assert(!mask&&prepared);if(active){assert(!swap_pending&&!first_frame_pending);pause_requested=1;}return MAIA_DSI_ACCESS_OK;}
int maia_lcd_async_is_paused(void){assert(!mask);return (int)paused;}
int maia_lcd_async_pending(void){assert(!mask);return (int)pending;}
void maia_lcd_async_report(maia_lcd_async_report_t *r){assert(!mask);if(r){for(unsigned i=0;i<sizeof *r;++i)((unsigned char *)r)[i]=0;r->frames=frames;r->presented=presented;}}
maia_dsi_access_result_t maia_lcd_async_finish(void){
 assert(!mask&&pending&&(active||paused));
 if(drain_once){drain_once=0;return MAIA_DSI_ACCESS_BUSY;}
 pending=0;++frames;
 if(active&&swap_pending){swap_pending=0;first_frame_pending=1;}
 else if(active&&first_frame_pending){first_frame_pending=0;++presented;}
 if(stop_after_frames&&frames==stop_after_frames)assert(maia_lcd_output_request_stop()==MAIA_OK);return MAIA_DSI_ACCESS_OK;
}
maia_dsi_access_result_t maia_lcd_async_cancel(void){assert(!mask);++cancels;active=paused=pending=0;return MAIA_DSI_ACCESS_OK;}
maia_dsi_access_result_t maia_lcd_async_stop(void){assert(!mask&&!active);++stops;if(!stop_fails)prepared=paused=0;return stop_fails?MAIA_DSI_ACCESS_VERIFY:MAIA_DSI_ACCESS_OK;}
static uint16_t pixels[4];
uint16_t *maia_lcd_async_edit_begin(void){assert(!mask&&prepared&&!swap_pending&&!first_frame_pending);++edits;return edit_fail?0:pixels;}
maia_dsi_access_result_t maia_lcd_async_edit_publish(void){
 assert(mask==8&&prepared);++publishes;if(publish_fail)return MAIA_DSI_ACCESS_VERIFY;
 if(active)swap_pending=1;else first_frame_pending=1;return MAIA_DSI_ACCESS_OK;
}
static void draw(uint16_t *p,uint32_t w,uint32_t h,void *arg){
 assert(mask==8&&p==pixels&&w==720&&h==1280&&arg==&drawn);++drawn;
 assert(maia_lcd_output_request_update(draw,arg)==MAIA_ERR_BUSY);
 p[0]=0xF800;
}
static void setup(void){maia_lcd_output_test_reset();mask=0;current=-1;ticks=0;prepared=active=paused=pause_requested=pending=notify_bit=frames=waits=cancels=stops=edits=publishes=edit_fail=publish_fail=drawn=timeout_mode=early_stop=drain_once=delays=stop_fails=update_during_run=update_queued=swap_pending=first_frame_pending=presented=0;stop_after_frames=2;}
int main(void){
 maia_lcd_output_stats_t r;
 setup();assert(maia_lcd_output_request_stop()==MAIA_ERR_INVALID);
 assert(maia_lcd_output_init(9)==MAIA_ERR_INVALID&&!prepared);
 assert(maia_lcd_output_init(7)==MAIA_OK&&prepared);assert(maia_lcd_output_init(7)==MAIA_ERR_BUSY);
 current=7;assert(maia_lcd_output_run()==MAIA_ERR_PERM);mask=8;
 assert(maia_lcd_output_run()==MAIA_OK&&frames==3&&waits>=3&&stops==1&&!cancels);
 assert(maia_lcd_output_stats(&r)==MAIA_OK&&r.frames==3&&r.stopped&&!r.errors&&!r.timeouts);
 assert(maia_lcd_output_run()==MAIA_ERR_PERM);
 current=8;assert(maia_lcd_output_restart()==MAIA_ERR_PERM);current=7;
 assert(maia_lcd_output_request_update(draw,&drawn)==MAIA_ERR_INVALID);
 assert(maia_lcd_output_restart()==MAIA_OK);frames=0;
 assert(maia_lcd_output_request_update(0,0)==MAIA_ERR_INVALID);
 assert(maia_lcd_output_request_update(draw,&drawn)==MAIA_OK);
 assert(maia_lcd_output_request_update(draw,&drawn)==MAIA_ERR_BUSY);
 assert(maia_lcd_output_run()==MAIA_OK&&drawn==1&&edits==1&&publishes==1);
 assert(maia_lcd_output_stats(&r)==MAIA_OK&&r.updates==1&&r.presented==1&&r.frames==3);
 setup();assert(maia_lcd_output_init(7)==MAIA_OK);mask=8;current=7;update_during_run=1;stop_after_frames=4;
 assert(maia_lcd_output_run()==MAIA_OK&&drawn==1&&frames==5&&edits==1&&publishes==1);
 assert(maia_lcd_output_stats(&r)==MAIA_OK&&r.updates==1&&r.presented==1&&r.frames==5);

 setup();assert(maia_lcd_output_init(7)==MAIA_OK);mask=8;current=7;
 assert(maia_lcd_output_request_update(draw,&drawn)==MAIA_OK);
 assert(maia_lcd_output_request_stop()==MAIA_OK);
 assert(maia_lcd_output_run()==MAIA_OK&&!drawn&&!frames&&!edits);
 setup();assert(maia_lcd_output_init(7)==MAIA_OK);mask=8;current=7;early_stop=drain_once=1;
 maia_status_t early_result=maia_lcd_output_run();
 assert(early_result==MAIA_OK&&frames==3&&delays==1&&stops==1);
 for(unsigned fail=0;fail<2;++fail){
  setup();assert(maia_lcd_output_init(7)==MAIA_OK);mask=8;current=7;
  edit_fail=!fail;publish_fail=fail;assert(maia_lcd_output_request_update(draw,&drawn)==MAIA_OK);
  assert(maia_lcd_output_run()==MAIA_ERR_BUSY&&!frames);
  assert(maia_lcd_output_restart()==MAIA_ERR_BUSY);
 }
 setup();assert(maia_lcd_output_init(7)==MAIA_OK);mask=8;current=7;timeout_mode=1;
 assert(maia_lcd_output_run()==MAIA_ERR_BUSY&&cancels==1&&stops==1);
 assert(maia_lcd_output_stats(&r)==MAIA_OK&&r.timeouts==1&&r.errors==1&&r.stopped);
 setup();assert(maia_lcd_output_init(7)==MAIA_OK);mask=8;current=7;timeout_mode=stop_fails=1;
 assert(maia_lcd_output_run()==MAIA_ERR_BUSY&&cancels==1);
 assert(maia_lcd_output_stats(&r)==MAIA_OK&&r.errors==2&&!r.stopped);
}

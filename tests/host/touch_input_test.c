#include <assert.h>
#include <string.h>
#include <maia/services/touch_input.h>
#include <maia/arch/irq.h>
void maia_touch_input_test_reset(void);
static uintptr_t mask;static maia_task_id_t current=-1;
static uint64_t ticks;static unsigned calls,fail_call,report_bytes,nested;
static maia_i2c_result_t fail_status=MAIA_I2C_TIMEOUT;
static uint8_t adv,bytes[70],identity[10]={1,0,0,0,0,2,0xD0,5,0,10};
uintptr_t maia_arch_irq_save(void){uintptr_t old=mask;mask=0;return old;}
void maia_arch_irq_restore(uintptr_t v){mask=v;}
maia_task_id_t maia_task_current_id(void){return current;}
uint64_t maia_tick_get(void){return ticks;}
maia_status_t maia_task_get_info(uint32_t id,maia_task_info_t *i){if(id!=3)return MAIA_ERR_INVALID;i->state=MAIA_TASK_READY;return MAIA_OK;}
static maia_i2c_result_t read_bytes(void *arg,uint16_t reg,uint8_t *out,uint32_t n){
 assert(arg==bytes);++calls;if(fail_call==calls)return fail_status;
 if(nested){nested=0;assert(maia_touch_input_stop()==MAIA_ERR_BUSY);assert(maia_touch_input_poll()==MAIA_ERR_BUSY);}
 if(reg==1){assert(n==1);*out=identity[1];}
 else if(reg==0){assert(n==10);memcpy(out,identity,n);}
 else if(reg==0x10){assert(n==1);*out=adv;}
 else {assert(reg==0x14&&n<=70);report_bytes=n;memcpy(out,bytes,n);}return MAIA_I2C_OK;
}
static void point(unsigned slot,unsigned x,unsigned y){bytes[7*slot]=(uint8_t)(0x80|(x>>8));bytes[7*slot+1]=(uint8_t)x;bytes[7*slot+2]=(uint8_t)(y>>8);bytes[7*slot+3]=(uint8_t)y;bytes[7*slot+4]=31;}
int main(void){
 maia_st712x_info_t info={0};
 identity[1]=1;unsigned before=calls;assert(maia_st712x_identify(read_bytes,bytes,&info)==MAIA_I2C_BUSY&&calls==before+1);identity[1]=0;
 assert(maia_st712x_identify(read_bytes,bytes,&info)==MAIA_I2C_OK&&info.firmware==1&&info.slots==10&&info.max_x==720&&info.max_y==1280);
 maia_touch_frame_t frame;memset(&frame,0xCC,sizeof frame);adv=8;point(9,719,1279);
 assert(maia_st712x_read(read_bytes,bytes,&info,&frame)==MAIA_I2C_OK&&frame.fresh&&frame.count==1&&frame.points[0].id==9&&report_bytes==70);
 for(unsigned n=1;n<=10;++n){info.slots=(uint8_t)n;assert(maia_st712x_read(read_bytes,bytes,&info,&frame)==MAIA_I2C_OK&&report_bytes==7*n);}info.slots=10;
 memset(bytes,0,sizeof bytes);for(unsigned i=0;i<10;++i)point(i,10*i,20*i);
 assert(maia_st712x_read(read_bytes,bytes,&info,&frame)==MAIA_I2C_OK&&frame.count==10);
 for(unsigned i=0;i<10;++i)assert(frame.points[i].id==i&&frame.points[i].x==10*i&&frame.points[i].y==20*i&&frame.points[i].area==31);
 maia_touch_frame_t saved;memset(&saved,0xCC,sizeof saved);frame=saved;fail_call=calls+2;
 assert(maia_st712x_read(read_bytes,bytes,&info,&frame)==MAIA_I2C_TIMEOUT&&!memcmp(&frame,&saved,sizeof frame));fail_call=0;
 adv=0x80;assert(maia_st712x_read(read_bytes,bytes,&info,&frame)==MAIA_I2C_VERIFY);adv=0;
 assert(maia_st712x_read(read_bytes,bytes,&info,&frame)==MAIA_I2C_OK&&!frame.fresh);
 identity[9]=11;assert(maia_st712x_identify(read_bytes,bytes,&info)==MAIA_I2C_VERIFY);identity[9]=10;
 identity[0]=2;assert(maia_st712x_identify(read_bytes,bytes,&info)==MAIA_I2C_VERIFY);identity[0]=1;
 assert(maia_st712x_identify(read_bytes,bytes,&info)==MAIA_I2C_OK);
 uint16_t x=99,y=99;
 assert(maia_touch_transform(0,0,720,1280,90,&x,&y)==MAIA_OK&&x==1279&&y==0);
 assert(maia_touch_transform(719,1279,720,1280,90,&x,&y)==MAIA_OK&&x==0&&y==719);
 assert(maia_touch_transform(719,0,720,1280,180,&x,&y)==MAIA_OK&&x==0&&y==1279);
 assert(maia_touch_transform(719,1279,720,1280,270,&x,&y)==MAIA_OK&&x==1279&&y==0);
 assert(maia_touch_transform(720,0,720,1280,90,&x,&y)==MAIA_ERR_INVALID&&x==1279&&y==0);
 assert(maia_touch_transform(0,0,720,1280,1,&x,&y)==MAIA_ERR_INVALID);
 maia_touch_input_test_reset();mask=0;current=-1;assert(maia_touch_input_init(3,read_bytes,bytes,&info,720,1280,90)==MAIA_OK);
 assert(maia_touch_input_init(3,read_bytes,bytes,&info,720,1280,90)==MAIA_ERR_BUSY);
 maia_touch_snapshot_t s;assert(maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY&&!s.valid);
 current=3;assert(maia_touch_input_poll()==MAIA_ERR_PERM);mask=8;memset(bytes,0,sizeof bytes);point(0,100,300);adv=8;nested=1;
 assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&s.valid&&s.downs==1&&s.frame.points[0].x==979&&s.frame.points[0].y==100);
 adv=0;assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&s.frame.count==1&&!s.ups);
 adv=8;point(0,102,302);assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&s.moves==1);
 fail_call=calls+1;assert(maia_touch_input_poll()==MAIA_ERR_BUSY&&maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY&&!s.valid&&!s.ups);fail_call=0;
 adv=0;assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY);
 adv=8;memset(bytes,0,sizeof bytes);assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&!s.frame.count&&!s.ups);
 point(0,100,300);assert(maia_touch_input_poll()==MAIA_OK);memset(bytes,0,sizeof bytes);assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&s.ups==1);
 uint32_t sequence_before_busy=s.sequence,polls_before_busy=s.polls;uint64_t last_good_poll=s.last_poll;
 fail_call=calls+1;fail_status=MAIA_I2C_BUSY;
 assert(maia_touch_input_poll()==MAIA_ERR_BUSY&&maia_touch_input_snapshot(&s)==MAIA_OK);
 assert(s.valid&&s.sequence==sequence_before_busy&&s.polls==polls_before_busy+1&&s.last_poll==last_good_poll);
 fail_call=0;fail_status=MAIA_I2C_TIMEOUT;
 ticks=101;assert(maia_touch_input_snapshot(&s)==MAIA_ERR_TIMEOUT&&!s.valid);
 point(0,720,100);assert(maia_touch_input_poll()==MAIA_ERR_BUSY&&maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY);
 current=4;assert(maia_touch_input_poll()==MAIA_ERR_PERM);
 current=3;point(0,100,300);assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&s.frame.count==1);
 uint32_t ups_before=s.ups;assert(maia_touch_input_stop()==MAIA_OK);
 unsigned before_stop=calls;assert(maia_touch_input_poll()==MAIA_ERR_BUSY&&calls==before_stop);
 assert(maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY&&s.stopped&&!s.valid&&!s.frame.count&&s.ups==ups_before);
 uint32_t sequence=s.sequence;
 identity[0]=3;assert(maia_touch_input_restart()==MAIA_ERR_BUSY);
 assert(maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY&&s.stopped);identity[0]=1;
 fail_call=calls+1;assert(maia_touch_input_restart()==MAIA_ERR_BUSY);fail_call=0;
 assert(maia_touch_input_restart()==MAIA_OK);
 assert(maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY&&!s.stopped&&s.sequence>sequence&&s.stops==1&&s.restarts==1);
 adv=0;assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_ERR_BUSY);
 adv=8;point(0,100,300);assert(maia_touch_input_poll()==MAIA_OK&&maia_touch_input_snapshot(&s)==MAIA_OK&&s.valid);
 assert(maia_touch_input_restart()==MAIA_ERR_BUSY);
 current=4;assert(maia_touch_input_stop()==MAIA_ERR_PERM&&maia_touch_input_restart()==MAIA_ERR_PERM);

}

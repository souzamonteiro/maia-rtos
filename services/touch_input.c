/* Original MaiaRTOS MIT. Single producer, IRQ-protected latest state. */
#include <maia/services/touch_input.h>
#include <maia/arch/irq.h>
static unsigned initialized,polling;
static maia_task_id_t owner;
static maia_touch_read_fn reader;
static void *reader_arg;
static maia_st712x_info_t info;
static uint16_t native_width,native_height,rotation;
static maia_touch_snapshot_t state;
maia_status_t maia_touch_transform(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t r,uint16_t *ox,uint16_t *oy){
    if(!ox||!oy||!w||!h||x>=w||y>=h||(r!=0U&&r!=90U&&r!=180U&&r!=270U))return MAIA_ERR_INVALID;
    uint16_t a=x,b=y;
    if(r==90U){a=(uint16_t)(h-1U-y);b=x;}
    else if(r==180U){a=(uint16_t)(w-1U-x);b=(uint16_t)(h-1U-y);}
    else if(r==270U){a=y;b=(uint16_t)(w-1U-x);}
    *ox=a;*oy=b;return MAIA_OK;
}
maia_status_t maia_touch_input_init(maia_task_id_t id,maia_touch_read_fn fn,void *arg,const maia_st712x_info_t *i,uint16_t w,uint16_t h,uint16_t r){
    if(!fn||!i||!i->slots||i->slots>10U||!i->max_x||!i->max_y||i->max_x>0x3FFFU||i->max_y>0x3FFFU||(i->firmware!=1U&&i->firmware!=3U))return MAIA_ERR_INVALID;
    uint16_t x,y;if(maia_touch_transform(0,0,w,h,r,&x,&y)!=MAIA_OK)return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=MAIA_OK;maia_task_info_t task;
    if(initialized)result=MAIA_ERR_BUSY;
    else if(irq||maia_task_current_id()>=0)result=MAIA_ERR_PERM;
    else if(id<0||maia_task_get_info((uint32_t)id,&task)!=MAIA_OK||task.state!=MAIA_TASK_READY)result=MAIA_ERR_INVALID;
    else {reader=fn;reader_arg=arg;info=*i;native_width=w;native_height=h;rotation=r;owner=id;initialized=1;}
    maia_arch_irq_restore(irq);return result;
}
maia_status_t maia_touch_input_poll(void){
    uintptr_t irq=maia_arch_irq_save();
    if(!initialized||!irq||maia_task_current_id()!=owner){maia_arch_irq_restore(irq);return MAIA_ERR_PERM;}
    if(polling){maia_arch_irq_restore(irq);return MAIA_ERR_BUSY;}
    polling=1;maia_arch_irq_restore(irq);
    maia_touch_frame_t frame;maia_i2c_result_t r=maia_st712x_read(reader,reader_arg,&info,&frame);
    if(r==MAIA_I2C_OK&&frame.fresh)for(unsigned i=0;i<frame.count;++i){maia_touch_point_t *p=&frame.points[i];
        if(maia_touch_transform(p->x,p->y,native_width,native_height,rotation,&p->x,&p->y)!=MAIA_OK){r=MAIA_I2C_VERIFY;break;}}
    irq=maia_arch_irq_save();++state.polls;state.last_poll=maia_tick_get();state.last_error=(int32_t)r;
    if(r!=MAIA_I2C_OK){++state.errors;state.valid=0;state.frame.count=0;++state.sequence;}
    else if(frame.fresh){
        if(!state.valid||!state.frame.count){if(frame.count)++state.downs;}
        else if(!frame.count)++state.ups;
        else if(frame.points[0].x!=state.frame.points[0].x||frame.points[0].y!=state.frame.points[0].y)++state.moves;
        state.frame=frame;state.valid=1;++state.sequence;++state.reports;
    }
    polling=0;maia_arch_irq_restore(irq);return r==MAIA_I2C_OK?MAIA_OK:MAIA_ERR_BUSY;
}
maia_status_t maia_touch_input_snapshot(maia_touch_snapshot_t *out){
    if(!out)return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();*out=state;
    maia_status_t r=!initialized?MAIA_ERR_INVALID:!state.valid?MAIA_ERR_BUSY:maia_tick_get()-state.last_poll>100U?MAIA_ERR_TIMEOUT:MAIA_OK;
    if(r!=MAIA_OK)out->valid=0;
    maia_arch_irq_restore(irq);return r;
}
#ifdef MAIA_TOUCH_INPUT_TEST
void maia_touch_input_test_reset(void);
void maia_touch_input_test_reset(void){initialized=polling=0;for(unsigned i=0;i<sizeof state;++i)((unsigned char *)&state)[i]=0;}
#endif

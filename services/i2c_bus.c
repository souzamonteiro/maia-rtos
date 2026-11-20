/* Original MaiaRTOS MIT. Single-core serialization around bounded wire calls. */
#include <maia/services/i2c_bus.h>
#include <maia/arch/irq.h>
maia_status_t maia_i2c_bus_init(maia_i2c_bus_t *b,const maia_i2c_lines_t *lines){
    if(!b||!lines||!lines->drive||!lines->sample||!lines->pause)return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();maia_status_t r=MAIA_OK;
    if(irq||maia_task_current_id()>=0)r=MAIA_ERR_PERM;
    else if(b->initialized)r=MAIA_ERR_BUSY;
    else {b->lines=lines;b->initialized=1;}
    maia_arch_irq_restore(irq);return r;
}
static maia_i2c_result_t begin(maia_i2c_bus_t *b){
    if(!b)return MAIA_I2C_INVALID;
    uintptr_t irq=maia_arch_irq_save();
    if(b->initialized!=1U){maia_arch_irq_restore(irq);return MAIA_I2C_INVALID;}
    if(!irq||maia_task_current_id()<0){maia_arch_irq_restore(irq);return MAIA_I2C_BUSY;}
    if(b->busy){++b->contentions;maia_arch_irq_restore(irq);return MAIA_I2C_BUSY;}
    b->busy=1;++b->transactions;maia_arch_irq_restore(irq);
    return MAIA_I2C_OK;
}
static maia_i2c_result_t end(maia_i2c_bus_t *b,maia_i2c_result_t r){
    uintptr_t irq=maia_arch_irq_save();if(r!=MAIA_I2C_OK)++b->errors;b->busy=0;maia_arch_irq_restore(irq);return r;
}
maia_i2c_result_t maia_i2c_bus_read16(maia_i2c_bus_t *b,uint8_t address,uint16_t reg,uint8_t *out,uint32_t count){
    maia_i2c_result_t r=begin(b);if(r!=MAIA_I2C_OK)return r;
    return end(b,maia_i2c_read_register16_block(b->lines,address,reg,out,count));
}
maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *b,uint8_t address,uint8_t reg,uint8_t *out,uint32_t count){
    maia_i2c_result_t r=begin(b);if(r!=MAIA_I2C_OK)return r;
    return end(b,maia_i2c_read_register8_block(b->lines,address,reg,out,count));
}
maia_i2c_result_t maia_i2c_bus_write8(maia_i2c_bus_t *b,uint8_t address,uint8_t reg,uint8_t value){
    maia_i2c_result_t r=begin(b);if(r!=MAIA_I2C_OK)return r;
    return end(b,maia_i2c_write_register8(b->lines,address,reg,value));
}
static maia_status_t transition(maia_i2c_bus_t *b,unsigned from,unsigned to){
    if(!b)return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();maia_status_t r=MAIA_OK;
    if(!irq||maia_task_current_id()<0)r=MAIA_ERR_PERM;
    else if(b->initialized!=from)r=MAIA_ERR_INVALID;
    else if(b->busy||b->lines->sample(b->lines->context)!=MAIA_I2C_BOTH)r=MAIA_ERR_BUSY;
    else b->initialized=to;
    maia_arch_irq_restore(irq);return r;
}
maia_status_t maia_i2c_bus_close(maia_i2c_bus_t *b){return transition(b,1,2);}
maia_status_t maia_i2c_bus_reopen(maia_i2c_bus_t *b){return transition(b,2,1);}

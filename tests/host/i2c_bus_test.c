#include <assert.h>
#include <maia/services/i2c_bus.h>
#include <maia/arch/irq.h>
static uintptr_t mask;static maia_task_id_t current=-1;
static maia_i2c_bus_t bus;
static unsigned calls,nested,levels=3;static maia_i2c_result_t response;
uintptr_t maia_arch_irq_save(void){uintptr_t old=mask;mask=0;return old;}
void maia_arch_irq_restore(uintptr_t v){mask=v;}
maia_task_id_t maia_task_current_id(void){return current;}
static void drive(void *p,unsigned m,unsigned r){(void)p;(void)m;(void)r;}
static unsigned sample(void *p){(void)p;return levels;}
static maia_i2c_result_t pause_step(void *p){(void)p;return MAIA_I2C_OK;}
maia_i2c_result_t maia_i2c_read_register16_block(const maia_i2c_lines_t *b,uint8_t address,uint16_t reg,uint8_t *out,uint32_t n){
 assert(mask==8&&b==bus.lines&&address==0x55&&reg==0x14&&n==1);++calls;
 if(nested){assert(maia_i2c_bus_close(&bus)==MAIA_ERR_BUSY);nested=0;assert(maia_i2c_bus_read8(&bus,0x34,4,out,n)==MAIA_I2C_BUSY);assert(maia_i2c_bus_write8(&bus,0x34,2,1)==MAIA_I2C_BUSY);}
 if(response==MAIA_I2C_OK)*out=42;return response;
}
maia_i2c_result_t maia_i2c_read_register8_block(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t *out,uint32_t n){
 assert(mask==8&&b==bus.lines&&address==0x34&&reg==4&&n==1);++calls;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,out,1)==MAIA_I2C_BUSY);
 if(response==MAIA_I2C_OK)*out=43;return response;
}
maia_i2c_result_t maia_i2c_write_register8(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t value){
 assert(mask==8&&b==bus.lines&&address==0x34&&reg==2&&value==1);++calls;return response;
}
maia_i2c_result_t maia_i2c_write_register8_block(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,const uint8_t *data,uint32_t count){
 assert(mask==8&&b==bus.lines&&address==0x68&&reg==0x5E&&count==3);
 assert(data[0]==0x10&&data[1]==0x20&&data[2]==0x30);++calls;return response;
}
int main(void){
 maia_i2c_lines_t b={0,drive,sample,pause_step};uint8_t out=99;
 assert(maia_i2c_bus_init(&bus,&b)==MAIA_OK&&maia_i2c_bus_init(&bus,&b)==MAIA_ERR_BUSY);
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_BUSY&&!calls);
 mask=8;current=3;nested=1;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_OK&&out==42&&!bus.busy&&bus.contentions==2);
 response=MAIA_I2C_TIMEOUT;out=99;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_TIMEOUT&&out==99&&!bus.busy&&bus.errors==1);
 response=MAIA_I2C_OK;assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_OK&&out==42&&bus.transactions==3);
 assert(maia_i2c_bus_close(&bus)==MAIA_OK);unsigned before=calls;out=99;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_INVALID&&calls==before&&out==99);
 assert(maia_i2c_bus_close(&bus)==MAIA_ERR_INVALID);levels=1;
 assert(maia_i2c_bus_reopen(&bus)==MAIA_ERR_BUSY);levels=3;
 assert(maia_i2c_bus_reopen(&bus)==MAIA_OK&&maia_i2c_bus_reopen(&bus)==MAIA_ERR_INVALID);
 levels=1;assert(maia_i2c_bus_close(&bus)==MAIA_ERR_BUSY);levels=3;
 mask=0;assert(maia_i2c_bus_close(&bus)==MAIA_ERR_PERM);mask=8;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_OK&&out==42);
 assert(maia_i2c_bus_read8(&bus,0x34,4,&out,1)==MAIA_I2C_OK&&out==43&&!bus.busy&&bus.contentions==3);
 assert(maia_i2c_bus_write8(&bus,0x34,2,1)==MAIA_I2C_OK&&!bus.busy);
 const uint8_t burst[]={0x10,0x20,0x30};
 assert(maia_i2c_bus_write8_block(&bus,0x68,0x5E,burst,sizeof(burst))==MAIA_I2C_OK&&!bus.busy);
 response=MAIA_I2C_NACK;
 assert(maia_i2c_bus_write8(&bus,0x34,2,1)==MAIA_I2C_NACK&&!bus.busy&&bus.errors==2);
 assert(maia_i2c_bus_close(&bus)==MAIA_OK);
 before=calls;assert(maia_i2c_bus_write8(&bus,0x34,2,1)==MAIA_I2C_INVALID&&calls==before);

}

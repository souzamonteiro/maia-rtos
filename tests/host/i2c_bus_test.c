#include <assert.h>
#include <maia/services/i2c_bus.h>
#include <maia/arch/irq.h>
static uintptr_t mask;static maia_task_id_t current=-1;
static maia_i2c_bus_t bus;
static unsigned calls,nested;static maia_i2c_result_t response;
uintptr_t maia_arch_irq_save(void){uintptr_t old=mask;mask=0;return old;}
void maia_arch_irq_restore(uintptr_t v){mask=v;}
maia_task_id_t maia_task_current_id(void){return current;}
static void drive(void *p,unsigned m,unsigned r){(void)p;(void)m;(void)r;}
static unsigned sample(void *p){(void)p;return 3;}
static maia_i2c_result_t pause_step(void *p){(void)p;return MAIA_I2C_OK;}
maia_i2c_result_t maia_i2c_read_register16_block(const maia_i2c_lines_t *b,uint8_t address,uint16_t reg,uint8_t *out,uint32_t n){
 assert(mask==8&&b==bus.lines&&address==0x55&&reg==0x14&&n==1);++calls;
 if(nested){nested=0;assert(maia_i2c_bus_read16(&bus,address,reg,out,n)==MAIA_I2C_BUSY);}
 if(response==MAIA_I2C_OK)*out=42;return response;
}
int main(void){
 maia_i2c_lines_t b={0,drive,sample,pause_step};uint8_t out=99;
 assert(maia_i2c_bus_init(&bus,&b)==MAIA_OK&&maia_i2c_bus_init(&bus,&b)==MAIA_ERR_BUSY);
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_BUSY&&!calls);
 mask=8;current=3;nested=1;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_OK&&out==42&&!bus.busy&&bus.contentions==1);
 response=MAIA_I2C_TIMEOUT;out=99;
 assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_TIMEOUT&&out==99&&!bus.busy&&bus.errors==1);
 response=MAIA_I2C_OK;assert(maia_i2c_bus_read16(&bus,0x55,0x14,&out,1)==MAIA_I2C_OK&&out==42&&bus.transactions==3);
}

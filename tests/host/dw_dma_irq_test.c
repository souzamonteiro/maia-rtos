#include <assert.h>
#include "dw_dma_irq.h"
#include "psram_internal.h"
void maia_dw_irq_test_reset(void);
static uint32_t maps[128],controls[48];
#define control controls[17]
static unsigned context,reads,writes,drop,active,posts;
int maia_psram_test_context(void){return (int)context;}
uint32_t maia_psram_test_read(uintptr_t p){
 ++reads;
 if(p==MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)return 0x2000U;
 if(p==MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG)return 0x20U;
 if(p>=MAIA_ESP32P4_INTERRUPT_CORE0_BASE&&p<MAIA_ESP32P4_INTERRUPT_CORE0_BASE+512U)return maps[(p-MAIA_ESP32P4_INTERRUPT_CORE0_BASE)/4U];
 if(p>=MAIA_ESP32P4_CLIC_CTRL_BASE&&p<MAIA_ESP32P4_CLIC_CTRL_BASE+48U*4U)return controls[(p-MAIA_ESP32P4_CLIC_CTRL_BASE)/4U];
 if(p==MAIA_ESP32P4_GDMA_BASE+0x10U)return active;
 assert(p==MAIA_ESP32P4_GDMA_BASE+0x18U||p==MAIA_ESP32P4_GDMA_BASE+0x1CU);return 0;
}
void maia_psram_test_write(uintptr_t p,uint32_t v){++writes;if(writes==drop)return;
 if(p>=MAIA_ESP32P4_CLIC_CTRL_BASE&&p<MAIA_ESP32P4_CLIC_CTRL_BASE+48U*4U){controls[(p-MAIA_ESP32P4_CLIC_CTRL_BASE)/4U]=v;return;}
 assert(p==MAIA_ESP32P4_INTR_CORE0_MAP_REG(24));maps[24]=v;
}
static void irq(void *p){assert(p==&posts);++posts;}
static void setup(void){maia_dw_irq_test_reset();context=1;reads=writes=drop=active=posts=0;for(unsigned i=0;i<48;++i)controls[i]=0xC00000U;for(unsigned i=0;i<128;++i)maps[i]=0;maps[24]=31;}
int main(void){
 setup();assert(!maia_soc_external_irq_dispatch(17)&&!reads&&!writes);
 assert(!maia_dw_irq_claim(0,0)&&!reads&&!writes);
 maps[80]=17;assert(maia_dw_irq_claim(irq,&posts)&&maia_dw_irq_line()==18&&control==0xC00000U);assert(maia_dw_irq_release());
 setup();control|=0x100;assert(maia_dw_irq_claim(irq,&posts)&&maia_dw_irq_line()==18&&control==0xC00100U);assert(maia_dw_irq_release());
 setup();for(unsigned i=17;i<48;++i)controls[i]|=0x100U;assert(!maia_dw_irq_claim(irq,&posts)&&!writes);
 setup();active=1;assert(!maia_dw_irq_claim(irq,&posts)&&!writes);
 setup();assert(maia_dw_irq_claim(irq,&posts)&&maps[24]==17&&control==0x3FC00100U);
 unsigned n=reads;assert(!maia_dw_irq_claim(irq,&posts)&&reads==n);
 assert(!maia_soc_external_irq_dispatch(16)&&!posts);assert(maia_soc_external_irq_dispatch(17)&&posts==1);
 active=1;assert(!maia_dw_irq_release());active=0;
 assert(maia_dw_irq_release()&&maps[24]==31&&control==0xC00000U);
 assert(!maia_soc_external_irq_dispatch(17));
 for(unsigned i=1;i<=5;++i){setup();drop=i;if(i<=2)assert(!maia_dw_irq_claim(irq,&posts));else{assert(maia_dw_irq_claim(irq,&posts));assert(!maia_dw_irq_release());}n=reads;assert(!maia_dw_irq_claim(irq,&posts)&&reads==n);}
}

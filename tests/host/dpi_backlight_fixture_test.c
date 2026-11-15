#include <assert.h>
#include <stdio.h>
#include "../hardware/dpi_backlight_fixture.h"
static uint32_t out,enable,pad,mux,matrix;static unsigned stores,drop;
static uint32_t read_reg(uintptr_t a) {
 if(a==MAIA_ESP32P4_GPIO_OUT_REG)return out;
 if(a==MAIA_ESP32P4_GPIO_ENABLE_REG)return enable;
 if(a==MAIA_ESP32P4_GPIO_PIN_REG(22))return pad;
 if(a==MAIA_ESP32P4_IO_MUX_GPIO_REG(22))return mux;
 assert(a==MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22));return matrix;
}
static void write_reg(uintptr_t a,uint32_t v) {
 if(++stores==drop)return;
 if(a==MAIA_ESP32P4_GPIO_OUT_W1TS_REG){assert(v==DPI_BL_MASK);out|=v;}
 else if(a==MAIA_ESP32P4_GPIO_OUT_W1TC_REG){assert(v==DPI_BL_MASK);out&=~v;}
 else if(a==MAIA_ESP32P4_GPIO_ENABLE_W1TS_REG){assert(v==DPI_BL_MASK);enable|=v;}
 else if(a==MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG){assert(v==DPI_BL_MASK);enable&=~v;}
 else if(a==MAIA_ESP32P4_GPIO_PIN_REG(22)){assert(((v^pad)&~4U)==0);pad=v;}
 else if(a==MAIA_ESP32P4_IO_MUX_GPIO_REG(22)){assert(((v^mux)&~0x7000U)==0);mux=v;}
 else {assert(a==MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22)&&((v^matrix)&~0xFFFU)==0);matrix=v;}
}
static void setup(unsigned high,unsigned active) {
 out=0xA50A050A | (high ? DPI_BL_MASK : 0);enable=0x9405850A | (active ? DPI_BL_MASK : 0);
 pad=0xA5000004;mux=0x95107200;matrix=0xA5010A7F;stores=drop=0;
}
int main(void) {
 dpi_backlight_snapshot_t s;
 for(unsigned h=0;h<2;++h)for(unsigned e=0;e<2;++e){
  setup(h,e);assert(dpi_bl_on(read_reg,write_reg,&s));
  assert(out==(s.out|DPI_BL_MASK)&&enable==(s.enable|DPI_BL_MASK)&&pad==(s.pad&~4U)&&mux==((s.mux&~0x7000U)|0x1000U)&&matrix==((s.matrix&~0xFFFU)|0x500U));
  assert(dpi_bl_restore(read_reg,write_reg,&s));
  assert(out==s.out&&enable==s.enable&&pad==s.pad&&mux==s.mux&&matrix==s.matrix&&stores==12);
 }
 for(unsigned n=1;n<=12;++n){
  setup(0,1);drop=n;
  if(n<=6){assert(!dpi_bl_on(read_reg,write_reg,&s)&&stores==n);}
  else {assert(dpi_bl_on(read_reg,write_reg,&s));
   assert(!dpi_bl_restore(read_reg,write_reg,&s)&&stores==n);
  }
 }
 puts("PASS: private GPIO22 light fixture preserves other pins, restores four initial states and detects dropped stores");
}

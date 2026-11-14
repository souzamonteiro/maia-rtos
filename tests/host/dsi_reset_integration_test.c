/* Exercise the actual boot integration's refusal/order boundaries. Lower-level
 * board/I2C/SoC APIs are fakes here; their real fixtures run separately. */
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <maia/console.h>
#include <maia/boards/tab5_panel_reset.h>
#include <maia/boards/tab5_touch.h>
#include <maia/soc/esp32p4_boot_i2c.h>
#include "vo3_cycle_fixture.h"
static jmp_buf stop;
static maia_mipi_gate_report_t gate;
static uint32_t cpu,sys,phy,reset,isp,norst,gpio,mux,route,dma;
static unsigned prepared,reset_calls,touch_calls,config_reads,writes,drop,supply_calls;
static unsigned firmware,fault;
static uint8_t banks[2][5];
static const char *reason;
static void halt(const char *s) { reason=s;longjmp(stop,1); }
static void field(const char *s,uint32_t n) { (void)s;(void)n; }
void maia_console_puts(const char *s) { (void)s; }
static uint32_t rd(uintptr_t p) {
 if (p == MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14) return cpu;
 if (p == MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) return sys;
 if (p == MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG) return phy;
 if (p == MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0) return reset;
 if (p == MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xA8) return isp;
 if (p == MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCC) return norst;
 if (p == MAIA_ESP32P4_GPIO_ENABLE_REG) return gpio;
 if (p == MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23)) return route;
 if (p == MAIA_ESP32P4_IO_MUX_GPIO_REG(23)) return mux;
 if (p>=MAIA_ESP32P4_GDMA_BASE && p<MAIA_ESP32P4_GDMA_BASE+0x500) return dma;
 assert(!"unexpected or gated MMIO read");return 0;
}
static void wr(uintptr_t p,uint32_t n) {
 ++writes;if (writes == drop) return;
 if (p == MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG) { assert(n == 1U<<23);gpio&=~n; }
 else if (p == MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23)) route=n;
 else if (p == MAIA_ESP32P4_IO_MUX_GPIO_REG(23)) mux=n;
 else assert(!"unexpected MMIO write");
}
static int vo3_same(const maia_ldo3_snapshot_t *a,const maia_ldo3_snapshot_t *b) {
 return !memcmp(a,b,sizeof(*a));
}
maia_ldo3_result_t maia_esp32p4_ldo3_snapshot(maia_ldo3_snapshot_t *s) {
 ++supply_calls;memset(s,0,sizeof(*s));s->control=0x180;s->analog=0xD5800000;
 if (fault == 1 && supply_calls == 2) s->control^=1;
 return MAIA_LDO3_OK;
}
maia_ldo3_result_t maia_esp32p4_ldo3_plan2500(const maia_ldo3_snapshot_t *s,maia_ldo3_plan_t *p) {
 (void)s;memset(p,0,sizeof(*p));p->dref=13;p->mul=3;return MAIA_LDO3_OK;
}
static unsigned sample(void *p) { (void)p;return MAIA_I2C_BOTH; }
maia_i2c_result_t maia_esp32p4_boot_i2c_prepare(maia_esp32p4_boot_i2c_t *b,uint32_t sda,uint32_t scl) {
 assert(sda == 31 && scl == 32 && !reset_calls);++prepared;b->lines.sample=sample;return MAIA_I2C_OK;
}
maia_i2c_result_t maia_tab5_touch_identify(const maia_i2c_lines_t *b,uint8_t *fw) {
 (void)b;++touch_calls;*fw=(uint8_t)firmware;
 if (fault == 4 && reset_calls) return MAIA_I2C_NACK;
 return MAIA_I2C_OK;
}
maia_i2c_result_t maia_pi4io_read_config(const maia_i2c_lines_t *b,uint8_t address,uint8_t reg,uint8_t *v) {
 (void)b;const uint8_t regs[]={3,5,7,0x0B,0x0D};assert(address == 0x43 || address == 0x44);
 for (unsigned i=0;i<5;++i) if (reg == regs[i]) {
  *v=banks[address-0x43][i];++config_reads;
  if (fault == 2 && config_reads == 10) phy|=1;
  return MAIA_I2C_OK;
 }
 assert(!"unexpected expander register");return MAIA_I2C_INVALID;
}
maia_i2c_result_t maia_tab5_panel_reset(const maia_i2c_lines_t *b,maia_tab5_reset_report_t *r) {
 (void)b;assert(prepared && touch_calls == 1 && config_reads == 10 && !reset_calls);
 assert(vo3_cycle_guard(&gate,cpu,rd));++reset_calls;r->may_have_changed=1;
 r->completed=fault == 3 ? MAIA_TAB5_RESET_HELD_LOW : MAIA_TAB5_RESET_VERIFIED;
 const uint8_t masks[]={0x30,0x30,0x30,0x10,0x10},bits[]={0x20,0x20,0,0x10,0x10};
 for (unsigned i=0;i<5;++i) banks[0][i]=(uint8_t)((banks[0][i]&~masks[i])|bits[i]);
 if (fault == 5) banks[1][0]^=1;
 if (fault == 6) gpio|=1U<<23;
 return fault == 3 ? MAIA_I2C_TIMEOUT : MAIA_I2C_OK;
}
maia_i2c_result_t maia_i2c_read_register16(const maia_i2c_lines_t *b,uint8_t a,uint16_t reg,uint8_t *v) {
 (void)b;assert(reset_calls == 1 && a == 0x55 && reg == 1);*v=0;return MAIA_I2C_OK;
}
uint64_t maia_soc_time_us(void) { return reset_calls ? 245000 : 1000; }
#include "native_dsi_reset.h"
static void setup(void) {
 memset(&gate,0,sizeof(gate));gate.completed=MAIA_MIPI_GATE_VERIFIED;
 cpu=0x2000;sys=gate.sys_after=0x20;phy=gate.phy_after=0;reset=gate.reset_after=0x3C000000;
 isp=gate.isp_clock=123;norst=gate.force_norst=0;dma=0;
 gpio=1U<<23;mux=route=0;prepared=reset_calls=touch_calls=config_reads=writes=drop=supply_calls=fault=0;
 firmware=1;reason=NULL;memset(banks,0xA5,sizeof(banks));
}
static void reject(const char *why,unsigned resets) {
 if (!setjmp(stop)) { dsi_panel_reset(&gate,cpu);assert(!"failure accepted"); }
 assert(reason && !strcmp(reason,why) && reset_calls == resets);
}
int main(void) {
 for (unsigned fw=1;fw<=3;fw+=2) {
  setup();firmware=fw;if (!setjmp(stop)) dsi_panel_reset(&gate,cpu);else assert(!"good sequence rejected");
  assert(reset_calls == 1 && config_reads == 20 && touch_calls == 2 && supply_calls == 3);
 }
 setup();phy=1;reject("panel-reset-quiet",0);assert(!prepared && !writes);
 setup();dma=1;reject("panel-reset-quiet",0);assert(!prepared && !writes);
 setup();firmware=2;reject("panel-reset-variant",0);assert(!writes);
 for (unsigned i=1;i<=3;++i) { setup();drop=i;reject("panel-reset-gpio23",0); }
 setup();fault=1;reject("panel-reset-precondition",0);
 setup();fault=2;reject("panel-reset-precondition",0);
 setup();fault=3;reject("panel-reset-sequence",1);assert(touch_calls == 1 && config_reads == 10);
 setup();fault=4;reject("panel-reset-recovery",1);
 setup();fault=5;reject("panel-reset-banks",1);
 setup();fault=6;reject("panel-reset-recovery",1);
 puts("PASS: actual reset integration rejects unsafe clocks/DMA/variant/GPIO/supply and stops on partial reset or recovery failure");
}

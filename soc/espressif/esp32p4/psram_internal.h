/* Original MIT, register facts only, SDK f5c3654 reg_base/spi_mem_s/
 * spi1_mem_s/hp_sys_clkrst/pmu, ldo_ll index map. No SDK headers/runtime. */
#ifndef MAIA_PSRAM_INTERNAL_H
#define MAIA_PSRAM_INTERNAL_H
#include <maia/soc/esp32p4_regs.h>
#include <maia/soc/esp32p4_psram.h>
#define PSRAM_MSPI0 0x5008E000U
#define PSRAM_MSPI1 0x5008F000U
#define PSRAM_SYS (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)
#define PSRAM_PERI (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x30U)
#define PSRAM_RESET (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U)
static const uintptr_t psram_roots[MAIA_PSRAM_ROOT_WORDS]={PSRAM_SYS,PSRAM_PERI,PSRAM_RESET,
    MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCCU,MAIA_ESP32P4_PMU_BASE+0x1D0U,MAIA_ESP32P4_PMU_BASE+0x1D4U,
    MAIA_ESP32P4_EFUSE_MAC_SYS2_REG,MAIA_ESP32P4_EFUSE_MAC_SYS3_REG,MAIA_ESP32P4_EFUSE_RS_ERR0_REG};
static const uint16_t psram_offsets[MAIA_PSRAM_CONTROLLER_WORDS]={
    0,8,0xC,0x40,0x44,0x48,0x4C,0x50,0xD8,0x180,0x184,0x188,0x18C,0x190,0x200,0x130,
    0,8,0xC,0x10,0x14,0x18,0x1C,0x20,0xD4,0x180,0x200};
static inline uintptr_t psram_address(unsigned i){return (i<16U?PSRAM_MSPI0:PSRAM_MSPI1)+psram_offsets[i];}
static inline int psram_accessible(const uint32_t roots[MAIA_PSRAM_ROOT_WORDS]){
    return (roots[0]&0x80000000U)&&(roots[1]&0xC000U)==0xC000U&&!(roots[2]&0x02800000U);
}
#ifdef MAIA_PSRAM_TEST
int maia_psram_test_context(void);
uint32_t maia_psram_test_read(uintptr_t);
void maia_psram_test_write(uintptr_t,uint32_t);
#define psram_context maia_psram_test_context
#define psram_read maia_psram_test_read
#define psram_write maia_psram_test_write
#else
static inline int psram_context(void){
    uint32_t hart,status;__asm__ volatile("csrr %0,mhartid":"=r"(hart));__asm__ volatile("csrr %0,mstatus":"=r"(status));return !hart&&!(status&8U);
}
static inline uint32_t psram_read(uintptr_t p){return MAIA_MMIO32(p);}
static inline void psram_write(uintptr_t p,uint32_t v){MAIA_MMIO32(p)=v;__asm__ volatile("fence iorw, iorw" ::: "memory");}
#endif
/* CACHE_FCTRL+3C, not CTRL1+0C: AXI_REQ_EN0 and CLOSE_AXI_INF_EN1.
 * Caller must establish accessible controller clocks before this read. */
static inline int psram_axi_quiet(void){return (psram_read(PSRAM_MSPI0+0x3CU)&0x80000001U)==0x80000000U;}
int maia_psram_x8_claimed(void);
int maia_psram_clock_claimed(void);
unsigned maia_psram_clock_mhz(void);
maia_psram_result_t maia_psram_fast_phase(unsigned phase);
maia_psram_result_t maia_psram_mpll_read(uint32_t budget,maia_psram_mpll_report_t *);
#endif

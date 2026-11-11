#include <maia/arch/riscv_trap_policy.h>
#include <stdio.h>
int main(void) {
    uint32_t resume = 0U;
    if (!maia_riscv_yield_resume_pc(0x3800000BU, 0x4FF00102U, &resume) || resume != 0x4FF00106U) return 1;
    const uint32_t invalid_causes[] = {2U, 3U, 8U, 0x3800010BU, 0xB800000BU,
                                      0xB8000010U, 0x7800000BU, 0x3000000BU};
    for (size_t i = 0U; i < sizeof(invalid_causes)/sizeof(invalid_causes[0]); ++i) {
        resume = 0xDEADBEEFU;
        if (maia_riscv_yield_resume_pc(invalid_causes[i], 0x4FF00102U, &resume)) return 2;
        if (resume != 0xDEADBEEFU) return 3;
    }
    if (maia_riscv_yield_resume_pc(0x3800000BU, 0x4FF00103U, &resume)) return 4;
    if (maia_riscv_yield_resume_pc(0x3800000BU, 0xFFFFFFFCU, &resume)) return 5;
    if (maia_riscv_yield_resume_pc(0x3800000BU, 0U, NULL)) return 6;
    puts("PASS: only valid enabled M-mode yields resume; fatal causes leave PC output unchanged");
    return 0;
}

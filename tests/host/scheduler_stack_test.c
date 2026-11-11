#include <maia/kernel.h>
#include <maia/scheduler_internal.h>
#include <maia/arch/riscv_stack.h>
#include <stdio.h>
#include <string.h>

static uint8_t buffers[3][2048] __attribute__((aligned(16)));
static uintptr_t low_published, high_published;
void maia_arch_stack_publish(uintptr_t low, uintptr_t high) { low_published=low; high_published=high; }
int maia_arch_stack_reserved(uintptr_t base, size_t size) {
    return maia_stack_overlap(base,size,(uintptr_t)buffers[2],sizeof buffers[2]);
}
static void task(void *arg) { (void)arg; }
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void) {
    for (unsigned offset=0U; offset<16U; ++offset) {
        CHECK(maia_sched_init() == MAIA_OK);
        memset(buffers[0],0xcc,sizeof buffers[0]);
        uintptr_t base=(uintptr_t)buffers[0]+offset;
        CHECK(maia_task_create("align",task,NULL,2U,(void *)base,MAIA_RISCV_MIN_STACK_BYTES,NULL) == MAIA_OK);
        uintptr_t sp=maia_sched_select_next_sp(0U);
        CHECK(sp == maia_stack_top(base,MAIA_RISCV_MIN_STACK_BYTES)-MAIA_RISCV_CONTEXT_BYTES);
        CHECK(maia_stack_frame_valid(base,MAIA_RISCV_MIN_STACK_BYTES,sp));
        CHECK(low_published == maia_stack_low(base) && high_published == maia_stack_top(base,MAIA_RISCV_MIN_STACK_BYTES));
        CHECK(!maia_stack_frame_valid(base,MAIA_RISCV_MIN_STACK_BYTES,sp+1U));
        CHECK(!maia_stack_frame_valid(base,MAIA_RISCV_MIN_STACK_BYTES,sp-1U));
        CHECK(!maia_stack_frame_valid(base,MAIA_RISCV_MIN_STACK_BYTES,high_published));
        CHECK(!maia_stack_frame_valid(base,MAIA_RISCV_MIN_STACK_BYTES,low_published-16U));
        CHECK(!maia_stack_frame_valid(UINTPTR_MAX-15U,MAIA_RISCV_MIN_STACK_BYTES,0U));
        for (unsigned i=0U; i<offset; ++i) CHECK(buffers[0][i] == 0xcc);
        for (unsigned i=offset+MAIA_RISCV_MIN_STACK_BYTES; i<sizeof buffers[0]; ++i) CHECK(buffers[0][i] == 0xcc);
        uintptr_t before_low=low_published;
        CHECK(maia_sched_select_next_sp(sp-1U) == 0U && low_published == before_low);
        volatile uint32_t *guard=(volatile uint32_t *)(maia_stack_low(base)-16U);
        guard[0]=0U;
        CHECK(maia_sched_select_next_sp(0U) == 0U);
    }
    CHECK(maia_sched_init() == MAIA_OK);
    uintptr_t idle=maia_sched_select_next_sp(0U);
    uintptr_t idle_base=idle+MAIA_RISCV_CONTEXT_BYTES-2048U;
    CHECK(maia_task_create("reserved",task,NULL,2U,(void *)(idle_base+128U),1024U,NULL) == MAIA_ERR_INVALID);
    CHECK(maia_task_create("port",task,NULL,2U,buffers[2],1024U,NULL) == MAIA_ERR_INVALID);
    maia_task_id_t a,b;
    CHECK(maia_task_create("A",task,NULL,3U,buffers[0],1024U,&a) == MAIA_OK);
    CHECK(maia_task_create("overlap",task,NULL,2U,buffers[0]+16U,1024U,NULL) == MAIA_ERR_INVALID);
    CHECK(maia_task_create("adjacent",task,NULL,2U,buffers[0]+1024U,1024U,&b) == MAIA_OK);
    uintptr_t a_sp=maia_sched_select_next_sp(idle);
    CHECK(maia_task_current_id() == a);
    volatile uint32_t *bad_guard=(volatile uint32_t *)maia_stack_low((uintptr_t)buffers[0]+1024U)-4;
    bad_guard[2]=0U;
    CHECK(maia_sched_delay_current_isr(1U) == MAIA_OK);
    uintptr_t before_low=low_published;
    CHECK(maia_sched_select_next_sp(a_sp) == 0U && low_published == before_low);
    maia_task_info_t info;
    CHECK(maia_task_get_info((uint32_t)b,&info) == MAIA_OK && info.state == MAIA_TASK_READY);
    CHECK(maia_task_count() == 2U);
    puts("PASS: guard/alignment/bounds, no out-of-buffer writes, overlap/reserved rejection and no bad-context publication");
    return 0;
}

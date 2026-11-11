#include <maia/kernel.h>
#include <maia/arch/irq.h>
#include <stdio.h>
#include <string.h>

static uintptr_t irq_state = 8U;
static unsigned saves, restores, copy_steps, violations, pending, injected;
static maia_queue_t *interrupt_queue;
static uint32_t interrupt_item = 0xabcdef12U;
uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = irq_state; irq_state = 0U; ++saves; return previous;
}
void maia_arch_irq_restore(uintptr_t previous) {
    irq_state = previous & 8U; ++restores;
    if (irq_state && pending) {
        pending = 0U; irq_state = 0U;
        if (maia_queue_send(interrupt_queue,&interrupt_item) != MAIA_OK) ++violations;
        if (irq_state) ++violations;
        ++injected; irq_state = 8U;
    }
}
void maia_queue_test_copy_step(void) {
    ++copy_steps;
    if (irq_state != 0U) ++violations;
}
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n",__LINE__); return 1; } } while (0)

int main(void) {
    maia_queue_t q, before;
    uint8_t storage[3U*MAIA_QUEUE_MAX_ITEM_BYTES+2U], item[MAIA_QUEUE_MAX_ITEM_BYTES], out[MAIA_QUEUE_MAX_ITEM_BYTES];
    memset(&q,0x5a,sizeof q); before=q;
    CHECK(maia_queue_init(&q,storage,0U,1U) == MAIA_ERR_INVALID);
    CHECK(maia_queue_init(&q,storage,MAIA_QUEUE_MAX_ITEM_BYTES+1U,1U) == MAIA_ERR_INVALID);
    CHECK(maia_queue_init(&q,storage,2U,SIZE_MAX) == MAIA_ERR_INVALID);
    CHECK(maia_queue_init(&q,(void *)(UINTPTR_MAX-1U),2U,1U) == MAIA_ERR_INVALID);
    CHECK(maia_queue_init(&q,storage,1U,0U) == MAIA_ERR_INVALID);
    CHECK(maia_queue_init(&q,NULL,1U,1U) == MAIA_ERR_INVALID);
    CHECK(maia_queue_init(NULL,storage,1U,1U) == MAIA_ERR_INVALID);
    CHECK(memcmp(&q,&before,sizeof q) == 0);
    memset(storage,0xcc,sizeof storage);
    CHECK(maia_queue_init(&q,storage+1U,sizeof item,3U) == MAIA_OK);
    for (unsigned turn=0U; turn<1000U; ++turn) {
        for (unsigned n=0U;n<3U;++n) {
            memset(item,(int)((turn+n)&255U),sizeof item);
            CHECK(maia_queue_send(&q,item) == MAIA_OK && !violations);
        }
        before=q;
        CHECK(maia_queue_send(&q,item) == MAIA_ERR_FULL && memcmp(&q,&before,sizeof q) == 0);
        unsigned saved=saves;
        CHECK(maia_queue_count(&q) == 3U && saves == saved+1U && irq_state == 8U);
        for (unsigned n=0U;n<3U;++n) {
            CHECK(maia_queue_recv(&q,out) == MAIA_OK);
            for (unsigned i=0U;i<sizeof out;++i) CHECK(out[i] == (uint8_t)((turn+n)&255U));
        }
        memset(out,0x55,sizeof out); before=q;
        CHECK(maia_queue_recv(&q,out) == MAIA_ERR_EMPTY && memcmp(&q,&before,sizeof q) == 0);
        for (unsigned i=0U;i<sizeof out;++i) CHECK(out[i] == 0x55U);
    }
    CHECK(storage[0] == 0xccU && storage[sizeof storage-1U] == 0xccU);
    uintptr_t outer=maia_arch_irq_save();
    CHECK(maia_queue_send(&q,item) == MAIA_OK && irq_state == 0U);
    CHECK(maia_queue_count(&q) == 1U && irq_state == 0U);
    CHECK(maia_queue_recv(&q,out) == MAIA_OK && irq_state == 0U);
    CHECK(maia_queue_recv(&q,out) == MAIA_ERR_EMPTY && irq_state == 0U);
    CHECK(maia_queue_send(NULL,item) == MAIA_ERR_INVALID && irq_state == 0U);
    CHECK(maia_queue_recv(&q,NULL) == MAIA_ERR_INVALID && irq_state == 0U);
    maia_queue_t invalid={0};
    CHECK(maia_queue_send(&invalid,item) == MAIA_ERR_INVALID && irq_state == 0U);
    CHECK(maia_queue_recv(&invalid,out) == MAIA_ERR_INVALID && irq_state == 0U);
    maia_arch_irq_restore(outer);
    uint32_t words[3], first=0x12345678U, actual;
    CHECK(maia_queue_init(&q,words,sizeof first,3U) == MAIA_OK);
    interrupt_queue=&q; pending=1U;
    CHECK(maia_queue_send(&q,&first) == MAIA_OK && injected == 1U);
    CHECK(maia_queue_recv(&q,&actual) == MAIA_OK && actual == first);
    CHECK(maia_queue_recv(&q,&actual) == MAIA_OK && actual == interrupt_item);
    CHECK(maia_queue_count(&q) == 0U && maia_queue_count(NULL) == 0U);
    CHECK(copy_steps > 1500000U && !violations && saves == restores && irq_state == 8U);
    puts("PASS: complete-copy exclusion, deferred IRQ producer, nested masks, FIFO/wrap, maximum payload, exhaustion and overflow rejection");
    return 0;
}

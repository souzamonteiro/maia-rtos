/* Original native executor MIT; attributed vendor table in st7121_program.h. */
#include <maia/boards/tab5_st7121.h>
#include "st7121_program.h"
static unsigned claimed;
#ifdef MAIA_ST7121_TEST
void maia_st7121_test_reset(void) { claimed=0; }
#endif
maia_st7121_result_t maia_tab5_st7121_program(uint8_t firmware,const maia_st7121_io_t *io,maia_st7121_report_t *r) {
    if (!r) return MAIA_ST7121_INVALID;
    r->attempted=r->completed=r->waited_ms=r->last_command=0;r->callback_result=0;
    if (!io || !io->write || !io->wait_ms) return MAIA_ST7121_INVALID;
    if (firmware != 1U) return MAIA_ST7121_UNSUPPORTED;
    if (claimed) return claimed == 1U ? MAIA_ST7121_BUSY : MAIA_ST7121_FAILED;
    claimed=2;
    for (unsigned i=0;i<ST7121_PROGRAM_COUNT;++i) {
        const st7121_step_t *p=&st7121_program[i];
        r->last_command=p->bytes[0];++r->attempted;
        r->callback_result=io->write(io->context,p->bytes,p->length);
        if (r->callback_result) return MAIA_ST7121_WRITE;
        r->callback_result=io->wait_ms(io->context,p->wait_ms);
        if (r->callback_result) return MAIA_ST7121_WAIT;
        r->waited_ms+=p->wait_ms;++r->completed;
    }
    claimed=1;return MAIA_ST7121_OK;
}

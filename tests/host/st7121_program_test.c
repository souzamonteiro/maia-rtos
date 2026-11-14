#include <maia/boards/tab5_st7121.h>
#include <assert.h>
#include <stdio.h>
void maia_st7121_test_reset(void);
static unsigned writes,waits,fail_write,fail_wait,total_bytes,total_ms,phase;
static uint32_t hash;
static void byte(unsigned b) { hash=(hash^(b&255U))*16777619U; }
static int write_packet(void *p,const uint8_t *bytes,uint32_t count) {
 assert(p == &hash && !phase && count && count<=61U);phase=1;++writes;
 if (writes == fail_write) return -71;
 byte(count);for (unsigned i=0;i<count;++i) byte(bytes[i]);total_bytes+=count;return 0;
}
static int wait_ms(void *p,uint32_t ms) {
 assert(p == &hash && phase && ms>=5 && ms<=805);phase=0;++waits;
 if (waits == fail_wait) return -72;
 byte(ms);byte(ms>>8);total_ms+=ms;return 0;
}
static maia_st7121_io_t io={&hash,write_packet,wait_ms};
static maia_st7121_report_t r;
static void setup(void) { maia_st7121_test_reset();writes=waits=fail_write=fail_wait=total_bytes=total_ms=phase=0;hash=2166136261U; }
int main(void) {
 setup();assert(maia_tab5_st7121_program(1,NULL,&r) == MAIA_ST7121_INVALID);
 assert(maia_tab5_st7121_program(1,&io,NULL) == MAIA_ST7121_INVALID && !writes);
 for (unsigned fw=0;fw<4;++fw) if (fw != 1) assert(maia_tab5_st7121_program((uint8_t)fw,&io,&r) == MAIA_ST7121_UNSUPPORTED && !writes);
 assert(maia_tab5_st7121_program(1,&io,&r) == MAIA_ST7121_OK);
 /* Pinned883e9a6 source stream includes length, bytes and little-endian wait;
    independent of native program header. Source extraction retained in audit. */
 assert(writes == 31 && waits == 31 && total_bytes == 461 && total_ms == 1150 && hash == 0xF644B840U);
 assert(r.attempted == 31 && r.completed == 31 && r.waited_ms == 1150 && r.last_command == 0x35);
 assert(maia_tab5_st7121_program(1,&io,&r) == MAIA_ST7121_BUSY && writes == 31 && waits == 31);
 for (unsigned i=1;i<=31;++i) {
  setup();fail_write=i;assert(maia_tab5_st7121_program(1,&io,&r) == MAIA_ST7121_WRITE);
  assert(writes == i && waits == i-1 && r.attempted == i && r.completed == i-1 && r.callback_result == -71);
  assert(maia_tab5_st7121_program(1,&io,&r) == MAIA_ST7121_FAILED && writes == i && waits == i-1);
  setup();fail_wait=i;assert(maia_tab5_st7121_program(1,&io,&r) == MAIA_ST7121_WAIT);
  assert(writes == i && waits == i && r.attempted == i && r.completed == i-1 && r.callback_result == -72);
  assert(maia_tab5_st7121_program(1,&io,&r) == MAIA_ST7121_FAILED && writes == i && waits == i);
 }
 puts("PASS: pinned31-packet stream/1150ms, supported selection,62 partial failures, duplicate/sticky refusal");
}

/* Native boot-only PSRAM full-frame diagnostic. No flash/SD/IRQ/foreign runtime.
 * Existing panel/clock/bridge path completed first, all graphics clients stopped.
 * ElectronicE=4frames/no backlight change; visualV=1000frames/temporary light. */
#include "../../soc/espressif/esp32p4/psram_internal.h"
#include "../../soc/espressif/esp32p4/dsi_framebuffer_probe.h"
#include "framebuffer_pattern.h"
#include "dpi_backlight_fixture.h"
static uint8_t fb_saved[4096],fb_readback[4096];
static uint32_t window_value(uint32_t index){
    uint32_t result=0;
    for(unsigned b=0;b<4U;++b){unsigned offset=index*4U+b;uint8_t value=0xA5;
        if(offset>=128U&&offset<128U+FB_BYTES){unsigned p=(offset-128U)/2U;value=(uint8_t)(fb_native_pixel(p%720U,p/720U)>>(((offset-128U)&1U)*8U));}
        result|=(uint32_t)value<<(b*8U);
    }
    return result;
}
#include "native_psram_dma.h"
static int fb_raw(unsigned phase,void *data,unsigned write){
    maia_psram_transfer_report_t r;maia_psram_result_t result=maia_esp32p4_psram_boot_transfer(0,data,4096,write,10000,&r);
    field("DPI_FB_RAW phase=",phase);field(" result=",(uint32_t)result);field(" bytes=",r.completed);field(" restored=",r.restored);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.completed==4096U&&r.restored;
}
static uint32_t fb_hash(volatile const uint8_t *p,unsigned n){uint32_t h=2166136261U;for(unsigned i=0;i<n;++i)h=(h^p[i])*16777619U;return h;}
#if defined(MAIA_NATIVE_PROBE_PSRAM_PROFILE) && !defined(MAIA_NATIVE_PROBE_PSRAM_BOOT)
static uint32_t fb_original_profile[3];
static void fb_profile(unsigned restore){
    maia_psram_profile_report_t r;maia_psram_result_t result=maia_esp32p4_psram_profile200(restore,10000,&r);
    field("PSRAM_PROFILE phase=",restore);field(" result=",(uint32_t)result);field(" stage=",r.stage);field(" attempts=",r.write_attempts);field(" polls=",r.polls);field(" restored=",r.restored);
    for(unsigned i=0;i<3U;++i){field(i==0U?" before0=":i==1U?" before4=":" before8=",r.before[i]);field(i==0U?" after0=":i==1U?" after4=":" after8=",r.after[i]);}
    maia_console_puts("\n");if(result!=MAIA_PSRAM_OK)halt("fb-profile");
    for(unsigned i=0;i<3U;++i){if(!restore)fb_original_profile[i]=r.before[i];else if(r.after[i]!=fb_original_profile[i])halt("fb-profile-original");}
}
#endif
#ifdef MAIA_NATIVE_PROBE_MPLL_INITIALIZE
static void fb_mpll_report(maia_psram_result_t result,const maia_psram_mpll_init_report_t *r){
    field("MPLL_BOOT result=",(uint32_t)result);field(" stage=",r->stage);field(" writes=",r->writes);field(" polls=",r->polls);field(" routing=",r->route_restored);field(" saw_low=",r->saw_not_done);field(" cal_before=",r->cal_before);field(" cal_after=",r->cal_after);
    for(unsigned i=0;i<3U;++i){field(i==0U?" before1=":i==1U?" before2=":" before3=",r->before[i]);field(i==0U?" after1=":i==1U?" after2=":" after3=",r->after[i]);}
    field(" mhz=",r->mhz);maia_console_puts("\n");
}
#endif
static void fb_finish(uint32_t hash,const maia_psram_snapshot_t *initial){
    volatile uint8_t *memory=(volatile uint8_t *)(uintptr_t)MAIA_PSRAM_WINDOW_VADDR;
    maia_psram_snapshot_t before=*initial,after;
    maia_psram_window_report_t window;maia_psram_clock_report_t clock;maia_psram_mode_switch_report_t mode;
    uint32_t after_hash=fb_hash(memory+128U,FB_BYTES);unsigned guards=1;
    for(unsigned i=0;i<MAIA_PSRAM_WINDOW_BYTES;++i)if((i<128U||i>=128U+FB_BYTES)&&memory[i]!=0xA5)guards=0;
    maia_dsi_framebuffer_report_t again;
    unsigned duplicate=maia_dsi_framebuffer_probe(1,1,&again)==MAIA_DSI_ACCESS_BUSY;
    field("DPI_FB_IDLE cfg=",rd(MAIA_ESP32P4_GDMA_BASE+0x10U));field(" channels=",rd(MAIA_ESP32P4_GDMA_BASE+0x18U));field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U));field(" mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U));field(" hash=",after_hash);field(" guards=",guards);field(" duplicate=",duplicate);maia_console_puts("\n");
    if(hash!=after_hash||!guards||!duplicate)halt("fb-transfer");
    if(maia_esp32p4_psram_window_stop(10000,&window)!=MAIA_PSRAM_OK||maia_esp32p4_psram_fast_stop(&clock)!=MAIA_PSRAM_OK)halt("fb-memory-stop");
    if(!fb_raw(1,fb_saved,1)||!fb_raw(2,fb_readback,0))halt("fb-raw-restore");
    unsigned restored=0;for(unsigned i=0;i<4096U;++i)if(fb_saved[i]==fb_readback[i])++restored;
    if(maia_esp32p4_psram_x8_mode(1,10000,&mode)!=MAIA_PSRAM_OK)halt("fb-mode-restore");
#if defined(MAIA_NATIVE_PROBE_PSRAM_PROFILE) && !defined(MAIA_NATIVE_PROBE_PSRAM_BOOT)
    fb_profile(1);mode.after=fb_original_profile[2];
#endif
#ifndef MAIA_NATIVE_PROBE_PSRAM_BOOT
    if(maia_esp32p4_psram_xtal20_stop(&clock)!=MAIA_PSRAM_OK)halt("fb-clock-restore");
#endif
    if(maia_esp32p4_psram_snapshot(&after)!=MAIA_PSRAM_OK)halt("fb-clock-snapshot");
    unsigned roots=1,words=1;for(unsigned i=0;i<9U;++i)if(before.roots[i]!=after.roots[i])roots=0;
    for(unsigned i=0;i<27U;++i)if(before.before[i]!=after.before[i])words=0;
    field("DPI_FB_MEMORY_RESTORE bytes=",restored);field(" roots=",roots);field(" words=",words);field(" mode=",mode.after);field(" axi=",rd(PSRAM_MSPI0+0x3CU));maia_console_puts("\n");
    if(restored!=4096U||!roots||!words)halt("fb-restore");
    for(unsigned i=0;i<4U;++i){maia_dsi_read_report_t p;maia_dsi_access_result_t reply=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_FB_REPLY index=",i);field(" result=",(uint32_t)reply);field(" word=",p.word);field(" restored=",p.restore_attempted);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if(reply!=MAIA_DSI_ACCESS_OK||(p.word&0x14U)!=0x14U)halt("fb-reply");}
}
#ifdef MAIA_NATIVE_PROBE_LCD_STREAM
#include "native_lcd_stream_probe.h"
#endif
static void dpi_framebuffer(void){
    maia_console_puts("DPI_FB_READY\n");unsigned command=0;
    for(unsigned i=0;i<10000000U;++i){int c=maia_serial_rx_read(NULL);if(c=='E'||c=='V'){command=(unsigned)c;break;}}
    if(!command)halt("fb-host-timeout");
    unsigned visual=command=='V',frames=visual?1000U:4U;
#ifdef MAIA_NATIVE_PROBE_LCD_STREAM
    frames=0; /* runtime session stopped by controller, not a frame batch */
#endif
    maia_psram_cache_report_t cache;maia_psram_result_t result;
#ifdef MAIA_NATIVE_PROBE_PSRAM_BOOT
    maia_psram_boot_report_t boot;maia_psram_result_t booted=maia_esp32p4_psram_boot_initialize200(10000,&boot);
    field("PSRAM_BOOT result=",(uint32_t)booted);field(" stage=",boot.stage);field(" writes=",boot.writes);field(" supply_stage=",boot.supply.stage);field(" supply_writes=",boot.supply.writes);field(" adopted=",boot.supply.adopted);field(" dref=",boot.supply.dref);field(" mul=",boot.supply.mul);
    field(" beforectl=",boot.supply.before[0]);field(" afterctl=",boot.supply.after[0]);field(" beforeana=",boot.supply.before[1]);field(" afterana=",boot.supply.after[1]);field(" clock_stage=",boot.clock.stage);field(" attempts=",boot.clock.attempts);field(" beforeperi=",boot.clock.before[0]);field(" afterperi=",boot.clock.after[0]);field(" bus0=",boot.clock.after[1]);field(" bus1=",boot.clock.after[2]);field(" profile_stage=",boot.profile.stage);field(" profile_attempts=",boot.profile.write_attempts);field(" profile_polls=",boot.profile.polls);
    for(unsigned i=0;i<3U;++i){field(i==0U?" before0=":i==1U?" before4=":" before8=",boot.profile.before[i]);field(i==0U?" after0=":i==1U?" after4=":" after8=",boot.profile.after[i]);}
    maia_console_puts("\n");
    if(boot.stage>=5U)fb_mpll_report(boot.stage>=6U?MAIA_PSRAM_OK:booted,&boot.mpll);
    if(booted!=MAIA_PSRAM_OK)halt("fb-native-boot");
#endif
    result=maia_esp32p4_psram_cache_boot(&cache);
    field("DPI_FB_CACHE result=",(uint32_t)result);field(" stage=",cache.stage);field(" size=",cache.size);field(" line=",cache.line);maia_console_puts("\n");
    if(result!=MAIA_PSRAM_OK)halt("fb-cache");
#if defined(MAIA_NATIVE_PROBE_PSRAM_SUPPLY) && !defined(MAIA_NATIVE_PROBE_PSRAM_BOOT)
    maia_psram_supply_report_t supply;maia_psram_result_t supplied=maia_esp32p4_psram_supply1900(&supply);
    field("PSRAM_SUPPLY result=",(uint32_t)supplied);field(" stage=",supply.stage);field(" adopted=",supply.adopted);field(" writes=",supply.writes);field(" dref=",supply.dref);field(" mul=",supply.mul);
    for(unsigned i=0;i<2U;++i){field(i?" before1=":" before0=",supply.before[i]);field(i?" after1=":" after0=",supply.after[i]);}
    maia_console_puts("\n");if(supplied!=MAIA_PSRAM_OK||!supply.adopted||supply.writes)halt("fb-supply");
#endif
    maia_psram_snapshot_t before;
    if(maia_esp32p4_psram_snapshot(&before)!=MAIA_PSRAM_OK)halt("fb-snapshot");
#ifndef MAIA_NATIVE_PROBE_PSRAM_BOOT
    maia_psram_clock_report_t clock;
    if(maia_esp32p4_psram_xtal20_start(&clock)!=MAIA_PSRAM_OK)halt("fb-xtal");
#endif
#ifdef MAIA_NATIVE_PROBE_PSRAM_PARK
    for(unsigned phase=0;phase<2U;++phase){
        maia_psram_park_report_t parked;maia_psram_result_t result=maia_esp32p4_psram_consumers_park(phase,&parked);
        field("PSRAM_PARK phase=",phase);field(" result=",(uint32_t)result);field(" stage=",parked.stage);field(" writes=",parked.writes);field(" restored=",parked.restored);
        for(unsigned i=0;i<3U;++i){field(i==0U?" before0=":i==1U?" before1=":" before2=",parked.before[i]);field(i==0U?" after0=":i==1U?" after1=":" after2=",parked.after[i]);}
        maia_console_puts("\n");if(result!=MAIA_PSRAM_OK)halt("fb-park");
    }
#endif
#if defined(MAIA_NATIVE_PROBE_MPLL_INITIALIZE) && !defined(MAIA_NATIVE_PROBE_PSRAM_BOOT)
    maia_psram_mpll_init_report_t initialized;
    maia_psram_result_t programmed=maia_esp32p4_psram_mpll_initialize400(10000,&initialized);
    fb_mpll_report(programmed,&initialized);
    if(programmed!=MAIA_PSRAM_OK)halt("fb-mpll-initialize");
#endif
#if defined(MAIA_NATIVE_PROBE_PSRAM_PROFILE) && !defined(MAIA_NATIVE_PROBE_PSRAM_BOOT)
    fb_profile(0);
#endif
    maia_psram_mode_switch_report_t mode;
    if(maia_esp32p4_psram_x8_mode(0,10000,&mode)!=MAIA_PSRAM_OK||!fb_raw(0,fb_saved,0))halt("fb-mode");
    maia_psram_fast_report_t fast;result=maia_esp32p4_psram_fast_start(200,10000,&fast);
    field("DPI_FB_FAST result=",(uint32_t)result);field(" rate=",fast.mhz);field(" mpll=",fast.mpll.mhz);field(" div=",fast.mpll.div);field(" dll0=",rd(PSRAM_MSPI0+0x180U));field(" dll1=",rd(PSRAM_MSPI0+0x190U));maia_console_puts("\n");
    if(result!=MAIA_PSRAM_OK)halt("fb-fast");
    maia_psram_window_report_t window;result=maia_esp32p4_psram_window_start(10000,&window);
    if(result!=MAIA_PSRAM_OK)halt("fb-window");
    volatile uint8_t *memory=(volatile uint8_t *)(uintptr_t)window.vaddr;
    for(unsigned i=0;i<MAIA_PSRAM_WINDOW_BYTES;++i)memory[i]=0xA5;
    volatile uint16_t *pixels=(volatile uint16_t *)(uintptr_t)DSI_FB_SOURCE;
    for(unsigned y=0;y<1280U;++y)for(unsigned x=0;x<720U;++x)pixels[y*720U+x]=fb_native_pixel(x,y);
    if(maia_esp32p4_psram_window_sync(10000,&window)!=MAIA_PSRAM_OK)halt("fb-sync");
    uint32_t hash=fb_hash(memory+128U,FB_BYTES);
    field("DPI_FB_IMAGE source=",DSI_FB_SOURCE);field(" bytes=",FB_BYTES);field(" hash=",hash);field(" native_width=",720);field(" native_height=",1280);field(" rotation=",90);field(" frames=",frames);field(" visual=",visual);maia_console_puts("\n");
    if(psram_dma(0)!=1)halt("fb-memory-dma");
    dpi_backlight_snapshot_t light={0};
    if(visual&&!dpi_bl_on(rd,wr,&light))halt("fb-light");
#ifdef MAIA_NATIVE_PROBE_LCD_STREAM
    lcd_stream_launch(visual,hash,&before,&light);
#endif
    maia_dsi_framebuffer_report_t r;uint64_t began=maia_soc_time_us();
    maia_dsi_access_result_t transferred=maia_dsi_framebuffer_probe(frames,100000,&r);
    uint32_t elapsed=(uint32_t)(maia_soc_time_us()-began);
    field("DPI_FB_TRANSFER result=",(uint32_t)transferred);field(" stage=",r.stage.completed);field(" frames=",r.refresh.frames_completed);field(" items=",r.refresh.items);field(" rearms=",r.refresh.rearm_writes);field(" polls=",r.refresh.polls);field(" raw=",r.refresh.last.count_raw);field(" done=",r.refresh.last.done);field(" recovered=",r.refresh.last.recovered);field(" released=",r.source_released);field(" status=",r.refresh.last.channel_status);field(" common=",r.refresh.last.common_status);field(" bridge=",r.refresh.last.bridge_status);field(" error0=",r.refresh.last.observation.error0);field(" error1=",r.refresh.last.observation.error1);field(" us=",elapsed);maia_console_puts("\n");
    if(!r.source_released)halt("fb-owned-or-uncertain");
    if(visual&&!dpi_bl_restore(rd,wr,&light))halt("fb-light-restore");
    if(transferred!=MAIA_DSI_ACCESS_OK)halt("fb-transfer-result");
    fb_finish(hash,&before);
    maia_console_puts("MAIA_NATIVE_DPI_FRAMEBUFFER_PASS\n");
}

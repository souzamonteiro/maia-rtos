/* Original MaiaRTOS MIT bounded framebuffer policy. Facts: pinned SDKf5c3654
 * DPI init ordering/host+bridge LL, DW-GDMA LL and chip1.3 TRM5.5.7/5.7.4.
 * docs/audits/native-dpi-dma-frame.md. No foreign runtime or ISR imported. */
#define FRAME_ERRORS 0x023F7FE0U
#define FRAME_CHSTAT_REQUIRED (FRAME_ERRORS|3U)
#define FRAME_COMMON_REQUIRED 0x001FFF8FU
static int frame_roots(void){
    uint32_t allowed_cfg=1U;
#ifdef MAIA_DSI_LCD_STREAM
    if(lcd_stream_mode)allowed_cfg=3U;
#endif
#ifdef MAIA_DSI_FRAMEBUFFER_PROBE
    if(framebuffer_claim&&!maia_psram_window_dma_live())return 0;
#endif
    if(!transfer_bus_guard(&dpi_staged_guard)||!supply_same(&dpi_staged_guard.supply))return 0;
    return (dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U)&1U)==1U &&
        !(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&0xEU) && !dsi_read(MAIA_ESP32P4_GDMA_BASE+0x1CU) &&
        !(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U) && !(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)&~allowed_cfg);
}
#ifdef MAIA_DSI_SPATIAL_PROBE
static int spatial_live(unsigned i){
    uint32_t v=dsi_read(dma_stage_address(i));
    if(i==0U)return v>=dma_staged_source&&v<=dma_staged_source+DSI_SPATIAL_SOURCE_BYTES;
    if(i==5U)return (v&dma_stage_masks[i])==0x001C1B41U||(v&dma_stage_masks[i])==0x001C1B51U;
    if(i==6U)return (v&0x07FFFFFFU)==0x000F8040U;
    if(i==9U){
        if(v<=1U)return 1;
        for(unsigned n=0;n<5U;++n)if(v==((dma_staged_list+n*64U)|1U))return 1;
        return 0;
    }
    return ((v^dma_staged_words[i])&dma_stage_masks[i])==0;
}
#endif
static int frame_guard(uint32_t mode,uint32_t lp,uint32_t dpi,uint32_t cfg){
    if(!frame_roots()||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)!=cfg)return 0;
    if(dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U)!=dpi_staged_phy[0] ||
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U)!=dpi_staged_phy[1] ||
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA4U)!=dpi_staged_phy[2] ||
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)!=dpi ||
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x68U)!=dpi_staged_mode ||
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x100U) || dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x50U))return 0;
    for(unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i){
        uint32_t expected=command_offsets[i]==0x34U?mode:command_offsets[i]==0x94U?lp:saved_command_table[i];
        if(dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+command_offsets[i])!=expected)return 0;
    }
    for(unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i)if(dsi_read(dpi_address(i))!=dpi_staged_words[i])return 0;
    for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i){
        if(i==4U)continue; /* live block-progress observation is separate */
#ifdef MAIA_DSI_FRAMEBUFFER_PROBE
        if(framebuffer_claim&&i==0U){
            uint32_t v=dsi_read(dma_stage_address(i));
            if((v&7U)||v<DSI_FB_SOURCE||v>DSI_FB_BACK_SOURCE+DSI_FB_BYTES)return 0;
            continue;
        }
#endif
#ifdef MAIA_DSI_SPATIAL_PROBE
        if(dma_staged_list){if(!spatial_live(i))return 0;continue;}
#endif
        if((dsi_read(dma_stage_address(i))^dma_staged_words[i])&dma_stage_masks[i])return 0;
    }
    for(unsigned c=1;c<4U;++c)if(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U+c*0x100U)&0x7FFFU)return 0;
    return (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x180U)&FRAME_CHSTAT_REQUIRED)==FRAME_CHSTAT_REQUIRED &&
        (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x40U)&FRAME_COMMON_REQUIRED)==FRAME_COMMON_REQUIRED;
}
static maia_dsi_access_result_t frame_observe(maia_dsi_dma_frame_report_t *r){
    if(!transfer_observe(&r->observation))return MAIA_DSI_ACCESS_IO;
    r->channel_status|=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
    r->common_status|=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x50U);
    r->bridge_status|=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x58U);
    r->count_raw=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x130U)&0x3FFFFFU;
    r->dma_fifo=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U)&0x7FFFU;
    r->bridge_fifo=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU;
    if((r->channel_status&FRAME_ERRORS)||r->common_status||(r->bridge_status&1U))return MAIA_DSI_ACCESS_IO;
    return (r->observation.status&1U)?MAIA_DSI_ACCESS_OK:MAIA_DSI_ACCESS_VERIFY;
}
static int frame_store(maia_dsi_dma_frame_report_t *r,unsigned bit,uintptr_t address,uint32_t desired){
    if(!frame_roots())return 0;
    r->attempted_mask|=1U<<bit;dsi_write(address,desired);
    return frame_roots()&&dsi_read(address)==desired;
}
static int frame_restore(maia_dsi_dma_frame_report_t *r,unsigned bit,uintptr_t address,uint32_t mask,uint32_t original){
    if(!frame_roots() || ((dsi_read(address)^original)&~mask))return 0;
    return frame_store(r,bit,address,original);
}
static maia_dsi_access_result_t frame_cleanup(uint32_t budget,maia_dsi_dma_frame_report_t *r){
    if(!frame_roots())return MAIA_DSI_ACCESS_VERIFY;
    r->attempted_mask|=1U<<6;dsi_write(MAIA_ESP32P4_GDMA_BASE+0x18U,0x100U);
    if(!frame_store(r,7,MAIA_ESP32P4_GDMA_BASE+0x10U,0))return MAIA_DSI_ACCESS_VERIFY;
    unsigned idle=0;
    for(unsigned i=0;i<budget;++i){
        ++r->recovery_polls;
        if(!frame_roots())return MAIA_DSI_ACCESS_VERIFY;
        r->dma_fifo=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U)&0x7FFFU;
        if(!dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U) && !(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&15U) && !r->dma_fifo){idle=1;break;}
        r->dma_fifo=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U)&0x7FFFU;
    }
    if(!idle)return MAIA_DSI_ACCESS_TIMEOUT;
    if(!frame_restore(r,8,MAIA_ESP32P4_DSI_BRG_BASE+0x40U,1U,dpi_staged_phy[4]))return MAIA_DSI_ACCESS_VERIFY;
    r->attempted_mask|=1U<<9;dsi_write(MAIA_ESP32P4_DSI_BRG_BASE+0x44U,1U); /* WT, no ACK */
    if(!frame_restore(r,10,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,1U,saved_command_table[0]) ||
        !frame_restore(r,11,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,3U,dpi_staged_phy[3]))return MAIA_DSI_ACCESS_VERIFY;
    idle=0;
    for(unsigned i=0;i<budget;++i){
        ++r->recovery_polls;
        if(!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0))return MAIA_DSI_ACCESS_VERIFY;
        /* Observe errors without abandoning necessary recovery. */
        (void)frame_observe(r);
        if(read_idle(&r->observation) && !r->dma_fifo && !r->bridge_fifo){idle=1;break;}
    }
    if(!idle)return MAIA_DSI_ACCESS_TIMEOUT;
    r->recovered=1;return MAIA_DSI_ACCESS_OK;
}
static maia_dsi_access_result_t frame_run(uint32_t budget,maia_dsi_dma_frame_report_t *r){
    if(!r)return MAIA_DSI_ACCESS_INVALID;
    volatile unsigned char *zero=(volatile unsigned char *)r;for(unsigned i=0;i<sizeof *r;++i)zero[i]=0;
    if(!budget||budget>100000U)return MAIA_DSI_ACCESS_INVALID;
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(dpi_frame_state==FAULTED||transfer_state==FAULTED||dpi_dma_state==FAULTED)return MAIA_DSI_ACCESS_FAILED;
    if(dpi_frame_state!=UNCLAIMED||dpi_dma_state!=OWNED||transfer_state!=UNCLAIMED)return MAIA_DSI_ACCESS_BUSY;
    if(dma_staged_words[0]!=dma_staged_source)return MAIA_DSI_ACCESS_VERIFY;
    uint32_t terminal_raw=dma_staged_words[4]&0x3FFFFFU;
#ifdef MAIA_DSI_SPATIAL_PROBE
    if(dma_staged_list){if(!spatial_memory(0))return MAIA_DSI_ACCESS_VERIFY;terminal_raw=57599U;}
#endif
    uint32_t mode=saved_command_table[0],lp=dpi_staged_phy[3],dpi=dpi_staged_phy[4],cfg=0;
    maia_dsi_dma_stage_report_t stage;
    for(unsigned i=0;i<sizeof stage;++i)((volatile unsigned char *)&stage)[i]=0;
    if(!dpi_guard(&dpi_staged_guard,dpi_staged_phy,dpi_staged_mode,dpi_staged_words))return MAIA_DSI_ACCESS_VERIFY;
    maia_dsi_access_result_t result=dma_stage_check(dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U),dma_staged_words,&stage);
    if(result!=MAIA_DSI_ACCESS_OK){if(result==MAIA_DSI_ACCESS_IO)dpi_frame_state=transfer_state=FAULTED;return result;}
    if(!frame_guard(mode,lp,dpi,cfg))return MAIA_DSI_ACCESS_VERIFY;
    result=frame_observe(r);if(result!=MAIA_DSI_ACCESS_OK){dpi_frame_state=transfer_state=FAULTED;return result;}
    r->completed=1;dpi_frame_state=FAULTED;transfer_state=OWNED;
    if(!frame_store(r,0,MAIA_ESP32P4_GDMA_BASE+0x10U,1U)){result=MAIA_DSI_ACCESS_VERIFY;goto recover;}
    cfg=1;r->completed=2;
    if(!frame_guard(mode,lp,dpi,cfg)){result=MAIA_DSI_ACCESS_VERIFY;goto recover;}
    r->attempted_mask|=2U;dsi_write(MAIA_ESP32P4_GDMA_BASE+0x18U,0x101U);
    if(!frame_roots()||!(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U)){result=MAIA_DSI_ACCESS_VERIFY;goto recover;}
    r->saw_active=1;r->completed=3;
    if(!frame_guard(mode,lp,dpi,cfg)||!frame_store(r,2,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,mode&~1U)){result=MAIA_DSI_ACCESS_VERIFY;goto recover;}
    mode&=~1U;r->completed=4;
    if(!frame_guard(mode,lp,dpi,cfg)||!frame_store(r,3,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,lp|3U)){result=MAIA_DSI_ACCESS_VERIFY;goto recover;}
    lp|=3U;r->completed=5;
    if(!frame_guard(mode,lp,dpi,cfg)||!frame_store(r,4,MAIA_ESP32P4_DSI_BRG_BASE+0x40U,dpi|1U)){result=MAIA_DSI_ACCESS_VERIFY;goto recover;}
    dpi|=1U;r->completed=6;
    r->attempted_mask|=1U<<5;dsi_write(MAIA_ESP32P4_DSI_BRG_BASE+0x44U,1U);r->completed=7;
    result=MAIA_DSI_ACCESS_TIMEOUT;
    for(unsigned i=0;i<budget;++i){
        ++r->polls;
        if(!frame_guard(mode,lp,dpi,cfg)){result=MAIA_DSI_ACCESS_VERIFY;break;}
        result=frame_observe(r);if(result!=MAIA_DSI_ACCESS_OK)break;
        uint32_t channels=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U);
        if(!(channels&1U)){
            /* Completion can race the preceding sequential status/count reads. */
            result=frame_observe(r);if(result!=MAIA_DSI_ACCESS_OK)break;
            if(r->count_raw>terminal_raw){result=MAIA_DSI_ACCESS_VERIFY;break;}
            unsigned completion_flag=!!(r->channel_status&2U);
#ifdef MAIA_DSI_SPATIAL_PROBE
            /* Qualified SRAM LLIs publish done in descriptor writeback while
             * channel CPU status can be0. Require all five exact done records;
             * standalone contiguous frame still requires its channel DONE bit. */
            if(dma_staged_list)completion_flag=spatial_memory(1);
#endif
            if(r->count_raw==terminal_raw && completion_flag && !r->dma_fifo&&!r->bridge_fifo){
                r->items=(dma_staged_words[4]&0x3FFFFFU)+1U;r->done=1;r->completed=8;break;}
        }
        result=MAIA_DSI_ACCESS_TIMEOUT;
    }
recover:
    r->transfer_result=(int32_t)result;
    maia_dsi_access_result_t restored=frame_cleanup(budget,r);r->recovery_result=(int32_t)restored;
    if(restored!=MAIA_DSI_ACCESS_OK){transfer_state=FAULTED;return restored;}
    if(result!=MAIA_DSI_ACCESS_OK || !r->done || (r->channel_status&FRAME_ERRORS)||r->common_status ||(r->bridge_status&1U)||
        r->observation.error0&0x1FFFFFU ||r->observation.error1&0x81FFFU){transfer_state=FAULTED;return result==MAIA_DSI_ACCESS_OK?MAIA_DSI_ACCESS_IO:result;}
    r->completed=9;dpi_frame_state=3U;transfer_state=UNCLAIMED;return MAIA_DSI_ACCESS_OK;
}

maia_dsi_access_result_t maia_esp32p4_dsi_dma_solid_frame(uint32_t budget,maia_dsi_dma_frame_report_t *r){
#ifdef MAIA_DSI_SPATIAL_PROBE
    if(dma_staged_list){
        if(!r)return MAIA_DSI_ACCESS_INVALID;
        for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
        return MAIA_DSI_ACCESS_BUSY;
    }
#endif
    return frame_run(budget,r);
}
maia_dsi_access_result_t maia_esp32p4_dsi_dma_solid_refresh(uint32_t frames,uint32_t budget,maia_dsi_dma_refresh_report_t *r){
    if(!r)return MAIA_DSI_ACCESS_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!frames||frames>400U||!budget||budget>100000U)return MAIA_DSI_ACCESS_INVALID;
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(dpi_refresh_state==FAULTED||dpi_frame_state==FAULTED||transfer_state==FAULTED)return MAIA_DSI_ACCESS_FAILED;
    if(dpi_refresh_state!=UNCLAIMED||dpi_frame_state!=UNCLAIMED||dpi_dma_state!=OWNED||transfer_state!=UNCLAIMED)return MAIA_DSI_ACCESS_BUSY;
    dpi_refresh_state=FAULTED;
    for(unsigned n=0;n<frames;++n){
        if(n){
            if(dpi_frame_state!=3U || !frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0) ||
                !dma_quiet() || (dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU))goto refused;
            uint32_t pending=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
            if(pending&~0x4000001BU)goto refused;
            ++r->rearm_writes;dsi_write(MAIA_ESP32P4_GDMA_BASE+0x198U,0x4000001BU);
            if(!frame_roots() || dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U) ||
                dsi_read(MAIA_ESP32P4_GDMA_BASE+0x50U) || (dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x58U)&1U))goto refused;
            dpi_frame_state=UNCLAIMED; /* only this checked sequence may rearm */
        }
        maia_dsi_access_result_t result=maia_esp32p4_dsi_dma_solid_frame(budget,&r->last);
        r->polls+=r->last.polls;
        if(result!=MAIA_DSI_ACCESS_OK){dpi_frame_state=transfer_state=FAULTED;return result;}
        ++r->frames_completed;r->items+=r->last.items;
    }
    dpi_refresh_state=OWNED;return MAIA_DSI_ACCESS_OK;
refused:
    dpi_frame_state=transfer_state=FAULTED;return MAIA_DSI_ACCESS_VERIFY;
}

#ifdef MAIA_DSI_SPATIAL_PROBE
/* Private diagnostic, same exclusive lifetime as native boot probes. No callers
 * can rearm a consumed claim; only exact completed writable LLIs are rebuilt. */
maia_dsi_access_result_t maia_dsi_spatial_probe(uint32_t source,uint32_t list,uint32_t frames,uint32_t budget,maia_dsi_spatial_probe_report_t *r){
    if(!r)return MAIA_DSI_ACCESS_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!dsi_spatial_layout(source,list)||!frames||frames>400U||!budget||budget>100000U)return MAIA_DSI_ACCESS_INVALID;
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(dpi_refresh_state==FAULTED||dpi_frame_state==FAULTED||transfer_state==FAULTED)return MAIA_DSI_ACCESS_FAILED;
    if(dpi_refresh_state!=UNCLAIMED||dpi_frame_state!=UNCLAIMED)return MAIA_DSI_ACCESS_BUSY;
    maia_dsi_access_result_t result=dma_stage_body(source,list,&r->stage);
    if(result!=MAIA_DSI_ACCESS_OK)return result;
    dpi_refresh_state=FAULTED;
    for(unsigned n=0;n<frames;++n){
        if(n){
            if(dpi_frame_state!=3U||!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0)||!dma_quiet()||
                (dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU)||!spatial_memory(1))goto refused_spatial;
            uint32_t status=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
            if(status&~0x4000001BU)goto refused_spatial;
            ++r->refresh.rearm_writes;dsi_write(MAIA_ESP32P4_GDMA_BASE+0x198U,0x4000001BU);
            if(!frame_roots()||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U)||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x50U)||
                (dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x58U)&1U))goto refused_spatial;
            for(unsigned node=0;node<5U;++node)for(unsigned w=0;w<16U;++w)
                dsi_write((uintptr_t)list+0x40000000U+node*64U+w*4U,dsi_spatial_word(source,list,node,w));
            if(!frame_roots()||!dma_quiet()||!spatial_memory(0))goto refused_spatial;
            /* Rebuild fetched channel registers only while verified idle. */
            for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i){
                if(!frame_roots()||!dma_quiet())goto refused_spatial;
                dsi_write(dma_stage_address(i),dma_staged_words[i]);
                if(dsi_read(dma_stage_address(i))!=dma_staged_words[i])goto refused_spatial;
            }
            dpi_frame_state=UNCLAIMED;
        }
        result=frame_run(budget,&r->refresh.last);r->refresh.polls+=r->refresh.last.polls;
        if(result!=MAIA_DSI_ACCESS_OK){dpi_frame_state=transfer_state=FAULTED;return result;}
        ++r->refresh.frames_completed;r->refresh.items+=r->refresh.last.items;
    }
    dpi_refresh_state=OWNED;return MAIA_DSI_ACCESS_OK;
refused_spatial:
    dpi_frame_state=transfer_state=FAULTED;return MAIA_DSI_ACCESS_VERIFY;
}
#endif

#ifdef MAIA_DSI_FRAMEBUFFER_PROBE
maia_dsi_access_result_t maia_dsi_framebuffer_probe(uint32_t frames,uint32_t budget,maia_dsi_framebuffer_report_t *r){
    if(!r)return MAIA_DSI_ACCESS_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!frames||frames>1200U||!budget||budget>100000U)return MAIA_DSI_ACCESS_INVALID;
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(dpi_refresh_state==FAULTED||dpi_frame_state==FAULTED||dpi_dma_state==FAULTED||transfer_state==FAULTED)return MAIA_DSI_ACCESS_FAILED;
    if(framebuffer_claim||dpi_refresh_state!=UNCLAIMED||dpi_frame_state!=UNCLAIMED||dpi_dma_state!=UNCLAIMED||transfer_state!=UNCLAIMED)return MAIA_DSI_ACCESS_BUSY;
    uint32_t source=maia_psram_window_dma_acquire();if(source!=DSI_FB_SOURCE)return MAIA_DSI_ACCESS_BUSY;
    framebuffer_claim=1;
    maia_dsi_access_result_t result=dma_stage_body(source,0,&r->stage);
    if(result!=MAIA_DSI_ACCESS_OK)return result; /* retain uncertain ownership */
    dpi_refresh_state=FAULTED;
    for(unsigned n=0;n<frames;++n){
        if(n){
            if(dpi_frame_state!=3U||!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0)||!dma_quiet())goto refused_fb;
            uint32_t pending=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
            if(pending&~0x4000001BU)goto refused_fb;
            ++r->refresh.rearm_writes;dsi_write(MAIA_ESP32P4_GDMA_BASE+0x198U,0x4000001BU);
            if(!frame_roots()||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U)||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x50U))goto refused_fb;
            /* Only incrementing SAR needs rewind; no arbitrary live rewrites. */
            dsi_write(dma_stage_address(0),source);
            if(!frame_roots()||!dma_quiet()||dsi_read(dma_stage_address(0))!=source)goto refused_fb;
            dpi_frame_state=UNCLAIMED;
        }
        result=frame_run(budget,&r->refresh.last);r->refresh.polls+=r->refresh.last.polls;
        if(result!=MAIA_DSI_ACCESS_OK)return result;
        ++r->refresh.frames_completed;r->refresh.items+=r->refresh.last.items;
    }
    if(!dma_quiet()||!maia_psram_window_dma_release())goto refused_fb;
    r->source_released=1;framebuffer_claim=0;dpi_refresh_state=OWNED;return MAIA_DSI_ACCESS_OK;
refused_fb:
    dpi_frame_state=transfer_state=FAULTED;return MAIA_DSI_ACCESS_VERIFY;
}
#endif

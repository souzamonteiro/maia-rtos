/* Execution adapter for the generated Pipeline/SimpleCommand tree. */
#include "../wc/wc-options.hpp"
static void pipelinePut(unsigned int at,unsigned int value)
{
    for(unsigned int i=0;i<4;++i)gPipelinePlan[at+i]=static_cast<uint8_t>(value>>(i*8));
}
static void pipelineError(const char* text) {if (!gCommandStatus) gCommandStatus=2;writeText(2,text);completeCommand();}
static void beginPipeline(unsigned int count)
{
    gPipelineOutput=gRedirectFlags?1:0;gPipelineDrained=0;
    gPipelineInput=0;gPipelineFed=0;gPipelineFeedCount=0;gPipelineFeedOffset=0;
    for(unsigned int i=0;i<gRedirectionCount;++i)if(gRedirectionFlags[i]==1)gPipelineInput=1;
    count-=gRedirectionCount;
    unsigned int first[4],sizes[4];
    for(unsigned int i=0;i<4;++i){first[i]=0;sizes[i]=0;}
    for(unsigned int i=0;i<count;++i){unsigned int stage=gExpandedStage[i];if(stage>=gPipelineStageCount){pipelineError("shell: invalid pipeline\n");return;}
        if(!sizes[stage])first[stage]=i;++sizes[stage];}
    for(unsigned int s=0;s<gPipelineStageCount;++s){
        if(!sizes[s]){pipelineError("shell: unsupported pipeline command\n");return;}
        const char* command=gCommandWords+first[s]*kMaxWordBytes;
        int cat=textEquals(command,"cat");
        int wc=textEquals(command,"wc") && s+1==gPipelineStageCount;
        if((!s && (gPipelineInput?sizes[s]!=1:sizes[s]<2)) || (!cat && !wc)) {pipelineError("shell: unsupported pipeline command\n");return;}
        for(unsigned int j=1;j<sizes[s];++j){char* word=gCommandWords+(first[s]+j)*kMaxWordBytes;
            if(wc){
                if(!maiaWcOption(reinterpret_cast<const uint8_t*>(word),textLength(word))){pipelineError("wc: expected -l, -w or -c options only\n");return;}
                continue;
            }
            if(word[0]=='-'){pipelineError("shell: unsupported pipeline command\n");return;}
            if(!resolvePath(word) || textLength(gResolvedPath)>=kMaxWordBytes){pipelineError("shell: pipeline path limit exceeded\n");return;}
            for(unsigned int k=0;k<=textLength(gResolvedPath);++k)word[k]=gResolvedPath[k];
        }
    }
    unsigned int base=(gPipelineInput || gPipelineOutput)?24:16;
    unsigned int start=base+gPipelineStageCount*8+count*8,position=start;
    pipelinePut(0,gPipelineInput?3:gPipelineOutput?2:1);
    if(gPipelineInput || gPipelineOutput){pipelinePut(16,(gPipelineInput?2:0)|(gPipelineOutput?1:0));pipelinePut(20,0);}
    pipelinePut(4,gPipelineStageCount);pipelinePut(8,count);
    for(unsigned int s=0;s<gPipelineStageCount;++s){pipelinePut(base+s*8,first[s]);pipelinePut(base+4+s*8,sizes[s]);}
    for(unsigned int i=0;i<count;++i){const char* word=gCommandWords+i*kMaxWordBytes;unsigned int length=textLength(word);
        pipelinePut(base+gPipelineStageCount*8+i*8,position);pipelinePut(base+4+gPipelineStageCount*8+i*8,length);
        for(unsigned int j=0;j<length;++j)gPipelinePlan[position++]=word[j];}
    pipelinePut(12,position-start);
    if(position>1536){pipelineError("shell: pipeline plan limit exceeded\n");return;}
    // Do not consume or close an unrelated owner endpoint on a failed bind.
    if(gPipelineOutput && maia_stream_read(3,static_cast<int32_t>(reinterpret_cast<uintptr_t>(gReadBuffer)),0)!=-7){
        gCommandStatus=1;pipelineError("shell: pipeline output busy\n");return;
    }
    if(gPipelineInput && maia_stream_write(4,static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPipelineFeed)),0)!=-7){
        gCommandStatus=1;pipelineError("shell: pipeline input busy\n");return;
    }
    for(unsigned int i=0;i<gRedirectionCount;++i){
        const char* path=gCommandWords+gRedirectionExpanded[i]*kMaxWordBytes;
        const int opened=gRedirectionFlags[i]==1?beginCommandInput(path):beginCommandOutput(path);
        if(!opened){completeCommand();return;}
    }
    int group=maia_pipeline_start(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPipelinePlan)),position);
    if(group<=0){gCommandStatus=1;pipelineError("shell: pipeline launch failed\n");return;}gPipelineGroup=group;
}
static void stopPipelineInput(void)
{
    if(gInputRedirectHandle>=0){if(maia_vfs_close(gInputRedirectHandle)<0)gCommandStatus=1;gInputRedirectHandle=-1;}
    if(gPipelineInput && !gPipelineFed)maia_stream_close(4);
    gPipelineFed=1;gPipelineFeedCount=0;gPipelineFeedOffset=0;
}
static void failPipelineInput(const char* message)
{
    gCommandStatus=1;writeText(2,message);stopPipelineInput();
    if(gPipelineOutput && !gPipelineDrained){maia_stream_close(3);gPipelineDrained=1;}
    maia_pipeline_cancel(gPipelineGroup);
}
static void resumePipeline(void)
{
    // Attempt both directions each turn: blocked input must not starve output.
    if(gPipelineInput && !gPipelineFed){
        if(gPipelineFeedOffset==gPipelineFeedCount){
            const int bytes=maia_vfs_read(gInputRedirectHandle,
                static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPipelineFeed)),128);
            if(bytes>0 && bytes<=128){gPipelineFeedOffset=0;gPipelineFeedCount=bytes;}
            else if(bytes==0){
                const int closed=maia_vfs_close(gInputRedirectHandle);gInputRedirectHandle=-1;
                const int endpoint=maia_stream_close(4);gPipelineFed=1;
                if(closed<0 || endpoint<0)failPipelineInput("shell: pipeline input close failed\n");
            }else if(bytes!=-6)failPipelineInput("shell: pipeline input read failed\n");
        }
        if(!gPipelineFed && gPipelineFeedOffset<gPipelineFeedCount){
            const int bytes=maia_stream_write(4,
                static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPipelineFeed+gPipelineFeedOffset)),
                gPipelineFeedCount-gPipelineFeedOffset);
            if(bytes>0 && static_cast<unsigned int>(bytes)<=gPipelineFeedCount-gPipelineFeedOffset)gPipelineFeedOffset+=bytes;
            else if(bytes!=-6)failPipelineInput("shell: pipeline input write failed\n");
        }
    }
    if(gPipelineOutput && !gPipelineDrained){
        int bytes=maia_stream_read(3,static_cast<int32_t>(reinterpret_cast<uintptr_t>(gReadBuffer)),kReadBufferBytes);
        if(bytes>0 && bytes<=static_cast<int>(kReadBufferBytes)){writeBytes(1,reinterpret_cast<const char*>(gReadBuffer),bytes);return;}
        if(bytes!=-6){
            int closed=maia_stream_close(3);gPipelineDrained=1;
            if(bytes!=0 || closed<0){
                gCommandStatus=1;
                if(bytes!=-7)writeText(2,"shell: pipeline output read failed\n");
                stopPipelineInput();maia_pipeline_cancel(gPipelineGroup);
            }
        }
    }
    // Input-only groups may roll back while a VFS read stays blocked. Poll the
    // nonblocking wait to observe that completion; never discard undrained output.
    if(gPipelineOutput && !gPipelineDrained)return;
    int count=maia_pipeline_wait(gPipelineGroup,static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPipelineResult)),48);
    if(count==-6)return;
    if(count<32 || count>48 || directoryUint32(gPipelineResult)!=1 || directoryUint32(gPipelineResult+4)!=static_cast<unsigned int>(gPipelineGroup)
        || directoryUint32(gPipelineResult+8)!=gPipelineStageCount || 16+gPipelineStageCount*8!=static_cast<unsigned int>(count)){
        gFatal=1;return;
    }
    if(gPipelineInput && !gPipelineFed){
        gCommandStatus=1;writeText(2,"shell: pipeline input ended early\n");stopPipelineInput();
    }
    gPipelineGroup=0;gPipelineOutput=0;gPipelineDrained=0;gPipelineInput=0;gPipelineFed=0;
    if(directoryUint32(gPipelineResult+12)){
        if(!gCommandStatus)gCommandStatus=directoryUint32(gPipelineResult+12);
        writeText(2,"shell: pipeline failed\n");
    }
    completeCommand();
}

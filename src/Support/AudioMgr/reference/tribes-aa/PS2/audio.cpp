#include "entropy.hpp"
#include "audio.hpp"
#include "ps2/iop/iop.hpp"
#include "audiodefs.hpp"
#include "audiovars.hpp"
#include "x_threads.hpp"

#if defined(X_DEBUG) && defined(bwatson)
//#define AUDIO_DEBUG
#endif

audio_vars *g_Audio=NULL;
s32 DebugDisableAudio = 0;

// Used during the channel update of audio update
static void WriteCmdBuff(void *pData,s32 Length)
{
    u8 *pBytes;

    ASSERT( &g_Audio->m_pCommand[Length] < &g_Audio->m_CommandBuffer[PACKAGE_OUT_BUFFER_SIZE] );
    pBytes = (u8 *)pData;
    while (Length)
    {
        *g_Audio->m_pCommand++ = *pBytes++;
        Length--;
    }

}

// Used during the status readback phase of the audio update
static void ReadCmdBuff(void *pData,s32 Length)
{
    u8 *pBytes;
    pBytes = (u8 *)pData;
    while (Length)
    {
        *pBytes++=*g_Audio->m_pCommand++;
        Length--;
    }
    g_Audio->m_pCommand+=Length;
}

static void InitCmdBuff(void)
{
    g_Audio->m_pCommand = g_Audio->m_CommandBuffer;
}

static void WriteCmd(s8 cmd)
{
    WriteCmdBuff(&cmd,1);
}

s32 CmdBuffLength(void)
{
    return g_Audio->m_pCommand - g_Audio->m_CommandBuffer;
}


void    ps2audio_Init(void)
{
    audio_init AudioInit;

    AudioInit.m_nChannels = AUD_MAX_CHANNELS;
    iop_SendSyncRequest(AUDCMD_INIT,&AudioInit,sizeof(audio_init),NULL,0);
}

void    ps2audio_Kill(void)
{
    ASSERT(g_Audio);
    iop_SendSyncRequest(AUDCMD_KILL);
}

s32 audio_LoadContainer(char *pFilename)
{
    container_reply Reply;
    char        filename[64];
    audio_container *pContainer;
    container_hdr_request HeaderRequest;
    u32 count,remaining,index;

    ASSERT(g_Audio);
#if defined(TARGET_PS2_DVD) || defined(TARGET_PS2_CLIENT)
    x_sprintf(filename,"cdrom:\\TRIBES\\%s;1",pFilename);
    x_strtoupper(&filename[5]);
#else
#if 1
    x_sprintf(filename,"host1:%s",pFilename);
#else
    x_sprintf(filename,"cdrom:\\TRIBES\\%s;1",pFilename);
    x_strtoupper(&filename[5]);
#endif
#endif

    // The container load request is now asynchronous on the IOP side so we make sure
    // we poll it to see if it completed ok. The Reply.Status will be 1 until the
    // load has finished.
    iop_SendSyncRequest(AUDCMD_LOADCONTAINER,filename,x_strlen(filename)+1,&Reply,sizeof(Reply));
    while(1)
    {
        iop_SendSyncRequest(AUDCMD_ISLOADCOMPLETE,NULL,0,&Reply,sizeof(Reply));
        if (Reply.Status != 1)
            break;
        x_DelayThread(32);
    }

    if (Reply.Status == 0)
    {
        pContainer = (audio_container *)x_malloc(sizeof(audio_container)+sizeof(cfx_info)*Reply.nCfxCount);
        ASSERT(pContainer);
        pContainer->Id          = Reply.Id;
        pContainer->SampleCount = Reply.nCfxCount;
        pContainer->Volume      = 1.0f;
        remaining               = pContainer->SampleCount;
        index                   = 0;
        while (remaining)
        {
            count = remaining;
            if (count > 1024/sizeof(cfx_info))
            {
                count = 1024/sizeof(cfx_info);
            }
            HeaderRequest.Id    = pContainer->Id;
            HeaderRequest.Count = count;
            HeaderRequest.Index = index;
            iop_SendSyncRequest(AUDCMD_GETCONTAINERHEADER,&HeaderRequest,sizeof(HeaderRequest),
                                                          &pContainer->CfxInfo[index],sizeof(cfx_info)*count);
            remaining -= count;
            index+=count;
        }
        pContainer->pNext = g_Audio->m_pContainerList;
        g_Audio->m_pContainerList = pContainer;
    }
    return Reply.Id;
}

void audio_UnloadContainer(s32 ContainerId)
{
    s32             Reply;
    audio_container *pThis,*pLast;

    ASSERT(g_Audio);
    pThis = g_Audio->m_pContainerList;
    pLast = NULL;
    while (pThis)
    {
        if (pThis->Id == ContainerId)
            break;
        pLast = pThis;
        pThis = pThis->pNext;
    }
    ASSERTS(pThis,"UnloadContainer: Container is already unloaded");
    if (pLast)
    {
        pLast->pNext = pThis->pNext;
    }
    else
    {
        g_Audio->m_pContainerList = pThis->pNext;
    }
    x_free(pThis);

    iop_SendSyncRequest(AUDCMD_UNLOADCONTAINER,&ContainerId,sizeof(s32),&Reply,sizeof(s32));
    ASSERTS(Reply,"UnloadContainer: Unload attempted when another container was above this one");
}

cfx_info    *audio_FindInfo(s32 SampleId)
{
    audio_container *pContainer;
    s32 containerid;

    pContainer = g_Audio->m_pContainerList;
    containerid = (SampleId & 0xff000000);
    SampleId = (SampleId & 0x00ffffff);

    while (pContainer)
    {
        if (pContainer->Id == containerid )
        {
            break;
        }
        pContainer = pContainer->pNext;
    }

    if (!pContainer)
        return NULL;

    ASSERTS(pContainer,"No container loaded holding that sample");
    ASSERTS(SampleId < pContainer->SampleCount,"Sample ID is too large for this container");

    return &pContainer->CfxInfo[SampleId];
}
//
// This performs an asynchronous update. This should be done in an audio management thread.
// All of the currently active audio channels are sent. It's up to the iop to make sure
// channels that are no longer active are removed.
//
// Audio updates are performed as so:
// UPDATESTART request is sent with global audio status information
// UPDATE requests are sent to make sure all the active channel info is sent
// UPDATEEND request is sent to show the end of an update cycle. Any channels
// that were not sent in the update cycle should be deleted as they are no
// longer required.
//
// The iop will send back an ID for each channel that has been requested an update
// but has already been stopped. This is used to release the channel but in the
// future, this will be used by the application specific audio management layer.
//
xbool	s_LowerVolume = FALSE;
f32		s_VolumeModifier = 1.0f;

void audio_Update(void)
{
    iop_request         *pRequest;
    audio_channel       *pChannel;
    s32                 i;
    s32                 nChannels;
    s32                 TotalChannels;
    audio_channel_list  *pChannelList;

#ifdef AUDIO_DEBUG
    static              s32 count=1;

    count--;
    if (count<0)
    {
        count = 120;
    }
#endif


    ASSERT(g_Audio);
    if (DebugDisableAudio)
        return;

    pRequest = (iop_request *)g_Audio->m_pqReadyAudioUpdate->Recv(MQ_BLOCK);
    // NOTE: On initial kick, the pRequest pointer will be NULL. This means
    // we did not send a request to the iop first. The iop handler will return
    // this message to which ever queue is passed in. Normally, it goes back to
    // the iop request available queue.
    //
    if (pRequest)
    {
        iop_FreeRequest(pRequest);
        InitCmdBuff();
        i=0;
        while (1)
        {
            u32 id;

            ReadCmdBuff(&id,sizeof(id));
            if (id==0)
                break;
			//x_DebugMsg("audio_Update: Sound id 0x%08x reported dead\n",id);
            audio_Free(id);
            if (id==g_Audio->m_DelayedChannelId)
            {
                g_Audio->m_DelayedChannelId=0;
            }
            i++;
        }
    }

    if (g_Audio->m_DelayedChannelId==0)
    {
        u32 id;

        id = (u32)g_Audio->m_pqDelayedSamples->Recv(MQ_NOBLOCK);
        if (id)
        {
            g_Audio->m_DelayedChannelId = audio_Play(id);
        }
    }
    InitCmdBuff();

    WriteCmd(AUD_COMMAND_UPDATE_MASTER);
    WriteCmdBuff(&g_Audio->m_GlobalStatus,sizeof(g_Audio->m_GlobalStatus));
    
    pChannelList = g_Audio->m_pChannelList;
    nChannels = 0;
    TotalChannels = 0;

	if (s_LowerVolume)
	{
		s_VolumeModifier = 0.5f;
	}
	else
	{
		s_VolumeModifier = 1.0f;
	}
	s_LowerVolume = FALSE;

    while (pChannelList)
    {
        pChannel = pChannelList->m_Channels;
        TotalChannels+=AUD_MAX_CHANNELS;
        for (i=0;i<AUD_MAX_CHANNELS;i++)
        {
            if (pChannel->m_Flags & AUDFLAG_ALLOCATED)
            {
				// If anything from package id #5 is playing, then we should lower the volume
				// of all other sounds by the s_VolumeModifier so they don't overrun the sound
				//
				// THIS IS A HACK JUST FOR TRIBES
				//
				if ( ( (pChannel->m_State.m_CfxId >> 24) & 0xff)==5)
				{
					s_LowerVolume = TRUE;
					audio_UpdateParams(pChannel,1.0f);
					s_VolumeModifier = 0.5f;
				}
				else
				{
					audio_UpdateParams(pChannel,s_VolumeModifier);
#if 0
					x_DebugMsg("audio_Update: Sound id 0x%08x, Volume=%d, Pan=%d\n",pChannel->m_State.m_Id,
																				pChannel->m_State.m_Volume,
																				pChannel->m_State.m_Pan);
#endif
				}
                // If we only want the position to be set once, we reset the 3D position
                // flag once it has been initially updated. That sound then becomes unmovable
                // with no volume/pan change when the players ears move.
                if (pChannel->m_Flags & AUDFLAG_LOCALIZED)
                {
					pChannel->m_Volume = ((f32)pChannel->m_State.m_Volume / (f32)AUD_FIXED_POINT_1);
                    pChannel->m_Flags &= ~ AUDFLAG_3D_POSITION;
                }
                nChannels++;
            }
            pChannel++;
        }
        pChannelList = pChannelList->m_pNext;
    }

    i=0;
    if (g_Audio->m_UpdateRequestIndex >= AUD_MAX_CHANNELS)
    {
        g_Audio->m_UpdateRequestIndex = AUD_MAX_CHANNELS;
    }
    while (i<g_Audio->m_UpdateRequestIndex)
    {
        pChannel = g_Audio->m_UpdateRequests[i];
#ifdef AUDIO_DEBUG
        if (count==0)
            x_DebugMsg("Sound playing, cfxid=%d, id=0x%x\n",pChannel->m_State.m_CfxId,pChannel->m_State.m_Id);

#endif
        if ( ((pChannel->m_Flags & AUDFLAG_PERSISTENT) == 0 ) &&
			 (pChannel->m_Flags & AUDFLAG_ALLOCATED) )
        {
            WriteCmd(AUD_COMMAND_UPDATE_CFX);
            WriteCmdBuff(&pChannel->m_State,sizeof(pChannel->m_State));
        }
        i++;
    }
    WriteCmd(AUD_COMMAND_END);
    g_Audio->m_UpdateRequestIndex=0;

#ifdef AUDIO_DEBUG
    if (count==0)
    {
        x_DebugMsg("Total available channels = %d\n",TotalChannels);
    }
#endif

    //
    // Now, send update information for all the channels. Note: this is asynchronous which means we will
    // not have status information available for the channels until the next cycle round
    //
    iop_SendAsyncRequest(AUDCMD_UPDATE,
                            g_Audio->m_CommandBuffer,CmdBuffLength(),
                            g_Audio->m_CommandBuffer,g_Audio->m_GlobalStatus.m_ChannelCount * sizeof(s32),
                            g_Audio->m_pqReadyAudioUpdate);

#if defined(bwatson) && defined(X_DEBUG) && defined(TARGET_PS2_DEVKIT)
    x_printfxy(1,20,"CHN:%d/%d",i,nChannels);
#endif
    g_Audio->m_ChannelsInUse = nChannels;

}

void    audio_SetContainerVolume(s32 ContainerId,f32 Volume)
{
    s32 Buffer[2];

    audio_container *pThis;

    ASSERT(g_Audio);
    pThis = g_Audio->m_pContainerList;
    while (pThis)
    {
        if (pThis->Id == ContainerId)
            break;
        pThis = pThis->pNext;
    }
    ASSERTS(pThis,"audio_SetContainerVolume: Container has not been loaded");

    pThis->Volume = Volume;
    Buffer[0]=ContainerId;
    Buffer[1]=(s32)(Volume * AUD_FIXED_POINT_1);
    iop_SendSyncRequest(AUDCMD_CONTAINERVOLUME,&Buffer,sizeof(Buffer),NULL,0);
  
}

f32 audio_GetContainerVolume (s32 ContainerId)
{
   audio_container *pThis;

    ASSERT(g_Audio);
    pThis = g_Audio->m_pContainerList;
    while (pThis)
    {
        if (pThis->Id == ContainerId)
            break;
        pThis = pThis->pNext;
    }
    ASSERTS(pThis,"audio_GetContainerVolume: Container has not been loaded");
    return pThis->Volume;

}
#include "entropy.hpp"
#include "audio.hpp"
#include "audiodefs.hpp"
#include "audiovars.hpp"
#include "driver/gcnaud_audio.hpp"
#include "driver/gcnaud_container.hpp"
#include <dolphin.h>

#if defined(X_DEBUG) && defined(bwatson)
#define AUDIO_DEBUG
#endif

audio_vars *g_Audio=NULL;
s32 DebugDisableAudio = 0;

static void WriteCmdBuff(void *pData,s32 Length);
static void ReadCmdBuff(void* pData,s32 Length);
static void InitCmdBuff(void);
static void WriteCmd(s8 cmd);

void    sys_audio_Init(void)
{
    gcnaudio_Init(NULL);
}

void    sys_audio_Kill(void)
{
    gcnaudio_Kill();
}

s32 audio_LoadContainer(char *pFilename)
{
    audio_container *pThis;
    s32 CfxCount;
    s32 ContainerId;

    ContainerId = gcncontainer_Load(pFilename,&CfxCount);
    ASSERT(ContainerId);
    pThis = g_Audio->m_pContainerList;
    while (pThis)
    {
        ASSERTS(pThis->Id != ContainerId,"Attempt to load a container that is already loaded.");
        pThis = pThis->pNext;
    }

    pThis = (audio_container*)x_malloc(sizeof(audio_container) + CfxCount * sizeof(cfx_info) );
    ASSERT(pThis);

    pThis->Id = ContainerId;
    pThis->SampleCount = CfxCount;
    pThis->Volume = 1.0f;
    x_strcpy(pThis->Filename,pFilename);

    gcncontainer_Header(ContainerId,pThis->CfxInfo);

    pThis->pNext = g_Audio->m_pContainerList;
    g_Audio->m_pContainerList = pThis;

    return pThis->Id;
}

void audio_UnloadContainer(s32 ContainerId)
{
    s32             Reply;
    audio_container *pThis,*pLast;

    Reply = 0;
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

//    iop_SendSyncRequest(AUDCMD_UNLOADCONTAINER,&ContainerId,sizeof(s32),&Reply,sizeof(s32));
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
void audio_Update(void)
{
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

    InitCmdBuff();

    WriteCmd(AUD_COMMAND_UPDATE_MASTER);
    WriteCmdBuff(&g_Audio->m_GlobalStatus,sizeof(g_Audio->m_GlobalStatus));
    
    pChannelList = g_Audio->m_pChannelList;
    nChannels = 0;
    TotalChannels = 0;
    while (pChannelList)
    {
        pChannel = pChannelList->m_Channels;
        TotalChannels+=AUD_MAX_CHANNELS;
        for (i=0;i<AUD_MAX_CHANNELS;i++)
        {
            if (pChannel->m_Flags & AUDFLAG_ALLOCATED)
            {
                audio_UpdateParams(pChannel);
                if (pChannel->m_Status == AUDSTAT_STARTING)
                {
                    pChannel->m_Status = AUDSTAT_PLAYING;
                }
                // If we only want the position to be set once, we reset the 3D position
                // flag once it has been initially updated. That sound then becomes unmovable
                // with no volume/pan change when the players ears move.
                if (pChannel->m_Flags & AUDFLAG_LOCALIZED)
                {
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
        if ( (pChannel->m_Flags & AUDFLAG_PERSISTENT) == 0)
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
    gcnaudio_Update(g_Audio->m_CommandBuffer);
    // On the PS2 driver, this update was checked before the new update is performed.
    // This was due to the lag between the iop and ee processors but on the gamecube,
    // we do the audio update synchronously so we don't really have to worry about
    // that. This may, however, need to change depending on overall system performance.
    InitCmdBuff();
    i=0;
    while (1)
    {
        u32 id;

        ReadCmdBuff(&id,sizeof(id));
        if (id==0)
            break;
        audio_Free(id);
        if (id==g_Audio->m_DelayedChannelId)
        {
            g_Audio->m_DelayedChannelId=0;
        }
        i++;
    }

    if (g_Audio->m_DelayedChannelId==0)
    {
        u32 id;

        id = (u32)mq_Recv(&g_Audio->m_qDelayedSamples,MQ_NOBLOCK);
        if (id)
        {
            g_Audio->m_DelayedChannelId = audio_Play(id);
        }
    }

#if defined(bwatson) && defined(X_DEBUG) && defined(TARGET_PS2_DEVKIT)
    x_printfxy(1,20,"CHN:%d/%d",i,nChannels);
#endif
    g_Audio->m_ChannelsInUse = nChannels;
}

void    audio_SetContainerVolume(s32 ContainerId,f32 Volume)
{
    gcncontainer_SetVolume(ContainerId,Volume);
}

f32 audio_GetContainerVolume (s32 ContainerId)
{
    return gcncontainer_GetVolume(ContainerId);
}


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


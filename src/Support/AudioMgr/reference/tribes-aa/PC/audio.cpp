#include "entropy.hpp"
#include "audio.hpp"
#include "audiodefs.hpp"
#include "audiovars.hpp"
#include "x_stdio.hpp"
#include "pcaud_audio.hpp"
#include <string.h>

#include "pcaud_audio.hpp"

#define    AUDIO_OFF


audio_vars *g_Audio=NULL;
s32 DebugDisableAudio = 0;

//==============================================================================
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

//==============================================================================
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

//==============================================================================
static void InitCmdBuff(void)
{
    g_Audio->m_pCommand = g_Audio->m_CommandBuffer;
}

//==============================================================================
static void WriteCmd(s8 cmd)
{
    WriteCmdBuff(&cmd,1);
}

//==============================================================================
void pcaudio_Init(void)
{
#ifdef AUDIO_OFF
    return;
#endif
    audio_init *pInit = NULL;
	pcAudio_Init(pInit);
}

//==============================================================================
void pcaudio_Kill(void)
{
#ifdef AUDIO_OFF
    return;
#endif	
    
    pcAudio_Kill();
}

//==============================================================================
s32 audio_LoadContainer(char *pFilename)
{
    //container_reply Reply;
//    char        filename[64];
//    audio_container *pContainer;
//    container_hdr_request HeaderRequest;
//    u32 count,remaining,index;
#ifdef AUDIO_OFF
    return 0;
#endif   
    
    ASSERT(g_Audio);
	s32 id;

	if ((strcmp(pFilename,"data\\audio\\effects.pkg") == 0))
	{

	    char fname[] = {"data\\audio\\effectsPC.pkg"};

	    s32 cfxCount;
		    
	    id = pccontainer_Load(fname, &cfxCount);
   		
	}
    else if (strcmp(pFilename,"data\\audio\\music.pkg") == 0)
    {
        char fname[] = {"data\\audio\\musicPC.pkg"};

        s32 cfxCount;
	        
        id = pccontainer_Load(fname, &cfxCount);    
    }
    else if (strcmp(pFilename,"data\\audio\\voice.pkg") == 0)
    {
        char fname[] = {"data\\audio\\voicePC.pkg"};

        s32 cfxCount;
	        
        id = pccontainer_Load(fname, &cfxCount);    
    }
    else
        return 0;

	return id;

#if 0
	return g_PCAudio.LoadContainer(fname);
#endif
    // The container load request is now asynchronous on the IOP side so we make sure
    // we poll it to see if it completed ok. The Reply.Status will be 1 until the
    // load has finished.

/*    iop_SendSyncRequest(AUDCMD_LOADCONTAINER,filename,x_strlen(filename)+1,&Reply,sizeof(Reply));
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
*/
	
//    return g_PCAudio.LoadSample(pFilename);
}

//==============================================================================
void audio_UnloadContainer(s32 ContainerId)
{
	if (ContainerId == 0)
		return;
	
	pccontainer_Unload(ContainerId);

#if 0
	g_PCAudio.UnloadSample(ContainerId);
#endif
}

//==============================================================================
cfx_info    *audio_FindInfo(s32 SampleId)
{
    static cfx_info DummyInfo;

    DummyInfo.m_Falloff = 10000;
    DummyInfo.m_Pan     = 0;
    DummyInfo.m_Pitch   = 10000;
    DummyInfo.m_Volume  = 10000;

    return &DummyInfo;
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
#ifdef AUDIO_OFF
    return;
#endif

    audio_channel       *pChannel;
    s32                 i;
    s32                 nChannels;
    s32                 TotalChannels;
    audio_channel_list  *pChannelList;
    static f32           farClip = 0;
    static f32           nearClip = 0;

    if (nearClip != g_Audio->m_NearClip || farClip != g_Audio->m_FarClip)
    {
            nearClip = g_Audio->m_NearClip;
            farClip = g_Audio->m_FarClip;
            g_Miles.SetClip(nearClip, farClip);
    }

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
                audio_UpdateParams(pChannel,1.0f);
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
            pChannel->m_State.m_Flags = pChannel->m_Flags;
            pChannel->m_State.m_Pitch = (s16)pChannel->m_Pitch;
            
            // Get the volume from the container, just give it the high byte of the sample.
            if (pChannel->m_State.m_CfxId != -1)
                pChannel->m_State.m_Volume = (s16)((pChannel->m_State.m_Volume)*audio_GetContainerVolume( ((pChannel->m_State.m_CfxId>>24)<<24) ));

            //pChannel->m_State.m_Position = pChannel->m_Position;
            WriteCmdBuff(&pChannel->m_State,sizeof(pChannel->m_State));
        }
		//audio_Free(pChannel->m_State.m_Id);
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
    pcAudio_Update(g_Audio->m_CommandBuffer);
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

        id = (u32)g_Audio->m_pqDelayedSamples->Recv(MQ_NOBLOCK);
        if (id)
        {
            g_Audio->m_DelayedChannelId = audio_Play(id);
        }
    }

#if defined(bwatson) && defined(X_DEBUG) && defined(TARGET_PS2_DEVKIT)
    x_printfxy(1,20,"CHN:%d/%d",i,nChannels);
#endif
    g_Audio->m_ChannelsInUse = nChannels;	
//    x_printf("%d\n",g_Audio->m_ChannelsInUse);
/*	
	audio_channel       *pChannel;
    s32                 i;
    s32                 nChannels;
    s32                 TotalChannels;
    audio_channel_list  *pChannelList;

    if (DebugDisableAudio)
        return;

    g_Audio->m_UpdateRequestIndex=0;

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
                nChannels++;
            }
            pChannel++;
        }
        pChannelList = pChannelList->m_pNext;
    }

    i=0;
    while (i<g_Audio->m_UpdateRequestIndex)
    {
        pChannel = g_Audio->m_UpdateRequests[i];
        audio_Free(pChannel->m_State.m_Id);
        i++;
    }

    g_Audio->m_ChannelsInUse = nChannels;
*/
}

/*
void    audio_InitStream(f32)
{
}

void    audio_SendStream(void *)
{
}

xbool    audio_WaitStream(void)
{
    return TRUE;
}

void    audio_KillStream(void)
{
}
*/
void    audio_SetContainerVolume(s32 ContainerId,f32 Volume)
{
#ifdef AUDIO_OFF
    return;
#endif

	pccontainer_SetVolume(ContainerId, Volume);
}

f32     audio_GetContainerVolume(s32 ContainerId)
{
#ifdef AUDIO_OFF
    return 0;
#endif

	return pccontainer_GetVolume(ContainerId);
}

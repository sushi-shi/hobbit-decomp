#include "x_files.hpp"
#include "pcaud_spu.hpp"
#include "pcaud_host.hpp"
#include "pcaud_audio.hpp"
//#include <dolphin.h>
//#include <dolphin/mix.h>

#define INITIAL_VOL 0
#define MAX_ARAM_BLOCKS 1024
//#define TRACK_MALLOCS
s32 SpuMemStart;
s32 SpuMemEnd;
s32 SpuMemCurr;

xsema* g_pqDmaComplete;
s32 verbose = 0;
s32 s_ZeroBlock;

u32 s_AramMemArray[MAX_ARAM_BLOCKS];

static void spu_TransferCallback(void);

static u32 AddressToNibble(u32 addr)
{
    return (addr*2)+2;
}

static u32 NibbleToAddress(u32 nibble)
{
    nibble = nibble - 2;
    return (nibble * 16 / 14);
}
//------------------------------------------------------------------------
void    spu_Init(void)
{
    spu_state BlankState;
    u8 *pBuff;


    g_pqDmaComplete = new xsema(2,0);
    //ARRegisterDMACallback(spu_TransferCallback);
    // Set base sample rate to 48khz

#if 0
    if (AUD_BASE_SAMPLE_RATE == 48000)
    {
        AISetDSPSampleRate(AI_SAMPLERATE_48KHZ);
    }
    else
    {
        AISetDSPSampleRate(AI_SAMPLERATE_32KHZ);
    }
#endif
    s_ZeroBlock = spu_Alloc(2048,"Zero block for end of non looped samples");
    pBuff = (u8 *)x_malloc(2048);
    ASSERT(pBuff);
    x_memset(pBuff,0,2048);
    spu_Transfer(pBuff,s_ZeroBlock,2048,SPUTRANS_WRITE);
    x_free(pBuff);

    x_memset(&BlankState,0,sizeof(BlankState));
#if 0
    for (i=0;i<AUD_MAX_PHYS_VOICES;i++)
    {
        spu_SetKey(i,0);
        spu_WriteVoice(i,&BlankState);
    }
#endif
    spu_WriteMaster(&g_pcAudio.m_MasterState);


}

//------------------------------------------------------------------------
s32  spu_Alloc(s32 size,char *pLabel)
{
    (void)pLabel;
    size = (size+31) & ~31;
#if 0
    return ARAlloc(size);
#else
	return 0;
#endif
}

//------------------------------------------------------------------------
s32     spu_Free(s32 MemBase)
{
#if 0
    ARFree((u32*)MemBase);
#endif
    return 0;
}

//------------------------------------------------------------------------
s32     spu_MemFree(void)
{
    return SpuMemEnd-SpuMemCurr;
}

s32     spu_Memtop(void)
{
    return SpuMemCurr;
}

//------------------------------------------------------------------------
//------------------------
static void spu_TransferCallback(void)
{
	g_pqDmaComplete->Release();
}

//-----------------------------------------------------------------------------
// Synchronous data transfer to Spu
void    spu_Transfer(void *pBase,s32 Dest,s32 length,s32 direction)
{
#if 0
    s32 mode;

    length = (length + 31) & ~31;
    ASSERT( ((s32)pBase & 31) == 0);
    ASSERT( (Dest & 31) == 0);


    if (verbose)
    {
        x_DebugMsg("spu_Transfer: Source=%08x, Dest=%08x, Length=%08x\n",pBase,Dest,length);
    }
    if (direction == SPUTRANS_WRITE)
    {
        mode = ARAM_DIR_MRAM_TO_ARAM;
    }
    else
    {
        mode = ARAM_DIR_ARAM_TO_MRAM;
    }
    DCStoreRange(pBase,length);
    ARStartDMA(mode,(u32)pBase,Dest,length);
    DCInvalidateRange(pBase,length);
    g_pqDmaComplete->Acquire();
#endif

}

//-----------------------------------------------------------------------------
void spu_InitVoice(audio_voice *pVoice)
{
#if 0
    AXPBADDR Addr;
    AXPBSRC  Src;
    AXVPB *  vpb;

    s32 start,end,zero;
    s32 pitch;

#if 0
    if (verbose)
        x_DebugMsg("SpuSetSampleAddress(%d,0x%08x)\n",pVoice->voice,start);
#endif
    vpb = (AXVPB*)pVoice->m_pSysVoice;

    // We have the ADPCM header. I don't know if we even need it right now
    // but it's there. Adjusting the start and end address to incorporate this
    // info.
    start = AddressToNibble(pVoice->m_pOwner->m_SpuLocation);
    end   = AddressToNibble(pVoice->m_pOwner->m_SpuLocation + pVoice->m_pOwner->m_Length);
    zero  = AddressToNibble(s_ZeroBlock);

    Addr.loopFlag           = AXPBADDR_LOOP_OFF;
    Addr.format             = AX_PB_FORMAT_ADPCM;
    Addr.loopAddressHi      = (zero  >> 16) & 0xffff;
    Addr.loopAddressLo      = (zero  >>  0) & 0xffff;
    Addr.currentAddressHi   = (start >> 16) & 0xffff;
    Addr.currentAddressLo   = (start >>  0) & 0xffff;
    Addr.endAddressHi       = (end   >> 16) & 0xffff;
    Addr.endAddressLo       = (end   >>  0) & 0xffff;

    MIXInitChannel(vpb,0,0,-960,-960,64,127,0);

    AXSetVoiceAddr(vpb,&Addr);
    AXSetVoiceSrcType(vpb, AX_SRC_TYPE_LINEAR);    // SRC type

    pitch = 32000 * 65536 / pVoice->m_pOwner->m_pAttributes->m_SampleRate;
    x_memset(&Src,0,sizeof(AXPBSRC));
    Src.ratioHi = pitch >> 16;
    Src.ratioLo = pitch & 0xffff;

    AXSetVoiceSrc(vpb,&Src);
    AXSetVoiceAdpcm(vpb,(AXPBADPCM*)&pVoice->m_pOwner->m_AdpcmHeader);
#endif
}

//-----------------------------------------------------------------------------
void    spu_WriteMaster(spu_master_state *pState)
{
    if (verbose)
        x_DebugMsg("SpuSetMasterVolume(%d,%d)\n",pState->m_State.m_LeftVolume,pState->m_State.m_RightVolume);
#if 0

    sceSdSetParam(SD_P_MVOLL|SD_CORE_0, pState->m_State.m_LeftVolume);
    sceSdSetParam(SD_P_MVOLL|SD_CORE_1, pState->m_State.m_LeftVolume);
    sceSdSetParam(SD_P_MVOLR|SD_CORE_0, pState->m_State.m_RightVolume);
    sceSdSetParam(SD_P_MVOLR|SD_CORE_1, pState->m_State.m_RightVolume);

    sceSdSetSwitch(SD_CORE_0|SD_S_KON,pState->m_Keyon0);
    sceSdSetSwitch(SD_CORE_1|SD_S_KON,pState->m_Keyon1);

    sceSdSetSwitch(SD_CORE_0|SD_S_KOFF,pState->m_Keyoff0);
    sceSdSetSwitch(SD_CORE_1|SD_S_KOFF,pState->m_Keyoff1);
#endif
    pState->m_Keyon0    = 0;
    pState->m_Keyon1    = 0;
    pState->m_Keyoff0   = 0;
    pState->m_Keyoff1   = 0;

}

//-----------------------------------------------------------------------------
void    spu_ReadMaster(spu_master_state *pState)
{
    x_memset(pState,0,sizeof(spu_master_state));
#if 0
    pState->m_Keyon0 = (u32)sceSdGetSwitch(SD_CORE_0|SD_S_ENDX);
    pState->m_Keyon1 = (u32)sceSdGetSwitch(SD_CORE_1|SD_S_ENDX);
    pState->m_State.m_LeftVolume = sceSdGetParam(SD_CORE_0|SD_P_MVOLL);
    pState->m_State.m_RightVolume= sceSdGetParam(SD_CORE_0|SD_P_MVOLR);
#endif
}

//-----------------------------------------------------------------------------
void    spu_WriteVoice(audio_voice *pVoice,spu_state *pState)
{
#if 0
    AXVPB *vpb;
    AXPBMIX Mix;
    f32 pitch;

    vpb = (AXVPB*)pVoice->m_pSysVoice;

    Mix.vL          = pState->LeftVolume;
    Mix.vDeltaL     = 0;
    Mix.vR          = pState->RightVolume;
    Mix.vDeltaR     = 0;

    Mix.vAuxAL      = 0;
    Mix.vDeltaAuxAL = 0;
    Mix.vAuxAR      = 0;
    Mix.vDeltaAuxAR = 0;

    Mix.vAuxBL      = 0;
    Mix.vDeltaAuxBL = 0;
    Mix.vAuxBR      = 0;
    Mix.vDeltaAuxBR = 0;

    Mix.vS          = 0;
    Mix.vDeltaS     = 0;
    Mix.vAuxAS      = 0;
    Mix.vDeltaAuxAS = 0;

    pitch = (f32)pState->Pitch / (f32)AUD_FIXED_POINT_1;
    AXSetVoiceSrcRatio(vpb,pitch);                       // Set the pitch
    AXSetVoiceMix(vpb,&Mix);
#endif
}

spu_state StateWatch;

void    spu_ReadVoice(audio_voice *pVoice,spu_state *pState)
{
#if 0
    AXVPB *vpb;
    s32 addr;

    vpb = (AXVPB*)pVoice->m_pSysVoice;
    pState->Finished    = vpb->pb.state == AX_PB_STATE_STOP;
    pState->LeftVolume  = vpb->pb.mix.vL;
    pState->RightVolume = vpb->pb.mix.vR;
    pState->Pitch       = AUD_FIXED_POINT_1;
    pState->Envelope    = 0;
    addr = (((s32)vpb->pb.currHi)<<16)|vpb->pb.currLo;
    pState->Current     = NibbleToAddress(addr);

    StateWatch = *pState;
#endif
}

//-----------------------------------------------------------------------------
void    spu_SetKey(audio_voice *pVoice,s32 on)
{
#if 0
    AXSetVoiceState((AXVPB*)pVoice->m_pSysVoice,on);
#endif
}


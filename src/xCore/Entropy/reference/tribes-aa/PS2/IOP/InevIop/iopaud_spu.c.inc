#include <kernel.h>
#include <libsd.h>

#include "ioptypes.h"
#include "iopaud_spu.h"
#include "iopaud_host.h"
#include "iopaudio.h"
#include "iopmain.h"

#define INITIAL_VOL 0
#define TRACK_MALLOCS
s32 SpuMemStart;
s32 SpuMemEnd;
s32 SpuMemCurr;

s32 g_DmaSemaphore = 0;
s32 g_DmaLockMutex = 0;
s32 verbose = 0;

s32     _SpuDmaInt(void *common);

static s32 VoiceToIndex(s32 voice)
{
    if (voice >= VOICES_PER_CORE)
    {
        return (voice-VOICES_PER_CORE)<<1|SD_CORE_1;
    }
    else
    {
        return (voice<<1)|SD_CORE_0;
    }
}
//------------------------------------------------------------------------
void    spu_Init(void)
{
    struct SemaParam sem;
    spu_state BlankState;
    s32 i;

    // Initialize internal vars
    SpuMemStart = SPU_MEM_BASE;
    SpuMemEnd   = SPU_MEM_SIZE;
    SpuMemCurr  = SpuMemStart;

    sem.initCount = 0;
    sem.maxCount = 1;
    sem.attr = AT_THFIFO;

    // We cannot use a standard iop mutex since it will be signalled within an interrupt context
    g_DmaSemaphore = CreateSema(&sem);
    g_DmaLockMutex = iop_CreateMutex();
    iop_ExitMutex(g_DmaLockMutex);
//    sceSdSetTransCallback(0,_SpuDmaInt);
    
    // Initialize the sound hardware.
    sceSdInit(0);
    sceSdSetCoreAttr(SD_C_SPDIF_MODE,(SD_SPDIF_MEDIA_DVD | SD_SPDIF_OUT_PCM | SD_SPDIF_COPY_NORMAL));
    sceSdSetTransIntrHandler (0, (sceSdSpu2IntrHandler) _SpuDmaInt, NULL);
    // 
    // Set initial parameters to 0. This makes sure we get no spurious sounds we don't expect.
    //

    memset(&BlankState,0,sizeof(BlankState));
    for (i=0;i<AUD_MAX_PHYS_VOICES;i++)
    {
        spu_SetKey(i,0);
        spu_WriteVoice(i,&BlankState);
    }

    spu_WriteMaster(&g_Audio.m_MasterState);


}

//------------------------------------------------------------------------
void spu_Kill(void)
{
    spu_state BlankState;
    s32 i;

    // On a kill, zero all the voices and set master volume to 0
    memset(&BlankState,0,sizeof(BlankState));
    for (i=0;i<AUD_MAX_PHYS_VOICES;i++)
    {
        spu_SetKey(i,0);
        spu_WriteVoice(i,&BlankState);
    }

    memset(&g_Audio.m_MasterState,0,sizeof(g_Audio.m_MasterState));
    spu_WriteMaster(&g_Audio.m_MasterState);

    iop_DestroyMutex(g_DmaLockMutex);
    DeleteSema(g_DmaSemaphore);
}

//------------------------------------------------------------------------
s32  spu_Alloc(s32 size,char *pLabel)
{
    s32 base;
    (void)pLabel;

    size = (size + 15) & ~15;
#ifdef TRACK_MALLOCS
    iop_DebugMsg("spu_Alloc: %10d bytes, Current top 0x%08x, new top 0x%08x, %s\n",size,SpuMemCurr,SpuMemCurr+size,pLabel);
#endif
    if (size+SpuMemCurr > SpuMemEnd)
    {
        base = 0;
    }
    else
    {
        base = SpuMemCurr;
        SpuMemCurr+=size;
    }
    return base;
}

//------------------------------------------------------------------------
s32     spu_Free(s32 MemBase)
{
    SpuMemCurr = MemBase;
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
s32     _SpuDmaInt(void *common)
{
    (void)common;

    if (verbose)
        iop_DebugMsg("DMA Interrupt occurred\n");
    iSignalSema(g_DmaSemaphore);
    return 1;
}

//-----------------------------------------------------------------------------
// Synchronous data transfer to Spu
void    spu_Transfer(void *pBase,s32 Dest,s32 length,s32 direction)
{
    s32 status;
    s32 xferlen;

    FlushDcache();
    length = (length + 15) & ~15;
    ASSERT( ((s32)pBase & 15) == 0);
    ASSERT( (Dest & 15) == 0);
    if (verbose)
        iop_DebugMsg("spu_Transfer(0x%08x,0x%08x,%d,%d)\n",pBase,Dest,length,direction);

    if (direction==SPUTRANS_WRITE)
    {
        direction = SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA;
    }
    else
    {
        direction = SD_TRANS_MODE_READ | SD_TRANS_BY_DMA;
    }

    while (length)
    {
        xferlen = length;
        if (xferlen > 16 * 1024)
            xferlen = 16 * 1024;
    
        iop_EnterMutex(g_DmaLockMutex);
        status = sceSdVoiceTrans(0,
                    direction,
                    pBase,
                    Dest,
                    xferlen);
        WaitSema(g_DmaSemaphore);
        iop_ExitMutex(g_DmaLockMutex);
        length -= xferlen;
        pBase += xferlen;
        Dest += xferlen;
    }
}

//-----------------------------------------------------------------------------
void spu_InitVoice(s32 voice,s32 start,s32 adsr1,s32 adsr2)
{
    if (verbose)
        iop_DebugMsg("SpuSetSampleAddress(%d,0x%08x)\n",voice,start);

    // For now, we just set all the other parameters to default values
        sceSdSetParam(VoiceToIndex(voice) | SD_VP_ADSR1,adsr1);         //ADSR1(0,0,0,15));
        sceSdSetParam(VoiceToIndex(voice) | SD_VP_ADSR2,adsr2);         //ADSR2(0,127,0,9));      // Release rate of 23 milliseconds
        sceSdSetAddr (VoiceToIndex(voice) | SD_VA_SSA,start);
}

//-----------------------------------------------------------------------------
void    spu_WriteMaster(spu_master_state *pState)
{
    if (verbose)
        iop_DebugMsg("SpuSetMasterVolume(%d,%d)\n",pState->m_State.m_LeftVolume,pState->m_State.m_RightVolume);

    sceSdSetParam(SD_P_MVOLL|SD_CORE_0, pState->m_State.m_LeftVolume);
    sceSdSetParam(SD_P_MVOLL|SD_CORE_1, pState->m_State.m_LeftVolume);
    sceSdSetParam(SD_P_MVOLR|SD_CORE_0, pState->m_State.m_RightVolume);
    sceSdSetParam(SD_P_MVOLR|SD_CORE_1, pState->m_State.m_RightVolume);

    sceSdSetSwitch(SD_CORE_0|SD_S_KON,pState->m_Keyon0);
    sceSdSetSwitch(SD_CORE_1|SD_S_KON,pState->m_Keyon1);

    sceSdSetSwitch(SD_CORE_0|SD_S_KOFF,pState->m_Keyoff0);
    sceSdSetSwitch(SD_CORE_1|SD_S_KOFF,pState->m_Keyoff1);

    pState->m_Keyon0    = 0;
    pState->m_Keyon1    = 0;
    pState->m_Keyoff0   = 0;
    pState->m_Keyoff1   = 0;

}

//-----------------------------------------------------------------------------
void    spu_ReadMaster(spu_master_state *pState)
{
    pState->m_Keyon0 = (u32)sceSdGetSwitch(SD_CORE_0|SD_S_ENDX);
    pState->m_Keyon1 = (u32)sceSdGetSwitch(SD_CORE_1|SD_S_ENDX);
    pState->m_State.m_LeftVolume = sceSdGetParam(SD_CORE_0|SD_P_MVOLL);
    pState->m_State.m_RightVolume= sceSdGetParam(SD_CORE_0|SD_P_MVOLR);
}

#if 0
spu_state chancopy[48];
#endif

//-----------------------------------------------------------------------------
void    spu_WriteVoice(s32 voice,spu_state *pState)
{
    sceSdSetParam(VoiceToIndex(voice)|SD_VP_VOLL,pState->LeftVolume);
    sceSdSetParam(VoiceToIndex(voice)|SD_VP_VOLR,pState->RightVolume);
    sceSdSetParam(VoiceToIndex(voice)|SD_VP_PITCH,pState->Pitch);
#if 0
	if (pState->LeftVolume != chancopy[voice].LeftVolume)
	{
		Kprintf("[%8d] spu_WriteVoice: Voice %2d,  left volume change from %5d to %5d\n",g_Audio.m_TimerTick,
				voice,
				chancopy[voice].LeftVolume,
				pState->LeftVolume);
	}

	if (pState->RightVolume != chancopy[voice].RightVolume)
	{
		Kprintf("[%8d] spu_WriteVoice: Voice %2d, right volume change from %5d to %5d\n",g_Audio.m_TimerTick,
				voice,
				chancopy[voice].RightVolume,
				pState->RightVolume);
	}

#if 0
	if (pState->Pitch != chancopy[voice].Pitch)
	{
		Kprintf("[%d] spu_WriteVoice: Voice %d, pitch change from %d to %d\n",g_Audio.m_TimerTick,voice,chancopy[voice].Pitch,pState->Pitch);
	}
#endif

	chancopy[voice] = *pState;
#endif
}

//-----------------------------------------------------------------------------
// Oh, don't you just *love* those crappy hardware bugs. This is a fix posted on
// the sce web site. Thanks Rob Vawter. Next time, make your fucking hardware work
// right.
//
s32     spu_GetGoodNaxValue(s32 voice)
{
    s32 check1,check2,check3;
    s32 ret;
    s32 v;

    v = VoiceToIndex(voice)|SD_VA_NAX;
    while (1)
    {
        check1=sceSdGetAddr(v);
        check2=sceSdGetAddr(v);
        check3=sceSdGetAddr(v);
        if (check1==check2)
        {
            ret = check1;
            break;
        }
        else if (check2==check3)
        {
            ret = check2;
            break;
        }
    }
    return ret;
}

void    spu_ReadVoice(s32 voice,spu_state *pState)
{
    pState->LeftVolume  = sceSdGetParam(VoiceToIndex(voice)|SD_VP_VOLL);
    pState->RightVolume = sceSdGetParam(VoiceToIndex(voice)|SD_VP_VOLR);
    pState->Pitch       = sceSdGetParam(VoiceToIndex(voice)|SD_VP_PITCH);
    pState->Envelope    = sceSdGetParam(VoiceToIndex(voice)|SD_VP_ENVX);
    pState->Current     = spu_GetGoodNaxValue(voice);

    if (voice >= VOICES_PER_CORE)
    {
        voice-=VOICES_PER_CORE;
        pState->Finished    = ((g_Audio.m_CurrentMasterState.m_Keyon1 & ~g_Audio.m_MasterState.m_Keyon1) & (1<<voice))!=0;
    }
    else
    {
        pState->Finished    = ((g_Audio.m_CurrentMasterState.m_Keyon0 & ~g_Audio.m_MasterState.m_Keyon0) & (1<<voice))!=0;
    }
}

//-----------------------------------------------------------------------------
void    spu_SetKey(s32 voice,s32 on)
{
    if (on)
    {
        if (voice >= VOICES_PER_CORE)
        {
            voice -= VOICES_PER_CORE;
            g_Audio.m_MasterState.m_Keyon1 |= (1<<voice);
            g_Audio.m_MasterState.m_Keyoff1 &= ~(1<<voice);
        }
        else
        {
            g_Audio.m_MasterState.m_Keyon0 |= (1<<voice);
            g_Audio.m_MasterState.m_Keyoff0 &= ~(1<<voice);
        }

    }
    else
    {
        if (voice >= VOICES_PER_CORE)
        {
            voice -= VOICES_PER_CORE;
            g_Audio.m_MasterState.m_Keyoff1 |= (1<<voice);
            g_Audio.m_MasterState.m_Keyon1 &= ~(1<<voice);
        }
        else
        {
            g_Audio.m_MasterState.m_Keyoff0 |= (1<<voice);
            g_Audio.m_MasterState.m_Keyon0 &= ~(1<<voice);
 
        }

    }
}


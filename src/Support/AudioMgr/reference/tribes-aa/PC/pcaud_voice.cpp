#include "x_files.hpp"
#include "x_plus.hpp"
#include "pcaud_audio.hpp"
#include "pcaud_element.hpp"
#include "pcaud_voice.hpp"
#include "pcaud_host.hpp"

///#include <dolphin.h>

//#define AUDIO_DEBUG
Audio	g_Miles;

//-----------------------------------------------------------------------------
void voice_Init(void)
{
    s32 i;

    g_pcAudio.m_pVoices = (audio_voice *)x_malloc(sizeof(audio_voice) * AUD_MAX_PHYS_VOICES);
    ASSERT(g_pcAudio.m_pVoices);
    x_memset(g_pcAudio.m_pVoices,0,sizeof(audio_voice)* AUD_MAX_PHYS_VOICES);
    g_pcAudio.m_pqFreeVoices = new xmesgq(AUD_MAX_PHYS_VOICES);
    for (i=0;i<AUD_MAX_PHYS_VOICES;i++)
    {
        g_pcAudio.m_pVoices[i].m_Status = VSTAT_IDLE;
        g_pcAudio.m_pVoices[i].m_Id     = i;
        g_pcAudio.m_pqFreeVoices->Send(&g_pcAudio.m_pVoices[i],MQ_BLOCK);
    }

    g_pcAudio.m_VoicesToKill = 0;
    g_pcAudio.m_VoicesInUse  = 0;

}

//-----------------------------------------------------------------------------
audio_voice *voice_Alloc(cfx_element *pElement)
{
    audio_voice *pVoice;


    pVoice = (audio_voice *)g_pcAudio.m_pqFreeVoices->Recv(MQ_NOBLOCK);

    // If we were unable to allocate some internal voice space, let's just discard the allocation
    if (!pVoice)
    {
        return NULL;
    }

	

    // Priorities within our defined system are 0..127 and within the Nintendo libs it's 0..31
    // so we manipulate our range so it's within what nintendo expect.
#if 0
    pVoice->m_pSysVoice = (void*)AXAcquireVoice(pElement->m_pAttributes->m_Priority >> 2,NULL,NULL);
#endif

    pVoice->m_Status        = VSTAT_IDLE;
    pVoice->m_ExpireDelay   = 100;
    pVoice->m_pOwner        = pElement;
    pVoice->m_SpuCurrent    = pElement->m_SpuLocation;
    pVoice->m_pSibling      = NULL;
    pVoice->m_Age           = 0;

    //
    // A non streamed element should have it's voice started immediately.
    //
    if (pElement->m_pAttributes->m_Type == CFXTYPE_ELEMENT)
    {
        pVoice->m_Status = VSTAT_PLAYING;

#ifndef sansari
        if (pElement->m_pOwner->m_State.m_CfxId == 33554625)
            x_DebugMsg("Sound level %d\n", pElement->m_pOwner->m_State.m_Volume);
#endif
                
  
        // Every sounds is positional, if its is a 2D sounds then its just located at the origin.
        // Even though all the sounds start with position, the 2D sounds stay at the origin and all 
        // that changes about them is the volume.
        pVoice->m_UniqueSampleId = g_Miles.Set3D(pElement->m_SpuLocation, &pElement->m_pOwner->m_State.m_Position);
        
        // Un able to play the sample.
        if (pVoice->m_UniqueSampleId == NO_FILE_LOADED)
            return NULL;

        // If the Cfx's count is greater than 1, then it holds all of its attributes.  But if the element count
        // is 1 then we have to ask the parents to get the correct attributes.
        if (pElement->m_pOwner->m_ElementCount > 1)
        {
            g_Miles.SetSampleVolume(pElement->m_SpuLocation, pVoice->m_UniqueSampleId, 
                                    pElement->m_pAttributes->m_Volume);

            // Must set the priority, that way we can remove sounds if the buffer starting getting full by priority.
            g_Miles.SetPriority(pElement->m_SpuLocation,pElement->m_pAttributes->m_Priority);
        }
        else
        {
            g_Miles.SetSampleVolume(pElement->m_SpuLocation, pVoice->m_UniqueSampleId, 
                                    pElement->m_pParent->m_pAttributes->m_Volume);

            // Must set the priority, that way we can remove sounds if the buffer starting getting full by priority.
            g_Miles.SetPriority(pElement->m_SpuLocation, pElement->m_pParent->m_pAttributes->m_Priority);
        }
        
        // Hardware loop, mainly for effects.
        if (pVoice->m_pOwner->m_Flags & AUDFLAG_HARDWARE_LOOP)
        {
				g_Miles.SetLoopCount(pVoice->m_SpuCurrent, INFINITE_LOOP);
        }
        // Software loop, mainly for music.
        if (pVoice->m_pOwner->m_Flags & AUDFLAG_LOOPED)
        {
				g_Miles.SetLoopCount(pVoice->m_SpuCurrent, INFINITE_LOOP);
        }

    }

#ifdef  sansari
//    x_DebugMsg("Voice Alloc\n");
//    x_DebugMsg("Falloff %d\n",pElement->m_pAttributes->m_Falloff);
//    x_DebugMsg("Sounds %d\n",g_Miles.m_3DSoundCount);
#endif
    
    // Start voice here from details in pElement
    return pVoice;
}

//-----------------------------------------------------------------------------
void voice_Free(audio_voice *pVoice)
{

    // Store the inder for miles to release the sound.
    s32 index = pVoice->m_SpuCurrent;
    s32 UniqueId = pVoice->m_UniqueSampleId;
	ASSERT(pVoice);
    ASSERT(pVoice->m_Status != VSTAT_FREE);
    //
    // Before we free it, we should make sure that it has been muted and stopped
    //
    pVoice->m_Status = VSTAT_FREE;
#if 0
    AXFreeVoice((AXVPB*)pVoice->m_pSysVoice);
#endif
    if (pVoice->m_pSibling)
    {
        voice_Free(pVoice->m_pSibling);
    }
    g_pcAudio.m_pqFreeVoices->Send(pVoice,MQ_BLOCK);

	// Release the sound.
    g_Miles.UnloadSample(index, UniqueId);

#ifdef sansari
//    x_DebugMsg("Sounds %d\n",g_Miles.m_3DSoundCount);
//    x_DebugMsg("Voice Free\n");
#endif

}

s32 qsort_compare(const void *arg1,const void *arg2)
{
    return (*(s32 *)arg1 - *(s32 *)arg2);
}
//-----------------------------------------------------------------------------

s32 voice_Cull(s32 NumberToKill)
{
    s32         NumberKilled;
    s32         SortKey[AUD_MAX_PHYS_VOICES];
    audio_voice *pVoices;
    s32         i,index;

    NumberKilled = 0;

    pVoices = g_pcAudio.m_pVoices;
    index = 0;
#ifdef X_DEBUG
//    x_DebugMsg("******************* Culling %d voices\n",NumberToKill);
#endif
    for (i=0;i<AUD_MAX_PHYS_VOICES;i++)
    {
        if ( (pVoices->m_Status == VSTAT_STOPPING) ||
             (pVoices->m_Status == VSTAT_FREE) )
        {
            NumberToKill--;
//            g_Miles.UnloadSample(pVoices->m_SpuCurrent);
            if (NumberToKill <= 0)
                return 0;
        }
        else
        {
            if ( (pVoices->m_pOwner->m_pAttributes->m_Type != CFXTYPE_ELEMENT_STREAM) &&
                 (pVoices->m_pOwner->m_pAttributes->m_Type != CFXTYPE_ELEMENT_HYBRID) &&
                 (pVoices->m_pOwner->m_pAttributes->m_Priority <= g_pcAudio.m_LowestKillPriority) &&
                 ( (pVoices->m_pOwner->m_Flags & AUDFLAG_LOOPED)==0 ) )
            {
                SortKey[index]  = pVoices->m_pOwner->m_pAttributes->m_Priority<<24;
                SortKey[index] |= (pVoices->m_pOwner->m_pOwner->m_State.m_Volume / (AUD_FIXED_POINT_1 / 128))<<16;
                SortKey[index] |= ((-pVoices->m_Age) & 0xff << 8);
                SortKey[index] |= i;
       
                index++;
            }
        }
        pVoices++;
    }
    // If index is still 0, this means we were unable to find any voices with higher priorities to kill
    //
    if (!index)
        return 0;
    x_qsort(SortKey,index,sizeof(s32),qsort_compare);
    i=0;
    while (NumberToKill--)
    {
        audio_voice *pVoice;
//**NOTE: Need to put in code to make sure that we do not kill a voice
// that has a priority greater than the sample that's trying to start.
// If we end up consuming the entire array, then we could not delete
// any more voices.

        pVoice = &g_pcAudio.m_pVoices[SortKey[i] & 0xff];
#ifdef X_DEBUG
        x_DebugMsg("Culled voice %d,%08x,%08x\n",pVoice->m_Id,SortKey[i],pVoice->m_pOwner->m_pOwner->m_Identifier);
#endif
//        g_Miles.UnloadSample(pVoice->m_SpuCurrent);
		spu_SetKey(pVoice,0);
        pVoice->m_Status = VSTAT_STOPPING;
        NumberKilled++;
        i++;
        if (i>=index)
            break;
    }
    return NumberKilled;

}

//-----------------------------------------------------------------------------
xbool       voice_Update(audio_voice *pVoice,cfx_state *pState,f32 DeltaTime)
{
    s32         offset;
    spu_state   CurrentState;
    s32         leftvol,rightvol;

    (void)DeltaTime;

    spu_ReadVoice(pVoice,&CurrentState);
    pVoice->m_Hardware.Current = CurrentState.Current;
    pVoice->m_Hardware.Envelope = CurrentState.Envelope;
	
	// If its positional sounds then all we have to do is adjust its position. If its not then we just mess with
    // its volume.  We don't want to do any extra calculation for the localized sound.
    if (pVoice->m_pOwner->m_pOwner->m_State.m_Flags & AUDFLAG_3D_POSITION && 
        !(pVoice->m_pOwner->m_pOwner->m_State.m_Flags & AUDFLAG_LOCALIZED))
    {
        // Positional Sound.
        g_Miles.UpdateSample(pVoice->m_SpuCurrent, pVoice->m_UniqueSampleId, 
                            &pVoice->m_pOwner->m_pOwner->m_State.m_Position, DeltaTime);
    }
    else if (!(pVoice->m_pOwner->m_pOwner->m_State.m_Flags & AUDFLAG_LOCALIZED))
    {
        // Just adjust the volume for this 3d sound.
        g_Miles.SetSampleVolume(pVoice->m_SpuCurrent, pVoice->m_UniqueSampleId,
                                pVoice->m_pOwner->m_pOwner->m_State.m_Volume);
    }


    pVoice->m_Age++;

    offset = pVoice->m_Hardware.Current - pVoice->m_SpuCurrent;
    if (pVoice->m_Status == VSTAT_STOPPING)
    {

		spu_SetKey(pVoice,0);
        pVoice->m_Status = VSTAT_IDLE;
        return TRUE;
    }

    switch (pVoice->m_pOwner->m_pAttributes->m_Type)
    {
    case CFXTYPE_ELEMENT:
        
        // Check if the sound is done playing.
        if (g_Miles.GetSoundStatus(pVoice->m_SpuCurrent, pVoice->m_UniqueSampleId) == SMP_DONE)
        {
		    
            CurrentState.Finished = 1;
            return TRUE;
        }
#ifdef sansari
        // SULTAN: Debug!!!!!
        else if (g_Miles.GetSoundStatus(pVoice->m_SpuCurrent, pVoice->m_UniqueSampleId) == UNDEFINED_SAMPLE)
        {
//            ASSERTS(FALSE, "Either we lost the sample or the Voices sampleid is incorrect");
        }
        // SULTAN: Debug!!!.
#endif
        else
            return FALSE;

        break;

    case CFXTYPE_ELEMENT_HYBRID:
    case CFXTYPE_ELEMENT_STREAM:
    case CFXTYPE_HOST_STREAM:
        break;
    default:
        ASSERT(FALSE);
    }

    pVoice->m_Hardware.Pitch = (pState->m_Pitch * pVoice->m_pOwner->m_pAttributes->m_SampleRate) / SPU_BASE_SAMPLE_RATE;

    leftvol = pVoice->m_Hardware.LeftVolume;
    rightvol = pVoice->m_Hardware.RightVolume;

    pVoice->m_Hardware.RightVolume = pState->m_Volume;
    pVoice->m_Hardware.LeftVolume = pState->m_Volume;




#if 0  
    if (pVoice->m_pSibling)
    {
        spu_ReadVoice(pVoice,&CurrentState);
		
		if (g_Miles.GetSoundStatus(pVoice->m_pSibling->m_SpuCurrent) == SMP_DONE)
			pVoice->m_Hardware.Finished = 1;
		else
			pVoice->m_Hardware.Finished = 0;
		
		if (g_Miles.GetSoundStatus(pVoice->m_pSibling->m_SpuCurrent) == SMP_DONE)
			pVoice->m_Hardware.Envelope = 0;

		if (g_Miles.GetSoundStatus(pVoice->m_pSibling->m_SpuCurrent) == SMP_PLAYING)
			pVoice->m_Hardware.Envelope = 1;

//        pVoice->m_pSibling->m_Hardware.Current = CurrentState.Current;
//        pVoice->m_pSibling->m_Hardware.Envelope = CurrentState.Envelope;
//        pVoice->m_pSibling->m_Hardware.Finished = CurrentState.Finished;
//        if (pState->m_RearVol==0)
//        {
            if (pVoice->m_pSibling->m_Hardware.Envelope==0)
            {
#ifdef AUDIO_DEBUG
//                Kprintf("Surround voice killed\n");
#endif
                voice_Free(pVoice->m_pSibling);
                pVoice->m_pSibling = NULL;
            }
            else
            {
                spu_SetKey(pVoice->m_pSibling,0);
//        		g_Miles.UnloadSample(pVoice->m_pSibling->m_SpuCurrent);
            }
//        }
//        else
//        {
//            s32 vol;

//            vol = (pState->m_RearVol * 16383)/AUD_FIXED_POINT_1;
//            if (vol > 16383)
//            {
//                vol = 16383;
//            }
//            pVoice->m_pSibling->m_Hardware.LeftVolume = vol;
//            pVoice->m_pSibling->m_Hardware.RightVolume = (-vol & 0x7fff);
//            pVoice->m_pSibling->m_Hardware.Pitch = pVoice->m_Hardware.Pitch;
//            spu_WriteVoice(pVoice->m_pSibling->m_Id,&pVoice->m_pSibling->m_Hardware);
//        }
    }
    else
    {
//        if (pState->m_RearVol)
//          {
            s32 current;

            current = CurrentState.Current - 128;
            if (current >= pVoice->m_pOwner->m_SpuLocation)
            {
                pVoice->m_pSibling = voice_Alloc(pVoice->m_pOwner);
                if (pVoice->m_pSibling)
                {
//                    s32 vol;

//                    vol = (pState->m_RearVol * 16383)/AUD_FIXED_POINT_1;
//                    if (vol > 16383)
//                    {
//                        vol = 16383;
//                    }
                    pVoice->m_SpuCurrent = pVoice->m_pOwner->m_SpuLocation;
                    spu_InitVoice(pVoice->m_pSibling);
                    pVoice->m_pSibling->m_Hardware.Pitch = pVoice->m_Hardware.Pitch;
//                    pVoice->m_pSibling->m_Hardware.LeftVolume = vol;
//                    pVoice->m_pSibling->m_Hardware.RightVolume = (-vol & 0x7fff);
                    spu_WriteVoice(pVoice->m_pSibling,&pVoice->m_pSibling->m_Hardware);
                    spu_SetKey(pVoice->m_pSibling,1);
//					g_Miles.Play3D(pVoice->m_pSibling->m_SpuCurrent);
#ifdef AUDIO_DEBUG
//                    Kprintf("Surround voice activated\n");
#endif
                }
            }
//        }

    }
#endif


    if (pVoice->m_Hardware.LeftVolume > 16383)
        pVoice->m_Hardware.LeftVolume = 16383;
    if (pVoice->m_Hardware.RightVolume > 16383)
        pVoice->m_Hardware.RightVolume = 16383;
    if (pVoice->m_Hardware.LeftVolume < 0)
        pVoice->m_Hardware.LeftVolume = 0;
    if (pVoice->m_Hardware.RightVolume < 0)
        pVoice->m_Hardware.RightVolume = 0;



#if 0
#ifdef X_DEBUG
    {
        static s32 i=10;

        i--;
        if (i<0)
        {
            spu_state spustate;
            Kprintf("Pan=%d, Left vol=%d, right vol=%d",pState->m_Pan,pVoice->m_Hardware.LeftVolume,pVoice->m_Hardware.RightVolume);
            if (pVoice->m_pSibling)
            {
                spu_ReadVoice(pVoice->m_pSibling->m_Id,&spustate);
                Kprintf(", Rear Volume =%d, delta pos=%d\n",pVoice->m_pSibling->m_Hardware.LeftVolume,CurrentState.Current-spustate.Current);
            }
            else
            {
                Kprintf("\n");
            }
            i=10;
        }
    }
#endif
#endif

    return FALSE;
}



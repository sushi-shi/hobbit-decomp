
#include "x_types.hpp"
#include "audio.hpp"
#include "audiovars.hpp"

extern s32 DebugDisableAudio;
#if defined(X_DEBUG) && defined(bwatson)
//#define AUDIO_DEBUG
#endif
//-----------------------------------------------------------------------------
audio_channel *FindChannel(u32 channelid)
{
    audio_channel *pChannel;
    audio_channel_list *pChannelList;
    s32 index;

    if (channelid==0)
        return NULL;

    index = channelid & AUD_CHANNEL_ID_MASK;

    pChannelList = g_Audio->m_pChannelList;

    while (pChannelList)
    {
        if ( (index >= pChannelList->m_BaseIndex) && (index<pChannelList->m_BaseIndex+AUD_MAX_CHANNELS) )
            break;
        pChannelList = pChannelList->m_pNext;
    }
    if (!pChannelList)
        return NULL;

    pChannel = &pChannelList->m_Channels[index-pChannelList->m_BaseIndex];
    if ( (pChannel->m_State.m_Id == channelid) && (pChannel->m_Flags & AUDFLAG_ALLOCATED) )
        return pChannel;
    else
        return NULL;
}

void    CalcPanAndDistance(vector3 &Position,f32 &Pan,f32 &RearPan,f32 &Distance,vector3 &newpos)
{
    f32 horizlength;
    s32 i;
    view    *pClosest;
    view    *pNextClosest;
    f32     closestlength;
    

    RearPan = 0;
    closestlength   = 100000.0f;
    pClosest        = NULL;
    pNextClosest    = NULL;

    for (i=0;i<g_Audio->m_EarViewCount;i++)
    {
        f32 length;

        newpos = g_Audio->m_pEarView[i]->ConvertW2V(Position);
        length = newpos.Length();
        if (length < closestlength)
        {
            pNextClosest = pClosest;
            pClosest = g_Audio->m_pEarView[i];
            closestlength = length;
        }
    }
    if (!pClosest)
    {
        pClosest = g_Audio->m_pEarView[0];
    }
    ASSERT(pClosest);
    newpos   = pClosest->ConvertW2V(Position);
    Distance = newpos.Length();
    //x_printfxy(0,1,"(%2.2f,%2.2f,%2.2f) dist=%2.2f",newpos.X,newpos.Y,newpos.Z,Distance);

    horizlength = x_sqrt(newpos.X * newpos.X + newpos.Z * newpos.Z);
    if (horizlength < 0.01f) horizlength = 0.01f;
    Pan = -(newpos.X/horizlength);
    if (g_Audio->m_SurroundEnabled)
    {
        f32 v1,v2;

        v1 = -newpos.Z + g_Audio->m_NearClip;
        v2 = g_Audio->m_NearClip+g_Audio->m_FallOff;

        RearPan = v1/v2;

        if (RearPan <0.0f)
            RearPan = 0.0f;
        if (RearPan > 1.0f)
        {
            f32 v1,v2;
            v1 = -newpos.Z - g_Audio->m_FallOff;
            v2 = (g_Audio->m_FarClip * 0.5f) - g_Audio->m_FallOff;
            //x_printfxy(0,3,"%2.2f / %2.2f = %2.2f\n",v1,v2,1.0f - (v1/v2) );
            RearPan = 1.0f - v1/v2;
            if (RearPan < 0.0f)
            {
                RearPan = 0.0f;
            }
        }

    }
    else
    {
        RearPan = 0.0f;
    }
    // If we have multiple views, modify the pan a small amount so we
    // take the other ear view in to account
    if (pNextClosest)
    {
        f32 otherpan,otherrear;
        f32 otherlength;
        f32 otherdistance;

        newpos = pNextClosest->ConvertW2V(Position);
        otherdistance = newpos.Length();

        otherlength = x_sqrt(newpos.X * newpos.X + newpos.Z * newpos.Z);
        if (otherlength < 0.01f) otherlength = 0.01f;

        otherpan = -(newpos.X / otherlength);

        if (g_Audio->m_SurroundEnabled)
        {
            f32 v1,v2;

            v1 = -newpos.Z + g_Audio->m_NearClip;
            v2 = g_Audio->m_NearClip+g_Audio->m_FallOff;

            otherrear = v1/v2;

            if (otherrear <0.0f)
                otherrear = 0.0f;
            if (otherrear > 1.0f)
            {
                f32 v1,v2;
                v1 = -newpos.Z - g_Audio->m_FallOff;
                v2 = (g_Audio->m_FarClip * 0.5f) - g_Audio->m_FallOff;
                otherrear = 1.0f - v1/v2;
                if (otherrear < 0.0f)
                {
                    otherrear = 0.0f;
                }
            }

        }
        else
        {
            otherrear = 0.0f;
        }

        Pan = Pan + ( (otherpan * Distance) / (Distance + otherdistance) );
        RearPan = RearPan + ((otherrear * Distance)/(Distance + otherdistance));

        if (RearPan > 1.0f)
        {
            RearPan = 1.0f;
        }
        if (RearPan < -1.0f)
        {
            RearPan = -1.0f;
        }

        if (Pan > 1.0f)
        {
            Pan = 1.0f;
        }

        if (Pan < -1.0f)
        {
            Pan = -1.0f;
        }
    }
#ifdef TARGET_PC
    // Modify for the right handed coordinate system
    newpos.Set(-1.0f*newpos.X, newpos.Y, -1.0f*newpos.Z);
#endif
}
//
// From the "global" channel volume,pan and pitch; set up the actual volume, 
// pan and pitch in the channel_info structure. This is what will be passed
// through to the IOP.
//
//-----------------------------------------------------------------------------
void audio_UpdateParams(audio_channel *pChannel,f32 Modifier)
{
    f32 RearVolume,FrontVolume;
    f32 Pan,RearPan;
            
    f32             length;
    f32             falloff;
    f32             farclip;
    f32             nearclip;

    falloff = (f32)pChannel->m_pInfo->m_Falloff / (f32)AUD_FIXED_POINT_1;

    ASSERT((falloff>=0.005f) && (falloff <= 2.0f));
    farclip = g_Audio->m_FarClip * falloff;
    nearclip = g_Audio->m_NearClip * falloff;

    if (pChannel->m_Flags & AUDFLAG_PERSISTENT)
    {
        if (pChannel->m_AttachedId)
        {
            audio_SetPosition(pChannel->m_AttachedId,&pChannel->m_Position);
            audio_SetPitch(pChannel->m_AttachedId,pChannel->m_Pitch);
            audio_SetVolume(pChannel->m_AttachedId,pChannel->m_Volume);
        }
    }

    if (pChannel->m_Flags & AUDFLAG_3D_POSITION)
    {
        ASSERT(g_Audio->m_pEarView);

        vector3 newpos;
        CalcPanAndDistance(pChannel->m_Position,Pan,RearPan,length,newpos);
#ifdef TARGET_PC
        pChannel->m_State.m_Position = newpos;
#endif
        //x_printfxy(0,0,"%2.2f",RearPan);
        //RearPan = MAX(-newpos.Z/horizlength,0);
        //RearPan = 0.0f;
        //
        // If we're outside the audio clipping plane, just kill the sound
        //
#ifdef AUDIO_DEBUG
        if (pChannel->m_Flags & AUDFLAG_PERSISTENT)
        {
            //x_printfxy(0,22,"Distance = %2.2f",length);
        }
#endif
        if (length > farclip)
        {
            // If sound is out of range and there is an attached sound id, let's just
            // kill that now and be done with it, but only if it manages to go outside
            // of 110% of the far clip range (to prevent audio start/stop thrashing)
            if (pChannel->m_Flags & AUDFLAG_PERSISTENT)
            {
                if ( (pChannel->m_AttachedId) && (length > farclip * 1.1f) )
                {
#ifdef AUDIO_DEBUG
                    x_DebugMsg("Killed persistent sound %x, attached to %x, id %d\n",pChannel->m_AttachedId,pChannel->m_State.m_Id,pChannel->m_State.m_CfxId);
#endif
                    audio_Stop(pChannel->m_AttachedId);
                    pChannel->m_AttachedId=0;
                }
            }
            // We only clip it, though, when there are more than half the audio
            // channels in use. This allows moving effects to track from outside
            // the audio frustrum to inside.
            if (g_Audio->m_ChannelsInUse > (g_Audio->m_GlobalStatus.m_ChannelCount / 2))
            {
                audio_Stop(pChannel->m_State.m_Id);
            }
            FrontVolume = 0.0f;
            RearVolume = 0.0f;
        }
        else
        {
            if (pChannel->m_Flags & AUDFLAG_PERSISTENT)
            {
                if ( (!pChannel->m_AttachedId) && (length < farclip * 0.9f) )
                {
                    pChannel->m_AttachedId = audio_Play(pChannel->m_State.m_CfxId,&pChannel->m_Position);
                    audio_SetVolume(pChannel->m_AttachedId,pChannel->m_Volume);
                    audio_SetPitch(pChannel->m_AttachedId,pChannel->m_Pitch);
#ifdef AUDIO_DEBUG
                    x_DebugMsg("Started persistent sound %x, attached to %x, id %d\n",pChannel->m_AttachedId,pChannel->m_State.m_Id,pChannel->m_State.m_CfxId);
#endif
                }
                return;
            }

            if (length < nearclip)
            {
                f32 T = (length / nearclip);
                T *= T;
                //Pan = 0.5f + T*(Pan-0.5f);
                Pan = Pan * T;
                RearPan = T*RearPan;
            }
            {
                f32 p1,p2;
                if (length > farclip )
                {
                    FrontVolume = 0.0f;
                }
                else
                {
                    p1 = farclip - length;
                    p2 = farclip;
                    FrontVolume = p1/p2;
                }

            }
//            RearVolume = 0.0f;
//            RearPan = 0;
        }
#if 0
        if (length < 100)
            x_printfxy(0,pChannel->m_State.m_CfxId & 0x0f,"cfx=%d, Pan = %2.2f,Dist=%2.2f",pChannel->m_State.m_CfxId,Pan,length);
#endif
        // Square it so we have a more environmentally correct falloff curve.
        ASSERT( (RearPan >=0.0f) && (RearPan <= 1.0f) );
        FrontVolume = FrontVolume * FrontVolume;
        RearVolume = FrontVolume * RearPan;
        FrontVolume = FrontVolume * (1.0f - RearPan);
    }
    else
    {
        if (pChannel->m_Flags & AUDFLAG_LOCALIZED)
        {
		    pChannel->m_State.m_Volume   = (s16)(pChannel->m_Volume * AUD_FIXED_POINT_1 * Modifier);
            audio_QueueUpdateRequest(pChannel);
            return;
        }
        RearPan = pChannel->m_RearPan;
        Pan = pChannel->m_Pan;

        FrontVolume = pChannel->m_Volume * (1.0f-RearPan); 
        RearVolume = FrontVolume * RearPan;

    }


    RearVolume = pChannel->m_Volume * RearVolume;

#ifdef TARGET_PS2
    pChannel->m_State.m_Pitch    = (s16)(pChannel->m_Pitch * AUD_FIXED_POINT_1);
    pChannel->m_State.m_Volume   = (s16)(FrontVolume * pChannel->m_Volume * AUD_FIXED_POINT_1 * Modifier);
    pChannel->m_State.m_Pan      = (s16)(Pan * AUD_FIXED_POINT_1);
	pChannel->m_State.m_Flags	 = pChannel->m_Flags;
    if (g_Audio->m_SurroundEnabled)
    {
        pChannel->m_State.m_RearVol  = (s16)(RearVolume * AUD_FIXED_POINT_1 * Modifier);

    }
    else
        pChannel->m_State.m_RearVol  = 0;
#endif
#ifdef TARGET_PC
    pChannel->m_State.m_Volume   = (s16)(FrontVolume * pChannel->m_Volume * AUD_FIXED_POINT_1 * Modifier);
#endif
    audio_QueueUpdateRequest(pChannel);

}

    // Allocates a channel. Doesn't start it but makes it persistent.
//-----------------------------------------------------------------------------
u32     audio_Allocate(void)
{
    s32                 index;
    audio_channel_list *pChannelList,*pChannelLast;
    audio_channel       *pChannel;
    s32                 first,count;

    audio_LockChannelList();

    pChannelList = g_Audio->m_pChannelList;

    while (pChannelList)
    {
        if (pChannelList->m_Available)
            break;
        pChannelList = pChannelList->m_pNext;
    }

    // If we have no channel list, we just add another one to the list of channels
    if (!pChannelList)
    {
        pChannelList = (audio_channel_list *)x_malloc(sizeof(audio_channel_list));
        if (!pChannelList)
        {
            audio_UnlockChannelList();
            return 0;
        }
        x_memset(pChannelList,0,sizeof(audio_channel_list));
#ifdef AUDIO_DEBUG
        if (g_Audio->m_pChannelList)
            x_DebugMsg("audio_Allocate: Had to extend channel list\n");
#endif

        pChannelList->m_Available       = AUD_MAX_CHANNELS;
        pChannelList->m_InUse           = 0;
        pChannelList->m_LastAllocated   = 0;
        pChannelList->m_pNext           = NULL;
        // Find the end of the list and add this channel block to it.
        pChannelLast = g_Audio->m_pChannelList;

        if (!pChannelLast)
        {
            pChannelList->m_BaseIndex   = 0;
            g_Audio->m_pChannelList = pChannelList;

        }
        else
        {
            while (pChannelLast->m_pNext)
            {
                pChannelLast = pChannelLast->m_pNext;
            }
            ASSERT(pChannelLast);
            ASSERT(pChannelLast->m_pNext == NULL);
            pChannelList->m_BaseIndex = pChannelLast->m_BaseIndex + AUD_MAX_CHANNELS;
            pChannelLast->m_pNext = pChannelList;
        }
    }

    first = pChannelList->m_LastAllocated;
    index = first;
    while (pChannelList->m_Channels[index].m_Flags & AUDFLAG_ALLOCATED)
    {
        index++;
        if (index == AUD_MAX_CHANNELS)
        {
            index=0;
            g_Audio->m_ChannelLoopId++;
            if (g_Audio->m_ChannelLoopId > 65535)
            {
                g_Audio->m_ChannelLoopId = 1;
                g_Audio->m_UniqueId++;
            }
        }
        // Even though the channel list block showed we had an available channel, all
        // of the enclosed channels have their allocated flags set. This is bad.
        ASSERT(index != first);
    }

    pChannel = &pChannelList->m_Channels[index];

    pChannelList->m_Available--;
    pChannelList->m_InUse++;

    ASSERT(pChannelList->m_Available+pChannelList->m_InUse == AUD_MAX_CHANNELS);
    pChannel->m_Flags           = AUDFLAG_ALLOCATED;
    pChannel->m_State.m_Id      = (g_Audio->m_ChannelLoopId << AUD_CHANNEL_ID_BITS) | (index+pChannelList->m_BaseIndex);
    pChannel->m_State.m_UniqueId= g_Audio->m_UniqueId;
    pChannel->m_AttachedId      = 0;

	ASSERT(pChannel->m_State.m_Id != 0);
    index++;
    if (index == AUD_MAX_CHANNELS)
    {
        index = 0;
        g_Audio->m_ChannelLoopId++;
        if (g_Audio->m_ChannelLoopId > 65535)
        {
            g_Audio->m_ChannelLoopId = 1;
            g_Audio->m_UniqueId++;
        }
    }
    pChannelList->m_LastAllocated = index;
    count = 0;
    for (index=0;index<AUD_MAX_CHANNELS;index++)
    {
        if (pChannelList->m_Channels[index].m_Flags & AUDFLAG_ALLOCATED)
        {
            count++;
        }
    }
    ASSERT(count==pChannelList->m_InUse);
    audio_UnlockChannelList();
	//x_DebugMsg("audio_Allocate: Channel ID 0x%08x allocated\n",pChannel->m_State.m_Id);
    return pChannel->m_State.m_Id;
}
                     
//-----------------------------------------------------------------------------
void    audio_Free(u32 ChannelId)
{
/*
	cfx *pCfx;
	pCfx = g_pcAudio.m_pRootCfx;

	while (pCfx)
	{
		if (pCfx->m_State.m_Id == ChannelId)
		{
			cfx_state State;
			State = pCfx->m_State;
			
			// Kill the sound.
			pCfx->m_State.m_CfxId = -1;
//			State.m_CfxId = -1;
//			cfx_UpdateState(pCfx, &State);
		}
		pCfx = pCfx->m_pNext;
	}
*/

	audio_channel_list  *pList,*pPrev;
    audio_channel       *pChannel;
    s32                 Index;

    pList = g_Audio->m_pChannelList;
    Index = ChannelId & AUD_CHANNEL_ID_MASK;
	pPrev = NULL;
    while (pList)
    {
        if ( (Index >= pList->m_BaseIndex) && (Index < pList->m_BaseIndex+AUD_MAX_CHANNELS) )
        {
            pChannel = &pList->m_Channels[Index-pList->m_BaseIndex];
            break;
        }
		pPrev = pList;
        pList = pList->m_pNext;
    }

    if ( pList && (pChannel->m_State.m_Id == ChannelId) && (pChannel->m_Flags & AUDFLAG_ALLOCATED) )
    {
		//x_DebugMsg("audio_Free: Channel id 0x%08x released\n",pChannel->m_State.m_Id);
		pChannel->m_State.m_Id = 0;
        pChannel->m_Flags = AUDFLAG_FREE;
		pChannel->m_State.m_CfxId = -1;
        pList->m_InUse--;
        pList->m_Available++;
        ASSERT(pList->m_InUse + pList->m_Available == AUD_MAX_CHANNELS);
#if 0
		if (pList->m_Available == AUD_MAX_CHANNELS)
		{
			if (pPrev)
			{
				pPrev->m_pNext = pList->m_pNext;
			}
			else
			{
				g_Audio->m_pChannelList = pList->m_pNext;
			}
			x_DebugMsg("audio_Free: Released channel list block\n");
			x_free(pList);
		}
#endif
    }
	{
		//x_DebugMsg("audio_Free: Channel id 0x%08x freed but was already free\n",pChannel->m_State.m_Id);
	}
}

//-----------------------------------------------------------------------------
s32     audio_Play(u32 SampleId)
{
    audio_channel *pChannel;
    s32     id;

    if (DebugDisableAudio)
        return 0;

    if ( (SampleId==0) || (SampleId == 0xffffffff) )
        return 0;
  
    id = audio_Allocate();
    if (id==0)
        return 0;
    pChannel = FindChannel(id);
    if (!pChannel) return 0;
    pChannel->m_State.m_CfxId   = SampleId;
    pChannel->m_pInfo           = audio_FindInfo(SampleId);
    pChannel->m_Volume          = 1.0f;
    pChannel->m_Pan             = 0.5f;
    pChannel->m_RearPan         = 0.0f;           // Set to below zero to disable surround support
    pChannel->m_Pitch           = 1.0f;
    pChannel->m_Status          = AUDSTAT_STARTING;

    ASSERTS(pChannel->m_pInfo,"audio_Play: Attempt to play a sample from a container that isn't loaded");
    
    // We fall in to a potential problem here 
    audio_QueueUpdateRequest(FindChannel(id));
    
    vector3 Position(0,0,0);
#ifndef TARGET_PC
    f32 Pan, RearPan, length;
    
    // All 2d sounds are 3d sounds that don't move.
    vector3 newpos;

    CalcPanAndDistance(Position,Pan,RearPan,length,newpos);
#endif
#ifdef TARGET_PC
    pChannel->m_State.m_Position = Position;
    // Pass the flag down to the bottom layer.
    pChannel->m_State.m_Flags = pChannel->m_Flags;
#endif   
    return id;
}

s32		audio_Play(u32 SampleId, s32 Flags)
{
	s32 id = audio_Play(SampleId);
	audio_SetFlags(id,Flags);
	return id;
}

//-----------------------------------------------------------------------------
s32     audio_Play(u32 SampleId, vector3 *pPosition)
{


    f32 Pan,RearPan,Distance;
	s32 ChannelId;
    vector3 newPos;

    CalcPanAndDistance(*pPosition,Pan,RearPan,Distance,newPos);
    if (Distance > g_Audio->m_FarClip)
    {
        return -1;
    }
    audio_channel *pChannel;

    ChannelId = audio_Play(SampleId) ;
    
    pChannel = FindChannel(ChannelId);
    if (!pChannel) 
        return 0;

	audio_SetPosition(ChannelId, pPosition) ;

    // Get the position of the sound that is relative to the player.
#ifdef TARGET_PC
    pChannel->m_State.m_Position = newPos;
    // Pass the flag down to the bottom layer.
    pChannel->m_State.m_Flags = pChannel->m_Flags;
#endif
    
	return ChannelId ;
}

//-----------------------------------------------------------------------------
s32     audio_Play(u32 SampleId, vector3 *pPosition,s32 Flags)
{
	s32 ChannelId;
    audio_channel *pChannel;
    
	if (Flags & AUDFLAG_PERSISTENT)
	{
#ifdef AUDIO_DEBUG
		x_DebugMsg("Persistent sound started, cfx id = %08x\n",SampleId);
#endif
	}
    if (Flags & AUDFLAG_LOOPED)
    {
        Flags = Flags;
    }

    if (Flags & AUDFLAG_HARDWARE_LOOP)
    {
        Flags = Flags;
    }
    
    if (DebugDisableAudio || (SampleId==0))
        return 0;
#if 0  
    if (Flags & AUDFLAG_PERSISTENT)
    {
        x_DebugMsg("Persistent started\n");
    
    }
#endif

    if (Flags & AUDFLAG_QUEUED)
    {
        if (Flags & AUDFLAG_CHANNELSAVER)
        {
            // Purge all queued delayed samples and make sure current
            // one is stopped
            while (g_Audio->m_pqDelayedSamples->Recv(MQ_NOBLOCK));
            if (g_Audio->m_DelayedChannelId)
            {
                audio_Stop(g_Audio->m_DelayedChannelId);
                g_Audio->m_DelayedChannelId=0;
            }
            Flags &= ~AUDFLAG_CHANNELSAVER;

        }
        g_Audio->m_pqDelayedSamples->Send((void *)SampleId,MQ_NOBLOCK);
        return 0;
    }
 
    ChannelId = audio_Play(SampleId) ;

    if (Flags & AUDFLAG_DEFER_PLAY)
    {
        audio_SetPitch(ChannelId,0.0f);
        Flags &= ~ AUDFLAG_DEFER_PLAY;
    }
    // We cannot use audio_SetPosition here as that should never be used
    // for a sound that can have the AUDIO_NO_POSITION_UPDATE flag set.
    pChannel = FindChannel(ChannelId);
    if (!pChannel) return 0;

    pChannel->m_Position = *pPosition;
    pChannel->m_Flags |=(AUDFLAG_3D_POSITION|Flags);

#ifdef TARGET_PC
    pChannel->m_State.m_Position = *pPosition;
    pChannel->m_State.m_Flags = pChannel->m_Flags;
#endif

	return ChannelId ;
}

//-----------------------------------------------------------------------------
void    audio_StopAll(void)
{
    audio_channel *pChannel;
    audio_channel_list *pChannelList;
    s32 i;

    // Empty out all the delayed samples in the queue
    while (g_Audio->m_pqDelayedSamples->Recv(MQ_NOBLOCK));

    // Make sure any streamed sounds are stopped
    if (g_Audio->m_DelayedChannelId)
    {
        audio_Stop(g_Audio->m_DelayedChannelId);
        g_Audio->m_DelayedChannelId=0;
    }

    // Now go through all the "active" channels in the channel list and
    // stop all those sounds currently marked down as playing. This will
    // not affect spot effects unfortunately.
    pChannelList = g_Audio->m_pChannelList;
    while (pChannelList)
    {
        pChannel = pChannelList->m_Channels;
        for (i=0;i<AUD_MAX_CHANNELS;i++)
        {
            if ( (pChannel->m_Flags & AUDFLAG_ALLOCATED) &&
                 (pChannel->m_Status != AUDSTAT_STOPPED) )
            {
                audio_Stop(pChannel->m_State.m_Id);
            }
            pChannel++;
        }
        pChannelList = pChannelList->m_pNext;
    }
}

//-----------------------------------------------------------------------------
void    audio_Stop(u32 ChannelId)
{
    audio_channel *pChannel;

    if (DebugDisableAudio)
        return;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) 
    {
        return;
    }

	//x_DebugMsg("audio_Stop: Stopping channel id 0x%08x, cfx id %d\n",pChannel->m_State.m_Id,pChannel->m_State.m_CfxId);
    if ( pChannel->m_Flags & AUDFLAG_PERSISTENT )
    {
        // Stop any attached sound to this channel. We free the channel
        // immediately because this particular channel will never be passed
        // to the IOP since it's a persistent sound. Only it's attached
        // channel will be sent.
        if (pChannel->m_AttachedId)
        {
			audio_channel *pCh;

			pCh = FindChannel(pChannel->m_AttachedId);
			if (pCh)
			{
				ASSERT((pCh->m_Flags & AUDFLAG_PERSISTENT)==0);
			}
			//x_DebugMsg("audio_Stop: Stopping attached id 0x%08x\n",pChannel->m_AttachedId);
            audio_Stop(pChannel->m_AttachedId);
            pChannel->m_AttachedId=0;
        }

		//x_DebugMsg("audio_Stop: Freeing channel id 0x%08x\n",pChannel->m_State.m_Id);
        audio_Free(pChannel->m_State.m_Id);
        return;
    }

    // Make sure the IOP tries to stop this sound.
    pChannel->m_Status = AUDSTAT_STOPPED;
    pChannel->m_State.m_CfxId = -1;

    //audio_QueueUpdateRequest(pChannel);

}

//-----------------------------------------------------------------------------
void    audio_Pause(u32 ChannelId)
{
    audio_channel *pChannel;

    if (DebugDisableAudio)
        return;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;
    pChannel->m_Status = AUDSTAT_PAUSED;
}

//-----------------------------------------------------------------------------
void    audio_SetVolume(u32 ChannelId,f32 Volume)
{
    audio_channel *pChannel;

    if (DebugDisableAudio)
        return;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;
    pChannel->m_Volume = Volume;
}

//-----------------------------------------------------------------------------
void    audio_SetPan(u32 ChannelId,f32 Pan)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;
    pChannel->m_Pan = Pan;
}

//-----------------------------------------------------------------------------
void    audio_SetRearPan(u32 ChannelId,f32 Pan)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;
    pChannel->m_RearPan = Pan;
}

//-----------------------------------------------------------------------------
void    audio_SetPitch(u32 ChannelId,f32 Pitch)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;
    pChannel->m_Pitch = Pitch;
}

//-----------------------------------------------------------------------------
void    audio_SetPosition(u32 ChannelId,vector3 *pPosition)
{
    audio_channel *pChannel;
    if (DebugDisableAudio)
        return;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;
    ASSERT(!(pChannel->m_Flags & AUDFLAG_LOCALIZED));
    pChannel->m_Position = *pPosition;
    pChannel->m_Flags |=AUDFLAG_3D_POSITION;
}

//-----------------------------------------------------------------------------
void    audio_SetFlags(u32 ChannelId,s32 Flags)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return;

    // Even though we really want users to modify the flags as they see fit, there
    // are some internal flags that should not ever be modified by the user
    pChannel->m_Flags &= AUDFLAG_PRIVATE_MASK;
    pChannel->m_Flags |= Flags;

}

//-----------------------------------------------------------------------------
s32     audio_GetStatus(u32 ChannelId)
{
    audio_channel *pChannel;

    if (ChannelId == (u32)-1)
    {
        if (g_Audio->m_pqDelayedSamples->IsEmpty())
        {
            ChannelId = g_Audio->m_DelayedChannelId;
        }
        else
        {
            return AUDSTAT_PLAYING;
        }
    }
    pChannel = FindChannel(ChannelId);
    if (!pChannel) return AUDSTAT_FREE;
    return pChannel->m_Status;
}

//-----------------------------------------------------------------------------
f32     audio_GetVolume(u32 ChannelId)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return 0.0f;
    return pChannel->m_Volume;
}

//-----------------------------------------------------------------------------
f32     audio_GetPan(u32 ChannelId)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return 0.0f;
    return pChannel->m_Pan;
}

//-----------------------------------------------------------------------------
f32     audio_GetRearPan(u32 ChannelId)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return 0;
    return pChannel->m_RearPan;
}

//-----------------------------------------------------------------------------
f32     audio_GetPitch(u32 ChannelId)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return 0.0f;
    return pChannel->m_Pitch;
}


//-----------------------------------------------------------------------------
xbool   audio_IsPlaying(u32 ChannelId)
{
    return FindChannel(ChannelId)!=NULL;
}

//==============================================================================
void audio_SetLooping(u32 ChannelId)
{
    audio_channel *pChannel;

    pChannel = FindChannel(ChannelId);
    if (!pChannel) return ;
#ifdef TARGET_PC
//    pChannel->m_State.m_Flags |= AUDFLAG_LOOPED;
#endif
}
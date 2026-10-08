#include "Miles.h"
#include "entropy.hpp"
#include "x_stdio.hpp"


#define PROVIDER                "DirectSound3D 7+ Software - Pan and Volume"

// Internal functions
//==============================================================================
void Audio::Set_Room(s32 value, s32 index)
{
  if ((m_hOpenedProvider) && (usingEAX))
  {
    AIL_set_3D_room_type(m_hOpenedProvider,value);

    // turn off EAX, if they choose no reverb
    if (value==ENVIRONMENT_GENERIC)
      AIL_set_3D_sample_effects_level(m_p3DSample[index].m_hVoice, 0.0F);
    else
    {
      // -1 is the provider default level
      AIL_set_3D_sample_effects_level(m_p3DSample[index].m_hVoice, -1.0F);
    }
  }
}

//==============================================================================
bool Audio::Load_Provider(void)
{
	HPROVIDER   hProviders[MAX_PROVIDERS];
	char*		names[MAX_PROVIDERS];
	DWORD		result;
	u32			count = 0;
	HPROENUM	enum3D = HPROENUM_FIRST;

	// Go through all the provider and find the one that is requested.
	while (AIL_enumerate_3D_providers( &enum3D, &hProviders[count], &names[count] ) )
	{  
		if (strcmp(names[count], PROVIDER) == 0 )
		{    
			m_hOpenedProvider = hProviders[count];
			curProviderIndex = count;
			break;
		}
		
		count++;
	}
	
	if ( curProviderIndex == -1)
	{
		return FALSE;
	}

	// Load the new provider.
	result = AIL_open_3D_provider(m_hOpenedProvider);
	if (result != M3D_NOERR) 
	{
		curProviderIndex =- 1;
        ASSERTS(FALSE, "Cannot open the 3D driver which is specified.");
		return FALSE;
	}

	//see if we're running under an EAX compatible provider
	result=AIL_3D_room_type(m_hOpenedProvider);
	usingEAX=(((S32)result)!=-1)?1:0; // will be something other than -1 on EAX


	return TRUE;
}

//==============================================================================
void Audio::Set_SampleValues( s32 index, f32 dX, f32 dY, f32 dZ)
{
	// Set new position.
	if (m_p3DSample[index].m_hVoice)
		AIL_set_3D_position(m_p3DSample[index].m_hVoice, dX, dY, dZ);
	
/*    // Face the Origin.
    vX=-X;
    vY=-Y;
    vZ=-Z;	
	
	AIL_set_3D_orientation(g_hOpenedSample[index], 0.0F, 0.0F, -1.0F, 0.0F, 1.0F, 0.0F);

	X += dX;
	Y += dY;
	Z += dZ;
*/
/*
	f32 magnitude, velX, velY, velZ, oldX, oldY, oldZ;

	// Do we really need all these extra calculations???
	oldX = X;
	oldY = Y;
	oldZ = Z;

	X += dX;
	Y += dY;
	Z += dZ;

	velX = X - dX;
	velY = Y - dY;
	velZ = Z - dZ;

	// Yuch to expensive.
	magnitude = (float)sqrt((float)abs(velX*velX) + (float)abs(velY*velY) + (float)abs(velZ*velZ));

	// set the velocity for doppler effects
	AIL_set_3D_velocity(g_hOpenedSample[index], velX, velY, velZ, magnitude);
//*/
}

//==============================================================================
s32 Audio::Set_Sample(s32 index, void* vpSampleData, s8 Flags)
{
	// Get a 3D sample handle.
	m_p3DSample[index].m_hVoice= AIL_allocate_3D_sample_handle(m_hOpenedProvider);
    m_p3DSample[index].m_flags = Flags;

	// Load the sample into the provider.
	if (AIL_set_3D_sample_file( m_p3DSample[index].m_hVoice, vpSampleData) == 0) 
	{
		return NO_FILE_LOADED;
	}

	// Set the sample to loop and set the distances, by default the loop count is 1.
	AIL_set_3D_sample_loop_count(m_p3DSample[index].m_hVoice, LOOP_COUNT);

    if (Flags == POSITIONAL_SOUND)
    {
	    AIL_set_3D_sample_distances(m_p3DSample[index].m_hVoice, FAR_CLIP, NEAR_CLIP);
	    AIL_set_3D_sample_cone(m_p3DSample[index].m_hVoice, INNER_CONE, OUTER_CONE, INNER_TO_OUTER_SCALE);

        // We may want to turn this on!!!.
        // This creates an extra thread that updates the position of the sample.
#ifdef AUDIO_AUTO_UPDATE
        AIL_auto_update_3D_position(m_p3DSample[index].m_hVoice, 1);
#endif
    }
    else
    {
	    AIL_set_3D_sample_distances(m_p3DSample[index].m_hVoice, FAR_CLIP, NEAR_CLIP);
	    AIL_set_3D_sample_cone(m_p3DSample[index].m_hVoice, INNER_CONE, OUTER_CONE, 127);
    }

	// Set the speaker types.
	AIL_set_3D_speaker_type(m_hOpenedProvider, SPEAKER_TYPE);

    AIL_set_3D_sample_distances(m_p3DSample[index].m_hVoice, m_FarClip, m_NearClip);

	Set_Room( ENVIRONMENT, index);

	return LOAD_COMPLETE;
}

//==============================================================================
void Audio::Release_3DSample( s32 index )
{
	// Is it even open?
	if (m_p3DSample[index].m_hVoice)
	{
		AIL_end_3D_sample(m_p3DSample[index].m_hVoice);
		AIL_release_3D_sample_handle(m_p3DSample[index].m_hVoice);
		
        // Clean the sample completely.
        m_p3DSample[index].m_hVoice = 0;
        m_p3DSample[index].position.Zero();
        m_p3DSample[index].m_ParentIndex = -1;
        m_p3DSample[index].m_volume = 0;
       
        // Sample isn't getting cleaned out right.
        ASSERT(!m_p3DSample[index].m_hVoice);

		m_3DSoundCount--;

        // Make sure that we don't try to release something that is already released.
        ASSERT(m_3DSoundCount >= 0);
	}
}

//==============================================================================
void Audio::Release_Stream( s32 index )
{
   if (m_pStreamSample[index].m_hStream)
   {
		AIL_close_stream( m_pStreamSample[index].m_hStream );
		m_pStreamSample[index].m_hStream = 0;
   
		m_StreamCount--;
   }
}

//==============================================================================
void Audio::Sound_Kill(void)
{
	s32 i;			

	// Release the 3D sounds.
    for (i = 0; i < MAX_SOUNDS; i++)
	{
		Release_3DSample(i);
	}
    // Release the stream.
	for (i = 0; i < MAX_STREAMS; i++)
	{
		Release_Stream(i);
	}
    // Close the provider.
	if (m_hOpenedProvider) 
	{
	    AIL_close_3D_provider(m_hOpenedProvider);
	    m_hOpenedProvider = 0;
	}
	
	if (m_vpEffects)
		delete [] m_vpEffects;

	AIL_close_digital_driver(DIG);
	AIL_shutdown();

	m_StreamCount = m_3DSoundCount = 0;
}


//==============================================================================
s32 Audio::Load_3DAudio(void* pData, s32 index, s8 Flags)
{
//	AIL_mem_free_lock(m_p3DSample[index].m_vpVoiceData);
//	g_p3DSample[index].m_vpVoiceData = pData;
	s32 check = Set_Sample(index, pData, Flags);
    
    if (check >= 0)
        m_3DSoundCount++;

	return check;
}

//==============================================================================
s32 Audio::Load_StreamSample(char* pString, s32 index)
{
	// Open the digital stream.
	m_pStreamSample[index].m_hStream = AIL_open_stream( DIG, pString, 0 );

	if (m_pStreamSample[index].m_hStream == NULL)
		return NO_FILE_LOADED;

	AIL_set_stream_loop_count(m_pStreamSample[index].m_hStream, STREAM_LOOP_COUNT);

	// Store the original stream playback rate in user data slot 0
	if (m_pStreamSample[index].m_hStream)
	{
		AIL_set_stream_user_data( m_pStreamSample[index].m_hStream, 0, 
								AIL_stream_playback_rate( m_pStreamSample[index].m_hStream ) );
	}

	m_StreamCount++;
	return LOAD_COMPLETE;
}

//==============================================================================
void *Audio::Load_File3D(char* pString)
{
	void* tempStore;
	void* data;
	s32 type;
	AILSOUNDINFO soundInfo;
	s32 size;
	s32	length = strlen(pString);

	char string[60] = {0};
	
	// Compare the marks.
	char slash[]= {"\\"};
	s32 i = 0;

	for (i = 0; i < length; i++)
	{
		if (pString[i] == slash[0])
			break;
	}

	char name[30];

	strcpy(name, &pString[i]);

	char audio[] = {"AUDIO\\"};

	// Even thought this is a hack, it is much faster then dynamically allocation string every where.
	if ( pString[0] == 'w' || pString[0] == 'W')
	{
		char foldername [] = {"WEAPONS"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}
	else if ( pString[0] == 'v' || pString[0] == 'V')
	{
		if (pString[1] == 'o' || pString[1] == 'O')
		{
			char foldername [] = {"VOICE"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
		else
		{
		    char foldername [] = {"VEHICLES"};	
		    x_sprintf(string,"%s%s%s",audio, foldername, name);

		}

	}
	else if ( pString[0] == 'e' || pString[0] == 'E')
	{
		char foldername	[] = {"EXPLOSIONS"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}

	else if ( pString[0] == 't' || pString[0] == 'T')
	{
		char foldername [] = {"TURRETS"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}
	else if ( pString[0] == 'a' || pString[0] == 'A')
	{
		char foldername [] = {"ARMOR"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}

	else if ( pString[0] == 'f' || pString[0] == 'F')
	{
		char foldername [] = {"FRONTEND"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}

	else if ( pString[0] == 'm' || pString[0] == 'M')
	{
		if (pString[1] == 'i' || pString[1] == 'I')
		{
			char foldername [] = {"MISC"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
		else
		{
			char foldername [] = {"MUSIC"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
  
	}

	else if ( pString[0] == 'p' || pString[0] == 'P')
	{
		if (pString[1] == 'o' || pString[1] == 'O')
		{
			char foldername [] = {"POWER"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
		else
		{
			char foldername [] = {"PACKS"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
	}
	size = AIL_file_size(string);
	tempStore = AIL_file_read(string,0);

	// Check if there was anything read.
	if ( tempStore == NULL )
	{
		return NULL;
	}

	type = AIL_file_type(tempStore,size);

	// Delegate the files.
	switch (type)
	{
		case AILFILETYPE_PCM_WAV:		// 3D sound.
		{	
			// Check if the container is full.
			if (m_3DDataCount >= MAX_DATA_COUNT)
			{
				AIL_mem_free_lock(tempStore);
				return NULL;
			}
						
			ASSERTS(tempStore,"Audio data is empyt");
			

			return tempStore;
		}

		case AILFILETYPE_ADPCM_WAV:		// 3D Sound.
		{
			
			// Check if the container is full.
			if (m_3DDataCount >= MAX_DATA_COUNT)
			{
				AIL_mem_free_lock(tempStore);
				return NULL;
			}
			
			AIL_WAV_info(tempStore,&soundInfo);
			AIL_decompress_ADPCM(&soundInfo,&data,0);
			AIL_mem_free_lock(tempStore);			

			ASSERTS(data,"Audio data is empyt");
		
						
			return data;
		}
        case AILFILETYPE_MPEG_L3_AUDIO: // Voices.
        {

			// Check if the container is full.
			if (m_3DDataCount >= MAX_DATA_COUNT)
			{
				AIL_mem_free_lock(tempStore);
				return NULL;
			}
              
            AIL_decompress_ASI(tempStore,size,string,&data,0,0);
            AIL_mem_free_lock(tempStore);

            ASSERTS(data,"Audio data is empyt");

            return data;
        }
		default:
		{
			AIL_mem_free_lock(tempStore);
			return NULL;
		}
   }

}

//==============================================================================
s32 Audio::LoadStream(char* pString, s32 Loop)
{
	s32	length = strlen(pString);

	char string[60] = {0};
	
	// Compare the marks.
	char slash[]= {"\\"};
	s32 i = 0;

	for (i = 0; i < length; i++)
	{
		if (pString[i] == slash[0])
			break;
	}

	char name[30];

	strcpy(name, &pString[i]);

	char audio[] = {"AUDIO\\"};

	// Even thought this is a hack, it is much faster then dynamically allocation string every where.
	if ( pString[0] == 'w' || pString[0] == 'W')
	{
		char foldername [] = {"WEAPONS"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}
	
	else if ( pString[0] == 'v' || pString[0] == 'V')
	{
		if (pString[1] == 'o' || pString[1] == 'O')
		{
			char foldername [] = {"VOICE"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
		else
		{
		    char foldername [] = {"VEHICLES"};	
		    x_sprintf(string,"%s%s%s",audio, foldername, name);

		}

	}

	else if ( pString[0] == 'e' || pString[0] == 'E')
	{
		char foldername	[] = {"EXPLOSIONS"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}

	else if ( pString[0] == 't' || pString[0] == 'T')
	{
		char foldername [] = {"TURRETS"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}
	else if ( pString[0] == 'a' || pString[0] == 'A')
	{
		char foldername [] = {"ARMOR"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}

	else if ( pString[0] == 'f' || pString[0] == 'F')
	{
		char foldername [] = {"FRONTEND"};	
		x_sprintf(string,"%s%s%s",audio, foldername, name);

	}

	else if ( pString[0] == 'm' || pString[0] == 'M')
	{
		if (pString[1] == 'i' || pString[1] == 'I')
		{
			char foldername [] = {"MISC"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
		else
		{
			char foldername [] = {"MUSIC"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
  
	}

	else if ( pString[0] == 'p' || pString[0] == 'P')
	{
		if (pString[1] == 'o' || pString[1] == 'O')
		{
			char foldername [] = {"POWER"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
		else
		{
			char foldername [] = {"PACKS"};	
			x_sprintf(string,"%s%s%s",audio, foldername, name);

		}
	}
	
	if (m_StreamCount >= MAX_STREAMS)
	{
		return STREAM_BUFFER_FULL;
	}

	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (!m_pStreamSample[i].m_hStream)
		{
			break;
		}
	}			
	
	// Get the name of the file loaded.
	memcpy(m_pStreamSample[i].m_name, string, sizeof(string));
	
	if (Load_StreamSample(string, i) != LOAD_COMPLETE)
		return NO_FILE_LOADED;

    // Set the looping, its either 0 (Infinite) or 1.
    SetStreamLoopCount(i, Loop);

    return i;
}

//==============================================================================
s32 Audio::Init(void)
{
    // Clear out the any fields.
    curProviderIndex = -1; 
    usingEAX = 0;
    m_hOpenedProvider = 0; 
    m_StreamCount = m_3DSoundCount= 0;

    // Initialize the Miles Sound System
	char *temp = AIL_set_redist_directory("..\\Demo1\\Data\\Miles6\\redist\\" MSS_REDIST_DIR_NAME);
	
	AIL_startup();

	DIG = AIL_open_digital_driver( DIGITALRATE, DIGITALBITS, DIGITALCHANNELS, 0 );

	if (DIG == NULL)
		return NO_DRIVER_LOADED;

	if (!Load_Provider())
		return NO_PROVIDER_LOADED;

	for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
		m_p3DSample[i].position.Zero();
    }

	m_vpEffects = NULL;

    // Set the volume level to 1.0 for all the types.
    m_MasterVolume = m_EffectVolume = m_MusicVolume = m_VoiceVolume = 1.0;

    // Set the sound level to max.
//    AIL_set_preference(DIG_DEFAULT_VOLUME, 127);

	MAX_DATA_COUNT = 0;

    m_NearClip = NEAR_CLIP;
    m_FarClip = FAR_CLIP;

    m_Pad = 0;

//    char data[128];
//    AIL_digital_configuration(DIG, NULL, NULL, data);
		
	return LOAD_COMPLETE;
}

//==============================================================================
void Audio::Kill(void)
{
	Sound_Kill();
}

//==============================================================================
void Audio::UnloadSample(s32 SampleId, s32 UniqueId)
{
	// Is it a valid Id.
    if (SampleId < MAX_DATA_COUNT)
	{
            m_pIndex[SampleId].m_Status = KILL_SOUND;
	}
    
    // Check all the sounds that are playing with the sampleId.
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
        if (m_3DSoundCount == 0)
            return;
	
        if (m_p3DSample[i].m_hVoice)
		{
	        if (m_p3DSample[i].m_ParentIndex == SampleId && m_p3DSample[i].m_UniqueId == UniqueId)
            {
                m_pIndex[SampleId].m_PositionIndex--;
                Release_3DSample(i);
                
                // There is only one sample to release so get out.
                return;   
            }
		}
    }
}

//==============================================================================
void Audio::UnloadStream(s32 SampleId)
{
    if (SampleId < MAX_STREAMS)
    {
        Release_Stream(SampleId);
    }
}

//==============================================================================
void Audio::SetSampleVolume(s32 SampleId, s32 UniqueId, f32 Volume)
{    
    Volume = Volume/78;

    
    // Max volume in Miles.
    if (Volume > 127)
        Volume = 127;
    else if ( Volume < 0)
        Volume = 0;

    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
        if (m_p3DSample[i].m_hVoice)
        {
            if (m_p3DSample[i].m_ParentIndex == SampleId && m_p3DSample[i].m_UniqueId == UniqueId)
            {
                // This is going to set the max volume for the sample.  Include the padding to the max volume.
                if (m_p3DSample[i].m_volume == 0)
                    m_p3DSample[i].m_volume = Volume + m_Pad;
                    
                // Cap the volume.
                if (Volume > m_p3DSample[i].m_volume)
                    Volume = m_p3DSample[i].m_volume;

                AIL_set_3D_sample_volume( m_p3DSample[i].m_hVoice, (s32)Volume);
                return;
            }
        }
    }
}

//==============================================================================
f32 Audio::GetSampleVolume(s32 SampleId, s32 UniqueId)
{
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
        if (m_p3DSample[i].m_hVoice)
        {
            if (m_p3DSample[i].m_ParentIndex == SampleId && m_p3DSample[i].m_UniqueId == UniqueId)
                return m_p3DSample[i].m_volume;
        }
    }

	return UNDEFINED_SAMPLE;
}

//==============================================================================
void Audio::SetStreamVolume(s32 SampleId,f32 Volume)
{ 
    if (SampleId >= 0 && SampleId < MAX_STREAMS)
    {
        // Does the stream exist.
        if (m_pStreamSample[SampleId].m_hStream)
        {
            if (m_pStreamSample[SampleId].m_volume == Volume)
                return;
            else
                m_pStreamSample[SampleId].m_volume = Volume;

            Volume = Volume/78;

            // Max volume in Miles.
            if (Volume > 127)
                Volume = 127;
            else if ( Volume < 0)
                Volume = 0;
  
            AIL_set_stream_volume(m_pStreamSample[SampleId].m_hStream, (s32)Volume);
        }
    }
}

//==============================================================================
f32 Audio::GetStreamVolume(s32 SampleId)
{

    if (SampleId >= 0 && SampleId < MAX_STREAMS)
    {
        if (m_pStreamSample[SampleId].m_hStream)
        {
            return m_pStreamSample[SampleId].m_volume;
        }
    }

    return UNDEFINED_SAMPLE;
}

//==============================================================================
void Audio::Update(void)
{
    if (m_3DSoundCount == 0 && m_StreamCount == 0)
        return;
    s16 lowPriority = 100;
    s32 sId;
    s32 uniqueId;

    s32 i;
    // Go through all the sounds.
    for (i = 0; i < MAX_SOUNDS; i++)
    {
        if (m_3DSoundCount == 0)
            break;
	
        // If there is a sound there then it has a Parent Index.
        if (m_p3DSample[i].m_hVoice)
		{
#if 0			// Is the sound supposed to be killed.
            if (m_pIndex[m_p3DSample[i].m_ParentIndex].m_Status == KILL_SOUND)
            {
                s32 Status = AIL_3D_sample_status(m_p3DSample[i].m_hVoice);
                s32 loop = AIL_3D_sample_loop_count(m_p3DSample[i].m_hVoice);
                
                // Don't kill a sound that isn't done.
                if (Status == SMP_DONE || loop == 0)// || Status == SMP_PLAYING)
                {
                    
                    m_pIndex[m_p3DSample[i].m_ParentIndex].m_PositionIndex--;
                  //  x_DebugMsg("Position Index Release %d\n",m_pIndex[m_p3DSample[i].m_ParentIndex].m_PositionIndex);
                    Release_3DSample(i);
                    if (m_pIndex[m_p3DSample[i].m_ParentIndex].m_PositionIndex < 0)
                        m_pIndex[m_p3DSample[i].m_ParentIndex].m_PositionIndex = 0;
                }
            }
#endif
            if (lowPriority > m_p3DSample[i].m_Priority)
            {
                lowPriority = m_p3DSample[i].m_Priority;
                sId = m_p3DSample[i].m_ParentIndex;
                uniqueId = m_p3DSample[i].m_UniqueId;
            }
		}
    }

    s32 streamlowPriority = 100;
    s32 streamId;

    for (i = 0; i < MAX_STREAMS; i++)
    {
        if (m_StreamCount == 0)
            break;
        
        if (m_pStreamSample[i].m_hStream)
        {
            if (GetStreamStatus(i) == SMP_DONE)
                UnloadStream(i);
            if (streamlowPriority > m_pStreamSample[i].m_Priority)
            {
                streamlowPriority = m_pStreamSample[i].m_Priority;
                streamId = i;
            }
        }
    }

    // To many sounds, release one.  The PAD are used so that the sound buffer isn't totally full before we start 
    // releasing lower priority sounds.
    if ((m_3DSoundCount+SOUND_PAD) > MAX_SOUNDS && lowPriority != 100)
          UnloadSample(sId, uniqueId);

    // To many streams.
    if ((m_StreamCount+STREAM_PAD) > MAX_STREAMS &&  streamlowPriority != 100)
        UnloadStream(streamId);

}

//==============================================================================
void Audio::UpdateSample(s32 SampleId, s32 UniqueId, vector3* pPosition, f32 DeltaTime)
{    
    // Move the samples position.
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
        if (m_p3DSample[i].m_hVoice)
        {
            if (m_p3DSample[i].m_ParentIndex == SampleId && m_p3DSample[i].m_UniqueId == UniqueId)
            {
                Set_SampleValues(i, pPosition->X, pPosition->Y, pPosition->Z);
                AIL_update_3D_position(m_p3DSample[i].m_hVoice, DeltaTime);

            }
        }
    }
}

//==============================================================================
void Audio::SetMasterVolume(f32 left,f32 right)
{
	// All the volume stuff is done up in the higher layer.
    return;

}

//==============================================================================
void Audio::SetMasterVolume(f32 vol)
{
	// All the volume stuff is done up in the higher layer.
    return;

    s32 i;

	for (i = 0; i < MAX_SOUNDS; i++)
	{
		if (m_p3DSample[i].m_hVoice)
		{
			m_p3DSample[i].m_volume = vol;
			AIL_set_3D_sample_volume( m_p3DSample[i].m_hVoice, (int)m_p3DSample[i].m_volume);
		}			
	}

	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (m_pStreamSample[i].m_hStream)
		{
			m_pStreamSample[i].m_volume = vol;
			AIL_set_stream_volume( m_pStreamSample[i].m_hStream, (int)m_pStreamSample[i].m_volume);
		}
	}
}

//==============================================================================
void Audio::SetSurround(xbool mode)
{
	// Set the speaker types.
	AIL_set_3D_speaker_type(m_hOpenedProvider, SPEAKER_TYPE);
}

//==============================================================================
void Audio::SetClip(f32 nearclip, f32 farclip)
{
    m_NearClip = nearclip;
    m_FarClip = farclip;
}

//==============================================================================
s32 Audio::PlayStream (s32 SampleId)
{
	if (m_pStreamSample[SampleId].m_hStream)
	{
		s32 result = AIL_stream_status(m_pStreamSample[SampleId].m_hStream);
		
		// Check if the stream isn't playing or freed.
		if ( result != SMP_PLAYING && result != SMP_FREE )
			AIL_start_stream(m_pStreamSample[SampleId].m_hStream);
	}

	return SampleId;
}

//==============================================================================
s32 Audio::Set3D (s32 SampleId)
{
	s32 i;
    
    // Make sure the Sample Id is Jolly Good.    
    if ( (SampleId < MAX_DATA_COUNT) && ((s32)SampleId >= 0))
	{
		// This should never fail.
        if (m_vpEffects[SampleId])
		{
			for (i = 0; i < MAX_SOUNDS; i++)
			{
				// Get the first place in the buffer that is empty.
                if (m_p3DSample[i].m_hVoice == NULL)
				{
					if (Load_3DAudio(m_vpEffects[SampleId], i, POSITIONAL_SOUND) == NO_FILE_LOADED)
                        return NO_FILE_LOADED;

                    // There are multiple instances of the same sound in the buffer.
                    m_pIndex[SampleId].m_PositionIndex++;
                    m_pIndex[SampleId].m_Status = PLAY_SOUND;
                    m_p3DSample[i].m_ParentIndex = SampleId;
                    
                    // This Unique per sample only.
                    m_p3DSample[i].m_UniqueId = m_pIndex[SampleId].m_PositionIndex;

                    SetSampleVolume(SampleId, m_pIndex[SampleId].m_PositionIndex, 0);
				    AIL_start_3D_sample(m_p3DSample[i].m_hVoice);

					return m_p3DSample[i].m_UniqueId;
				}
			}
		}
        // You are missing the audio file.
        else
        {
//            ASSERTS(FALSE, "You are missing audio files.");
        }
	}

	return SOUND_BUFFER_FULL;
}

//==============================================================================
void Audio::Stop(s32 SampleId, s32 UniqueId)
{
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
        if (m_p3DSample[i].m_hVoice)
        {
            if (m_p3DSample[i].m_ParentIndex == SampleId && m_p3DSample[i].m_UniqueId == UniqueId)
            {
                AIL_stop_3D_sample(m_p3DSample[i].m_hVoice);
                return;
            }
        }
    }
}

//==============================================================================
s32 Audio::Set3D (s32 SampleId, vector3* pPosition)
{	
	s32 i;
    
    // Make sure the Sample Id is Jolly Good.    
	if (SampleId < MAX_DATA_COUNT && SampleId >= 0)
	{
		// This should never fail.
        if (m_vpEffects[SampleId])
		{
			for (i = 0; i < MAX_SOUNDS; i++)
			{
				// Get the first place in the buffer that is empty.
                if (m_p3DSample[i].m_hVoice == NULL)
				{
					if (Load_3DAudio(m_vpEffects[SampleId], i, POSITIONAL_SOUND) == NO_FILE_LOADED)
                        return NO_FILE_LOADED;

					Set_SampleValues( i, pPosition->X, pPosition->Y, pPosition->Z);	
					
                    // There are multiple instances of the same sound in the buffer.
                    m_pIndex[SampleId].m_PositionIndex++;
                    m_pIndex[SampleId].m_Status = PLAY_SOUND;
                    m_p3DSample[i].m_ParentIndex = SampleId;
                    
                    // This Unique per sample only.
                    m_p3DSample[i].m_UniqueId = m_pIndex[SampleId].m_PositionIndex;

                    SetSampleVolume(SampleId, m_pIndex[SampleId].m_PositionIndex, 0);
                    s32 latency = AIL_digital_latency(DIG);
                    AIL_start_3D_sample(m_p3DSample[i].m_hVoice);

                    latency = AIL_digital_latency(DIG);

					return m_p3DSample[i].m_UniqueId;

				}
			}
		}
        // You are missing the audio file.
        else
        {
//            ASSERTS(FALSE, "You are missing audio files.");
        }

	}

	return SOUND_BUFFER_FULL;
}

//==============================================================================
s32 Audio::Set3D (s32 SampleId, vector3* pPosition,s32 Falloff)
{
	s32 i;

	// Make sure the Sample Id is Jolly Good.    
    if (SampleId < MAX_DATA_COUNT && SampleId >= 0)
	{
		// This should never fail.
        if (m_vpEffects[SampleId])
		{
			for (i = 0; i < MAX_SOUNDS; i++)
			{
				// Get the first place in the buffer that is empty.
                if (m_p3DSample[i].m_hVoice == NULL)
				{
					if (Load_3DAudio(m_vpEffects[SampleId], i, POSITIONAL_SOUND) == NO_FILE_LOADED)
                        return NO_FILE_LOADED;

					Set_SampleValues( i, pPosition->X, pPosition->Y, pPosition->Z);
					
                    // There are multiple instances of the same sound in the buffer.
                    m_pIndex[SampleId].m_PositionIndex++;
                    m_p3DSample[i].m_ParentIndex = SampleId;
                    
                    // This Unique per sample only.
                    m_p3DSample[i].m_UniqueId = m_pIndex[SampleId].m_PositionIndex;

                    SetSampleVolume(SampleId, m_pIndex[SampleId].m_PositionIndex, 0);
				    AIL_start_3D_sample(m_p3DSample[i].m_hVoice);

					return m_p3DSample[i].m_UniqueId;

				}
			}
		}
        // You are missing the audio file.
        else
        {
//            ASSERTS(FALSE, "You are missing audio files");
        }
	}

	return SOUND_BUFFER_FULL;
}

//==============================================================================
void Audio::StopAll (void)
{
	s32 i;
	// 3D sample.
	for (i = 0; i < MAX_SOUNDS; i++)
	{
		if (m_p3DSample[i].m_hVoice)
			AIL_stop_3D_sample(m_p3DSample[i].m_hVoice);
	}
	
	// Stream.
	for (i = 0; i < MAX_STREAMS; i++)
	{
		// Pause the stream and reset its position.
		if (m_pStreamSample[i].m_hStream)
		{
			AIL_pause_stream(m_pStreamSample[i].m_hStream, 1);
			AIL_set_stream_position(m_pStreamSample[i].m_hStream, 0);
		}
	}
}

//==============================================================================
void Audio::SetPriority(s32 SampleId, s32 Priority)
{
    s32 count = 0;
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
        if (count == m_3DSoundCount)
            return;

        // We don't care about the unique id here because all the sounds in the buffer with this SampleId have the same 
        // priority.
        if (m_p3DSample[i].m_ParentIndex == SampleId)
        {
            m_p3DSample[i].m_Priority = Priority;
            count++;
        }
    }
}

//==============================================================================
void Audio::SetStreamPriority(s32 SampleId, s32 Priority)
{
    m_pStreamSample[SampleId].m_Priority = Priority;
}

//==============================================================================
void Audio::Pause (u32 ChannelId)
{
#if 0
	// Its a streaming container.
	if (ChannelId >= MAX_SOUNDS && ChannelId < (MAX_SOUNDS + MAX_STREAMS))
	{
		ChannelId -= MAX_SOUNDS;

		if (m_pStreamSample[ChannelId].m_hStream)
		{
			s32 result = AIL_stream_status(m_pStreamSample[ChannelId].m_hStream);
			
			// Check if the stream isn't playing or freed.
			if ( result != SMP_STOPPED && result != SMP_FREE )
			{
				AIL_pause_stream(m_pStreamSample[ChannelId].m_hStream, 1);
			}
		}

	}
	// Its a 3D sound container.
	else if (ChannelId < MAX_SOUNDS && ChannelId >= 0)
	{
		if (m_p3DSample[ChannelId].m_hVoice)
		{
			AIL_stop_3D_sample(m_p3DSample[ChannelId].m_hVoice);
		}
	}	
#endif
}

//==============================================================================
void Audio::AllocSampleData(s32 Count)
{
	Count += ADD_EFFECT;
    m_vpEffects = new void *[Count + ADD_EFFECT];
	ASSERT(m_vpEffects);
	m_pIndex = new Index[Count];
	
	MAX_DATA_COUNT = Count;
	m_current_count = 0;
}

//==============================================================================
s32	Audio::LoadSampleData(char* pFilename,  s32 Id)
{
	// This should never happen, all the effects need to be stored in RAM.
    if (m_current_count >= MAX_DATA_COUNT)
		ASSERTS(FALSE, " Need to expand the array of data to contain all the sample");
	

	m_vpEffects[m_current_count] = Load_File3D(pFilename);
	m_pIndex[m_current_count].m_PositionIndex = 0;
    m_pIndex[m_current_count].m_Status = PLAY_SOUND;
    memcpy(m_pIndex[m_current_count].name, pFilename, 40);
    m_pIndex[m_current_count].OwnerId = Id;
	m_current_count++;

	return (m_current_count-1);
}

//==============================================================================
u32 Audio::GetSoundStatus(s32 SampleId, s32 UniqueId)
{
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {
		if (m_p3DSample[i].m_hVoice)
		{
			if (m_p3DSample[i].m_ParentIndex == SampleId && m_p3DSample[i].m_UniqueId == UniqueId)
                return AIL_3D_sample_status(m_p3DSample[i].m_hVoice);
		}
	}
		
	return UNDEFINED_SAMPLE;
}

//==============================================================================
u32 Audio::GetStreamStatus(s32 SampleId)
{
    return AIL_stream_status(m_pStreamSample[SampleId].m_hStream);
}

//==============================================================================
void Audio::SetLoopCount(s32 SampleId, s16 LoopCount)
{
    s32 count = m_pIndex[SampleId].m_PositionIndex;
    
    // Check all the sounds that are playing with the sampleId.
    for (s32 i = 0; i < MAX_SOUNDS; i++)
    {	
        if (count == 0)
            return;
        
        if (m_p3DSample[i].m_hVoice)
		{
	        if (m_p3DSample[i].m_ParentIndex == SampleId)
            {
			    if (LoopCount == INFINITE_LOOP)
			    {
				    AIL_set_3D_sample_loop_count(m_p3DSample[i].m_hVoice, INFINITE_LOOP);
                    s32 Status = AIL_3D_sample_status(m_p3DSample[i].m_hVoice);
                    AIL_set_3D_sample_loop_block(m_p3DSample[i].m_hVoice, m_pIndex[SampleId].m_LoopStart, m_pIndex[SampleId].m_LoopEnd);
                    
                    if (Status != SMP_PLAYING)
                    {
                        AIL_start_3D_sample(m_p3DSample[i].m_hVoice);
			        }
                }
                count--;
            }
		}
    }

}

//==============================================================================
void Audio::SetStreamLoopCount(s32 SampleId, s16 LoopCount)
{
    if (SampleId >= 0 && SampleId < MAX_STREAMS)
    {
        if (m_pStreamSample[SampleId].m_hStream)
        {
            AIL_set_stream_loop_count(m_pStreamSample[SampleId].m_hStream, LoopCount);
        }
    }
}

//==============================================================================
void Audio::SetLoop(s32 Index, s32 Start, s32 End)
{
    // This mean that if this is a looping effect than this effect is to be looped front to back.
    if (Start == End)
    {
        // In miles to loop front to back you have to set the start at 0 and finish at -1.
        m_pIndex[Index].m_LoopStart = 0;
        m_pIndex[Index].m_LoopStart = -1;
    }
    else
    {
        // This is a looping effect since the Start and End offset are diffrent.
        m_pIndex[Index].m_LoopStart = Start;
        m_pIndex[Index].m_LoopEnd = End;
    }
}

//==============================================================================
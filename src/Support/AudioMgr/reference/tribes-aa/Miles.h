#ifndef _MILES_H_
#define _MILES_H_

#include "x_types.hpp"
#include "e_engine.hpp"
#include "3rdparty\miles6\include\mss.h"

#define MAX_PROVIDERS           64                      // Enumerate all the provider and pick the one that we want.
#define SPEAKER_TYPE            AIL_3D_2_SPEAKER        // Can be AIL_3D_2_SPEAKER, AIL_3D_4_SPEAKER or AIL_3D_HEADPHONE.
#define LOOP_COUNT              1                       // Use 0 to loop infinitly.
#define INFINITE_LOOP           0                       // Non stop looping till the handle is released.
#define NEAR_CLIP               10                      // Default near clip.
#define FAR_CLIP                1000                    // Default far clip
#define INNER_CONE              120                     // Default inner cone.
#define OUTER_CONE              180                     // Default outer cone.
#define INNER_TO_OUTER_SCALE    127                     // 0 = normal value scale to zero,  127 = no scale at all.              
#define ENVIRONMENT             ENVIRONMENT_GENERIC     // ENVIRONMENT_MOUNTAINS, ENVIRONMENT_CITY, ENVIRONMENT_HALLWAY
#define DIGITALRATE             22050                   // Opening rate.
#define DIGITALBITS             16                      // Opening bits.
#define DIGITALCHANNELS         2                       // Opening channels
#define MAX_STREAMS             8                       // Max stream.
#define MAX_SOUNDS              64                      // Max 3d sounds.
#define STREAM_LOOP_COUNT       1                       // 0 is infinity.
#define MIN_STREAM_SIZE         500000                  // Minimun stream size in bytes. 250K
#define POSITIONAL_SOUND        1
#define NON_POSITIONAL_SOUND    0
#define KILL_SOUND              1
#define PLAY_SOUND              0
#define ADD_EFFECT              32                      // All the effects need to be loaded and that includes the effects in voice.
#define SOUND_PAD               5                       // Add this to the current sound count, that way we can get ride of low priority sounds.
#define STREAM_PAD              2                       // Add this to the current stream count, that way we can get ride of low priority sounds.

//#define AUDIO_AUTO_UPDATE                               // Make miles create a seperate thread that updates the sounds position.

// Error code.
#define NO_FILE_LOADED          -1                      
#define SOUND_BUFFER_FULL       -2                      
#define STREAM_BUFFER_FULL      -3                      
#define UNDEFINED_SAMPLE        -4
#define NO_DRIVER_LOADED        -5
#define NO_PROVIDER_LOADED      -6

#define LOAD_COMPLETE           0


// Holds the the 2D and 3D Sounds.
struct positional_audio
{
    H3DSAMPLE       m_hVoice;
    s32             m_SampleId;
    s8              m_name[40];
    void            *m_vpVoiceData;
    vector3         position;           
    f32             m_volume;
    s32             m_flags;
    s32             m_Priority;
    s32             m_ParentIndex;
    s32             m_UniqueId;

    positional_audio()
    {   m_hVoice = 0; m_vpVoiceData = 0; m_volume = 0;}
};

// Holds the streaming sounds.
struct streaming_audio
{
    HSTREAM         m_hStream;
    s8              m_name[40];
    f32             m_pitch;
    f32             m_pan;
    s32             m_SampleId;
    f32             m_volume;
    s32             m_Priority;
    s32             m_flags;
    streaming_audio()
    {   m_hStream = 0; }
};

// This get maped to the sample.
struct Index
{
    s32             m_PositionIndex;
    s8              m_Status;
    s8              name[256];
    s32             OwnerId;
    s32             m_LoopStart;
    s32             m_LoopEnd;
    Index()
    {   m_PositionIndex = 0; }
};

// Contains all the routines for miles.
class Audio
{
    private:



        s32                 curProviderIndex;       
        s32                 usingEAX;
        HPROVIDER           m_hOpenedProvider;  
        HDIGDRIVER          DIG;
        streaming_audio     m_pStreamSample[MAX_STREAMS];     
        positional_audio    m_p3DSample[MAX_SOUNDS];
        s32                 MAX_DATA_COUNT;         // This is used directly with the data that we load into the m_vpEffects.
        s32                 m_current_count;
        f32                 m_NearClip;
        f32                 m_FarClip;
        f32                 m_MasterVolume, m_EffectVolume, m_VoiceVolume, m_MusicVolume;
        f32                 m_Pad;                  // Since diffrent providers have diffrent 3D sounds level, we need to add pading to it.
            
        // Internal function.
        
        bool    Load_Provider(void);
        void    Sound_Shutdown(void);
        s32     Load_3DAudio(void* pData, s32 index, s8 Flags);
        s32     Load_StreamSample(char* pString, s32 index);
        void    *Load_File3D(char* pString);
        s32     Set_Sample( s32 index, void* vpSampleData, s8 Flags);
        void    Set_SampleValues( s32 index, f32 dX, f32 dY, f32 dZ);
        void    Set_Room(s32 value, s32 index);
        
        void    Sound_Kill(void);
        void    Release_3DSample( s32 index);
        void    Release_Stream(s32 index);
    
    public:
        
        // Pointer to UNCOMPRESSED AUDIO data.
        void**          m_vpEffects;
        s32                 m_StreamCount, m_3DSoundCount, m_3DDataCount;
        // The refrence in to this array is the SampleId.
        Index*          m_pIndex;

        Audio(){ }

        ~Audio() { }

        s32     Init(void);
        void    Kill(void);

        // Load and Release calls.
        void    AllocSampleData(s32 Count);
        s32     LoadStream(char* pString, s32 Loop);
        void    UnloadSample(s32 SampleId, s32 UniqueId);
        void    UnloadStream(s32 SampleId);
        s32     LoadSampleData(char* pFilename, s32 Id);

        // Volume calls.
        // NOTE:    The container and channels are responsible for adjusting the volume before it comer down to
        //          the dirver layer.
        void    SetSampleVolume(s32 SampleId, s32 UniqueId, f32 Volume);
        f32     GetSampleVolume(s32 SampleId, s32 UniqueId);              // Returns a value between 0 and 127.
        void    SetStreamVolume(s32 SampleId,f32 Volume);
        f32     GetStreamVolume(s32 SampleId);
        void    SetMasterVolume(f32 left,f32 right);
        void    SetMasterVolume(f32 vol);
        void    SetLoop(s32 Index, s32 Start, s32 End);
        
        // Update the position for all the sounds.
        void    Update(void);
        void    UpdateSample(s32 SampleId, s32 UniqueId, vector3* pPosition, f32 DeltaTime);
        void    SetSurround(xbool mode);

        void    SetClip(f32 nearclip, f32 farclip);  
        
        // Returns a UniqueId that is unique per sample.
        s32     Set3D (s32 SampleId);
        s32     Set3D (s32 SampleId, vector3* pPosition);
        s32     Set3D (s32 SampleId, vector3* pPosition,s32 Falloff);
        void    SetPriority(s32 SampleId, s32 Priority);
        void    SetStreamPriority(s32 SampleId, s32 Priority);
        
        // Play calls.
        s32     PlayStream (s32 SampleId);
        void    Stop(s32 SampleId, s32 UniqueId);
        void    StopAll (void);
        void    Pause (u32 ChannelId);
        
        u32     GetSoundStatus(s32 SampleId, s32 UniqueId);   
        u32     GetStreamStatus(s32 SampleId);
        void    SetLoopCount(s32 SampleId, s16 LoopCount);
        void    SetStreamLoopCount(s32 SampleId, s16 LoopCount);
};

#endif

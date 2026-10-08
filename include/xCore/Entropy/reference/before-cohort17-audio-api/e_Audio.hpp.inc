// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#ifndef AUDIO_MGR_HPP
#define AUDIO_MGR_HPP

#include <xCore/Entropy/Audio/audio_private.hpp>
#include <xCore/Entropy/Audio/audio_package.hpp>

//==============================================================================

enum
{
    SPEAKERS_MONO,
    SPEAKERS_STEREO,
    SPEAKERS_PROLOGIC,
    SPEAKERS_DOLBY_DIGITAI_5_1
};

class audio_mgr
{

#define PAN_TABLE_ENTRIES (360)
#define MAX_EARS          (4)
#define MAX_ZONES         (257)
#define ZONELESS          (256)

struct audio_mgr_ear
{
            audio_mgr_ear*      pNext;
            matrix4             W2V;
            vector3             Position;
            f32                 Volume;
            u32                 Sequence;
            s32                 ZoneID;
            f32                 ZoneVolumes[MAX_ZONES];
};

public:
 void GetEar(matrix4&,f32&,f32&);
 void SetEar(const matrix4&,f32,f32);

                            audio_mgr               ( void );
                           ~audio_mgr               ( void );
                                                    
            void            Init                    ( s32 MemSize );
            void            Kill                    ( void );
            void            ResizeMemory            ( s32 NewMemSize );

      const char*           GetLocalizedName        ( const char* pFileName ) const;

            xbool           LoadPackage             ( const char*       pFilename );
            xbool           UnloadPackage           ( const char*       pFilename );
            xbool           IsPackageLoaded         ( const char*       pFilename );
            xbool           LoadPackageStrings      ( const char*       pFilename,
                                                      xarray<xstring>&  Strings );
            void            GetLoadedPackages       ( xarray<xstring>&  Packages );
            s32             GetPackageARAM          ( const char*       pPackage );
            char*           GetMusicType            ( const char*       pFilename );
            s32             GetMusicIntensity       ( const char*       pFilename,
                                                      music_intensity* &Intensity );

            void            UnloadAllPackages       ( void );
                                                    
            void            Update                  ( f32               DeltaTime );
            void            PeriodicUpdate          ( void );

            void            SetClip                 ( f32               NearClip,
                                                      f32               FarClip );

            void            GetClip                 ( f32&              NearClip,
                                                      f32&              FarClip );


                                                    


            void            UpdateEarZoneVolume     ( ear_id            EarID,
                                                      s32               ZoneID,
                                                      f32               Volume );

            void            UpdateEarZoneVolumes    ( ear_id            EarID,
                                                      f32*              pVolumes );

            ear_id          GetFirstEar             ( void );
            ear_id          GetNextEar              ( void );
            void            ResetCurrentEar         ( void );

            void            SetVoiceEar             ( voice_id          VoiceID,
                                                      ear_id            EarId );
            
            ear_id          CreateEar               ( void );
            
            void            DestroyEar              ( ear_id            EarId );

            voice_id        Play                    ( const char*       pIdentifier,
                                                      xbool             AutoStart = TRUE );





            // Earlier genuine qualified APIs from original Xbox audio_mgr.obj.
            voice_id Play(const char*, const vector3&, xbool);
            voice_id PlayVolumeClipped(const char*, const vector3&, xbool);
private:
            voice_id Play(const char*, xbool, const vector3&, xbool, xbool);
public:


            void            EnableAudioDucking      ( void );
            void            DisableAudioDucking     ( void );
            xbool           IsAudioDuckingEnabled   ( void )            ;

            void            Pause                   ( voice_id          VoiceID );
            void            PauseAll                ( void );

            void            Resume                  ( voice_id          VoiceID );
            void            ResumeAll               ( void );

            void            Release                 ( voice_id          VoiceID,
                                                      f32               Time );
            void            ReleaseLoop             ( voice_id          VoiceID );
            void            ReleaseAll              ( void );

            xbool           Start                   ( voice_id          VoiceID );

            xbool           Segue                   ( voice_id          VoiceID,
                                                      voice_id          VoiceToQ );

            xbool           SetReleaseTime          ( voice_id          VoiceID,
                                                      f32               Time );
            
            xbool           IsReleasing             ( voice_id          VoiceID );
            xbool           IsValidVoiceId          ( voice_id          VoiceID );
            xbool           IsVoiceReady            ( voice_id          VoiceID );

            void            DisplayPackages();
            
            f32             GetVolume               ( voice_id          VoiceID );
            xbool           SetVolume               ( voice_id          VoiceID,
                                                      f32               Volume );                                                    
            f32             GetPan                  ( voice_id          VoiceID );
            xbool           SetPan                  ( voice_id          VoiceID,
                                                      f32               Pan );
                                                    
            f32             GetPitch                ( voice_id          VoiceID );
            xbool           SetPitch                ( voice_id          VoiceID,
                                                      f32               Pitch );
                                                    
            xbool           GetPosition             ( voice_id          VoiceID,
                                                      vector3&          Position );
            xbool           SetPosition             ( voice_id          VoiceID,
                                                      const vector3&    Position );
                                                    
            xbool           SetFalloff              ( voice_id          VoiceID,
                                                      f32               Near,
                                                      f32               Far );

            f32             GetEffectSend           ( voice_id          VoiceID );
            xbool           SetEffectSend           ( voice_id          VoiceID,
                                                      f32               EffectSend );

            s32             GetPriority             ( voice_id          VoiceID );
            u32             GetUserData             ( const char*       pIdentifier );
            s32             GetPriority             ( const char*       pIdentifier );
            f32             GetFarFalloff           ( const char*       pIdentifier );
            f32             GetNearFalloff          ( const char*       pIdentifier );


            f32             GetLengthSeconds        ( const char*       pIdentifier );
            f32             GetLengthSeconds        ( voice_id          VoiceID );

            f32             GetCurrentPlayTime      ( voice_id          VoiceID );
            const char*     GetVoiceDescriptor      ( voice_id          VoiceID );

            xbool           HasLipSync              ( voice_id          VoiceID );
            f32             GetLipSync              ( voice_id          VoiceID );
            s32             GetBreakPoints          ( voice_id          VoiceID,
                                                      f32* &            BreakPoints );
                                                    
            xbool           GetIsReady              ( voice_id          VoiceID );

            void            SetMasterVolume         ( f32               Volume );
            void            SetMusicVolume          ( f32               Volume );
            void            SetSFXVolume            ( f32               Volume );
            void            SetVoiceVolume          ( f32               Volume );
            f32             GetAudioTime            ( void ) { return m_Time; }
            void            SetSpeakerConfig        ( s32               SpeakerConfig );
            s32             GetSpeakerConfig        ( void )            { return m_SpeakerConfig; }



            xbool           IsValidDescriptor       ( const char*       pIdentifier );
            void            ReMergeIdentifierTables ( void );
            void            SetReverbWetDryMix      ( f32               WetDryMix );
            f32             GetAudioWetDryMix       ( void );
            void            SetPitchLock            ( voice_id          VoiceID,
                                                      xbool             bPitchLock );

private:                                            
                                                    
            void            SetSpeakerAngles        ( s32               FrontLeft, 
                                                      s32               FrontRight, 
                                                      s32               BackRight, 
                                                      s32               BackLeft, 
                                                      s32               nSpeakers );
            void            GetSpeakerAngles        ( s32&              FrontLeft,
                                                      s32&              FrontRight, 
                                                      s32&              BackRight, 
                                                      s32&              BackLeft, 
                                                      s32&              nSpeakers );
            audio_package*  FindPackageByName       ( const char*       pFilename );

            void            GetElementParameters    ( uncompressed_parameters*  pParams,
                                                      u16*                      pDescriptor, 
                                                      voice*                    pVoice );
        
            void            MergeIdentifierTables   ( void );
            s32             AppendDescriptor        ( f32               BaseTime,
                                                      u16*              pDescriptor,
                                                      voice*            pVoice, 
                                                      audio_package*    pPackage );
            s32             AppendSimple            ( f32               BaseTime,
                                                      u16*              pDescriptor,
                                                      voice*            pVoice, 
                                                      audio_package*    pPackage );
            s32             AppendComplex           ( f32               BaseTime,
                                                      u16*              pDescriptor, 
                                                      voice*            pVoice, 
                                                      audio_package*    pPackage );
            s32             AppendRandomList        ( f32               BaseTime,
                                                      u16*              pDescriptor, 
                                                      voice*            pVoice,                                                       
                                                      audio_package*    pPackage );
            s32             AppendWeightedList      ( f32               BaseTime,
                                                      u16*              pDescriptor,
                                                      voice*            pVoice,  
                                                      audio_package*    pPackage );
            s32             AppendHot               ( u32               Index,
                                                      f32               DeltaTime,
                                                      u16*              pDescriptor, 
                                                      voice*            pVoice,
                                                      audio_package*    pPackage );
            s32             AppendWarm              ( u32               Index,
                                                      f32               DeltaTime,
                                                      u16*              pDescriptor, 
                                                      voice*            pVoice, 
                                                      audio_package*    pPackage );
            s32             AppendCold              ( u32               Index,
                                                      f32               DeltaTime,
                                                      u16*              pDescriptor, 
                                                      voice*            pVoice, 
                                                      audio_package*    pPackage );
            s32             IsDescriptorCold        ( u16*              pDescriptor,
                                                      audio_package*    pPackage );
            s32             IsSimpleCold            ( u16*              pDescriptor,
                                                      audio_package*    pPackage );
            s32             IsComplexCold           ( u16*              pDescriptor, 
                                                      audio_package*    pPackage );
            s32             IsRandomListCold        ( u16*              pDescriptor, 
                                                      audio_package*    pPackage );
            s32             IsWeightedListCold      ( u16*              pDescriptor,
                                                      audio_package*    pPackage );
public:            
            s32             IsCold                  ( char*             pIdentifier );

private:
            void Calculate3dVolume(f32,f32,s32,const vector3&,f32&);
public:
            u16* FindDescriptorByName(const char*,audio_package**,s32*,char*&);
private:
            void GetVoiceParameters(uncompressed_parameters*,u16*,audio_package*);
public:
 
private:
            // Genuine original Xbox API; corresponding PC pan-only provider remains unresolved.
            void Calculate3dVolumeAndPan(f32,f32,s32,f32,f32,const vector3&,f32&,vector4&);

            void            Calculate2dPan          (f32                Pan2d, 
                                                     vector4&           Pan3d );

            f32             ComputeFalloff          ( f32               PercentToFarClip, 
                                                      s32               TableID );
            
            xbool           GetVoiceParameters      ( const char* pIdentifier,
                                                      uncompressed_parameters& Params );

            audio_mgr_ear*  IdToEar                 ( ear_id EarID );
            ear_id          EarToId                 ( audio_mgr_ear* pEar );

private:
            vector4             m_Pan[PAN_TABLE_ENTRIES];
            vector4             m_StereoPan[181];
            vector4             m_Diffuse;
            s32                 m_FrontLeft;
            s32                 m_FrontRight;
            s32                 m_BackRight;
            s32                 m_BackLeft;
            s32                 m_nSpeakers;
            s32                 m_SpeakerConfig;
            f32                 m_Time;
            matrix4             m_W2V;
            f32                 m_NearClip;
            f32                 m_FarClip;
xarray<descriptor_identifier*>  m_pIdentifiers;
audio_package::package_link     m_Link;

friend class audio_voice_mgr;

};

extern audio_mgr g_AudioMgr;

#endif

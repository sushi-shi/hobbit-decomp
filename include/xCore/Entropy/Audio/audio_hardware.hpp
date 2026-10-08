// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#ifndef AUDIO_HARDWARE_HPP
#define AUDIO_HARDWARE_HPP

#include <xCore/Entropy/Audio/audio_private.hpp>
#if defined(TARGET_PC)
#include <dsound.h> // Genuine DX8 SDK owns LPDIRECTSOUND8.
#endif

class audio_hardware
{

//------------------------------------------------------------------------------
// Public functions.

public:

                            audio_hardware      ( void );
                           ~audio_hardware      ( void );
            
#if defined(TARGET_PC)
            // Inferred descriptive API name; actual PC member-ABI pointer getter.
            LPDIRECTSOUND8  GetDirectSound       ( void );
#endif
            void            Init                ( s32 MemSize );
            void            Kill                ( void );
            void            ResizeMemory        ( s32 MemSize );

            void            Update              ( void );
            void            UpdateStream        ( channel* pChannel );

            void*           AllocAudioRam       ( s32 nBytes );
            void            FreeAudioRam        ( void* pBuffer );
            s32             GetAudioRamFree     ( void );
            void            SetReverbWetDryMix  ( f32           WetDryMix );
            f32             GetAudioWetDryMix    ( void );


            //void            ReleaseLoop         ( channel*      pChannel );
            xbool           AcquireChannel      ( channel*      pChannel );
            void            ReleaseChannel      ( channel*      pChannel );
            void            ClearChannel        ( channel*      pChannel );
            void            DuplicatePriority   ( channel*      pDest, channel* pSrc );
            xbool           IsChannelActive     ( channel*      pChannel );
            void            StartChannel        ( channel*      pChannel );
            void            StopChannel         ( channel*      pChannel );
            void            PauseChannel        ( channel*      pChannel );
            void            ResumeChannel       ( channel*      pChannel );
            void            InitChannel         ( channel*      pChannel );
#if defined(TARGET_PC) && !defined(HOBBIT_AUDIO_LATER_IAL_IMPLEMENTATION)
            // Earlier PC helper signatures decoded from real bodies; identifiers inferred.
            // ADPCM signed byte arithmetic and ret20, PCM ret16 independently decoded; names inferred.
            void DecodeADPCMBuffer(LPDIRECTSOUNDBUFFER,void*,s32,s32,s32);
            void CopyPCMBuffer(LPDIRECTSOUNDBUFFER,void*,s32,s32);
#endif
            void            InitChannelStreamed ( channel*      pChannel );
            u32             GetSamplesPlayed    ( channel*      pChannel );
            void            Lock                ( void );
            void            Unlock              ( void );

            void            SetPitchFactor      ( f32           PitchFactor )   { m_PitchFactor = PitchFactor; }
            void            SetVolumeFactor     ( f32           VolumeFactor )  { m_VolumeFactor = VolumeFactor; }
            f32             GetPitchFactor      ( void )        { return m_PitchFactor; }
            f32             GetVolumeFactor     ( void )        { return m_VolumeFactor; }
                        
inline      u32             GetDirtyBits        ( void )        { return m_Dirty; }
inline      void            SetDirtyBits        ( u32 Bits )    { m_Dirty = Bits; }
inline      u32             GetDirtyBit         ( u32 Mask )    { return m_Dirty & Mask; }
inline      void            SetDirtyBit         ( u32 Mask )    { m_Dirty |= Mask; }
inline      void            ClearDirtyBit       ( u32 Mask )    { m_Dirty &= ~Mask; }
            s32             NumChannels         ( void );
            channel*        GetChannelBuffer    ( void );
inline      xbool           IsValidChannel      ( channel*      pChannel ) { return (pChannel >= m_FirstChannel) && (pChannel <= m_LastChannel); }
inline      xbool           CanModifyChannelList( void )        { return (m_InterruptLevel != 0); }     
#if !defined(TARGET_PC) || defined(HOBBIT_AUDIO_LATER_PC_CLOCK)
inline      f32             GetTicks            ( void )        { return (f32)m_TickCount * m_TickTime; }
#endif
#if !defined(TARGET_PC) || defined(HOBBIT_AUDIO_LATER_PC_CLOCK)
inline      void            Tick                ( void )        { m_TickCount++; }
#endif
inline      xbool           GetDoHardwareUpdate ( void )        { return m_bDoHardwareUpdate; }
inline      void            SetDoHardwareUpdate ( void )        { m_bDoHardwareUpdate = TRUE; }
inline      void            ClearDoHardwareUpdate ( void )      { m_bDoHardwareUpdate = FALSE; }
            void            UpdateMP3           ( audio_stream* pStream );

//------------------------------------------------------------------------------
// Private variables.

private:

volatile    u32             m_Dirty;
#if !defined(TARGET_PC) || defined(HOBBIT_AUDIO_LATER_PC_CLOCK)
volatile    u32             m_TickCount;
            f32             m_TickTime;
#endif
            f32             m_PitchFactor;
            f32             m_VolumeFactor;
            channel*        m_FirstChannel;
            channel*        m_LastChannel;
            xbool           m_InterruptState;
            xbool           m_InterruptLevel;
volatile    xbool           m_bDoHardwareUpdate;

#ifdef TARGET_XBOX
            LPDIRECTSOUND8  m_pdsSystem;
#endif
            xmutex          m_LockMutex;
};

extern audio_hardware g_AudioHardware;

#endif //AUDIO_HARDWARE_HPP

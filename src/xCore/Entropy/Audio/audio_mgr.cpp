// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#include <rva.h>
#include <xCore/Entropy/e_Audio.hpp>
#include <xCore/Entropy/Audio/audio_private_pkg.hpp>
#include <xCore/Entropy/Audio/audio_hardware.hpp>
#include <xCore/Entropy/Audio/audio_channel_mgr.hpp>
#include <xCore/Entropy/Audio/audio_voice_mgr.hpp>
#include <xCore/Entropy/Audio/audio_package.hpp>
#include <xCore/Entropy/Audio/audio_stream_mgr.hpp>
#include <xCore/Entropy/Audio/audio_inline.hpp>
#include <xCore/Entropy/Audio/audio_debug.hpp>
#include <xCore/x_files/x_log.hpp>

#if (!defined(X_RETAIL) || defined(X_QA)) && defined(TARGET_PS2)
#include <xCore/3rdParty/PS2/SN/EE/Include/sntty.h>  
#define ENABLE_AUDIO_DEBUG
#endif

#ifdef TARGET_PC
// Complete natural source-owned extent: 0x4 bytes.
DATA(0x003edabc)
static xbool    s_bDisableAudio = FALSE;
RVA(0x00257d30, 0x11)
void EnableAudio( xbool Enable )
{
    s_bDisableAudio = !Enable;
}
#else
static xbool    s_bDisableAudio = FALSE;
#endif

#if defined(rbrannon) || defined(mreed)
#define LOG_PLAY_SUCCESS "audio_mgr::Play(success)"
#define LOG_PLAY_CLIPPED "audio_mgr::Play(clipped)"
#define LOG_PLAY_FAILURE "audio_mgr::Play(failure)"
#define LOG_PLAY_WARNING "audio_mgr::Play(warning)"

voice*   g_DebugVoice   = NULL;
element* g_DebugElement = NULL;
channel* g_DebugChannel = NULL;
f32      g_DebugTime    = 0.0f; 
#endif

//#define TRAP_ON_IDENTIFIER

#ifdef TRAP_ON_IDENTIFIER
xbool    g_EnableIdentifierTrap = 1;
char     g_DebugIdentifier[64] = "FORCE_FIELD_ACTIVE";
#endif

//------------------------------------------------------------------------------

#define AUDIO_ENABLE
static xbool    s_Initialized = FALSE;

static xthread* s_PeriodicUpdateThread = NULL;
#if !defined(TARGET_PC)
static xthread* s_HardwareUpdateThread = NULL;
#endif
// Complete natural source-owned extent: 0x4 bytes.
DATA(0x003edac8)
s32 s_LastPlayResult = 0;
// Complete natural source-owned extent: 0x4 bytes.
DATA(0x003edacc)
voice_id s_LastDXVoice = 0;

#define FALLOFF_TABLE_SIZE (9)

typedef f32 falloff_table[FALLOFF_TABLE_SIZE];

// Complete natural source-owned extent: 0x6c bytes.
DATA(0x0034e0f0)
static falloff_table s_FalloffTables[NUM_ROLLOFFS] = 
{
    { 1.00000f, 0.87500f, 0.75000f, 0.62500f, 0.50000f, 0.37500f, 0.25000f, 0.12500f, 0.00000f }, // LINEAR_ROLLOFF
    { 1.00000f, 0.76562f, 0.56250f, 0.39062f, 0.25000f, 0.14062f, 0.06250f, 0.01562f, 0.00000f }, // FAST_ROLLOFF
    { 1.00000f, 0.93541f, 0.86602f, 0.79056f, 0.70710f, 0.61237f, 0.50000f, 0.35355f, 0.00000f }  // SLOW_ROLLOFF
};
//------------------------------------------------------------------------------

// Later AUDIO_TWEAK globals/type retained with their complete incompatible source revision.

//------------------------------------------------------------------------------

// Complete natural source-owned extent: 0x226c bytes.
DATA(0x003eb850)
audio_mgr g_AudioMgr;

//------------------------------------------------------------------------------

static void AudioMgrPeriodicUpdate( void )
{
    xthread* pThread = x_GetCurrentThread();

    while( pThread->IsActive() )
    {
        // Check the queued voices...
        g_AudioVoiceMgr.UpdateCheckQueued();
        // Now do the periodic stream update.
        g_AudioMgr.PeriodicUpdate();
        x_DelayThread( 10 );
    }
}

//------------------------------------------------------------------------------

static void AudioMgrHardwareUpdate( void )
{
    xthread* pThread = x_GetCurrentThread();

    while( pThread->IsActive() )
    {
        // Now do the hardware update.
        g_AudioHardware.Lock();
        g_AudioHardware.Update();
        g_AudioHardware.Unlock();
        x_DelayThread( 10 );
    }
}

//------------------------------------------------------------------------------

#ifdef ENABLE_AUDIO_DEBUG
void AudioDebug( const char* pString )
{
#ifdef X_QA    // this used to be excluded from a QA build... 
    (void) pString;
#else
    if (x_IsAtomic())
    {
        //***** BIG NOTE *****
        // If you get here when running normally, this means text was attempted to be printed
        // while interrupts were disabled (a problem on PS2). Please contact Biscuit since, if
        // this happens, this should only be in a system defined function.
        BREAK;
    }

    s32 length = snputs( pString );
    if (length < 0)
        scePrintf("%s",pString);
#endif // X_QA
}
#endif // ENABLE_AUDIO_DEBUG

//------------------------------------------------------------------------------

RVA(0x0025ac70, 0x264)
static void DecodeParameters( uncompressed_parameters* pParams, u16* pDescriptor )
{
    xbool bHasParams = GET_DESCRIPTOR_HAS_PARAMS( *pDescriptor );
    
    // Get the flags.
    pParams->Flags = *(pDescriptor+1);

    // Parameter defined?
    if( bHasParams )
    {
        u8* pDescriptorBytes;
        u32 Bits;
        u16 DataU16;

        // Skip past descriptor type, index, etc..
        pDescriptor++;

        // Skip past the flags
        pDescriptor++;

        // Skip past the size of the parameters.
        pDescriptor++;

        // Get the parameter bits.
        Bits = (u32)(*pDescriptor++);

        // Set the bits
        pParams->Bits = Bits;

        //--------------------------------//
        // Process the 16-bit parameters. //
        //--------------------------------//

        // Pitch specified?
        if( GET_PITCH_BIT( Bits ) )
        {
            DataU16 = (*pDescriptor++);
            
            // Is it a compressed 1.0f?
            if( DataU16 == FLOAT4_TO_U16BIT( 1.0f ) )
            {
                // Set it to exactly 1.0.
                pParams->Pitch = 1.0f;
            }
            else
            {
                // Decompress it to a float.
                pParams->Pitch = U16BIT_TO_FLOAT4( DataU16 );
            }
        }
        else
        {
            // Default is 1.0 since this is multiplied.
            pParams->Pitch = 1.0f;
        }

        // Pitch variance specified?
        if( GET_PITCH_VARIANCE_BIT( Bits ) )
        {
            DataU16 = (*pDescriptor++);
         
            // Is it a compressed 0.0f?
            if( DataU16 == FLOAT1_TO_U16BIT( 0.0f ) )
            {
                pParams->PitchVariance = 0.0f;
            }
            else
            {  
                pParams->PitchVariance = U16BIT_TO_FLOAT1( DataU16 );
            }
        }
        
        // Volume specified?
        if( GET_VOLUME_BIT( Bits )  )
        {
            DataU16 = (*pDescriptor++);

            // Is it a compressed 1.0f?
            if( DataU16 == FLOAT1_TO_U16BIT( 1.0f ) )
            {
                // Set it exactly to 1.0.
                pParams->Volume = 1.0f;
            }
            else
            {
                // Decompress it to a float.
                pParams->Volume = U16BIT_TO_FLOAT1( DataU16 );
            }
        }
        else
        {
            // Default is 1.0 since this is multiplied.
            pParams->Volume = 1.0f;
        }

        // Volume variance specified?
        if( GET_VOLUME_VARIANCE_BIT( Bits ) )
        {
            DataU16 = (*pDescriptor++);

            // Is it a compressed 0.0f?
            if( DataU16 == FLOAT1_TO_U16BIT( 0.0f ) )
            {
                // Set it to exactly 0.0f.
                pParams->VolumeVariance = 0.0f;
            }
            else
            {
                // Decompress it to a float.
                pParams->VolumeVariance = U16BIT_TO_FLOAT1( DataU16 );
            }
        }

        // Center volume specified?
        if( GET_VOLUME_CENTER_BIT( Bits ) )
        {
            DataU16 = (*pDescriptor++);

            // Is it a compressed 1.0f?
            if( DataU16 == FLOAT1_TO_U16BIT( 1.0f ) )
            {
                // Set it exactly to 1.0.
                pParams->VolumeCenter = 1.0f;
            }
            else
            {
                // Decompress it to a float.
                pParams->VolumeCenter = U16BIT_TO_FLOAT1( DataU16 );
            }
        }
        else
        {
            // Default is 1.0 since this is multiplied.
            pParams->VolumeCenter = 1.0f;
        }

        // LFE Volume specified?
        if( GET_VOLUME_LFE_BIT( Bits ) )
        {
            DataU16 = (*pDescriptor++);

            // Is it a compressed 1.0f?
            if( DataU16 == FLOAT1_TO_U16BIT( 1.0f ) )
            {
                // Set it exactly to 1.0.
                pParams->VolumeLFE = 1.0f;
            }
            else
            {
                // Decompress it to a float.
                pParams->VolumeLFE = U16BIT_TO_FLOAT1( DataU16 );
            }
        }
        else
        {
            // Default is 1.0 since this is multiplied.
            pParams->VolumeLFE = 1.0f;
        }

        // Volume duck specified?
        if( GET_VOLUME_DUCK_BIT( Bits ) )
        {
            DataU16 = (*pDescriptor++);

            // Is it a compressed 1.0f?
            if( DataU16 == FLOAT1_TO_U16BIT( 1.0f ) )
            {
                // Set it exactly to 1.0.
                pParams->VolumeDuck = 1.0f;
            }
            else
            {
                // Decompress it to a float.
                pParams->VolumeDuck = U16BIT_TO_FLOAT1( DataU16 );
            }
        }
        else
        {
            // Default is 1.0 since this is multiplied.
            pParams->VolumeDuck = 1.0f;
        }

        // User data specified?
        if( GET_USER_DATA_BIT( Bits ) )
            pParams->UserData = (u32)*pDescriptor++;

        //-------------------------------//
        // Process the 8-bit parameters. //
        //-------------------------------//

        // Coerce to 8-bit pointer.
        pDescriptorBytes = (u8*)pDescriptor;
        
        // Pan specified?        
        if( GET_PAN_2D_BIT( Bits ) )
            pParams->Pan2d = S8BIT_TO_FLOAT1( (*pDescriptorBytes++) );
        
        // Priority specified?
        if( GET_PRIORITY_BIT( Bits ) )
            pParams->Priority = (u32)(*pDescriptorBytes++);
        
        // Effect send specified? Default is 1.0 since this is multiplied.
        if( GET_EFFECT_SEND_BIT( Bits ) )
            pParams->EffectSend = U8BIT_TO_FLOAT1( (*pDescriptorBytes++) );
        else
            pParams->EffectSend = 1.0f;

        // Near falloff specified? Default is 1.0 since this is multiplied.
        if( GET_NEAR_FALLOFF_BIT( Bits ) )
            pParams->NearFalloff = U8BIT_TO_FLOAT10( (*pDescriptorBytes++) );
        else
            pParams->NearFalloff = 1.0f;

        // Far falloff specified? Default is 1.0 since this is multiplied.
        if( GET_FAR_FALLOFF_BIT( Bits ) )
            pParams->FarFalloff = U8BIT_TO_FLOAT10( (*pDescriptorBytes++) );
        else
            pParams->FarFalloff = 1.0f;

        // Rolloff curve specified?
        if( GET_ROLLOFF_METHOD_BIT( Bits ) )
            pParams->RolloffCurve = (u32)(*pDescriptorBytes++);

        // Near diffuse specified? Default is 1.0 since this is multiplied.
        if( GET_NEAR_DIFFUSE_BIT( Bits ) )
            pParams->NearDiffuse = U8BIT_TO_FLOAT10( (*pDescriptorBytes++) );
        else
            pParams->NearDiffuse = 1.0f;

        // Far diffuse specified? Default is 1.0 since this is multiplied.
        if( GET_FAR_DIFFUSE_BIT( Bits ) )
            pParams->FarDiffuse = U8BIT_TO_FLOAT10( (*pDescriptorBytes++) );
        else
            pParams->FarDiffuse = 1.0f;

    }
    else
    {
        // None are defined...
        pParams->Bits = 0;

        // These are multiplied, so set to 1.0
        pParams->Pitch        =
        pParams->Volume       =
        pParams->VolumeCenter =
        pParams->VolumeLFE    =
        pParams->VolumeDuck   =
        pParams->EffectSend   =
        pParams->NearFalloff  =
        pParams->FarFalloff   = 
        pParams->NearDiffuse  =
        pParams->FarDiffuse   = 1.0f;
   }

// Later descriptor-name tweak extension retained in reference/area51-audio-later; absent from complete old PC provider.
}

//------------------------------------------------------------------------------

inline voice* IdToVoice( voice_id VoiceID )
{
    voice* pVoice   = g_AudioVoiceMgr.GetVoiceBuffer();
    s32    Index    = ((VoiceID & 0xffff)-1);
    u32    Sequence = ((VoiceID >> 16) & 0x0000ffff);

    // Error check.
    if( (Index < 0) || (Index >= g_AudioVoiceMgr.GetNumVoices()) )
    {
        return NULL;
    }

    // Does the sequence match?
    if( ((pVoice+Index)->Sequence & 0x0000ffff) == Sequence )
        return pVoice+Index;
    else
        return NULL;
}

//------------------------------------------------------------------------------

inline voice_id VoiceToId( voice* pVoice )
{
    s32 Index = pVoice-g_AudioVoiceMgr.GetVoiceBuffer();

    // Error check.
    ASSERT( VALID_VOICE(pVoice) );

    // Encode index and sequence.
    return (((Index+1) & 0xffff) + (pVoice->Sequence << 16));
}

//------------------------------------------------------------------------------
// Class functions.

RVA(0x00257d90, 0x5f)
audio_mgr::audio_mgr( void )
{
    m_NearClip = 0.0f;
    m_FarClip = 1.0f;
    m_Time = 0.0f;
    m_SpeakerConfig = 0;
    m_Link.pPrev = &m_Link;
    m_Link.pNext = &m_Link;
    m_Link.pPackage = NULL;
}

//------------------------------------------------------------------------------

RVA(0x00257df0, 0x17)
audio_mgr::~audio_mgr( void )       
{
}

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

void audio_mgr::Init( s32 MemSize )
{
    MEMORY_OWNER( "audio_mgr::Init" );

        // Error check.
    ASSERT( s_Initialized == FALSE );

    m_NearClip = 0.0f;
    m_FarClip = 1.0f;
    m_Time = 0.0f;
    m_SpeakerConfig = 0;

    // Initialize the channel manager.
    g_AudioChannelMgr.Init();

    ASSERTS((MemSize & 2047)==0,"Memory must be in 2K increments");
    // Initialize the audio hardware.
    g_AudioHardware.Init(MemSize);

    // Initialize the stream manager.
    g_AudioStreamMgr.Init();

    // Initialize the virtual voices.
    g_AudioVoiceMgr.Init();

    // Nuke the identifier table.
    m_pIdentifiers.Clear();
    m_pIdentifiers.SetCapacity( 2048 );

    // The original PC branch initializes one retained worker pointer; it creates no workers here.
#if defined(TARGET_PC)
    s_PeriodicUpdateThread = NULL;
#else
    // Create the periodic update thread.
    ASSERT( s_PeriodicUpdateThread == NULL );
    ASSERT( s_HardwareUpdateThread == NULL );
    s_PeriodicUpdateThread = new xthread( AudioMgrPeriodicUpdate, (const char*)"AudioMgr PeriodicUpdate", 8192, 1 );
	s_HardwareUpdateThread = new xthread( AudioMgrHardwareUpdate, (const char*)"AudioMgr HardwareUpdate", 8192, 4 );

#endif

    // Set flag.
    s_Initialized = TRUE;
}

//------------------------------------------------------------------------------

void audio_mgr::Kill( void )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    // Release all voices.
    ReleaseAll();

    // Nuke the periodic thread.
#if !defined(TARGET_PC)
    delete s_HardwareUpdateThread;
#endif
    delete s_PeriodicUpdateThread;

#if !defined(TARGET_PC)
    s_HardwareUpdateThread = NULL;
#endif
    s_PeriodicUpdateThread = NULL;

    // Nuke the packages
    UnloadAllPackages();

    // Kill the virtual voices.
    g_AudioVoiceMgr.Kill();

    // Kill the streams.
    g_AudioStreamMgr.Kill();

    // Kill the audio hardware.
    g_AudioHardware.Kill();

    // Kill the channel manager.
    g_AudioChannelMgr.Kill();

    // Clear flag.
    s_Initialized = FALSE;
}

//------------------------------------------------------------------------------
void audio_mgr::ResizeMemory(s32 NewSize)
{
    g_AudioStreamMgr.Kill();
    g_AudioHardware.ResizeMemory( NewSize );
    g_AudioStreamMgr.Init();
}

//------------------------------------------------------------------------------
f32 audio_mgr::GetLengthSeconds( const char* pIdentifier )
{
#ifdef TARGET_PC
    f32 Result;
    voice_id VoiceID;
    ASSERT( s_Initialized);
    VoiceID = Play( pIdentifier, FALSE );
    Result = g_AudioVoiceMgr.GetVoiceTime( IdToVoice( VoiceID ) );
    Release( VoiceID, 0.0f );
    return Result;
#else
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif
    f32            Result = 0.0f;
    audio_package* pPackage;
    u16*           pDescriptor;
    char*          DescriptorName   = NULL;
    
    // Find the decriptor by name.
    pDescriptor = FindDescriptorByName( pIdentifier, &pPackage, NULL, DescriptorName );

    // Only if it could be found...
    if( pDescriptor )
    {
        u32 DescriptorIndex = (u32)(*pDescriptor++);
        u32 DescriptorType  = GET_DESCRIPTOR_TYPE( DescriptorIndex );
        u32 ParameterSize   = 0;

        // Does it have parameters?
        if( GET_DESCRIPTOR_HAS_PARAMS( DescriptorIndex ) )
        {
            // Get parameter size in WORDS (skip over flags to get to the size).
            ParameterSize = *(pDescriptor+1) >> 1;
        }

        // Bump past the flags and parameters.
        pDescriptor += 1+ParameterSize;

        switch( DescriptorType )
        {
            case SIMPLE:
            {
                u32 ElementIndex = (u32)(*pDescriptor);
                u32 ElementType  = GET_INDEX_TYPE( ElementIndex );
                u32 Index        = GET_INDEX( ElementIndex );

                switch( ElementType )
                {        
                    case COLD_INDEX:
                    {
                        // Calculate number of channels.
                        ASSERT( pPackage->m_SampleIndices[ COLD ] );
                        ASSERT( Index < (u32)pPackage->m_Header.nSampleIndices[ COLD ] );
                        if( Index >= (u32)pPackage->m_Header.nSampleIndices[ COLD ] || !pPackage->m_SampleIndices[ COLD ] )
                            return 0;
                        s32 nChannels = (s32)(pPackage->m_SampleIndices[ COLD ][ Index+1 ] - pPackage->m_SampleIndices[ COLD ][ Index ]);
                        (void)nChannels;
                        ASSERT( nChannels == 1 || nChannels == 2 );

                        u32 ColdIndex = (u32)pPackage->m_SampleIndices[ COLD ][ Index ];
                        u32 Base      = (u32)pPackage->m_ColdSamples + (ColdIndex * pPackage->m_Header.HeaderSizes[ COLD ]);
                        cold_sample* pColdSample = (cold_sample*)Base;
                
                        Result = (f32)pColdSample->nSamples / (f32)pColdSample->SampleRate;
                        break;
                    }

                    default:
                    {
                        ASSERT( 0 );
                        break;
                    }
                }
                break;
            }

            default:
                ASSERT( 0 );
                break;
        }
    }

    return Result;
#endif
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetLengthSeconds( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    f32 Result;

    Result = g_AudioVoiceMgr.GetVoiceTime( IdToVoice( VoiceID ) );
    return Result;
}

//=========================================================================
// Returns a localized filename for the current langauge. 
// The name will change ONLY if the file starts with "DX_" which indicates
// that the file is a dialogue. Note also, that this is different from 
// the naming for text files which are prepended.
const char* audio_mgr::GetLocalizedName( const char* pFileName ) const
{
    char Drive[X_MAX_DRIVE], Path[X_MAX_PATH], FName[X_MAX_FNAME], Ext[X_MAX_EXT];

    ASSERT(pFileName != NULL);
    x_splitpath(pFileName, Drive, Path, FName, Ext);

    // check that this is a dialog file
    if( (FName[0] == 'D') && (FName[1] == 'X') && (FName[2] == '_') &&
        // if this is English, leave the file name "unmangled". This should probably be fixed in the future.
        (x_strcmp(x_GetLocaleString(XL_LANG_ENGLISH), x_GetLocaleString()) != 0)
      )
    {
        static char Name[X_MAX_PATH];
        x_sprintf(Name, "%s%s%s_%s%s", Drive, Path, FName, x_GetLocaleString(), Ext);

#if defined(X_DEBUG) && defined(ctetrick)
        LOG_MESSAGE("audio_mgr::GetLocalizedName", "voice file (%s) returned as (%s)", pFileName, Name);
#endif

        return (const char*)Name;
    }
    else
    {
        return pFileName;
    }
}

//------------------------------------------------------------------------------

xbool audio_mgr::LoadPackage( const char* pFilename )
{
    CONTEXT( "audio_mgr::LoadPackage()" );
    MEMORY_OWNER( "audio_mgr::LoadPackage()" );

#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif
    xtimer t;

    t.Start();
    // Create a new audio package.
    audio_package* pPackage = new audio_package;

    char LocalizedName[X_MAX_PATH];

    x_strcpy(LocalizedName, GetLocalizedName(pFilename));

    // Load the package.
    if( pPackage->Init( LocalizedName ) )
    {
        // Set the package.
        pPackage->m_Link.pPackage = pPackage;

        // Now insert the package into the package list.
        pPackage->m_Link.pNext = m_Link.pNext;
        pPackage->m_Link.pPrev = &m_Link;
        m_Link.pNext->pPrev    = &pPackage->m_Link;
        m_Link.pNext           = &pPackage->m_Link;

        // Re-merge the identifier tables.
        MergeIdentifierTables();

        t.Stop();
        LOG_MESSAGE("audio_mgr::LoadPackage","Loaded package %s in %2.02fms",pFilename, t.ReadMs());
        // Its all good!

        return TRUE;
    }
    else
    {
        // Nuke it.
        delete pPackage;

        // Oops...
        return FALSE;
    }
}

//------------------------------------------------------------------------------

xbool audio_mgr::IsPackageLoaded( const char* pFilename )
{
    return( FindPackageByName( pFilename ) != NULL );
}

//------------------------------------------------------------------------------

xbool audio_mgr::UnloadPackage( const char* pFilename )
{
    // Find the package by its name.
    audio_package* pPackage = FindPackageByName( pFilename );

    if( (pPackage ) != NULL )
    {
        // Take it out of the list
        pPackage->m_Link.pPrev->pNext = pPackage->m_Link.pNext;
        pPackage->m_Link.pNext->pPrev = pPackage->m_Link.pPrev;

        // Kill it.
        pPackage->Kill();

        // Nuke it.
        delete pPackage;

        // Re-merge the identifier tables.
        MergeIdentifierTables();

        // All good!
        return TRUE;
    }
    else
    {
        // Oops couldn't find it.
        return FALSE;
    }
}

//------------------------------------------------------------------------------

xbool audio_mgr::LoadPackageStrings( const char* pFilename, xarray<xstring>& Strings )
{
    xbool          bNeedToLoad;
    xbool          bNeedToUnload;
    audio_package* pPackage;

    // Find the package by its name.
    pPackage    = FindPackageByName( pFilename );
    bNeedToLoad = (pPackage == NULL);

    // Need to load it?
    if( bNeedToLoad )
    {
        // Load it and find the package.
        bNeedToUnload = LoadPackage( pFilename );
        pPackage      = FindPackageByName( pFilename );
    }
    else
    {
        // No need to unload it.
        bNeedToUnload = FALSE;
    }
        
    // Found it?
    if( pPackage )
    {
        for( s32 i=0 ; i < pPackage->m_Header.nIdentifiers ; i++ )
        {
            u32   Offset;
            char* pString;

            // Calculate the string table offsets.
            Offset = pPackage->m_IdentifierTable[ i ].StringOffset;
            pString = pPackage->m_IdentifierStringTable + Offset;
            Strings.Append() = pString;
        }

        // Unload the package.
        if( bNeedToUnload )
            UnloadPackage( pFilename );         

        // Its all good!
        return TRUE;
    }
    else
    {
        // Could not find it!
        return FALSE;
    }
}

//------------------------------------------------------------------------------

void audio_mgr::Update( f32 DeltaTime )
{
    CONTEXT("audio_mgr::Update");

    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    // Update the time.
    m_Time += DeltaTime;

#if defined( rbrannon )
    void AudioThrashUpdate(void);
    AudioThrashUpdate();
#endif

    // Update the channel manager.
    g_AudioChannelMgr.Update();

    // Update the voice manager.
    g_AudioVoiceMgr.Update( DeltaTime );

    // Ok to do the audio update in hardware now.
    g_AudioHardware.SetDoHardwareUpdate();
}

//------------------------------------------------------------------------------
void audio_mgr::PeriodicUpdate( void )
{
    // Update the streams.
    g_AudioStreamMgr.Update();

    // Update the hardware.
    //Now done in it's own thread: g_AudioHardware.Update();
}

//------------------------------------------------------------------------------

RVA(0x00258410, 0x5e)
void audio_mgr::GetEar( matrix4& W2V, f32& NearClip, f32& FarClip )
{
    W2V = m_W2V;
    NearClip = m_NearClip;
    FarClip = m_FarClip;
}

//------------------------------------------------------------------------------

RVA(0x00258470, 0x5a)
void audio_mgr::SetEar( const matrix4& W2V, f32 NearClip, f32 FarClip )
{
    m_W2V = W2V;
    m_NearClip = NearClip;
    m_FarClip = FarClip;
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendHot( u32 Index, f32 DeltaTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    element* pElements[2];
    s32      nChannels;
    s32      i;

    // Calculate number of channels.
    ASSERT( pPackage->m_SampleIndices[ HOT ] );
    ASSERT( Index < (u32)pPackage->m_Header.nSampleIndices[ HOT ] );
    if( Index >= (u32)pPackage->m_Header.nSampleIndices[ HOT ] || !pPackage->m_SampleIndices[ HOT ] )
        return 0;
    nChannels = (s32)(pPackage->m_SampleIndices[ HOT ][ Index+1 ] - pPackage->m_SampleIndices[ HOT ][ Index ]);
    ASSERT( (nChannels > 0) && (nChannels <= 2 ) );
    if( (nChannels <=0) || (nChannels > 2) )
        return 0;

    // Aquire an element for each channel.
    for( i=0 ; i<nChannels ; i++ )
    {
        if( (pElements[ i ] = g_AudioVoiceMgr.AquireElement()) == NULL)
        {
            for( s32 j=0 ; j<i ; j++ )
                if( pElements[ j ] )
                    g_AudioVoiceMgr.ReleaseElement( pElements[ j ], FALSE );

            return 0;
        }
    }

    switch( nChannels )
    {
        case 1:
        {
            pElements[ 0 ]->pStereoElement = NULL;
            break;
        }

        // Stereo
        case 2:
        {
            pElements[ 0 ]->pStereoElement = pElements[ 1 ];
            pElements[ 1 ]->pStereoElement = pElements[ 0 ];
            break;
        }
    }

    // For each channel...
    u32 HotIndex = (u32)pPackage->m_SampleIndices[ HOT ][ Index ];
    u32 Base     = (u32)pPackage->m_HotSamples + (HotIndex * pPackage->m_Header.HeaderSizes[ HOT ]);
    for( i=0 ; i<nChannels ; i++, Base+=pPackage->m_Header.HeaderSizes[ HOT ] )
    {
        element* pElement = pElements[ i ];

        // Put element at end of voices element list.
        g_AudioVoiceMgr.AppendElementToVoice( pElement, pVoice );

        // Get the elements parameters.
        GetElementParameters( &pElement->Params, pDescriptor, pVoice );
        
        switch( nChannels )
        {
            case 1:
            {
                // If pan was specified, then it cannot be changed.
                if( pElement->Params.Bits & PAN_2D )
                    pElement->IsPanChangeable = FALSE;
                else
                    pElement->IsPanChangeable = TRUE;
                break;
            }

            case 2:
            {
                // Can't modify pan on stereo sample.
                pElement->IsPanChangeable = FALSE;
                if( i == 0 )
                    pElement->Params.Pan2d = -1.0f;
                else
                    pElement->Params.Pan2d =  1.0f;
            }
        }

        pElement->DeltaTime         = DeltaTime;
        pElement->State             = ELEMENT_READY;
        pElement->Type              = HOT_SAMPLE;
        pElement->Sample.pHotSample = (hot_sample*)Base;

        // Initialize the elements parameters.
        g_AudioVoiceMgr.InitSingleElement( pElement );
    }

    return nChannels;
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendWarm( u32 Index, f32 DeltaTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    (void)Index;
    (void)DeltaTime;
    (void)pDescriptor;
    (void)pVoice;
    (void)pPackage;
    return 0;
}

//------------------------------------------------------------------------------


s32 audio_mgr::AppendCold( u32 Index, f32 DeltaTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    element* pElements[2];
    s32      nChannels;
    s32      i;

    // Calculate number of channels.
    ASSERT( pPackage->m_SampleIndices[ COLD ] );
    ASSERT( Index < (u32)pPackage->m_Header.nSampleIndices[ COLD ] );
    if( Index >= (u32)pPackage->m_Header.nSampleIndices[ COLD ] || !pPackage->m_SampleIndices[ COLD ] )
        return 0;
    nChannels = (s32)(pPackage->m_SampleIndices[ COLD ][ Index+1 ] - pPackage->m_SampleIndices[ COLD ][ Index ]);
    ASSERT( (nChannels > 0) && (nChannels <= 2 ) );
    if( (nChannels <=0) || (nChannels > 2) )
        return 0;

    // Aquire an element for each channel.
    for( i=0 ; i<nChannels ; i++ )
    {
        if( (pElements[ i ] = g_AudioVoiceMgr.AquireElement()) == NULL)
        {
            for( s32 j=0 ; j<i ; j++ )
                if( pElements[ j ] )
                    g_AudioVoiceMgr.ReleaseElement( pElements[ j ], FALSE );

            return 0;
        }
    }

    switch( nChannels )
    {
        case 1:
        {
            pElements[ 0 ]->pStereoElement = NULL;
            break;
        }

        // Stereo
        case 2:
        {
            pElements[ 0 ]->pStereoElement = pElements[ 1 ];
            pElements[ 1 ]->pStereoElement = pElements[ 0 ];
            break;
        }
    }

    // For each channel...
    u32 ColdIndex = (u32)pPackage->m_SampleIndices[ COLD ][ Index ];
    u32 Base      = (u32)pPackage->m_ColdSamples + (ColdIndex * pPackage->m_Header.HeaderSizes[ COLD ]);
    for( i=0 ; i<nChannels ; i++, Base+=pPackage->m_Header.HeaderSizes[ COLD ] )
    {
        element* pElement = pElements[ i ];

        // Put element at end of voices element list.
        g_AudioVoiceMgr.AppendElementToVoice( pElement, pVoice );

        // Get the elements parameters.
        GetElementParameters( &pElement->Params, pDescriptor, pVoice );
        
        switch( nChannels )
        {
            case 1:
            {
                // If pan was specified, then it cannot be changed.
                if( pElement->Params.Bits & PAN_2D )
                    pElement->IsPanChangeable = FALSE;
                else
                    pElement->IsPanChangeable = TRUE;
                break;
            }

            case 2:
            {
                // Can't modify pan on stereo sample.
                pElement->IsPanChangeable = FALSE;
                if( i == 0 )
                    pElement->Params.Pan2d = -1.0f;
                else
                    pElement->Params.Pan2d =  1.0f;
            }
        }

        pElement->DeltaTime          = DeltaTime;
        pElement->State              = ELEMENT_NEEDS_TO_LOAD;
        pElement->Type               = COLD_SAMPLE;
        pElement->Sample.pColdSample = (cold_sample*)Base;

        // Nuke the aram (shouldn't mattter).
        pElement->Sample.pColdSample->AudioRam = 0;

        // Initialize the elements parameters.
        g_AudioVoiceMgr.InitSingleElement( pElement );
    }


    return nChannels;
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendSimple( f32 BaseTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    u32 ElementIndex  = (u32)(*pDescriptor);
    u32 ElementType   = GET_INDEX_TYPE( ElementIndex );
    u32 Index         = GET_INDEX( ElementIndex );

    switch( ElementType )
    {
        case DESCRIPTOR_INDEX:
        {
            // Look up the descriptor in the table...here we go again...WHEEE!!!
            u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
            return AppendDescriptor( BaseTime, pNewDescriptor, pVoice, pPackage );
            break;
        }

        case HOT_INDEX:
        {
            return AppendHot( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }

        case WARM_INDEX:
        {
            return AppendWarm( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }
        
        case COLD_INDEX:
        {
            return AppendCold( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }

        default:
        {
            ASSERT( 0 );
            return 0;
            break;
        }
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendComplex( f32 BaseTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    s32 Result       = 0;
    s32 ElementCount = (s32)(*pDescriptor++);

    // For each element...
    for( s32 i=0 ; i<ElementCount ; i++ )
    {
        u16 U16DeltaTime  = *pDescriptor++;
        f32 DeltaTime     = U16BIT_TO_FLOAT100( U16DeltaTime );
        u32 ElementIndex  = (u32)(*pDescriptor);
        u32 ElementType   = GET_INDEX_TYPE( ElementIndex );
        u32 Index         = GET_INDEX( ElementIndex );
        u32 ParameterSize = 0;
        
        // Does it have parameters?
        if( GET_INDEX_HAS_PARAMS( ElementIndex ) )
        {
            // Get the parameter size (convert from bytes to words)
            ParameterSize = (*(pDescriptor+2)) >> 1;
        }

        switch( ElementType )
        {
            case DESCRIPTOR_INDEX:
            {
                // Look up the descriptor in the table...here we go again...WHEEE!!!
                u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
                Result += AppendDescriptor( BaseTime+DeltaTime, pNewDescriptor, pVoice, pPackage );
                break;
            }

            case HOT_INDEX:
            {
                Result += AppendHot( Index, BaseTime+DeltaTime, pDescriptor, pVoice, pPackage );
                break;
            }

            case WARM_INDEX:
            {
                Result += AppendWarm( Index, BaseTime+DeltaTime, pDescriptor, pVoice, pPackage );
                break;
            }

            case COLD_INDEX:
            {
                Result += AppendCold( Index, BaseTime+DeltaTime, pDescriptor, pVoice, pPackage );
                break;
            }

            default:
            {
                ASSERT( 0 );
                break;
            }
        }

        // Bump past index, flags and parameters.
        pDescriptor += (ParameterSize+2);
    }

    return Result;
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendRandomList( f32 BaseTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    s32  ElementCount    = (s32)(*pDescriptor++);
    u16* pUsedWordBuffer = (u16*)pDescriptor;
    u16* pUsedWords;
    u64  UsedBits;
    s32  i;
    s32  nToPickFrom;
    s32  Choice;
    s32  Shift;
    u32  ElementIndex;
    u32  ElementType;
    u32  Index;
    u32  ParameterSize;
    u8   RandomList[64];

    // Error check
    ASSERT( ElementCount < 64 );
    
    // Construct the used bitfield (data is not 64-bit aligned, *sigh*)
    pUsedWords = pUsedWordBuffer;
    for( i=0, UsedBits=0, Shift=0 ; i<4 ; i++, Shift+=16 )
        UsedBits |= (((u64)(*pUsedWords++)) << Shift);
    
    // Have we used em all? If so, start fresh.
    if( UsedBits == (((u64)1<<ElementCount)-1) )
        UsedBits = 0;

    // Now construct the random list
    for( i=0, nToPickFrom=0 ; i<ElementCount ; i++ )
    {
        if( (UsedBits & 1<<i) == 0 )
            RandomList[ nToPickFrom++ ] = i;
    }

    // Pick one
    Choice = RandomList[ x_irand( 0, nToPickFrom-1 ) ];

    // Mark it used.
    UsedBits |= (1<<Choice);

    // Update the used bits (data *still* isn't 64-bit aligned...)
    pUsedWords = pUsedWordBuffer;
    for( i=0, Shift=0 ; i<4 ; i++, Shift+=16 )
        *pUsedWords++ = (u16)((UsedBits >> Shift) & 0xffff);

    // Now find the one we picked (skip over the used bits first).
    pDescriptor += 4;
    while( Choice-- )
    {
        ElementIndex  = (u32)(*pDescriptor);
        ParameterSize = 0;
        
        // Has parameters?
        if( GET_INDEX_HAS_PARAMS( ElementIndex ) )
        {
            // Get size of parameters in words.
            ParameterSize = (*(pDescriptor+2)) >> 1;
        }

        // Next...
        pDescriptor += (2+ParameterSize);
    }

    ElementIndex = (u32)(*pDescriptor);
    ElementType  = GET_INDEX_TYPE( ElementIndex );
    Index        = GET_INDEX( ElementIndex );

    switch( ElementType )
    {
        case DESCRIPTOR_INDEX:
        {
            // Look up the descriptor in the table...here we go again...WHEEE!!!
            u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
            return AppendDescriptor( BaseTime, pNewDescriptor, pVoice, pPackage );
            break;
        }

        case HOT_INDEX:
        {
            return AppendHot( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }

        case WARM_INDEX:
        {
            return AppendWarm( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }
        
        case COLD_INDEX:
        {
            return AppendCold( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }

        default:
        {
            ASSERT( 0 );
            return 0;
            break;
        }
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendWeightedList( f32 BaseTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{   
    s32  ElementCount  = (s32)(*pDescriptor++);
    u16* pWeights      = (u16*)pDescriptor;
    s32  Choice;
    u32  ElementIndex;
    u32  ElementType;
    u32  Index;
    u32  ParameterSize;
    u16  Weight;

    // Pick a random weight
    Weight = (u16)((x_irand( 0, 0x7fff )*2) & 0xffff);

    // Find the weight
    Choice   = 0;
    while( Choice < ElementCount )
    {
        if( Weight <= *pWeights++ )
            break;
        Choice++;
    }

    // This assert should NEVER fire unless the data gets porked.
    ASSERT( Choice < ElementCount );
    
    // Skip over the weights.
    pDescriptor += ElementCount;

    // Now find the one we picked
    while( Choice-- )
    {
        ElementIndex  = (u32)(*pDescriptor);
        ParameterSize = 0;

        // Has parameters?
        if( GET_INDEX_HAS_PARAMS( ElementIndex ) )
        {
            // Get size of parameters in WORDS.
            ParameterSize = (*(pDescriptor+2)) >> 1;
        }

        // Next!
        pDescriptor += (2+ParameterSize);
    }

    ElementIndex = (u32)(*pDescriptor);
    ElementType  = GET_INDEX_TYPE( ElementIndex );
    Index        = GET_INDEX( ElementIndex );

    switch( ElementType )
    {
        case DESCRIPTOR_INDEX:
        {
            // Look up the descriptor in the table...here we go again...WHEEE!!!
            u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
            return AppendDescriptor( BaseTime, pNewDescriptor, pVoice, pPackage );
            break;
        }

        case HOT_INDEX:
        {
            return AppendHot( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }

        case WARM_INDEX:
        {
            return AppendWarm( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }
        
        case COLD_INDEX:
        {
            return AppendCold( Index, BaseTime, pDescriptor, pVoice, pPackage );
            break;
        }

        default:
        {
            ASSERT( 0 );
            return 0;
            break;
        }
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::AppendDescriptor( f32 BaseTime, u16* pDescriptor, voice* pVoice, audio_package* pPackage )
{
    u32 DescriptorIndex = (u32)(*pDescriptor++);
    u32 DescriptorType  = GET_DESCRIPTOR_TYPE( DescriptorIndex );
    u32 ParameterSize   = 0;
    s32 Result          = 0;

    // Does it have parameters?
    if( GET_DESCRIPTOR_HAS_PARAMS( DescriptorIndex ) )
    {
        // Get parameter size in WORDS (skip over flags to get to the size).
        ParameterSize = *(pDescriptor+1) >> 1;
    }

    // Bump the recursion depth.
    if( ++pVoice->RecursionDepth > MAX_DESCRIPTOR_RECURSION_DEPTH )
    {
        ASSERT( 0 );
    }
    else
    {
        // Bump past the flags and parameters.
        pDescriptor += 1+ParameterSize;

        // Lets build us a voice!
        switch( DescriptorType )
        {
            case SIMPLE:
                Result = AppendSimple( BaseTime, pDescriptor, pVoice, pPackage );
                break;

            case COMPLEX:
                Result = AppendComplex( BaseTime, pDescriptor, pVoice, pPackage );
                break;

            case RANDOM_LIST:
                Result = AppendRandomList( BaseTime, pDescriptor, pVoice, pPackage );
                break;

            case WEIGHTED_LIST:
                Result = AppendWeightedList( BaseTime, pDescriptor, pVoice, pPackage );
                break;

            default:
                ASSERT( 0 );
                Result = 0;
                break;
        }
    }

    // Tell the world.
    --pVoice->RecursionDepth;
    return Result;
}

//------------------------------------------------------------------------------

s32 audio_mgr::IsCold( char* pIdentifier )
{
    audio_package* pPackage;
    u16*           pDescriptor;
    char*          pString;

    // Find the descriptor.
    pDescriptor = FindDescriptorByName( pIdentifier, &pPackage, NULL, pString );

    if( pDescriptor )
    {
        return IsDescriptorCold( pDescriptor, pPackage );
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::IsDescriptorCold( u16* pDescriptor, audio_package* pPackage )
{
    u32 DescriptorIndex = (u32)(*pDescriptor++);
    u32 DescriptorType  = GET_DESCRIPTOR_TYPE( DescriptorIndex );
    u32 ParameterSize   = 0;
    s32 Result          = 0;

    // Does it have parameters?
    if( GET_DESCRIPTOR_HAS_PARAMS( DescriptorIndex ) )
    {
        // Get parameter size in WORDS (skip over flags to get to the size).
        ParameterSize = *(pDescriptor+1) >> 1;
    }
    // Bump past the flags and parameters.
    pDescriptor += 1+ParameterSize;

    // Lets build us a voice!
    switch( DescriptorType )
    {
        case SIMPLE:
            Result = IsSimpleCold( pDescriptor, pPackage );
            break;

        case COMPLEX:
            Result = IsComplexCold( pDescriptor, pPackage );
            break;

        case RANDOM_LIST:
            Result = IsRandomListCold( pDescriptor, pPackage );
            break;

        case WEIGHTED_LIST:
            Result = IsWeightedListCold( pDescriptor, pPackage );
            break;

        default:
            ASSERT( 0 );
            Result = 0;
            break;
    }

    // Tell the world.
    return Result;
}

//------------------------------------------------------------------------------

s32 audio_mgr::IsSimpleCold( u16* pDescriptor, audio_package* pPackage )
{
    u32 ElementIndex  = (u32)(*pDescriptor);
    u32 ElementType   = GET_INDEX_TYPE( ElementIndex );
    u32 Index         = GET_INDEX( ElementIndex );

    switch( ElementType )
    {
        case DESCRIPTOR_INDEX:
        {
            // Look up the descriptor in the table...here we go again...WHEEE!!!
            u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
            return IsDescriptorCold( pNewDescriptor,pPackage );
            break;
        }

        case HOT_INDEX:
            return 0;

        case WARM_INDEX:
        case COLD_INDEX:
            return 1;


        default:
            ASSERT( 0 );
            return 0;
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::IsComplexCold( u16* pDescriptor, audio_package* pPackage )
{
    s32 Result       = 0;
    s32 ElementCount = (s32)(*pDescriptor++);

    // For each element...
    for( s32 i=0 ; i<ElementCount ; i++ )
    {
        // Skip over time.
        pDescriptor++;

        u32 ElementIndex  = (u32)(*pDescriptor);
        u32 ElementType   = GET_INDEX_TYPE( ElementIndex );
        u32 Index         = GET_INDEX( ElementIndex );
        u32 ParameterSize = 0;

        // Does it have parameters?
        if( GET_INDEX_HAS_PARAMS( ElementIndex ) )
        {
            // Get the parameter size (convert from bytes to words)
            ParameterSize = (*(pDescriptor+2)) >> 1;
        }

        switch( ElementType )
        {
            case DESCRIPTOR_INDEX:
            {
                // Look up the descriptor in the table...here we go again...WHEEE!!!
                u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
                if( IsDescriptorCold( pNewDescriptor, pPackage ) )
                    return 1;
                break;
            }

            case HOT_INDEX:
                break;

            case WARM_INDEX:
            case COLD_INDEX:
                return 1;

            default:
                ASSERT( 0 );
                break;
        }

        // Bump past index, flags and parameters.
        pDescriptor += (ParameterSize+2);
    }

    return Result;
}

//------------------------------------------------------------------------------

s32 audio_mgr::IsRandomListCold( u16* pDescriptor, audio_package* pPackage )
{
    s32  ElementCount = (s32)(*pDescriptor++);
    s32  i;
    u32  ElementIndex;
    u32  ElementType;
    u32  Index;
    u32  ParameterSize;

    // Error check
    ASSERT( ElementCount < 64 );

    // Skip over the used bits first.
    pDescriptor += 4;

    for( i=0 ; i<ElementCount ; i++ )
    {
        ElementIndex  = (u32)(*pDescriptor);
        ParameterSize = 0;
        ElementType   = GET_INDEX_TYPE( ElementIndex );
        Index         = GET_INDEX( ElementIndex );

        switch( ElementType )
        {
            case DESCRIPTOR_INDEX:
            {
                // Look up the descriptor in the table...here we go again...WHEEE!!!
                u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
                if( IsDescriptorCold( pNewDescriptor, pPackage ) )
                    return 1;
                break;
            }

            case HOT_INDEX:
                break;

            case WARM_INDEX:
            case COLD_INDEX:
                return 1;

            default:
                ASSERT( 0 );
                return 0;
        }

        // Has parameters?
        if( GET_INDEX_HAS_PARAMS( ElementIndex ) )
        {
            // Get size of parameters in words.
            ParameterSize = (*(pDescriptor+2)) >> 1;
        }

        // Next...
        pDescriptor += (2+ParameterSize);
    }

    return 0;
}

//------------------------------------------------------------------------------

s32 audio_mgr::IsWeightedListCold( u16* pDescriptor, audio_package* pPackage )
{    
    s32  ElementCount = (s32)(*pDescriptor++);
    s32  i;
    u32  ElementIndex;
    u32  ElementType;
    u32  Index;
    u32  ParameterSize;

    // Skip over the weights.
    pDescriptor += ElementCount;

    for( i=0 ; i<ElementCount ; i++ )
    {
        ElementIndex  = (u32)(*pDescriptor);
        ParameterSize = 0;
        ElementType   = GET_INDEX_TYPE( ElementIndex );
        Index         = GET_INDEX( ElementIndex );

        switch( ElementType )
        {
            case DESCRIPTOR_INDEX:
            {
                // Look up the descriptor in the table...here we go again...WHEEE!!!
                u16* pNewDescriptor = (u16*)pPackage->m_DescriptorTable[ Index ];
                if( IsDescriptorCold( pNewDescriptor, pPackage ) )
                    return 1;
                break;
            }

            case HOT_INDEX:
                break;

            case WARM_INDEX:
            case COLD_INDEX:
                return 1;

            default:
                ASSERT( 0 );
                return 0;
        }

        // Has parameters?
        if( GET_INDEX_HAS_PARAMS( ElementIndex ) )
        {
            // Get size of parameters in words.
            ParameterSize = (*(pDescriptor+2)) >> 1;
        }

        // Next...
        pDescriptor += (2+ParameterSize);
    }

    return 0;
}

//------------------------------------------------------------------------------

void audio_mgr::UnloadAllPackages(void)
{
#if 0
    audio_package::package_link* pLink;
    audio_package::package_link* pNext;

    // Get first package in the list.
    pLink = m_Link.pNext;

    // While we have packages...
    while( pLink->pPackage )
    {
        pNext = m_Link.pNext;
        UnloadPackage( pLink->pPackage->m_Filename );
        pLink = pNext;
    }

#else 
    // who put this code here?
    while (m_Link.pNext->pPackage)
        UnloadPackage(m_Link.pNext->pPackage->m_Filename);
#endif
}

//------------------------------------------------------------------------------

RVA(0x00258e60, 0x84)
f32 audio_mgr::ComputeFalloff( f32 PercentToFarClip, s32 TableID )
{
    ASSERT( TableID >= 0 );
    ASSERT( TableID < NUM_ROLLOFFS );
    ASSERTS( PercentToFarClip >= 0.0f, xfs("%f", PercentToFarClip) );
    ASSERTS( PercentToFarClip <= 1.0f, xfs("%f", PercentToFarClip) );

    if( PercentToFarClip <= 0.0f )
    {
        return( s_FalloffTables[TableID][0] );
    }
    else if( PercentToFarClip >= 1.0f )
    {
        return( s_FalloffTables[TableID][FALLOFF_TABLE_SIZE-1] );
    }
    else
    {
        s32 i;
        f32 f;
        f32 d;

        // Convert from [0..1] to [0..FALLOFF_TABLE_SIZE-1]
        f = PercentToFarClip * (f32)(FALLOFF_TABLE_SIZE-1);

        // Snag integer portion.
        i = (s32)f;

        // Compute delta.
        d = f - (f32)i;

        // Tell the world...
        return (s_FalloffTables[TableID][i] * (1.0f - d)) + (s_FalloffTables[TableID][i+1] * d);
    }
}

//------------------------------------------------------------------------------

RVA(0x00258ef0, 0x11e)
void audio_mgr::Calculate3dVolume( f32 NearClip, f32 FarClip, s32 VolumeRolloff,
                                 const vector3& WorldPosition, f32& Volume )
{
    vector3 Position = g_AudioMgr.m_W2V * WorldPosition;
    f32 Distance2 = Position.LengthSquared();
    FarClip *= g_AudioMgr.m_FarClip;
    if( Distance2 >= FarClip*FarClip )
        Volume = 0.0f;
    else
    {
        NearClip *= g_AudioMgr.m_NearClip;
        if( (Distance2 <= NearClip*NearClip) || ((NearClip+1.0f) >= FarClip) )
            Volume = 1.0f;
        else
        {
            f32 Distance = x_sqrt(Distance2);
            f32 Percent = (Distance-NearClip)/(FarClip-NearClip);
            Volume = ComputeFalloff(Percent,VolumeRolloff);
        }
    }
}

//------------------------------------------------------------------------------

RVA(0x002592a0, 0x51)
void audio_mgr::Calculate2dPan( f32      Pan2d,
                                vector4& Pan3d )
{
    s32     i;

    // Convert to [-90..90]
    Pan2d *= 90;

    i = (s32)Pan2d;
    if( i < -90 )
        i = -90;
    if( i > 90 )
        i = 90;

    // Get the stereo pan.
    Pan3d = m_StereoPan[ i+90 ];
}

f32 s_EarVolume       = 0.0f;
f32 s_ZoneVolume      = 0.0f;
s32 s_ZoneID          = 0;
f32 s_ZoneFinalVolume = 0.0f;

//------------------------------------------------------------------------------

// Incompatible later-revision implementation retained in reference/area51-audio-later/audio_mgr.cpp.inc; PC provider unresolved.

//------------------------------------------------------------------------------

RVA(0x00259600, 0x21)
voice_id audio_mgr::Play( const char* pIdentifier, xbool AutoStart )
{
    vector3  Dummy;
    vector3& DummyRef = Dummy;

    return Play( pIdentifier, AutoStart, DummyRef, FALSE, FALSE );
}

//------------------------------------------------------------------------------

RVA(0x00259630, 0x1b)
voice_id audio_mgr::Play( const char* pIdentifier, const vector3& Position, xbool AutoStart )
{
    return Play( pIdentifier, AutoStart, Position, TRUE, FALSE );
}

//------------------------------------------------------------------------------

RVA(0x00259650, 0x1b)
voice_id audio_mgr::PlayVolumeClipped( const char* pIdentifier, const vector3& Position, xbool AutoStart )
{
    return Play( pIdentifier, AutoStart, Position, TRUE, TRUE );
}

//------------------------------------------------------------------------------

// Later zone/volume-clip overload retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

xbool DEBUG_PLAY_IDENTIFIER_NOT_FOUND = 0;
xbool DEBUG_PLAY_ACQUIRE_VOICE_FAILED = 0;
xbool DEBUG_PLAY_VOLUME_CLIPPED       = 0;
xbool DEBUG_PLAY_SUCCESS              = 0;
xbool DEBUG_FILTER_FOOTFALL           = 0;
xbool DEBUG_FILTER_VOX                = 0;

RVA(0x00259670, 0x39d)
voice_id audio_mgr::Play( const char*    pIdentifier, 
                          xbool          AutoStart, 
                          const vector3& Position, 
                          xbool          IsPositional, 
                          xbool          bVolumeClip )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0;
#else
    ASSERT( s_Initialized );
#endif

    audio_package* pPackage;
    u16*           pDescriptor;
    s32            Index;
    f32            PositionalVolume = 1.0f;
    f32            UserVolume       = 1.0f;
    char*          DescriptorName   = NULL;

    // Later descriptor debug filtering retained in the complete reference revision; absent from this PC body.

    if( s_bDisableAudio )
    {
        return 0;
    }

#ifdef TRAP_ON_IDENTIFIER
    if( g_EnableIdentifierTrap && (x_stricmp( pIdentifier, g_DebugIdentifier ) == 0) )
        BREAK;
#endif

    // Find the decriptor by name.
    pDescriptor = FindDescriptorByName( pIdentifier, &pPackage, &Index, DescriptorName );

    // Did not find it?            
    if( pDescriptor )
    {
        uncompressed_parameters Params;
        f32                     AbsoluteVolume;

        // Decode the voices parameters, this is required to determine the priority and volume.
        GetVoiceParameters( &Params, pDescriptor, pPackage );
        
        // Is it positional?
        if( IsPositional )
        {
            // Calculate the falloffs.
            f32 Near = Params.NearFalloff * pPackage->GetComputedNearFalloff();
            f32 Far  = Params.FarFalloff  * pPackage->GetComputedFarFalloff();

            // Calculate the 3d volume.
            g_AudioMgr.Calculate3dVolume( Near, Far, Params.RolloffCurve, Position, PositionalVolume );
            
            // Is this sound volume clipped?
            if( bVolumeClip )
            {
                // Is it *REALLY* quiet...
                if( PositionalVolume <= 0.05f )
                {
                    // I'm sorry cap'n but I canna play this...the dilithium crystals are cracked!
                    #ifdef LOG_PLAY_CLIPPED            
                    LOG_MESSAGE( LOG_PLAY_CLIPPED, "'%s' was VOLUME CLIPPED!", pIdentifier );
                    #endif // LOG_PLAY_CLIPPED                   
                    
                    #if defined(ENABLE_AUDIO_DEBUG)
                    if( DEBUG_PLAY_VOLUME_CLIPPED && bDebug )
                        AudioDebug( xfs("'%s' was VOLUME CLIPPED!\n", pIdentifier) );
                    #endif //!defined(X_RETAIL)

                    s_LastPlayResult = 1;
                    return 0;
                }
            }
        }

        // Calculate voices volume.
        AbsoluteVolume = PositionalVolume * UserVolume * Params.Volume * pPackage->m_Volume;

        // TODO: Put in master fader calculation - apply it to AbsoluteVolume.

        // Attempt to aquire a voice.
        voice* pVoice = g_AudioVoiceMgr.AcquireVoice( Params.Priority, AbsoluteVolume );
        if( pVoice )
        {
#if defined(rbrannon) && defined(TRAP_ON_IDENTIFIER)
            if( g_EnableIdentifierTrap && (x_stricmp( pIdentifier, g_DebugIdentifier ) == 0) )
            {
                if( g_DebugVoice == NULL )
                {
                    g_EnableIdentifierTrap = 0;
                    g_DebugVoice = pVoice;
                    LOG_MESSAGE( "AudioDebug(audio_mgr::Play)",
                        "Trapped: %s, pVoice: %08x",
                        pIdentifier,
                        pVoice );
                }
            }
#endif
            #ifdef LOG_PLAY_SUCCESS
            LOG_MESSAGE( LOG_PLAY_SUCCESS, "pVoice: 0x%08x [Id:%08x] '%s'", pVoice, VoiceToId(pVoice), pIdentifier );
            #endif //LOG_PLAY_SUCCESS

            #if defined(ENABLE_AUDIO_DEBUG)
            if( DEBUG_PLAY_SUCCESS && bDebug )
                AudioDebug( xfs("'%s' success!!\n", pIdentifier)  );
            #endif //!defined(X_RETAIL)
                
            if( x_stristr(pPackage->m_Filename, "DX_") )
            {
                if( IsValidVoiceId(s_LastDXVoice) )
                    Release(s_LastDXVoice, 0.0f);
                s_LastDXVoice = VoiceToId(pVoice);
            }

            // Save descriptor name
            pVoice->pDescriptorName = DescriptorName;

            // Now that we have a voice, copy the parameters.
            pVoice->Params = Params;

            // Default is no ear specified!


            // Positional sound?
			if( IsPositional )
			{
                // Set the voices position.
				pVoice->Position = Position;

			}
            else
            {
                // Not positional so use the 2d pan.
                g_AudioMgr.Calculate2dPan( pVoice->Params.Pan2d, pVoice->Params.Pan3d );
            }

            // If pan was specified, then it cannot be changed.
            if( pVoice->Params.Bits & PAN_2D && !IsPositional )
                pVoice->IsPanChangeable = FALSE;
            else
                pVoice->IsPanChangeable = TRUE;

            // Force voice positional flag until parameters have been set.
            pVoice->IsPositional = TRUE;
            g_AudioVoiceMgr.SetVoiceUserFalloff( pVoice, 1.0f, 1.0f );
            g_AudioVoiceMgr.SetVoiceUserDiffuse( pVoice, 1.0f, 1.0f );
            pVoice->IsPositional = IsPositional;

            // Set the voices parameters.
            g_AudioVoiceMgr.SetVoiceUserVolume( pVoice, UserVolume );
            g_AudioVoiceMgr.SetVoiceUserPitch( pVoice, 1.0f );
            g_AudioVoiceMgr.SetVoiceUserEffectSend( pVoice, 1.0f );
            
            // Ok, now initialize the voice.
            g_AudioVoiceMgr.InitSingleVoice( pVoice, pPackage );
            
            // Set the voices descriptor.
            pVoice->pDescriptor = pDescriptor;
            
            // Append the descriptor to the voices element list.
            if( AppendDescriptor( 0.0f, pDescriptor, pVoice, pPackage ) )
            {
                // Start the voice.
                if( AutoStart )
                    g_AudioVoiceMgr.StartVoice( pVoice );

                // Its all good!
                s_LastPlayResult = 0;
                return VoiceToId(pVoice);
            }
            else
            {
                #if defined(ENABLE_AUDIO_DEBUG)
                if( DEBUG_PLAY_ACQUIRE_VOICE_FAILED && bDebug )
                    AudioDebug( xfs("'%s' could not acquire voice! (element error)\n", pIdentifier) );
                #endif //!defined(X_RETAIL)

                // Had problems, no elements in voice, so release the voice...
                g_AudioVoiceMgr.ReleaseVoice( pVoice, 0.0f );
            }
        }
        else
        {
            s_LastPlayResult = 3;
            #ifdef LOG_PLAY_FAILURE
            LOG_MESSAGE( LOG_PLAY_FAILURE, "'%s' could not acquire voice!", pIdentifier );
            #endif // LOG_PLAY_FAILURE            

            #if defined(ENABLE_AUDIO_DEBUG)
            if( DEBUG_PLAY_ACQUIRE_VOICE_FAILED && bDebug )
                AudioDebug( xfs("'%s' could not acquire voice! (voice error)\n", pIdentifier) );
            #endif //!defined(X_RETAIL)
        }
    }
    else
    {
        s_LastPlayResult = 2;
        #ifdef LOG_PLAY_WARNING
        LOG_WARNING( LOG_PLAY_WARNING, "'%s' identifier NOT FOUND!!!", pIdentifier );
        #endif // LOG_PLAY_WARNING

        #if defined(ENABLE_AUDIO_DEBUG)
        if( DEBUG_PLAY_IDENTIFIER_NOT_FOUND && bDebug )
            AudioDebug( xfs("'%s' identifier NOT FOUND!!!\n", pIdentifier) );
        #endif //!defined(X_RETAIL)
    }
   
    return 0;
}

//------------------------------------------------------------------------------

xbool audio_mgr::Segue( voice_id VoiceID, voice_id VoiceToQ )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    voice* pVoice    = IdToVoice( VoiceID );
    voice* pVoiceToQ = IdToVoice( VoiceToQ );
    if( pVoice )
    {
        return g_AudioVoiceMgr.Segue( pVoice, pVoiceToQ );
    }
    else
    {
        return FALSE;
    }
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetReleaseTime( voice_id VoiceID, f32 Time )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    voice* pVoice = IdToVoice( VoiceID );
    if( pVoice )
    {
        return g_AudioVoiceMgr.SetReleaseTime( pVoice, Time );
    }
    else
    {
        return FALSE;
    }
}

//------------------------------------------------------------------------------

void audio_mgr::Pause( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    g_AudioVoiceMgr.PauseVoice( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

void audio_mgr::PauseAll( void )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    g_AudioVoiceMgr.PauseAllVoices();
    Update( 0.015f ); // Run an update
    x_DelayThread( 15 );
    Update( 0.015f ); // Run an update
    x_DelayThread( 15 );
}

//------------------------------------------------------------------------------

void audio_mgr::Resume( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    g_AudioVoiceMgr.ResumeVoice( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

void audio_mgr::ResumeAll( void )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    while( g_IoMgr.GetDeviceQueueStatus( IO_DEVICE_DVD ) )
    {
        x_DelayThread( 10 );
//        Update( 10 );
    }
    g_AudioVoiceMgr.ResumeAllVoices();
    Update( 0.015f ); // Run an update
    x_DelayThread( 15 );
    Update( 0.015f ); // Run an update
    x_DelayThread( 15 );
}

//------------------------------------------------------------------------------
/*
void audio_mgr::ReleaseLoop( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    g_AudioVoiceMgr.ReleaseVoiceLoop( IdToVoice( VoiceID ) );
}
*/
//------------------------------------------------------------------------------

void audio_mgr::Release( voice_id VoiceID, f32 Time )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    g_AudioVoiceMgr.ReleaseVoice( IdToVoice( VoiceID ), Time );
}

//------------------------------------------------------------------------------

void audio_mgr::ReleaseAll( void )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    g_AudioVoiceMgr.ReleaseAllVoices();

    // Update
    Update( 1.0f );

    // Wait a bit...
    x_DelayThread( 15 );

    // Update
    Update( 1.0f );

    // Wait just a bit (make sure the hardware updates run).
    x_DelayThread( 15 );
}

//------------------------------------------------------------------------------

void audio_mgr::GetLoadedPackages( xarray<xstring>& Packages )
{
    ASSERT( s_Initialized );

    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    while( pLink->pPackage )
    {
        // search backwards looking for a \ or /
        s32   i = x_strlen( pLink->pPackage->m_Filename );
        char* p = &pLink->pPackage->m_Filename[ i ];

        while( (i--) && (*(p-1) != '\\') && (*(p-1) != '/') )
            p--;
        
        // Put it in the list
        Packages.Append( (xstring)p );

        // Walk the list.
        pLink = pLink->pNext;
    }
}

//------------------------------------------------------------------------------

void audio_mgr::DisplayPackages()
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return;
#else
    ASSERT( s_Initialized );
#endif

    ASSERT( s_Initialized );

    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    while( pLink->pPackage )
    {
#ifdef ENABLE_AUDIO_DEBUG
        AudioDebug( (const char*)xfs("%s\n", pLink->pPackage->m_Filename ) );
#endif // ENABLE_AUDIO_DEBUG
        x_DebugMsg( (const char*)xfs("%s\n", pLink->pPackage->m_Filename ) );
        // Walk the list.
        pLink = pLink->pNext;
    }
}

//------------------------------------------------------------------------------

xbool audio_mgr::Start( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    voice* pVoice = IdToVoice( VoiceID );
    g_AudioVoiceMgr.StartVoice( pVoice );
    return( pVoice != NULL );
}

//------------------------------------------------------------------------------

xbool audio_mgr::IsReleasing( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.IsVoiceReleasing( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

xbool audio_mgr::IsValidVoiceId( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return( IdToVoice( VoiceID ) != NULL );
}

//------------------------------------------------------------------------------

xbool audio_mgr::IsVoiceReady( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.IsVoiceReady( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

const char* audio_mgr::GetVoiceDescriptor( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return "NULL";
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetVoiceDescriptor( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetVolume( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetVoiceUserVolume( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetVolume( voice_id VoiceID, f32 Volume )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.SetVoiceUserVolume( IdToVoice( VoiceID ), Volume );
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetPan( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetVoicePan( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetPan( voice_id VoiceID, f32 Pan )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.SetVoicePan( IdToVoice( VoiceID ), Pan );
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetPitch( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetVoiceUserPitch( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetPitch( voice_id VoiceID, f32 Pitch )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    if( Pitch < 0.99f )
        LOG_MESSAGE( "audio_mgr::SetPitch", "pVoice: %08x, Pitch: %f", IdToVoice( VoiceID ), Pitch );
    return g_AudioVoiceMgr.SetVoiceUserPitch( IdToVoice( VoiceID ), Pitch );
}

//------------------------------------------------------------------------------

xbool audio_mgr::GetPosition( voice_id VoiceID, vector3& Position )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetVoicePosition( IdToVoice( VoiceID ), Position );
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetPosition( voice_id VoiceID, const vector3& Position )
{
    // Error check.
    ASSERT( s_Initialized );
    return g_AudioVoiceMgr.SetVoicePosition( IdToVoice( VoiceID ), Position );
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetFalloff( voice_id VoiceID, f32 Near, f32 Far )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.SetVoiceUserFalloff( IdToVoice( VoiceID ), Near, Far );
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetEffectSend( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetVoiceUserEffectSend( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

xbool audio_mgr::SetEffectSend( voice_id VoiceID, f32 EffectSend )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.SetVoiceUserEffectSend( IdToVoice( VoiceID ), EffectSend );
}

//------------------------------------------------------------------------------

xbool audio_mgr::HasLipSync( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.HasLipSync( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetLipSync( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetLipSync( IdToVoice( VoiceID ) ) * 2.0f;
}

//------------------------------------------------------------------------------

s32 audio_mgr::GetBreakPoints( voice_id VoiceID, f32* &BreakPoints )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetBreakPoints( IdToVoice( VoiceID ), BreakPoints );
}

//------------------------------------------------------------------------------

xbool audio_mgr::GetIsReady( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return FALSE;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetIsReady( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetCurrentPlayTime( voice_id VoiceID )
{
    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return 0.0f;
#else
    ASSERT( s_Initialized );
#endif

    return g_AudioVoiceMgr.GetCurrentPlayTime( IdToVoice( VoiceID ) );
}

//------------------------------------------------------------------------------

s32 audio_mgr::GetPackageARAM( const char* pName )
{
    ASSERT( s_Initialized );

    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    while( pLink->pPackage )
    {
        // search backwards looking for a \ or /
        s32   i = x_strlen( pLink->pPackage->m_Filename );
        char* p = &pLink->pPackage->m_Filename[ i ];

        while( (i--) && (*(p-1) != '\\') && (*(p-1) != '/') )
            p--;

        if( x_stricmp( pName, p ) == 0 )
        {
            return pLink->pPackage->m_Header.Aram;
        }

        // Walk the list.
        pLink = pLink->pNext;
    }

    return 0;
}

//------------------------------------------------------------------------------

audio_package* audio_mgr::FindPackageByName( const char* pFilename )
{
    audio_package::package_link* pLink;

    // Error check.
#ifndef AUDIO_ENABLE
    if( !s_Initialized )
        return NULL;
#else
    ASSERT( s_Initialized );
#endif

    ASSERT( pFilename );

    // Get first package in the list.
    pLink = m_Link.pNext;

    // While we have packages...
    while( pLink->pPackage )
    {
        // Names match?
        if( !x_strncmp( pFilename, pLink->pPackage->m_Filename, AUDIO_PACKAGE_FILENAME_LENGTH ) )
        {
            // All good!
            return pLink->pPackage;
        }             

        // Walk the list.
        pLink = pLink->pNext;
    }

    // Oops...couldn't find it...
    return NULL;
}

//------------------------------------------------------------------------------

char* audio_mgr::GetMusicType( const char* pFilename )
{
    audio_package* pPackage = FindPackageByName( pFilename );
    if( pPackage )
    {
        return pPackage->GetMusicType();
    }
    else
    {
        return NULL;
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::GetMusicIntensity( const char* pFilename, music_intensity* &Intensity )
{
    audio_package* pPackage = FindPackageByName( pFilename );
    if( pPackage )
    {
        return pPackage->GetMusicIntensity( Intensity );
    }
    else
    {
        Intensity = NULL;
        return      0;
    }
}


//------------------------------------------------------------------------------

void audio_mgr::ReMergeIdentifierTables( void )
{
    m_pIdentifiers.Clear();
    m_pIdentifiers.SetCapacity( 0 );
    m_pIdentifiers.FreeExtra();
    MergeIdentifierTables();
}

//------------------------------------------------------------------------------

static s32 s_nMerges = 0;

void audio_mgr::MergeIdentifierTables( void )
{
    CONTEXT("audio_mgr::MergeIdentifierTables");

    audio_package::package_link* pLink;
    xarray<audio_package*>       pPackages;
    xarray<s32>                  PackageIndices;
    s32                          i;

    // Nuke the identifer table
    m_pIdentifiers.Clear();
    pPackages.Clear();
    PackageIndices.Clear();

    // Calculate the size of the table
    pLink = m_Link.pNext;
    s32 TotalIdentifiers = 0;
    while( pLink->pPackage )
    {
        // If the package is loaded
        if( pLink->pPackage->m_IsLoaded )
        {
            // Add this package to the list
            pPackages.Append() = pLink->pPackage;

            // Initialize the number of indices processd to 0.
            PackageIndices.Append() = 0;

            // Keep track of total number of identifiers
            TotalIdentifiers += pLink->pPackage->m_Header.nIdentifiers;
        }

        // Walk the package list.
        pLink = pLink->pNext;
    }

    // Set the size of the identifier table.
    m_pIdentifiers.SetCapacity( TotalIdentifiers );

    // Perform the merge sort.
    while( pPackages.GetCount() )
    {
        i = 0;
        for( s32 j=1 ; j<pPackages.GetCount() ; j++ )
        {
            char* pSmallest;
            char* pCurrent;
            u32   SmallestOffset;
            u32   CurrentOffset;
            s32   Result;

            // Calculate the string table offsets.
            SmallestOffset = pPackages[ i ]->m_IdentifierTable[ PackageIndices[ i ] ].StringOffset;
            CurrentOffset  = pPackages[ j ]->m_IdentifierTable[ PackageIndices[ j ] ].StringOffset;

            // Get pointer to the string.
            pSmallest = pPackages[ i ]->m_IdentifierStringTable + SmallestOffset;
            pCurrent  = pPackages[ j ]->m_IdentifierStringTable + CurrentOffset;

            Result = x_strcmp( pCurrent, pSmallest );
            if( Result < 0 )
            {
                // It's smaller!
                i = j;
            }
            else if( Result == 0 )
            {
                // BAD! Had a name collision
                // TODO: Put in warning...
            }
        }

        // Merge 'em
        m_pIdentifiers.Append() = &pPackages[ i ]->m_IdentifierTable[ PackageIndices[ i ] ];
        PackageIndices[ i ]++;
        if( PackageIndices[ i ] >= pPackages[ i ]->m_Header.nIdentifiers )
        {
            pPackages.Delete( i );
            PackageIndices.Delete( i );
        }
    }
/*
    X_FILE* f = x_fopen( xfs( "ident%03d.txt", s_nMerges ), "w+t" );
    if( f )
    {
        descriptor_identifier* pIdentifier;
        audio_package*         pPackage;
        char*                  pString;

        for( i=0 ; i<m_pIdentifiers.GetCount() ; i++ )
        {
            // Get package, index and offset from table
            pIdentifier = m_pIdentifiers[ i ];
            pPackage    = pIdentifier->pPackage;
            pString     = pPackage->m_IdentifierStringTable + pIdentifier->StringOffset;
            x_fprintf( f, "%s\n", pString );
        }

        x_fclose( f );
    }
*/
    s_nMerges++;
}
//=========================================================================

xbool audio_mgr::IsValidDescriptor( const char* pName )
{
    u16*  pDescriptor;
    char* pString;

    // Find the descriptor.
    pDescriptor = g_AudioMgr.FindDescriptorByName( pName, NULL, NULL, pString );

    // Find it?
    return ( pDescriptor ) ? TRUE : FALSE;
}

//------------------------------------------------------------------------------

RVA(0x0025aac0, 0x12a)
u16* audio_mgr::FindDescriptorByName( const char* pName, audio_package** pPackageResult, s32* pIndexResult, char* &DescriptorName )
{
    s32  Left  = 0;
    s32  Right = m_pIdentifiers.GetCount()-1;
    s32  Mid;
    char ucName[128];

    ASSERT( x_strlen(pName) < 128 );
    x_strncpy( ucName, pName, 128 );
    ucName[127] = 0;
    x_strtoupper( ucName );

    if( Right < 0 )
    {
        if( pPackageResult )
            *pPackageResult = NULL;
        if( pIndexResult )
                    *pIndexResult = -1;
                return NULL;
    }

    while( 1 )
    {
        descriptor_identifier* pIdentifier;
        audio_package*         pPackage;
        char*                  pString;
        s32                    Result;

        // Get package, index and offset from table
        pIdentifier = m_pIdentifiers[ Mid = (Left+Right) >> 1 ];
        pPackage    = pIdentifier->pPackage;
        pString     = pPackage->m_IdentifierStringTable + pIdentifier->StringOffset;
        
        // Exact match?
        Result = x_strcmp( ucName, pString );

        // Smaller?
        if( Result < 0 )
        {
            if( Right == Left )
            {
                if( pPackageResult )
                    *pPackageResult = NULL;
                if( pIndexResult )
                    *pIndexResult = -1;
                return NULL;
            }
            else if (Right == Mid )
            {
                Mid = Left;
            }

            Right = Mid;
        }
        // Bigger?
        else if( Result > 0 )
        {
            if( Left == Right )
            {
                if( pPackageResult )
                    *pPackageResult = NULL;
                if( pIndexResult )
                    *pIndexResult = -1;
                return NULL;
            }
            else if (Left == Mid )
            {
                Mid = Right;
            }

            Left = Mid;
        }
        // Oooh! Found it!
        else
        {
            DescriptorName = pString;
            if( pPackageResult )
                *pPackageResult = pPackage;
            if( pIndexResult )
                *pIndexResult = pIdentifier->Index;
            return( (u16*)pPackage->m_DescriptorTable[ pIdentifier->Index ] );
        }        
    }

    if( pIndexResult )
                    *pIndexResult = -1;
                return NULL;
}

//------------------------------------------------------------------------------

RVA(0x0025abf0, 0x73)
void audio_mgr::GetVoiceParameters( uncompressed_parameters* pParams, u16* pDescriptor, audio_package* pPackage )
{
    // Inherit some parameters from parent.
    pParams->PitchVariance  = pPackage->m_Header.Params.PitchVariance;
    pParams->VolumeVariance = pPackage->m_Header.Params.VolumeVariance;
    pParams->Pan2d          = pPackage->m_Header.Params.Pan2d;
    pParams->Pan3d          = pPackage->m_Header.Params.Pan3d;
    pParams->Priority       = pPackage->m_Header.Params.Priority;
    pParams->UserData      = pPackage->m_Header.Params.UserData;
    pParams->RolloffCurve   = pPackage->m_Header.Params.RolloffCurve;

// Later descriptor-name tweak extension retained in reference/area51-audio-later; absent from complete old PC provider.

    // Decode the parameters.
    DecodeParameters( pParams, pDescriptor );

// Later descriptor-name tweak extension retained in reference/area51-audio-later; absent from complete old PC provider.
}

//------------------------------------------------------------------------------

void audio_mgr::GetElementParameters( uncompressed_parameters* pParams, u16* pDescriptor, voice* pVoice )
{
    // Inherit some parameters from parent.
    pParams->PitchVariance  = pVoice->Params.PitchVariance;
    pParams->VolumeVariance = pVoice->Params.VolumeVariance;
    pParams->Pan2d          = pVoice->Params.Pan2d;
    pParams->Pan3d          = pVoice->Params.Pan3d;
    pParams->Priority       = pVoice->Params.Priority;
    pParams->UserData       = pVoice->Params.UserData;
    pParams->RolloffCurve   = pVoice->Params.RolloffCurve;

    // Decode the parameters.
    DecodeParameters( pParams, pDescriptor );
}

//------------------------------------------------------------------------------

void audio_mgr::SetMasterVolume( f32 Volume )
{
    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    // While we have packages...
    while( pLink->pPackage )
    {
        // Set the volume
        pLink->pPackage->SetUserVolume( Volume );

        // Walk the list.
        pLink = pLink->pNext;
    }
}

//------------------------------------------------------------------------------

void audio_mgr::SetMusicVolume( f32 Volume )
{
    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    // While we have packages...
    while( pLink->pPackage )
    {
        // Names match?
        if( x_strstr( pLink->pPackage->m_Filename, "MUSIC_" ) )
        {
            // All good!
            pLink->pPackage->SetUserVolume( Volume );
        }             

        // Walk the list.
        pLink = pLink->pNext;
    }
}

//------------------------------------------------------------------------------

void audio_mgr::SetSFXVolume( f32 Volume )
{
    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    // While we have packages...
    while( pLink->pPackage )
    {
        // Names match?
        // for SFX, if it's not music and not voice, it has to be a SFX
        if( (x_strstr( pLink->pPackage->m_Filename, "MUSIC_" ) == 0) &&
            (x_strstr( pLink->pPackage->m_Filename, "DX_"    ) == 0) )
        {
            // All good!
            pLink->pPackage->SetUserVolume( Volume );
        }             

        // Walk the list.
        pLink = pLink->pNext;
    }
}

//------------------------------------------------------------------------------

void audio_mgr::SetVoiceVolume( f32 Volume )
{
    audio_package::package_link* pLink;

    // Get first package in the list.
    pLink = m_Link.pNext;

    // While we have packages...
    while( pLink->pPackage )
    {
        // Names match?
        if( x_strstr( pLink->pPackage->m_Filename, "DX_" ) )
        {
            // All good!
            pLink->pPackage->SetUserVolume( Volume );
        }             

        // Walk the list.
        pLink = pLink->pNext;
    }
}

//------------------------------------------------------------------------------

u32 audio_mgr::GetUserData( const char* pIdentifier )
{
    audio_package* pPackage;
    u16*           pDescriptor;
    char*          pString;
    u32            Result = 0;

    // Find the descriptor.
    pDescriptor = FindDescriptorByName( pIdentifier, &pPackage, NULL, pString );

    // Find it?
    if( pDescriptor )
    {
        uncompressed_parameters Params;
    
        // Decode the parameters.
        GetVoiceParameters( &Params, pDescriptor, pPackage );

        // Get the user data.
        Result = Params.UserData;
    }

    // Tell the world!
    return Result;
}

//------------------------------------------------------------------------------

xbool audio_mgr::GetVoiceParameters( const char* pIdentifier, uncompressed_parameters& Params )
{
    audio_package* pPackage;
    u16*           pDescriptor;
    char*          pString;

    // Find the descriptor.
    pDescriptor = FindDescriptorByName( pIdentifier, &pPackage, NULL, pString );

    // Find it?
    if( pDescriptor )
    {
        // Decode the parameters.
        GetVoiceParameters( &Params, pDescriptor, pPackage );
        return TRUE;
    }
    return FALSE;
}

//------------------------------------------------------------------------------

s32 audio_mgr::GetPriority( voice_id VoiceID )
{
    voice* pVoice = IdToVoice( VoiceID );
    if( pVoice )
    {
        return pVoice->Params.Priority;
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------

s32 audio_mgr::GetPriority( const char* pIdentifier )
{
    uncompressed_parameters Params;
    
    // Decode the parameters.
    if( GetVoiceParameters( pIdentifier, Params ) )
        return Params.Priority;
    else
        return 0;
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetFarFalloff( const char* pIdentifier )
{
    uncompressed_parameters Params;
    
    // Decode the parameters.
    if( GetVoiceParameters( pIdentifier, Params ) )
        return Params.FarFalloff;
    else
        return 0.0f;
}

//------------------------------------------------------------------------------

f32 audio_mgr::GetNearFalloff( const char* pIdentifier )
{
    uncompressed_parameters Params;
    
    // Decode the parameters.
    if( GetVoiceParameters( pIdentifier, Params ) )
        return Params.NearFalloff;
    else
        return 0.0f;
}

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

void audio_mgr::SetSpeakerAngles( s32 FrontLeft, s32 FrontRight, s32 BackRight, s32 BackLeft, s32 nSpeakers )
{
    s32 i;
    s32 j;
    f32 DeltaTheta;
    f32 Angle;
    f32 Q;
    f32 P0;
    f32 P1;

    // Set class variables
    m_FrontLeft  = FrontLeft;
    m_FrontRight = FrontRight;
    m_BackRight  = BackRight;
    m_BackLeft   = BackLeft;
    m_nSpeakers  = nSpeakers;

    // Default the stereo pan.
    DeltaTheta = 180.0f;
    for( i=0 ; i<180 ; i++ )
    {
        Angle = (f32)(i);
        Q = Angle / DeltaTheta;
        P0 = x_sqrt( 1.0f - Q );
        P1 = x_sqrt( Q );
        m_StereoPan[i].Set( P0, P1, 0.0f, 0.0f );
    }
    m_StereoPan[180].Set( 0.0f, 1.0f, 0.0f, 0.0f );

    switch( nSpeakers )
    {
        case 1:
            // Set the diffuse vector for 1 speaker.
            m_Diffuse.Set( 1.0f, 1.0f, 0.0f, 0.0f );

            for( i=0 ; i<PAN_TABLE_ENTRIES ; i++ )
            {
                m_Pan[i].Set( 1.0f, 1.0f, 0.0f, 0.0f );
            }
        
            for( i=0 ; i<=180 ; i++ )
            {
                m_StereoPan[i].Set( 1.0f, 0.0f, 0.0f, 0.0f );
            }
            break;
        
        // Stereo
        case 2:
            ASSERT( FrontLeft  <= 0 );
            ASSERT( FrontRight >= 0 );

            // Set the diffuse vector for 2 speakers.
            P0 = 1.0f / x_sqrt( 2.0f );
            m_Diffuse.Set( P0, P0, 0.0f, 0.0f );

            DeltaTheta = (f32)x_abs( FrontRight - FrontLeft );
            for( i=FrontLeft ; (i<=FrontRight)  && (i<360) ; i++ )
            {
                j = i;
                if( j<0 )
                    j += 360;
                Angle = (f32)(i-FrontLeft);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[j].Set( P0, P1, 0.0f, 0.0f );
                /*
                LOG_MESSAGE( "audio_mgr::SetSpeakerAngles",
                            "%d: [%f,%f]",
                            j, m_Pan[j].GetX(), m_Pan[j].GetY() );
                            */
            }
            
            DeltaTheta = (f32)x_abs( FrontLeft+360 - FrontRight );
            for( i=FrontRight ; (i<= FrontLeft+360) && (i<360) ; i++ )
            {
                Angle = (f32)(i-FrontRight);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[i].Set( P1, P0, 0.0f, 0.0f );
                /*
                LOG_MESSAGE( "audio_mgr::SetSpeakerAngles",
                            "%d: [%f,%f]",
                            i, m_Pan[i].GetX(), m_Pan[i].GetY() );
                            */
            }
            break;

        // Surround
        case 3:
            ASSERT( FrontLeft  <= 0 );
            ASSERT( FrontRight >= 0 );
            ASSERT( BackRight  >= 0 );
            ASSERT( FrontLeft  <= FrontRight );
            ASSERT( FrontRight <= BackRight );
            ASSERT( BackRight  <= (FrontLeft+360) );

            // Set the diffuse vector for 3 speakers.
            P0 = 1.0f / x_sqrt( 3.0f );
            m_Diffuse.Set( P0, P0, P0, 0.0f );

            DeltaTheta = (f32)x_abs( FrontRight - FrontLeft );
            for( i=FrontLeft ; i<=FrontRight ; i++ )
            {
                j = i;
                if( j<0 )
                    j += 360;
                Angle = (f32)(i-FrontLeft);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[j].Set( P0, P1, 0.0f, 0.0f );
                /*
                LOG_MESSAGE( "audio_mgr::SetSpeakerAngles",
                            "%d: [%f,%f]",
                            j, m_Pan[j].GetX(), m_Pan[j].GetY() );
                            */
            }

            DeltaTheta = (f32)x_abs( BackRight - FrontRight );
            for( i=FrontRight ; i<=BackRight ; i++ )
            {
                Angle = (f32)(i-FrontRight);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[i].Set( 0.0f, P0, P1, 0.0f );
                /*
                LOG_MESSAGE( "audio_mgr::SetSpeakerAngles",
                            "%d: [%f,%f]",
                            i, m_Pan[i].GetX(), m_Pan[i].GetY() );
                            */
            }

            DeltaTheta = (f32)x_abs( FrontLeft+360 - BackRight );
            for( i=BackRight ; i<= FrontLeft+360 ; i++ )
            {
                Angle = (f32)(i-BackRight);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[i].Set( P1, 0.0f, P0, 0.0f );
                /*
                LOG_MESSAGE( "audio_mgr::SetSpeakerAngles",
                            "%d: [%f,%f]",
                            i, m_Pan[i].GetX(), m_Pan[i].GetY() );
                            */
            }
            break;

        // 5.1
        case 4:
            ASSERT( FrontLeft  <= 0 );
            ASSERT( FrontRight >= 0 );
            ASSERT( BackRight  >= 0 );
            ASSERT( BackLeft   >= 0 );
            ASSERT( FrontLeft  <= FrontRight );
            ASSERT( FrontRight <= BackRight );
            ASSERT( BackRight  <= BackLeft );
            ASSERT( BackLeft   <= (FrontLeft+360) );

            // Set the diffuse vector for 4 speakers.
            P0 = 1.0f / x_sqrt( 4.0f );
            m_Diffuse.Set( P0, P0, P0, P0 );

            DeltaTheta = (f32)x_abs( FrontRight - FrontLeft );
            for( i=FrontLeft ; i<FrontRight ; i++ )
            {
                j = i;
                if( j<0 )
                    j += 360;
                Angle = (f32)(i-FrontLeft);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[j].Set( P0, P1, 0.0f, 0.0f );
            }

            DeltaTheta = (f32)x_abs( BackRight - FrontRight );
            for( i=FrontRight ; i<BackRight ; i++ )
            {
                Angle = (f32)(i-FrontRight);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[i].Set( 0.0f, P0, P1, 0.0f );
            }

            DeltaTheta = (f32)x_abs( BackLeft - BackRight );
            for( i=BackRight ; i<BackLeft ; i++ )
            {
                Angle = (f32)(i-BackRight);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[i].Set( 0.0f, 0.0f, P0, P1 );
            }

            DeltaTheta = (f32)x_abs( FrontLeft+360 - BackLeft );
            for( i=BackLeft ; i< FrontLeft+360 ; i++ )
            {
                Angle = (f32)(i-BackLeft);
                Q = Angle / DeltaTheta;
                P0 = x_sqrt( 1.0f - Q );
                P1 = x_sqrt( Q );
                m_Pan[i].Set( P1, 0.0f, 0.0f, P0 );
            }
            break;

        default:
            ASSERT(0);
            break;
    }
}

//------------------------------------------------------------------------------

RVA(0x0025b6b0, 0x3f)
void audio_mgr::GetSpeakerAngles( s32& FrontLeft, s32& FrontRight, s32& BackRight, s32& BackLeft, s32& nSpeakers )
{
    // Set class variables
    FrontLeft  = m_FrontLeft;
    FrontRight = m_FrontRight;
    BackRight  = m_BackRight;
    BackLeft   = m_BackLeft;
    nSpeakers  = m_nSpeakers;
}

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision implementation retained in reference/area51-audio-later/audio_mgr.cpp.inc; PC provider unresolved.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

void audio_mgr::SetSpeakerConfig( s32 SpeakerConfig )
{
    m_SpeakerConfig = SpeakerConfig;

    switch( SpeakerConfig )
    {
        case SPEAKERS_MONO:
            SetSpeakerAngles( 0, 0, 0, 0, 1 );
            break;
        case SPEAKERS_STEREO:
            SetSpeakerAngles( -90, 90, 0, 0, 2 );
            break;
        case SPEAKERS_PROLOGIC:
            SetSpeakerAngles( -45, 45, 45+90, 45+180, 4 );
            //SetSpeakerAngles( -90, 90, 0, 0, 2 );
            break;
        case SPEAKERS_DOLBY_DIGITAI_5_1:
            SetSpeakerAngles( -45, 45, 45+90, 45+180, 4 );
            break;
        default:
            ASSERT( 0 );
            break;
    }
}

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.
            
//------------------------------------------------------------------------------

void audio_mgr::SetClip( f32 NearClip, f32 FarClip )
{
    m_NearClip = NearClip;
    m_FarClip  = FarClip;
}

//------------------------------------------------------------------------------

void audio_mgr::GetClip( f32& NearClip, f32& FarClip )
{
    NearClip = m_NearClip;
    FarClip  = m_FarClip;
}
//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

//------------------------------------------------------------------------------

// Incompatible later-revision method retained in reference/area51-audio-later/audio_mgr.cpp.inc.

#if defined( rbrannon )

xbool AUDIO_THRASH = FALSE;
s32 ThrashID[32] = {
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

char* ThrashDescriptors[5] = {
"Mutant1_Footfall_Toe_Metal",
"Mutant1_Vox_Run",
"SMP_Fire",
"Mutant1_PainGrunt",
"Mutant1_Vox_LeapAttack"
};

s32 iThrash = 0;

void AudioThrashUpdate( void )
{
    if( AUDIO_THRASH )
    {
        s32 i = x_rand() % 32;
        g_AudioMgr.Release( ThrashID[i], 0.0f );
        s32 j = x_rand() % 5;
        ThrashID[i] = g_AudioMgr.Play( ThrashDescriptors[j] );
        x_DebugMsg( "voice[%d]: %08x\n", i,ThrashID[i] );
    }
    else
    {
        for( s32 i=0 ; i<32 ; i++ )
        {
            if( g_AudioMgr.IsValidVoiceId( ThrashID[i] )  )
            {
                g_AudioMgr.Release( ThrashID[i], 0.0f );
            }
        }
    }
}

void AudioThrashCheck( void )
{
    s32 i;
    return;
    if( !AUDIO_THRASH )
    {
        // Turn it on.
        for( i=0 ; i<32 ; i++ )
        {
            s32 j = x_rand() % 5;
            ThrashID[i] = g_AudioMgr.Play( ThrashDescriptors[j] );
            x_DebugMsg( "voice[%d]: %08x\n", i, ThrashID[i] );
        }
        iThrash = 0;
    }
    else
    {
        // Turn it off.
        for( i=0 ; i<32 ; i++ )
        {
            g_AudioMgr.Release( ThrashID[i], 0.0f );
        }
    }
}
#endif

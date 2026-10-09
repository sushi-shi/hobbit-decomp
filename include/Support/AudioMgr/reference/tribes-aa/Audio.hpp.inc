//==============================================================================
//  Audio.hpp
//==============================================================================

#ifndef AUDIO_HPP
#define AUDIO_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/e_View.hpp>

//==============================================================================
//  DEFINES
//==============================================================================

#define SOUNDTYPE_NONE          0
#define AUDFLAG_LOCALIZED       0
#define AUDFLAG_PERSISTENT      0
#define AUDFLAG_DEFER_PLAY      0
#define AUDFLAG_CHANNELSAVER    0

//==============================================================================
//  TYPES
//==============================================================================

struct audio_channel { };

//==============================================================================
//  FUNCTIONS
//==============================================================================

void    audio_Init                  ( void );
void    audio_Kill                  ( void );
s32     audio_LoadContainer         ( char* pFileName );
void    audio_UnloadContainer       ( s32 ContainerId );
void    audio_SetContainerVolume    ( s32 ContainerId, f32 Volume );
f32     audio_GetContainerVolume    ( s32 ContainerId );
void    audio_Update                ( void) ;
void    audio_SetMasterVolume       ( f32 left, f32 right );
void    audio_SetMasterVolume       ( f32 vol );
void    audio_SetSurround           ( xbool mode );
void    audio_LockChannelList       ( void );
void    audio_UnlockChannelList     ( void );
void    audio_SetEarView            ( view* pView );
void    audio_SetEarView            ( view* pView, f32 nearclip, f32 falloff, f32 farclip, f32 clipvol );
void    audio_AddEarView            ( view* pView );
void    audio_SetClip               ( f32 nearclip, f32 falloff, f32 farclip, f32 clipvol );
void    audio_QueueUpdateRequest    ( audio_channel* pChannel );
s32     audio_Play                  ( u32 SampleId );
s32		audio_Play			        ( u32 SampleId, s32 Flags );
s32     audio_Play                  ( u32 SampleId, vector3* pPosition );
s32     audio_Play                  ( u32 SampleId, vector3* pPosition, s32 Flags );
void    audio_StopAll               ( void );
void    audio_Stop                  ( u32 ChannelId );
void    audio_Pause                 ( u32 ChannelId );
void    audio_SetVolume             ( u32 ChannelId, f32 Volume );
void    audio_SetPan                ( u32 ChannelId, f32 Pan );
void    audio_SetRearPan            ( u32 ChannelId, f32 Pan );
void    audio_SetPitch              ( u32 ChannelId, f32 Pitch);
void    audio_SetPosition           ( u32 ChannelId, vector3* pPosition );
void    audio_SetFlags              ( u32 ChannelId, s32 Flags );
xbool   audio_IsPlaying             ( u32 ChannelId );
void    audio_InitLooping           ( s32& ChannelId );
void    audio_PlayLooping           ( s32& ChannelId, u32 SampleId );
void    audio_PlayLooping           ( s32& ChannelId, u32 SampleId, vector3* pPos );
void    audio_StopLooping           ( s32& ChannelId );

//==============================================================================
#endif 
//==============================================================================

#ifndef __audio_COMMON_H
#define __audio_COMMON_H

#ifdef TARGET_PS2_IOP
#include "ioptypes.h"
#else
#include "x_types.hpp"
#include "x_math.hpp"
#endif

/*
    This file contains all common structures and defs used between the iop and ee.

    DEFINITION OF TERMS:

  CHANNEL:          a virtual audio stream existing on the EE. May not necessarily have an associated voice
  VOICE:            a physical audio playback stream
  CFX:              Sound effect made from multiple samples or cfx's
  SAMPLE:           Individual audio atom. This is what will be processed by the SPU to produce audio
  COMPLEX EFFECT:   Same as cfx
  SFX:              Same as sample
  PAN:              Position of the sound. 0 is far left, 1 is far right
  REARPAN:          Position of the sound in surround space, 0 is far forward, 1 is far back
  PITCH:            Amount of pitch shift placed on sample 0.5=1 octave down, 2.0=1 octave up

*/

typedef enum {
    AUDSTAT_FREE = 0x00,            // Channel is free for allocation
    AUDSTAT_IDLE,                   // Channel is idle
    AUDSTAT_STARTING,               // Channel is just beginning to play (fade up)
    AUDSTAT_PLAYING,                // Channel is playing
    AUDSTAT_DELAYING,               // Channel was started and allocated but has a delay attached
    AUDSTAT_PAUSING,                // Channel is in the process of pausing
    AUDSTAT_PAUSED,                 // Channel is paused
    AUDSTAT_STOPPING,               // Channel is stopping (in fade cycle)
    AUDSTAT_STOPPED,                // Channel is stopped
    AUDSTAT_DELAYED,                // Channel is delayed (due to no hardware available)
    AUDSTAT_OUTOFRANGE,             // Channel sound is out of audio focal range
} audstat;

enum {
    B_AUDFLAG_LOOPED=0,             // Software looped effect
    B_AUDFLAG_3D_POSITION,          // 3d positioned sound
    B_AUDFLAG_CHANNELSAVER,         // remove any channels currently playing of this type before playing this
    B_AUDFLAG_QUEUED,               // Queue the sound to be played when the previous streamed sound is finished
    B_AUDFLAG_DEFER_PLAY,           // Suspend playing until an audio_SetPitch(1.0f) is issued. Will pre-buffer streams
// All permanent flags need to be at the end
    B_AUDFLAG_PERSISTENT=8,
    B_AUDFLAG_LOCALIZED,
    B_AUDFLAG_ALLOCATED,
    B_AUDFLAG_PAUSED,
    B_AUDFLAG_DELAYED,
    B_AUDFLAG_STEREO,
    B_AUDFLAG_HARDWARE_LOOP,
    B_AUDFLAG_ENVIRONMENT,
};

#define AUDFLAG_FREE            (0)
#define AUDFLAG_PERSISTENT      (1<<B_AUDFLAG_PERSISTENT)       // Sound is an effect that may move in and out of range and should be dynamically started
#define AUDFLAG_ALLOCATED       (1<<B_AUDFLAG_ALLOCATED)        // This channel is allocated
#define AUDFLAG_PAUSED          (1<<B_AUDFLAG_PAUSED)           // This channel is paused
#define AUDFLAG_DELAYED         (1<<B_AUDFLAG_DELAYED)          // This channel is delayed due to insufficent resources
#define AUDFLAG_LOOPED          (1<<B_AUDFLAG_LOOPED)           // This is a software looped channel
#define AUDFLAG_LOCALIZED       (1<<B_AUDFLAG_LOCALIZED)        // Sound initially has it's volume/pan set but will not be updated when the environment moves
#define AUDFLAG_3D_POSITION     (1<<B_AUDFLAG_3D_POSITION)      // This is a 3d positioned sound.
#define AUDFLAG_CHANNELSAVER    (1<<B_AUDFLAG_CHANNELSAVER)     // if an effect of the same id is being played, kill that one first
#define AUDFLAG_STEREO          (1<<B_AUDFLAG_STEREO)           // It's stereo!
#define AUDFLAG_HARDWARE_LOOP   (1<<B_AUDFLAG_HARDWARE_LOOP)    // This sfx is looped using hardware
#define AUDFLAG_ENVIRONMENT     (1<<B_AUDFLAG_ENVIRONMENT)      // This is an environmental emitter.
#define AUDFLAG_QUEUED          (1<<B_AUDFLAG_QUEUED)
#define AUDFLAG_PURGE           (1<<B_AUDFLAG_PURGE)
#define AUDFLAG_DEFER_PLAY      (1<<B_AUDFLAG_DEFER_PLAY)       // Suspend playing until an audio_SetPitch(1.0f) is issued. Will pre-buffer streams

#define AUDFLAG_PRIVATE_MASK    0xff00

#define AUD_MAX_CHANNELS        128
#define AUD_TICKS_PER_SECOND    1000
#define AUD_MAX_VOLUME          16383
#define AUD_BASE_PITCH          0x1000
#define AUD_DEFAULT_PAN         (0.5f)
#define AUD_DEFAULT_PITCH       (1.0f)
#define AUD_DEFAULT_VOL         (1.0f)
#define AUD_CENTER_PAN          (0)
#define AUD_MAX_PAN             (32767)
#define AUD_PAN_CENTER_SURROUND (16384)
#define AUD_CLIP_SCALAR         (256)
#define AUD_FIXED_POINT_1       (10000)
#define AUD_CHANNEL_ID_BITS     (16)
#define AUD_CHANNEL_ID_MASK     ((1<<AUD_CHANNEL_ID_BITS)-1)

#define AUD_DEFAULT_NEARCLIP    (10.0f)
#define AUD_DEFAULT_FARCLIP     (1000.0f)
#define AUD_DEFAULT_FALLOFF     (40.0f)
#define AUD_DEFAULT_CLIPVOLUME  (0.2f)
#define AUD_MAX_EAR_VIEWS       (2)
enum {
    AUDCMD_BASE = 0x00010000,
    AUDCMD_INIT,
    AUDCMD_KILL,
    AUDCMD_UPDATEMASTER,
    AUDCMD_UPDATE,
    AUDCMD_LOADCONTAINER,
    AUDCMD_UNLOADCONTAINER,
    AUDCMD_GETCONTAINERHEADER,
    AUDCMD_STREAM_INIT,
    AUDCMD_STREAM_KILL,
    AUDCMD_STREAM_SEND_DATA_LEFT,
    AUDCMD_STREAM_SEND_DATA_RIGHT,
    AUDCMD_CONTAINERVOLUME,
    AUDCMD_ISLOADCOMPLETE,
};

enum {
    AUD_COMMAND_UPDATE_CFX = 0x00000000,
    AUD_COMMAND_UPDATE_MASTER,
    AUD_COMMAND_END,
};


typedef struct s_audio_global_status
{
    s32 m_ChannelCount;
    s16 m_LeftVolume;
    s16 m_RightVolume;
    s32 m_NearClip;
    s32 m_FallOff;
    s32 m_FarClip;
} audio_global_status;

typedef struct s_audio_init
{
    s32     m_nChannels;
} audio_init;

#ifdef TARGET_PC
typedef struct s_cfx_state
{
	u32 m_Id;
	u32 m_UniqueId;
	s32 m_CfxId;
	vector3 m_Position;
	s16 m_Pitch;
	s16 m_Volume;
    s16 m_Flags;
	
} cfx_state;
#else

typedef struct s_cfx_state
{
    u32 m_Id;                       // Passed back to the application
    u32 m_UniqueId;                 // Unique loop identifier used within sound manager
    s32 m_CfxId;                    // Sound effect id (-1 to kill this one)
    s16 m_Pitch;                    // Pitch (1.0 = 10000)
    s16 m_Pan;                      // Pan   (1.0 = 10000)
    s16 m_Volume;                   // Volume (1.0 = 10000)
    s16 m_RearVol;                  // Rear volume (1.0 = 10000)
	s16 m_Flags;
} cfx_state;
#endif

typedef struct s_cfx_info
{
    s16 m_Pitch;
    s16 m_Pan;
    s16 m_Volume;
    s16 m_Falloff;
} cfx_info;

typedef struct s_container_reply
{
    s32 nCfxCount;
    s32 Status;
    s32 Id;
} container_reply;

typedef struct s_container_hdr_request
{
    s32         Id;        // Container to get header info for
    s32         Index;              // Starting index to pull data for
    s32         Count;              // Number that we're requesting
} container_hdr_request;

#define PACKAGE_OUT_BUFFER_SIZE ((sizeof(cfx_state)+1) * (AUD_MAX_CHANNELS+10) + sizeof(audio_global_status)+2)
#define PACKAGE_IN_BUFFER_SIZE (sizeof(s32) * AUD_MAX_CHANNELS * 2)

#endif
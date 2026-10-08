#ifndef __IOPAUDIO_H
#define __IOPAUDIO_H

#include "x_types.hpp"
#include "pcaud_cfx.hpp"
#include "pcaud_voice.hpp"
#include "pcaud_container.hpp"
#include "pcaud_host.hpp"
#include "pcaud_stream.hpp"

#define ENABLE_SURROUND

#define CORES_IN_USE            2
#define VOICES_PER_CORE         24
#define AUD_MAX_PHYS_VOICES     (VOICES_PER_CORE * CORES_IN_USE)
#define AUD_PRIVATE_CHANNELS    16      // Used when we're processing complex effects
#define AUD_MAX_STREAMS         6       // Total # of streaming channels possible
#define AUD_STREAM_BUFFER_SIZE  (2048)  // Buffer size on spu for each stream (*2 for double buffer)
#define AUD_STREAM_READAHEAD    24

#define AUD_BASE_SAMPLE_RATE    48000
#define SET_KEY_ON(chn) SpuSetKey(chn,1)
#define SET_KEY_OFF(chn) SpuSetKey(chn,0)


typedef struct s_pc_audio_vars
{
    cfx_pool*       m_pCfxPool;
    cfx*            m_pRootCfx;
    audio_voice*    m_pVoices;
    xmesgq*         m_pqFreeVoices;
    container*      m_pContainers;
    f64             m_TimerTick;                    // Ticks in 10ms intervals with 0.1ms accuracy
    s32             m_ChannelCount;
    u32             m_LastVoiceId;
    s32             m_VoicesToKill;
    s32             m_VoicesInUse;
    s32             m_LowestKillPriority;
    s32             m_SysTimerId;
    u32             m_TopCfxId;
    u32             m_TopUniqueId;
    xthread*        m_pUpdateThread;
    xmutex*			m_pMutex;
    u8*				m_pInBuffer;
    u8*				m_pOutBuffer;
    spu_master_state m_MasterState;                 // State we want to write to hardware
    spu_master_state m_CurrentMasterState;          // State we just read from hardware on this update cycle
    xthread*		m_pStreamThread;
    stream_request  m_StreamRequests[AUD_MAX_STREAMS * AUD_STREAM_READAHEAD];
    byte*			m_pStreamBuffers;
    xmesgq*			m_pqAvailableStreamRequests;
    xmesgq*			m_pqPendingStreamRequests;
    xmesgq*			m_pqFreeStreams;
    cfx_element     m_StreamerElement[2];
    cfx_element_attributes m_StreamerAttributes[2];
    cfx             *m_pStreamerCfx[2];
} pc_audio_vars;

s32 AudioDispatch(u32 Command,void *Data,s32 Size);

void    pcaudio_DupState(cfx_state *pDest,cfx_state *pParent,cfx_state *pOurs);

extern pc_audio_vars g_pcAudio;
void *  pcAudio_Init(audio_init *pInit);
void    pcAudio_Kill(void);
void    pcAudio_Update(u8 *pBuffer);

#endif // __IOPAUDIO_H
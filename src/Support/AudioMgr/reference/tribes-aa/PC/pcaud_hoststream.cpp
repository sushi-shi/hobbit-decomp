#include "x_files.hpp"
#include "pcaud_audio.hpp"
#include "pcaud_element.hpp"
#include "pcaud_voice.hpp"
#include "pcaud_hoststream.hpp"
#include "x_threads.hpp"

static cfx *hs_Init(s32 index,s32 Pitch)
{
    cfx *pCfx;
    cfx_element *pElement;

    pCfx             = cfx_AllocFromPool();
    pElement = &g_pcAudio.m_StreamerElement[index];

    pCfx->m_pElements       = pElement;
    pCfx->m_Count           = 1;
    pCfx->m_State.m_Volume  = AUD_FIXED_POINT_1;

//    if (index)
//        pCfx->m_State.m_Pan     = AUD_FIXED_POINT_1;
//    else
//        pCfx->m_State.m_Pan     = -AUD_FIXED_POINT_1;

    pCfx->m_State.m_Pitch   = AUD_FIXED_POINT_1;
    pCfx->m_State.m_UniqueId= (u32)-1;
//    pCfx->m_State.m_RearVol = 0;

    pElement->m_Flags         = AUDFLAG_ALLOCATED;
    pElement->m_Length       = (1<<24);
    pElement->m_pAttributes = &g_pcAudio.m_StreamerAttributes[index];
    pElement->m_pAttributes->m_SampleRate   = 44100;
    pElement->m_pAttributes->m_Type         = CFXTYPE_HOST_STREAM;
    pElement->m_pAttributes->m_Pitch        = Pitch;
    pElement->m_pAttributes->m_Volume       = AUD_FIXED_POINT_1;
    pElement->m_pAttributes->m_Pan          = 0;

    pElement->m_pStream       = voicestream_Alloc(pElement);
    cfx_InsertForUpdate(pCfx);
    return pCfx;
}

void hs_Kill(s32 index)
{
    s32 status;
    cfx_RemoveFromUpdate(g_pcAudio.m_pStreamerCfx[index]);
    while (1)
    {
        status = voicestream_Free(g_pcAudio.m_StreamerElement[index].m_pStream);
        if (status)
            break;
        x_DelayThread(10);
    }
    cfx_Free(g_pcAudio.m_pStreamerCfx[index]);
}

void hoststream_Init(s32 Pitch)
{
    // We hook in host streamed data in to the main cfx update list by
    // constructing a series of dummy cfx and cfx element structs and
    // adding in to the update list accordingly.
    g_pcAudio.m_pStreamerCfx[0] = hs_Init(0,Pitch);
    g_pcAudio.m_pStreamerCfx[1] = hs_Init(1,Pitch);

}

void hoststream_Kill(void)
{
    hs_Kill(0);
    hs_Kill(1);

}

s32 hoststream_Update(u8 *pData,s32 Length,s32 Index)
{
    stream_request *pStreamRequest;
    s32 SimpleReturn;
    cfx *pCfx;

    pStreamRequest = (stream_request *)g_pcAudio.m_pqAvailableStreamRequests->Recv(MQ_NOBLOCK);
    if (pStreamRequest)
    {
        pCfx = g_pcAudio.m_pStreamerCfx[Index];
        ASSERT(pStreamRequest->m_pData);
        ASSERT(Length <= AUD_STREAM_BUFFER_SIZE);
        x_memcpy(pStreamRequest->m_pData,pData,Length);

        ASSERT(pCfx->m_pElements->m_pStream);
        SimpleReturn = pCfx->m_pElements->m_pStream->m_pqReadyData->Send(pStreamRequest,MQ_NOBLOCK);
        if (!SimpleReturn)
        {
            g_pcAudio.m_pqAvailableStreamRequests->Send(pStreamRequest,MQ_BLOCK);
        }
        else
        {
            pCfx->m_pElements->m_pStream->m_PendingReadahead++;
            pCfx->m_pElements->m_pStream->m_ReadaheadInProgress++;
        }
    }
    else
    {
        SimpleReturn = FALSE;
    }
    return SimpleReturn;
}
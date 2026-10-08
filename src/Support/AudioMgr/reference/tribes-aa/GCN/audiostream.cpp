#include "entropy.hpp"
#include "audio.hpp"
#include "audiodefs.hpp"
#include "audiovars.hpp"

static message_queue s_Complete[2];
static s32 s_Status[2];

void    audio_InitStream(f32 pitch)
{
    s32 Pitch;

    Pitch = (s32)(pitch * AUD_FIXED_POINT_1);
//    iop_SendSyncRequest(AUDCMD_STREAM_INIT,&Pitch,sizeof(Pitch),NULL,0);
    mq_Create(&s_Complete[0],1);
    mq_Create(&s_Complete[1],1);
}

void    audio_SendStream(void *pData,s32 length,s32 Index)
{
    (void)pData;
    (void)length;
    (void)Index;
//    iop_SendAsyncRequest(AUDCMD_STREAM_SEND_DATA_LEFT+Index,pData,length,&s_Status[Index],sizeof(s32),&s_Complete[Index]);
}

xbool   audio_WaitStream(s32 Index)
{
    void *pRet;

    pRet = mq_Recv(&s_Complete[Index],MQ_BLOCK);
//    iop_FreeRequest((iop_request *)pRet);
    return (s_Status[Index]);

}

void    audio_KillStream(void)
{
//    iop_SendSyncRequest(AUDCMD_STREAM_KILL);
    mq_Destroy(&s_Complete[0]);
    mq_Destroy(&s_Complete[1]);
}

#include "entropy.hpp"
#include "audio.hpp"
#include "ps2/iop/iop.hpp"
#include "audiodefs.hpp"
#include "audiovars.hpp"

static xmesgq* s_pqComplete[2];
static s32 s_Status[2];

void    audio_InitStream(f32 pitch)
{
    s32 Pitch;

    Pitch = (s32)(pitch * AUD_FIXED_POINT_1);
    iop_SendSyncRequest(AUDCMD_STREAM_INIT,&Pitch,sizeof(Pitch),NULL,0);
    s_pqComplete[0] = new xmesgq(1);
    s_pqComplete[1] = new xmesgq(1);
}

void    audio_SendStream(void *pData,s32 length,s32 Index)
{
    iop_SendAsyncRequest(AUDCMD_STREAM_SEND_DATA_LEFT+Index,pData,length,&s_Status[Index],sizeof(s32),s_pqComplete[Index]);
}

xbool   audio_WaitStream(s32 Index)
{
    void *pRet;

    pRet = s_pqComplete[Index]->Recv(MQ_BLOCK);
    iop_FreeRequest((iop_request *)pRet);
    return (s_Status[Index]);

}

void    audio_KillStream(void)
{
    iop_SendSyncRequest(AUDCMD_STREAM_KILL);
    delete s_pqComplete[0];
    delete s_pqComplete[1];
}

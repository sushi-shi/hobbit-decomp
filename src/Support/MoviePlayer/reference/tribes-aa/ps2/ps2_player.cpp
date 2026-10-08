#include "entropy.hpp"
#include "movieplayer/movieplayer.hpp"
#include "x_threads.hpp"
#include "ps2/iop/iop.hpp"
#include "audiomgr/audio.hpp"
#include <libmpeg.h>


//#define DUMP_STREAM
static xbool s_InUse = FALSE;

#define MPEG_FILE_BUFFER_SIZE   32768
#define MPEG_BUFFER_BLOCK_SIZE  2048
#define MPEG_VIDEO_BUFFER_COUNT 100
#define MPEG_AUDIO_BUFFER_COUNT 300
#define MPEG_BITMAP_HEIGHT      448
#define MPEG_BITMAP_WIDTH       512

typedef struct s_mpeg_private
{
    sceMpeg mpeg;
    void    *CurrentBlock;
} mpeg_private;

static  s32 mpeg_Callback      (sceMpeg *mp,sceMpegCbData *cbdata,void *pData);
static  s32 mpeg_Videostream   (sceMpeg *mp,sceMpegCbData *cbdata,void *pData);
static  s32 mpeg_AudiostreamLeft (sceMpeg *mp,sceMpegCbData *cbdata,void *pData);
static  s32 mpeg_AudiostreamRight(sceMpeg *mp,sceMpegCbData *cbdata,void *pData);
static void InitStream(mpeg_av_stream &Stream,s32 buffcount);
static void KillStream(mpeg_av_stream &Stream);
static void PurgeStream(mpeg_av_stream &Stream);

static void         mpeg_Streamer     (void);
static movie        *s_pMovie;
static X_FILE       *s_FileHandle;
static s32          s_FileLength;
static s32          s_FileIndex;
static u8           *s_pFileBuffer;
static s32          s_FileBufferIndex;
static s32          s_FileBufferRemain;
static xthread*     s_pStreamerThread;
static volatile xbool        s_ShutdownStreamer;
static sceIpuDmaEnv s_Env;
#ifdef DUMP_STREAM
static X_FILE       *pTestFile;
#endif

extern void* x_malloctop(s32 size);

//-----------------------------------------------------------------------------
xbool   movie::Open        (const char *pFilename,s32 width,s32 height,xbool LoadIntoMemory)
{
    mpeg_private *pPrivate;

    s32     bufflength;
    u8      *pBuff;
    s32     i;
    s32     w,h;

    ASSERT(!s_InUse);

    s_FileHandle = x_fopen(pFilename,"rb");
    ASSERTS(s_FileHandle,pFilename);
    if (!s_FileHandle)
        return FALSE;

    m_InMemory = LoadIntoMemory;

    x_fseek(s_FileHandle,0,X_SEEK_END);
    s_FileLength = x_ftell(s_FileHandle);
    x_fseek(s_FileHandle,0,X_SEEK_SET);

    if (m_InMemory)
    {
        m_pResidentStream = (byte *)x_malloctop(s_FileLength);
        ASSERT(m_pResidentStream);
        x_fread(m_pResidentStream,s_FileLength,1,s_FileHandle);
        x_fclose(s_FileHandle);
    }
	eng_PageFlip();
    m_Width         = width;
    m_Height        = height;
    m_Finished      = FALSE;
    m_AudioFirst[0] = TRUE;
    m_AudioFirst[1] = TRUE;
    s_FileIndex     = 0;
    s_FileBufferIndex = 0;
    s_FileBufferRemain = 0;
    m_CurrentFrame  = 0;

    ASSERT(width <= MPEG_BITMAP_WIDTH);
    ASSERT(height <= MPEG_BITMAP_HEIGHT);
    bufflength = MPEG_BITMAP_WIDTH * 16 * sizeof(u32);
    m_nBitmaps = MPEG_BITMAP_HEIGHT / 16;
    m_pBitmaps = new xbitmap[m_nBitmaps];
    for (i=0;i<m_nBitmaps;i++)
    {
        m_pBitmaps[i].Setup(xbitmap::FMT_32_UBGR_8888,MPEG_BITMAP_WIDTH,16,FALSE,(u8 *)x_malloctop(bufflength));
        x_memset((u8 *)m_pBitmaps[i].GetPixelData(),0x80,bufflength);
		vram_Register(m_pBitmaps[i]);
    }

    s_InUse = TRUE;
    bufflength = SCE_MPEG_BUFFER_SIZE(m_Width,m_Height) + 64*1024;

    m_pBuffer = x_malloctop(bufflength);
    m_pPrivate = x_malloctop(sizeof(mpeg_private));

    m_pWorkspace = x_malloctop(sizeof(sceIpuRGB32)*(m_Width / 16)*(m_Height/16) + 64);
    ASSERT(m_pWorkspace);
    pPrivate = (mpeg_private *)m_pPrivate;
    ASSERT(m_pPrivate);
    ASSERT(m_pBuffer);
    pPrivate->CurrentBlock = NULL;

    InitStream(m_VideoStream,MPEG_VIDEO_BUFFER_COUNT);
    InitStream(m_AudioStream[0],MPEG_AUDIO_BUFFER_COUNT);
    InitStream(m_AudioStream[1],MPEG_AUDIO_BUFFER_COUNT);

    m_pReadBuffer = x_malloctop(MPEG_BUFFER_BLOCK_SIZE * (MPEG_VIDEO_BUFFER_COUNT + MPEG_AUDIO_BUFFER_COUNT * 2) + 64);
    ASSERT(m_pReadBuffer);
    pBuff = (u8 *)(((u32)m_pReadBuffer+63) &~ 63);
    for (i=0;i<MPEG_VIDEO_BUFFER_COUNT;i++)
    {
        m_VideoStream.pqAvail->Send(pBuff,MQ_BLOCK);
        pBuff += MPEG_BUFFER_BLOCK_SIZE;
    }
    for (i=0;i<MPEG_AUDIO_BUFFER_COUNT;i++)
    {
        m_AudioStream[0].pqAvail->Send(pBuff,MQ_BLOCK);
        pBuff += MPEG_BUFFER_BLOCK_SIZE;
        m_AudioStream[1].pqAvail->Send(pBuff,MQ_BLOCK);
        pBuff += MPEG_BUFFER_BLOCK_SIZE;
    }
    s_pFileBuffer = (u8 *)x_malloctop(MPEG_FILE_BUFFER_SIZE);
    s_FileBufferRemain=0;
    s_FileBufferIndex=0;
    ASSERT(s_pFileBuffer);

    sceMpegInit();
    s_pMovie = this;
    sceMpegCreate(&pPrivate->mpeg,(u8 *)m_pBuffer,bufflength);
    eng_GetRes(w,h);
    // If we're in PAL mode, we slow the stream down by the difference between the update rates
	// BW 11/8 - Changed the limit check to much higher so it'll never succeed. This will keep the
	// resulting code image the same.
    if (h>768)
        audio_InitStream(25.0f/29.975f);
    else
        audio_InitStream(1.0f);

    sceMpegAddCallback(&pPrivate->mpeg,sceMpegCbNodata,     mpeg_Callback,this);
    sceMpegAddCallback(&pPrivate->mpeg,sceMpegCbStopDMA,    mpeg_Callback,this);
    sceMpegAddCallback(&pPrivate->mpeg,sceMpegCbRestartDMA, mpeg_Callback,this);
    // Now add the decode callback for the PSS streamed data
    sceMpegAddStrCallback(&pPrivate->mpeg,sceMpegStrM2V,0,mpeg_Videostream,this);
    sceMpegAddStrCallback(&pPrivate->mpeg,sceMpegStrADPCM,0,mpeg_AudiostreamLeft,this);
    sceMpegAddStrCallback(&pPrivate->mpeg,sceMpegStrADPCM,1,mpeg_AudiostreamRight,this);
    //
    // Decode initial file header so we can get the file length info
    //
#ifdef DUMP_STREAM
    pTestFile = x_fopen("testdump.dat","wb");
    ASSERT(pTestFile);
#endif
    s_ShutdownStreamer=FALSE;
    s_pStreamerThread = new xthread(mpeg_Streamer,"Data decode streamer",32768,1);

    m_FrameCount = 1024;
    return TRUE;
}

//-----------------------------------------------------------------------------
void    movie::Close       (void)
{
    mpeg_private *pPrivate;
    s32 ShutdownDelay,i;

    ASSERT(s_InUse);
    pPrivate = (mpeg_private *)m_pPrivate;
    ASSERT(m_pPrivate);
    ASSERT(m_pWorkspace);
    ASSERT(m_pBuffer);

    // We need to try and shut down the streamer thread gracefully since it
    // may be in the middle of read requests when the movie is to quit. This
    // would be bad. We consume the target message queue entries so we can
    // make sure it will shut down (since it may be blocked on one of the
    // queues being full)
    s_ShutdownStreamer = TRUE;
    for (ShutdownDelay=0;ShutdownDelay < 60;ShutdownDelay++)
    {
        if (!s_ShutdownStreamer)
            break;
        PurgeStream(m_VideoStream);
        PurgeStream(m_AudioStream[0]);
        PurgeStream(m_AudioStream[1]);
        x_DelayThread(1);
    }

    audio_KillStream();
    delete s_pStreamerThread;
    sceMpegReset(&pPrivate->mpeg);
    sceMpegDelete(&pPrivate->mpeg);
    KillStream(m_VideoStream);
    KillStream(m_AudioStream[0]);
    KillStream(m_AudioStream[1]);
    x_DelayThread(10);

    if (m_InMemory)
    {
        x_free(m_pResidentStream);
    }
    else
    {
        x_fclose(s_FileHandle);
    }

    x_free(m_pWorkspace);
    x_free(m_pPrivate);
    x_free(m_pBuffer);
    for (i=0;i<m_nBitmaps;i++)
    {
		vram_Unregister(m_pBitmaps[i]);
        x_free((void *)m_pBitmaps[i].GetPixelData());
    }

    delete []m_pBitmaps;

    x_free(s_pFileBuffer);

    ASSERT(m_pReadBuffer);
    x_free(m_pReadBuffer);

    m_pPrivate = NULL;
    m_pBuffer = NULL;
    m_pReadBuffer = NULL;
    s_InUse = FALSE;
#ifdef DUMP_STREAM
    x_fclose(pTestFile);
#endif
}

//-----------------------------------------------------------------------------
xbitmap *movie::GetFrame   (void)
{
    mpeg_private *pPrivate;
    ASSERT(s_InUse);
    pPrivate = (mpeg_private *)m_pPrivate;
    return m_pBitmaps;
}

//-----------------------------------------------------------------------------
void    movie::SetPosition (s32 Index)
{
    mpeg_private *pPrivate;
    ASSERT(s_InUse);
    pPrivate = (mpeg_private *)m_pPrivate;

    if (Index < m_FrameCount)
    {
        m_Position = Index;
    }
}

//-----------------------------------------------------------------------------
xbool   movie::Advance     (void)
{
    mpeg_private *pPrivate;
    ASSERT(s_InUse);
    pPrivate = (mpeg_private *)m_pPrivate;

    return FALSE;
}

//-----------------------------------------------------------------------------
xbool   movie::Decode      (void)
{
    mpeg_private *pPrivate;
    void *pAudioLeft,*pAudioRight;
    s32 macroblocks;
    s32 i,j,k;
    xbool status;

    sceIpuRGB32    *pBuffer;

    ASSERT(s_InUse);
    pPrivate = (mpeg_private *)m_pPrivate;
    m_Finished = sceMpegIsEnd((sceMpeg *)m_pPrivate);
    if (m_Finished) return FALSE;

    // If we have a block of audio data, let's send it to the iop for
    // playing
    pAudioLeft = m_AudioStream[0].pqReady->Recv(MQ_NOBLOCK);
    if (pAudioLeft)
    {
        audio_SendStream(pAudioLeft,MPEG_BUFFER_BLOCK_SIZE,0);
    }
    pAudioRight = m_AudioStream[1].pqReady->Recv(MQ_NOBLOCK);
    if (pAudioRight)
    {
        audio_SendStream(pAudioRight,MPEG_BUFFER_BLOCK_SIZE,1);
    }


    macroblocks = (m_Width / 16) * (m_Height / 16);
    
    pBuffer = (sceIpuRGB32 *)( ((u32)m_pWorkspace + 63) & ~63);
    ASSERT(pBuffer);
    sceMpegGetPicture(&pPrivate->mpeg,pBuffer,macroblocks);

    // now convert image from ps2 internal format to xbitmap format
    {
        u8 *pSource;
        u8 *pDest;

        pSource = (u8 *)pBuffer;

        for (i=0;i<m_Width;i+=16)
        {
            for (j=0;j<m_Height;j+=16)
            {
                pDest = (u8 *)m_pBitmaps[j/16].GetPixelData() + i*sizeof(s32);
                for (k=0;k<16;k++)
                {
                    // Copy 16 pixels, 64 bytes
                    {
                        u_long128 *pDst128,*pSrc128;

                        pDst128 = (u_long128 *)pDest;
                        pSrc128 = (u_long128 *)pSource;

                        *pDst128++ = *pSrc128++;
                        *pDst128++ = *pSrc128++;
                        *pDst128++ = *pSrc128++;
                        *pDst128++ = *pSrc128++;
                    }
                    pSource+=64;
                    pDest += m_pBitmaps[j/16].GetWidth() * sizeof(s32);
                }
            }
        }
    }

    // audio_WaitStream returns TRUE if the sound block was enqueued to the IOP
    // FALSE if the IOP buffers were all full. If that's the case, we jam it to the
    // start of the audio queue ready for next time round.
    if (pAudioLeft)
    {
        status = audio_WaitStream(0);
        if (status)
        {
            m_AudioStream[0].pqAvail->Send(pAudioLeft,MQ_BLOCK);
        }
        else
        {
            m_AudioStream[0].pqReady->Send(pAudioLeft,MQ_JAM|MQ_BLOCK);
        }
    }

    if (pAudioRight)
    {
        status = audio_WaitStream(1);
        if (status)
        {
            m_AudioStream[1].pqAvail->Send(pAudioRight,MQ_BLOCK);
        }
        else
        {
            m_AudioStream[1].pqReady->Send(pAudioRight,MQ_JAM|MQ_BLOCK);
        }
    }
    m_CurrentFrame++;
    return FALSE;

}

//-----------------------------------------------------------------------------
void movie::UpdateStream(void)
{
    mpeg_private *pPrivate;
    s32 length;

    pPrivate = (mpeg_private *)m_pPrivate;
    
    if (s_FileIndex == s_FileLength)
    {
        if (m_VideoStream.pBuffer)
        {
            m_VideoStream.pqReady->Send(m_VideoStream.pBuffer,MQ_BLOCK);
            m_VideoStream.pBuffer = NULL;
        }

        if (m_AudioStream[0].pBuffer)
        {
            m_AudioStream[0].pqReady->Send(m_AudioStream[0].pBuffer,MQ_BLOCK);
            m_AudioStream[0].pBuffer = NULL;
        }

        if (m_AudioStream[1].pBuffer)
        {
            m_AudioStream[1].pqReady->Send(m_AudioStream[1].pBuffer,MQ_BLOCK);
            m_AudioStream[1].pBuffer = NULL;
        }

        x_DelayThread(1);             // Give the main thread some time to decode
        return;
    }

    length = MPEG_FILE_BUFFER_SIZE - s_FileBufferRemain;

    if (s_FileIndex!=s_FileLength)
    {

        if (m_InMemory)
        {
            ASSERT(m_pResidentStream);
            if (length > s_FileLength - s_FileIndex)
            {
                length = s_FileLength - s_FileIndex;
            }
            x_memcpy(s_pFileBuffer+s_FileBufferIndex,m_pResidentStream+s_FileIndex,length);
        }
        else
        {
            length = x_fread(s_pFileBuffer+s_FileBufferIndex,1,length,s_FileHandle);
        }
    }
    else
    {
        length=0;
        x_DelayThread(1);
    }
    s_FileIndex += length;
    if (length==0)
    {
        length = MPEG_FILE_BUFFER_SIZE - s_FileBufferRemain;
        x_memset(s_pFileBuffer+s_FileBufferIndex,0,length);

    }
    s_FileBufferRemain += length;

	FlushCache(0);
    length = sceMpegDemuxPss(&pPrivate->mpeg,s_pFileBuffer,s_FileBufferRemain);
    if (length==0)
    {
        x_DelayThread(1);
        return;
    }

    s_FileBufferRemain -= length;
    if (s_FileBufferRemain)
    {
//        x_memcpy(s_pFileBuffer,s_pFileBuffer+s_FileBufferIndex,s_FileBufferRemain);
    }
    s_FileBufferIndex = MPEG_FILE_BUFFER_SIZE - length;
    FlushCache(0);
}
//-----------------------------------------------------------------------------
//------- Seperate thread for updating the streaming data coming from CD
//-----------------------------------------------------------------------------
// This is a very simple thread. It's entire goal in life is to read data from
// the storage media and split it in to the seperate audio/video streams to be
// consumed by the main thread when it calls sceMpegGetPicture. There is no explicit
// delays placed in here for other thread activation since it will implictly block
// when the audio or video streams become full. We do not have to deal with initial
// startup buffering because when the second file read request is issued, this thread
// should block sufficiently long enough to allow the frame decoding to start. This
// will implictly call the mpeg_AudioStream and mpeg_VideoStream callbacks. They do 
// most of the work :)
void mpeg_Streamer(void)
{
    movie *pMovie;

    pMovie = s_pMovie;
    while (!s_ShutdownStreamer)
    {
        pMovie->UpdateStream();
    }

    s_ShutdownStreamer=0;
#if 0
    while(1)
    {
        ps2_DelayThread(10);
    }
#endif
}


//-----------------------------------------------------------------------------
//------- CALLBACK FUNCTIONS NEEDED FOR PS2
//-----------------------------------------------------------------------------
// This is called from the sceMpegGetPicture codeblock. A very messed up way to
// do things. Very very clumsy.
static  s32 mpeg_Callback      (sceMpeg *mp,sceMpegCbData *cbdata,void *pData)
{
    static s32 firsttime=TRUE;

    movie *pThis = (movie *)pData;
    mpeg_private *pPrivate;

    (void)mp;
    (void)cbdata;
    (void)pData;

    ASSERT(pThis);
    pPrivate = (mpeg_private *)pThis->m_pPrivate;
    switch (cbdata->type)
    {
    case sceMpegCbError:
        break;
    case sceMpegCbNodata:

        //PauseAllDma();
        if (s_FileLength == s_FileIndex)
        {
            pData = pThis->m_VideoStream.pqReady->Recv(MQ_NOBLOCK);
            if (!pData)
            {
                x_memset(s_pFileBuffer,0,MPEG_BUFFER_BLOCK_SIZE);
                *D4_MADR = (u32)s_pFileBuffer;
                *D4_QWC  = MPEG_BUFFER_BLOCK_SIZE >> 4;
                *D4_CHCR = 0x100;
                return 0;
            }
        }
        else
        {
            pData = pThis->m_VideoStream.pqReady->Recv(MQ_BLOCK);
        }

        ASSERT(pData);
        ASSERT( ((u32)pData & 63)==0 );

        *D4_MADR = (u32)pData;
        *D4_QWC  = MPEG_BUFFER_BLOCK_SIZE >> 4;
        *D4_CHCR = 0x100;
        if (pPrivate->CurrentBlock)
        {
            pThis->m_VideoStream.pqAvail->Send(pPrivate->CurrentBlock,MQ_BLOCK);
        }
        pPrivate->CurrentBlock = pData;

          //  RestartAllDma();
        firsttime = 0;

        break;
    case sceMpegCbStopDMA:
        sceIpuStopDMA(&s_Env);
        break;
    case sceMpegCbRestartDMA:
        {
            u32 bp;
            u32 ifc;
            u32 fp;

            bp  = (s_Env.ipubp & IPU_BP_BP_M) >> IPU_BP_BP_O;
            ifc = (s_Env.ipubp & IPU_BP_IFC_M) >> IPU_BP_IFC_O;
            fp  = (s_Env.ipubp & IPU_BP_FP_M) >> IPU_BP_FP_O;
            s_Env.d4qwc     += (fp + ifc);
            s_Env.d4madr    -= 16 * (fp + ifc);
            s_Env.ipubp     &= IPU_BP_BP_M;
            sceIpuRestartDMA(&s_Env);
        }
        break;
    case sceMpegCbTimeStamp:
        ((sceMpegCbDataTimeStamp *)cbdata)->pts = -1;
        ((sceMpegCbDataTimeStamp *)cbdata)->dts = -1;
        break;
    default:
        ASSERT(FALSE);
    }

    return 1;
}

//-----------------------------------------------------------------------------
void UpdateStream(mpeg_av_stream &Stream,u8 *pData,s32 count)
{
    s32 copylen;

    //
    // Collect up a buffers worth of data and send it to the
    // appropriate message queue
    //
    while (count)
    {
        if ( Stream.Remain==0 )
        {
            if (Stream.pBuffer)
            {
                Stream.pqReady->Send(Stream.pBuffer,MQ_BLOCK);
            }
            Stream.pBuffer = (u8 *)Stream.pqAvail->Recv(MQ_BLOCK);
            ASSERT(Stream.pBuffer);
            Stream.Remain=MPEG_BUFFER_BLOCK_SIZE;
            Stream.Index=0;
        }
        copylen = count;
        if (copylen > Stream.Remain)
        {
            copylen = Stream.Remain;
        }
        x_memcpy(Stream.pBuffer+Stream.Index,pData,copylen);
        Stream.Index+=copylen;
        Stream.Remain-=copylen;
        pData+=copylen;
        count-=copylen;
    }
}

//-----------------------------------------------------------------------------
static  s32 mpeg_Videostream   (sceMpeg *,sceMpegCbData *cbdata,void *pData)
{
    movie *pThis = (movie *)pData;

    UpdateStream(pThis->m_VideoStream,cbdata->str.data,cbdata->str.len);

    return 1;
}

//-----------------------------------------------------------------------------
static  s32 mpeg_Audiostream   (sceMpegCbData *cbdata,void *pData,s32 channel)
{
    movie *pThis = (movie *)pData;

    if (pThis->m_AudioFirst[channel])
    {
        pThis->m_AudioFirst[channel] = FALSE;
        cbdata->str.data+=40;
        cbdata->str.len -=40;
    }
    cbdata->str.data+=4;
    cbdata->str.len-=4;
    if (cbdata->str.len <=0)
        return 1;
#ifdef DUMP_STREAM
    x_fwrite(cbdata->str.data,cbdata->str.len,1,pTestFile);
#endif
    UpdateStream(pThis->m_AudioStream[channel],cbdata->str.data,cbdata->str.len);

    if (s_FileLength == s_FileIndex)
        return 0;
    return 1;
}

//-----------------------------------------------------------------------------
static  s32 mpeg_AudiostreamLeft   (sceMpeg *,sceMpegCbData *cbdata,void *pData)
{
    return mpeg_Audiostream(cbdata,pData,0);
}

//-----------------------------------------------------------------------------
static  s32 mpeg_AudiostreamRight  (sceMpeg *,sceMpegCbData *cbdata,void *pData)
{
   return mpeg_Audiostream(cbdata,pData,1);
}

//-----------------------------------------------------------------------------
static void InitStream(mpeg_av_stream &Stream,s32 buffcount)
{
    Stream.Remain = 0;
    Stream.Index     = 0;
    Stream.pBuffer   = NULL;
    Stream.pqReady = new xmesgq(buffcount);
    Stream.pqAvail = new xmesgq(buffcount);
}

//-----------------------------------------------------------------------------
static void KillStream(mpeg_av_stream &Stream)
{
    delete Stream.pqAvail;
    delete Stream.pqReady;
}

//-----------------------------------------------------------------------------
static void PurgeStream(mpeg_av_stream &Stream)
{
    void *pMsg;

    pMsg = Stream.pqReady->Recv(MQ_NOBLOCK);
    if (pMsg)
    {
        Stream.pqAvail->Send(pMsg,MQ_BLOCK);
    }
}
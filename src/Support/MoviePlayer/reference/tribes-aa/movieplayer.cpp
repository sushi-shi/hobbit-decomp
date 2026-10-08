#include "entropy.hpp"
#include "x_threads.hpp"
#include "audiomgr/audio.hpp"
#include "movieplayer/movieplayer.hpp"
#include "ui/ui_manager.hpp"
#include "ui/ui_font.hpp"
#include "../demo1/specialversion.hpp"

#ifdef TARGET_PS2
#include "ps2/iop/iop.hpp"
#include "ps2/ps2_gsreg.hpp"
#endif

#define AVG_FRAMES      20
#ifdef X_DEBUG
#define SHOW_PLAYTIMES
#endif

static s32      s_LoopCount=0;
static movie*   s_pMovie=NULL;
static xthread* s_pMovieThread;
static volatile xbool    s_KillThread;
static  s32     s_MovieWidth;
static s32      s_MovieHeight;

void CheckMovieValid(void)
{
	xthread *pCurr;
	pCurr = x_GetCurrentThread();
	if (s_pMovieThread)
	{
		ASSERT(pCurr==s_pMovieThread);
	}
}

static void movie_Update(s32 width,s32 height)
{
    xbitmap *pBitmap;
    xtimer time;
    xtimer decodetime;
    vector3 pos;
    vector2 size;
    s32     dispwidth,dispheight;
    s32 nBitmaps;
    s32 i ;

    (void)width;
    ASSERT(s_pMovie);

    time.Reset();
    time.Start();
    eng_Begin();
    decodetime.Reset();
    decodetime.Start();
    s_pMovie->Decode();
    decodetime.Stop();
    pBitmap = s_pMovie->GetFrame();
    nBitmaps = s_pMovie->GetBitmapCount();
    eng_GetRes(dispwidth,dispheight);

    size.X = (f32)pBitmap->GetWidth();
#ifdef PAL_RELEASE
	size.Y = 18.0f;
	dispheight = 464;
#else
	size.Y = 16.0f;
#endif

    pos.X = (dispwidth - size.X) / 2.0f;
    pos.Y = (dispheight - height) / 2.0f;
    pos.Z = 0.0f;

    eng_SetBackColor( XCOLOR_BLACK );
	vram_Flush();
    draw_Begin(DRAW_SPRITES, DRAW_2D|DRAW_TEXTURED);

    for (i=0;i<nBitmaps;i++)
    {
        draw_SetTexture(*pBitmap);

#ifdef TARGET_PS2
        gsreg_Begin();
        gsreg_SetClamping( TRUE );
#ifndef PAL_RELEASE
        gsreg_SetMipEquation( FALSE, 1.0f, 0, MIP_MAG_POINT, MIP_MIN_POINT );
#else
		// Dummy code to try and force the right alignment of code between pal/ntsc
		if (dispheight>1024)
		{
			asm("nop;");
			//gsreg_End();
		}
#endif
        asm("nop;nop");
        gsreg_End();
#endif
        draw_Sprite(pos,size,XCOLOR_WHITE);
        pBitmap++;
        pos.Y += size.Y;
    }
    draw_End();

    s_pMovie->Advance();
    eng_End();
    while (time.ReadMs() < 23)
    {
        x_DelayThread(2);
    }

    #ifdef TARGET_PS2_DEV
    if (input_WasPressed(INPUT_PS2_BTN_SELECT,0)||
		input_WasPressed(INPUT_PS2_BTN_SELECT,1) )
    {
        eng_ScreenShot();
    }
    #endif

}

static void movie_ASyncPlayer(void)
{
    while (!s_KillThread)
    {
#ifdef TARGET_PS2
		s32 index = eng_GetFrameIndex();
#endif

        audio_Update();
        if (s_pMovie->End())
            break;
        movie_Update(s_MovieWidth,s_MovieHeight);

#ifdef TARGET_PS2
		// Would only assert if something else had tried to
		// flip the frame buffers via eng_PageFlip in a seperate
		// thread.
		ASSERT(index==eng_GetFrameIndex());
#endif

        eng_PageFlip();
    }
	// When we have finished playing the movie, we still need to
	// wait until s_KillThread is set which notifies us when the
	// main thread is ready for this thread to die. It MUST be done
	// like this to avoid multithread end issues
    while (!s_KillThread)
    {
        x_DelayThread(1);
    }
    s_KillThread = FALSE;
}

void movie_PlayAsync(const char *pFilename,s32 width,s32 height)
{
    ASSERT(!s_pMovie);
    s_pMovie = new movie;
    ASSERT(s_pMovie);

    s_MovieWidth = width;
    s_MovieHeight = height;

    s_pMovie->Open(pFilename,width,height,TRUE);
    s_KillThread = FALSE;
    s_pMovieThread = new xthread(movie_ASyncPlayer,"Movie Asynchronous Player",16384,1);
}

void movie_WaitAsync(xbool ForceQuit)
{
    xtimer t;

    t.Reset();
    t.Start();
    if (!ForceQuit)
    {
        while (!s_pMovie->End())
        {
            input_UpdateState();
//            while (input_UpdateState());

            x_DelayThread(32);
            if (input_WasPressed(INPUT_PS2_BTN_START)||
				input_WasPressed(INPUT_PS2_BTN_START) )
            {
                break;
            }
        }
    }
    s_KillThread = TRUE;
    while (s_KillThread)
    {
        x_DelayThread(1);
    }
    delete s_pMovieThread;
	s_pMovieThread= NULL;
    s_pMovie->Close();
    delete s_pMovie;
    s_pMovie = NULL;
    t.Stop();
}

void movie_Play(const char *pFilename,s32 width,s32 height, ui_manager* pUIManager)
{
    s32     frame;
    s32     FlashCount;

#ifdef SHOW_PLAYTIMES
    xtimer  decodetime;
    s32     i;
    f32     decodetimes[AVG_FRAMES];
    f32     avg,max,min;
    s32     index;

    index=0;
    for (i=0;i<AVG_FRAMES;i++)
    {
        decodetimes[i]=0.0f;
    }
#endif

	ASSERT(!s_pMovie);
    s_pMovie = new movie;
    ASSERT(s_pMovie);

    s_pMovie->Open(pFilename,width,height,FALSE);
    frame = 0;
    FlashCount = 0;

    while (!s_pMovie->End())
    {
        audio_Update();
        input_UpdateState();
//        while (input_UpdateState());
        movie_Update(width,height);

        frame++;
        if ( pUIManager )
        {
            s32 Transparency;

            FlashCount++;

            if (FlashCount >= 32)
                FlashCount = 0;

            if (FlashCount < 16)
            {
                Transparency = FlashCount * 16;
            }
            else
            {
                Transparency = (31-FlashCount)*16;
            }

            eng_Begin( "Start Message" );
        
#ifdef PAL_RELEASE
            pUIManager->RenderText( 1, 
                                    irect(0, 464,512,24), 
                                    ui_font::h_center, 
                                    xcolor(255,255,255,Transparency), 
                                    xwstring("Press the START button") );
#else
            pUIManager->RenderText( 1, 
                                    irect(0, 400,512,24), 
                                    ui_font::h_center, 
                                    xcolor(255,255,255,Transparency), 
                                    xwstring("Press the START button") );
#endif
            eng_End();
        }

#ifdef SHOW_PLAYTIMES
        decodetimes[index]=decodetime.ReadMs();
        index++;
        if (index>=AVG_FRAMES)
            index=0;

        avg=0.0f;
        min=10000.0f;
        max=0.0f;

        for (i=0;i<AVG_FRAMES;i++)
        {
            avg+=decodetimes[i];
            if (decodetimes[i] > max)
                max = decodetimes[i];
            if (decodetimes[i] < min)
                min = decodetimes[i];
        }
        avg /= AVG_FRAMES;

        x_printfxy(0,1,"Loop %d, Frame %d\n",s_LoopCount,frame);
        //x_printfxy(0,2,"Decode time %2.2fms\n",decodetime.ReadMs());
        //x_printfxy(0,3,"Decode avg=%2.2f,max=%2.2f,min=%2.2f\n",avg,max,min);
#endif

#ifdef X_DEBUG
        while (input_IsPressed(INPUT_PS2_BTN_R2))
        {
            input_UpdateState();
//            while (input_UpdateState());
            x_DelayThread(32);

        }
#endif

        eng_PageFlip();
        if (input_WasPressed(INPUT_PS2_BTN_START,0) ||
			input_WasPressed(INPUT_PS2_BTN_START,1) )
        {
            break;
        }
    }
    s_pMovie->Close();
    delete s_pMovie;
	s_pMovie = NULL;
    s_LoopCount++;
}

//-----------------------------------------------------------------------------
movie::movie(void)
{
    m_Width     = 0;
    m_Height    = 0;
    m_BitDepth  = 0;
    m_Position  = 0;
    m_FrameCount= 0;
    m_pBitmaps  = 0;
    m_nBitmaps  = 0;
    m_pWorkspace= NULL;
}

//-----------------------------------------------------------------------------
movie::~movie(void)
{
}



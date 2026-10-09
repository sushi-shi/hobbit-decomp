//=========================================================================
//
// E_ENGINE.CPP
//
//=========================================================================

#ifdef VENDOR_MW
#include "mwUtils.h"
#endif

#include <xCore/Entropy/e_Engine.hpp>
#include "iop/iop.hpp"
#include <xCore/Entropy/Common/e_cdfs.hpp>
#include "ps2_dmaman.hpp"
#ifdef TARGET_PS2_CLIENT
#include "e_netfs.hpp"
#include "../support/netlib/netlib.hpp"
#endif
#include <xCore/Entropy/PS2/ps2_except.hpp>
#include <xCore/x_files/x_threads.hpp>
#include "sifcmd.h"

#include "../../demo1/specialversion.hpp"

#include "libscedemo.h"

//=========================================================================

//#define BIT16

#define WINDOW_LEFT    (2048-(512/2))
#define WINDOW_TOP     (2048-(512/2))

#ifdef VIDEO_FORMAT_PAL
xbool            s_PalMode   = TRUE;
s32				 s_XOff		 = 0;
s32				 s_YOff		 = 0;
#define			RESOLUTION_X 512
#define			RESOLUTION_Y 512
#else
xbool            s_PalMode   = FALSE;
s32				 s_XOff		 = 0;
s32				 s_YOff		 = 0;
#define			RESOLUTION_X 512
#define			RESOLUTION_Y 448
#endif

static sceGsDBuff       s_db;
static fsAABuff         s_aaBuff;
       fsAABuff*        s_pAAbuff;
       s32              s_FrameCount;
static s32              s_XRes=RESOLUTION_X;
static s32              s_YRes=RESOLUTION_Y;
static s32              s_DListSize = 1536*1024;
static s32              s_SMem      = (450*1024) + s_DListSize;
static byte*            s_DListBegin;

static matrix4          s_MWorldToScreen;
static matrix4          s_MWorldToScreenClip;
static matrix4          s_MWorldToView;
static matrix4          s_MWorldToClip;
static matrix4          s_MClipToScreen;
static matrix4          s_MLocalToWorld;
static matrix4          s_MLocalToScreen;
static matrix4          s_MLocalToClip;
static xbool            s_LocalToScreenComputed;
static xbool            s_LocalToClipComputed;
static xbool			s_ForceRefresh;
//static s64              s_StartTime;

static s32              s_VU1Stats[2][4];

static s32              s_BGColor[3] = {0,0,0};

//static s64              ENGStartTime;
static xbool            s_InsideTask = FALSE;
static s32              s_NTasks = 0;

static s32              s_ContextStack[8];
static s32              s_ContextStackIndex;
static s32              s_Context;
static char             s_TaskName[32];
static s32              s_EngBeginVRAMStart;
static s32              s_EngBeginDListUsed;

static xtimer           RoundTrip;
//
// View variables
//
static view             s_View[ ENG_MAX_VIEWS ];
static view*            s_ActiveView[ ENG_MAX_VIEWS ];
static s32              s_nActiveViews;
//static s32              s_nViews;
static u32              s_ViewMask;


// Screen shot variables
static xbool            s_ScreenShotRequest  = FALSE ;
static xbool            s_ScreenShotActive   = FALSE ;
static s32              s_ScreenShotSize     = 1 ;
static s32              s_ScreenShotX        = 0 ;
static s32              s_ScreenShotY        = 0 ;
static s32              s_ScreenShotNumber   = 0 ;
static char             s_ScreenShotName[X_MAX_PATH] = {0} ;

// Writes out next multi part screen shot
static void ProcessNextScreenShot( void ) ;

//
// DLIST variables
//
#define DLIST_STACK_SIZE 4
       dlist DLIST;         
static dlist s_DListStack[DLIST_STACK_SIZE];
static s32   s_DListStackIndex;
static s32   s_DListMaxUsed;

//
// Microcode control
//
#define MAX_MICROCODES  8

struct microcode
{
    char    Name[32];
    byte*   pCode;
    s32     NBytes;
};

static microcode s_MCode[ MAX_MICROCODES ];
static s32       s_NMCodes=0;
static s32       s_ActiveMCode=-1;

//=========================================================================

static xtimer STAT_CPUTIMER;
static f32 STAT_GSMS;
static f32 STAT_CPUMS;
static f32 STAT_IMS;
void draw_Init( void );
void draw_Kill( void );
extern s32 VRAM_BytesLoaded;
static s32 s_LastVRAMBytesLoaded;
static xbool s_Initialized = FALSE;

void input_VsyncKick(void);

int vSyncCallback(int)
{
    input_VsyncKick();
	ExitHandler();
    return 1;
}


//=========================================================================
void ClearVram(void)
{
    sceGsLoadImage gsImage PS2_ALIGNMENT(64);
    s32 i;
    u8 *pData;

    pData = (u8 *)malloc(256 * 256 * 4);
    ASSERT(pData);
    x_memset(pData,0x00,256*256*4);
    for (i=0;i<16;i++)
    {
        sceGsSetDefLoadImage(&gsImage,i * 1024, 4 ,SCE_GS_PSMCT32,0,0,256,256);
        FlushCache(0);
        sceGsExecLoadImage(&gsImage,(u_long128 *)pData);
        sceGsSyncPath(0,0);
    }
    free(pData);

}
//
// Copied from x_bitmap_io.cpp
//
struct io_buffer
{
    s32             DataSize;
    s32             ClutSize;
    s32             Width;
    s32             Height;
    s32             PW;
    u32             Flags;
    s32             NMips;
    xbitmap::format Format;
};

static s32 ShowSplashGetPS2SwizzledIndex( s32 I )
{
    static u8 swizzle_lut[32] = 
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    return swizzle_lut[(I) & 31] + (I & ~31);
}

void ShowSplash(void)
{
	s32				handle,length;
	io_buffer*		mem;
	u8*				pData;
	s32				i;
	s32				xoffset,yoffset;
    sceGsLoadImage	gsimage;
	u32*			pPalette;
	u32*			pLineBuffer;
	s32				height;


	ClearVram();
	//
	// Initialize CD system
	//
	//sceSifInitRpc(0);
	//sceCdInit(SCECdINIT);
	//sceCdMmode(SCECdCD);

	//
    // GS
    //
    scePrintf("EarlyStart: Initializing graphics environment\n");
    sceGsResetPath();
    sceDevVu1PutDBit(1);
    *(u_long *)&s_db.clear0.rgbaq = SCE_GS_SET_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x3f800000);
    *(u_long *)&s_db.clear1.rgbaq = SCE_GS_SET_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x3f800000);
    if (s_PalMode)
    {
        sceGsResetGraph( 0, SCE_GS_INTERLACE, SCE_GS_PAL, SCE_GS_FIELD);
		height = 512;
    }
    else
    {
        sceGsResetGraph( 0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FIELD);
		height = 448;
    }
    s_pAAbuff       = (fsAABuff*) (((u_int)(&s_aaBuff)) | 0x20000000);

#ifdef BIT16

    SetupFS_AA_buffer( s_pAAbuff, 
                       512, 
                       512, 
                       SCE_GS_PSMCT16,  
                       SCE_GS_ZGEQUAL,//SCE_GS_ZGREATER, 
                       SCE_GS_PSMZ16,  
                       SCE_GS_CLEAR);   
#else

    SetupFS_AA_buffer( s_pAAbuff, 
                       512, 
                       512, 
                       SCE_GS_PSMCT32,  
                       SCE_GS_ZGEQUAL,//SCE_GS_ZGREATER, 
                       SCE_GS_PSMZ32,  
                       SCE_GS_CLEAR);   
#endif
    //
    // VBLANK
    //
    scePrintf("EarlyStart: Waiting for vblank\n");
    FlushCache(0);
    while(sceGsSyncV(0) == 0){};  

#ifdef TARGET_PS2_DEV
	handle = sceOpen("host:splash.xbmp",SCE_RDONLY);
#elif defined(DEMO_DISK_HARNESS)
	handle = sceOpen("cdrom0:\\TRIBES\\SPLASH.XBM;1",SCE_RDONLY);
#else
	handle = sceOpen("cdrom0:\\SPLASH.XBM;1",SCE_RDONLY);
#endif
	ASSERT(handle>=0);

	length = sceLseek(handle,0,SCE_SEEK_END);
	sceLseek(handle,0,SCE_SEEK_SET);

	mem = (io_buffer*)malloc(length);
	ASSERT(mem);

	sceRead(handle,mem,length);
	sceClose(handle);

	// Now, download the image to display memory
	pData = (u8*)((u8*)mem+sizeof(io_buffer));
	pPalette = (u32*)(pData+mem->DataSize);

	yoffset = (height - mem->Height)/2;
	xoffset = (512 - mem->Width)/2;

	ASSERT(yoffset>=0);
	ASSERT(xoffset>=0);

	pLineBuffer = (u32*)malloc(mem->Width * sizeof(u32));
	ASSERT(pLineBuffer);

	for (i=0;i<mem->Height;i++)
	{
		for (s32 j=0;j<mem->Width;j++)
		{
			pLineBuffer[j] = pPalette[ShowSplashGetPS2SwizzledIndex(*pData)];
			pData++;
		}
		sceGsSetDefLoadImage(&gsimage,  0,					// Base address of dest buffer
										mem->Width/64,		// Width of transfer
										SCE_GS_PSMCT32,		// Data format
										xoffset,			// Dest X
										i+yoffset,			// Dest Y
										mem->Width,			// Dest width
										1					// Dest height
										);
		FlushCache(0);
		sceGsExecLoadImage(&gsimage, (u_long128*)pLineBuffer);
		sceGsSyncPath(0, 0);
	}

	free(pLineBuffer);
	free(mem);
	while(sceGsSyncV(0) == 0){};  
	sceGsDispEnv disp;

	// ////////////////////////////
	// 
	//  Initilize display
	// 
	sceGsSetDefDispEnv(
    	&disp,
	SCE_GS_PSMCT32,
	512,
	height,
	s_XOff,
	s_YOff
	);
	sceGsPutDispEnv(&disp);
}

void eng_Init(void)
{
    //s_StartTime = TIME_GetTicks();
#ifdef TARGET_PS2_CLIENT
    if (s_Initialized)
        return;
#endif
    ASSERT(!s_Initialized);
    s_Initialized = TRUE;

    s_Context = 0;
    s_ContextStackIndex = 0;
    s_ContextStack[0] = 0;
    s_DListStackIndex = -1;
    s_DListMaxUsed = 0;


    //
    // INPUT
    //
    scePrintf("ENGINE: Initializing input\n");
    input_Init();

    //
    // VRAM
    //
    scePrintf("ENGINE: Initializing vram module\n");
    vram_Init();

    //
    // FONT
    //
    scePrintf("ENGINE: Initializing font module\n");
    font_Init();

    //
    // TEXT
    //
    scePrintf("ENGINE: Initializing text module\n");
    text_Init();
    text_SetParams( 512, 448, 8, 8,
                    13, 
                    18,
                    8 );
    text_ClearBuffers();

    //
    // DRAW
    //
    draw_Init();

    //
    // DMAS
    //
    scePrintf("ENGINE: Initializing dmas\n");
    {
        sceDmaEnv denv;
	    sceDmaReset(1);
	    sceDmaGetEnv(&denv);
	    denv.notify = 0x0100; // enable Ch.8 CPCOND 
	    sceDmaPutEnv(&denv);
    }

    //
    // DMAMAN
    //
    scePrintf("ENGINE: Initializing dma task manager\n");
    dmaman_Init();

    //
    // SMEM
    //
    scePrintf("ENGINE: Initializing scratch memory\n");
    smem_Init(s_SMem);

    //
    // DLIST
    //
    scePrintf("ENGINE: Initializing first dlist\n");
    {
        byte* DListMem;
        DListMem = smem_BufferAlloc(s_DListSize);
        ASSERT(DListMem);
        DLIST.Setup( DListMem, s_DListSize );
        s_DListStackIndex = 0;
        s_DListStack[s_DListStackIndex] = DLIST;
    }

    //
    // TIME
    //
    scePrintf("ENGINE: Initializing time\n");
    //ENGStartTime = TIME_GetTicks();
	//TIME_Push();
    RoundTrip.Start();

    //
    // GS
    //
    scePrintf("ENGINE: Initializing graphics environment\n");
//#ifdef TARGET_PS2_DVD
#if 0
    // In their infinite wisdom, sony wants to hide whether or not
    // the game is working on PAL or NTSC hardware so there is no
    // actual mechanism for determining this. The only solution Sony
    // support gave was to check the SYSTEM.CNF file on the CDROM.
    // Lame.
    {
        s32 file;
        s32 length;
        char *pByte;

        char Temp[512];

        file = sceOpen("cdrom0:\\SYSTEM.CNF;1",SCE_RDONLY);
        if (file >= 0)
        {
            length = sceRead(file,Temp,512);

            pByte = Temp;
            s_PalMode = FALSE;
            while (length)
            {
                if (x_strncmp(pByte,"VMODE",5)==0)
                {
                    pByte+=5;
                    while (*pByte==' ') pByte++;
                    ASSERT(*pByte == '=');
                    pByte++;
                    while (*pByte==' ') pByte++;

                    if (x_strncmp(pByte,"PAL",3)==0)
                    {
                        s_PalMode=TRUE;
                    }
                    break;
                }
                length--;
                pByte++;
            }
            sceClose(file);
        }
    }
#endif
	s_ForceRefresh = TRUE;

    //ClearVram();

    //
    // AA
    //
    scePrintf("ENGINE: Initializing Anti-aliasing settings\n");
    s_pAAbuff       = (fsAABuff*) (((u_int)(&s_aaBuff)) | 0x20000000);

#ifdef BIT16

    SetupFS_AA_buffer( s_pAAbuff, 
                       512, 
                       512, 
                       SCE_GS_PSMCT16,  
                       SCE_GS_ZGEQUAL,//SCE_GS_ZGREATER, 
                       SCE_GS_PSMZ16,  
                       SCE_GS_CLEAR);   
#else

    SetupFS_AA_buffer( s_pAAbuff, 
                       512, 
                       512, 
                       SCE_GS_PSMCT32,  
                       SCE_GS_ZGEQUAL,//SCE_GS_ZGREATER, 
                       SCE_GS_PSMZ32,  
                       SCE_GS_CLEAR);   
#endif
    eng_SetBackColor( XCOLOR_BLACK );

}

//=========================================================================

void eng_Kill(void)
{
    dmaman_Kill();
    smem_Kill();
    draw_Kill();
    text_Kill();
    sceGsSyncVCallback(NULL);
	sceCdInit(SCECdEXIT);
	sceSifExitCmd();
    //TEXT_Kill();
    //FONT_Kill();
    //VRAM_Kill();
}

//=========================================================================

void eng_GetRes( s32& XRes, s32& YRes )
{
    XRes = s_XRes;
    YRes = s_YRes;
}

//=========================================================================

void eng_MaximizeViewport( view& View )
{
    View.SetViewport( 0, 0, s_XRes, s_YRes );
}

//=========================================================================

void eng_SetBackColor( xcolor Color )
{
    s_BGColor[0] = Color.R;
    s_BGColor[1] = Color.G;
    s_BGColor[2] = Color.B;
    FB_SetBackgroundColor(s_pAAbuff,Color.R,Color.G,Color.B);
}

//=========================================================================

void eng_Sync( void )
{
    dmaman_WaitForTasks();
}

//=========================================================================
xbool s_ProgressiveScan=FALSE;

void ps2_ResetHardware(void)
{
	s32 pmode;
	s32 omode;

	sceDmaReset( 1 );
	sceDevVif0Reset();
	sceDevVu0Reset();
	sceDevVif1Reset();
	sceDevVu1Reset();
	sceDevGifReset();
	sceDevVu1PutDBit(1);
	sceGsResetPath();

#if 0
	if ( s_ProgressiveScan && !s_PalMode)
	{
		pmode = SCE_GS_NOINTERLACE;
		omode = SCE_GS_DTV480P;
	}
	else
#endif
	{
		pmode = SCE_GS_INTERLACE;
		if (s_PalMode)
		{
			omode = SCE_GS_PAL;
			s_YRes = 512;
		}
		else
		{
			omode = SCE_GS_NTSC;
		}
	}

	sceGsResetGraph(0,pmode,omode,SCE_GS_FIELD);
	sceGsResetPath();
	sceGsSyncVCallback(vSyncCallback);
}

volatile s32 TIMER_WAIT = 28;
volatile s32 FRAME_THROTTLE = 0;
volatile xbool CLEAR_SCREEN = TRUE;

void eng_PageFlip(void)
{
    //x_printfxy(0,2,"%1d",s_DListMaxUsed);

	x_WatchdogReset();
    //ASSERT(!eng_InsideTask());
void CheckMovieValid(void);
	CheckMovieValid();

	if (s_ForceRefresh)
	{
		s_ForceRefresh = FALSE;

		while(sceGsSyncV(0) == 0){};  
		*(u_long *)&s_db.clear0.rgbaq = SCE_GS_SET_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x3f800000);
		*(u_long *)&s_db.clear1.rgbaq = SCE_GS_SET_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x3f800000);
		ps2_ResetHardware();
		//
		// VBLANK
		//
		scePrintf("ENGINE: Waiting for vblank\n");
		s_FrameCount=0;
		FlushCache(0);
		while(sceGsSyncV(0) == 0){};  
	}

    // Collect bytes loaded by vram manager
    s_LastVRAMBytesLoaded = VRAM_BytesLoaded;
    VRAM_BytesLoaded = 0;

    //s32 oddFrame;
	STAT_CPUTIMER.Stop();
    STAT_CPUMS = STAT_CPUTIMER.ReadMs();

    xtimer InternalTime;
    InternalTime.Start();
	//printf("Pageflip\n");

    //
    // ADD TEXT RENDER
    //
    if( 1 )
    {
        eng_Begin("Text");
	    //SetAlphaBlending();
        text_Render();
        eng_End();
    }    


    //
    // WAIT FOR PREVIOUS TASKS TO FINISH
    //
	dmaman_WaitForTasks();
    STAT_GSMS = dmaman_GetRunTime();

    //
    // TURN OFF DMA HANDLERS
    //
    dmaman_HandlersOff();

    //
    // Be sure 30ms is used up (40ms for pal)
    //
    while( RoundTrip.ReadMs() < (f32)TIMER_WAIT ) 
    {
        x_DelayThread( 1 );
    }

    if( RoundTrip.ReadMs() < (f32)FRAME_THROTTLE )
    {
        sceGsSyncV(0);
    }

    //
    // WAIT FOR VBLANK
    //
    sceGsSyncPath(0,0);      
    sceGsSyncV(0);

    RoundTrip.Reset();
    RoundTrip.Start();

    //
    // READ LAST LINE OF VU1 MEMORY
    //
    s_VU1Stats[0][0] = *((s32*)(0x1100C000 + (1022)*16 + (4)*0));
    s_VU1Stats[0][1] = *((s32*)(0x1100C000 + (1022)*16 + (4)*1));
    s_VU1Stats[0][2] = *((s32*)(0x1100C000 + (1022)*16 + (4)*2));
    s_VU1Stats[0][3] = *((s32*)(0x1100C000 + (1022)*16 + (4)*3));
    s_VU1Stats[1][0] = *((s32*)(0x1100C000 + (1023)*16 + (4)*0));
    s_VU1Stats[1][1] = *((s32*)(0x1100C000 + (1023)*16 + (4)*1));
    s_VU1Stats[1][2] = *((s32*)(0x1100C000 + (1023)*16 + (4)*2));
    s_VU1Stats[1][3] = *((s32*)(0x1100C000 + (1023)*16 + (4)*3));


    //
    // GET LENGTH OF OLD DLIST
    //
    {
        s_DListMaxUsed = MAX( DLIST.GetLength(), s_DListMaxUsed );
    }
    //
	// FLUSH DCACHE BACK TO MEMORY
    //
	FlushCache(WRITEBACK_DCACHE);

    //
    // TOGGLE BUFFERS
    //
    {
        xbool AA = TRUE;//(((s32)TIME_Get())/2)%2;
        PutDispBuffer       ( s_pAAbuff, !(s_FrameCount&0x01), AA);     // toggle 2 circuits
        PutDrawBufferSmall  ( s_pAAbuff,  (s_FrameCount&0x01), CLEAR_SCREEN );
        sceGsSyncPath(0,0);      
    }

    //
    // RENDER NEW TASKS
    //
    dmaman_HandlersOn();
    dmaman_ExecuteTasks();


    // Write out next multi-part screen shot?
    if ((s_ScreenShotRequest) || (s_ScreenShotActive))
        ProcessNextScreenShot() ;



    //
    // TOGGLE SYSTEMS
    //
    text_ClearBuffers();
    smem_Toggle();

    //
    // CHECK INPUT
    //
    // bw 7/22/01 input_CheckDevices() no longer needed since it's done in a seperate thread

    //
    // INCREMENT NUMBER OF FRAME PROCESSED
    //
    s_FrameCount++;


    //
    // GET LENGTH OF OLD DLIST
    //
    {
        s_DListMaxUsed = MAX( DLIST.GetLength(), s_DListMaxUsed );
    }

    //
    // SETUP NEW DLIST
    //
    {
        byte* DListMem;
        DListMem = smem_BufferAlloc(s_DListSize);
        ASSERT(DListMem);
        DLIST.Setup( DListMem, s_DListSize );
        s_DListStackIndex = 0;
        s_DListStack[s_DListStackIndex] = DLIST;
        s_NTasks = 0;
    }

    //
    // RETURN TO USER
    //
    STAT_IMS = InternalTime.ReadMs();
    STAT_CPUTIMER.Reset();
    STAT_CPUTIMER.Start();
	//TIME_Push();

#ifdef BIT16

    u64 DMX=0;

    DMX |= ((u64)((-4)&0x7))<<( 0*4);
    DMX |= ((u64)(( 2)&0x7))<<( 1*4);
    DMX |= ((u64)((-3)&0x7))<<( 2*4);
    DMX |= ((u64)((-3)&0x7))<<( 3*4);

    DMX |= ((u64)(( 0)&0x7))<<( 4*4);
    DMX |= ((u64)((-2)&0x7))<<( 5*4);
    DMX |= ((u64)(( 1)&0x7))<<( 6*4);
    DMX |= ((u64)((-1)&0x7))<<( 7*4);

    DMX |= ((u64)((-3)&0x7))<<( 8*4);
    DMX |= ((u64)(( 3)&0x7))<<( 9*4);
    DMX |= ((u64)((-4)&0x7))<<(10*4);
    DMX |= ((u64)(( 2)&0x7))<<(11*4);

    DMX |= ((u64)(( 1)&0x7))<<(12*4);
    DMX |= ((u64)((-1)&0x7))<<(13*4);
    DMX |= ((u64)(( 0)&0x7))<<(14*4);
    DMX |= ((u64)((-2)&0x7))<<(15*4);

    eng_Begin("Dither");
    gsreg_Begin();
    gsreg_Set(0x45,1);
    gsreg_Set(0x44,DMX);
    gsreg_End();
    eng_End();

#endif

    //
    // CHECK ON GS CONTEXT
    //
    ASSERT( s_ContextStackIndex == 0 );
    s_Context = 0;
    s_ContextStackIndex = 0;
    s_ContextStack[0] = 0;


    // Setup initial state
    eng_Begin("GSReset");
    gsreg_Begin();
        eng_PushGSContext(0);
        gsreg_SetAlphaBlend ( ALPHA_BLEND_OFF );
        gsreg_SetZBuffer    ( TRUE );
        gsreg_SetClamping   ( FALSE );
        eng_PopGSContext();
        eng_PushGSContext(1);
        gsreg_SetAlphaBlend ( ALPHA_BLEND_OFF );
        gsreg_SetZBuffer    ( TRUE );
        gsreg_SetClamping   ( FALSE );
        eng_PopGSContext();
    gsreg_End();
    eng_End();
    vram_SetMipEquation(-10,TRUE);
    //x_printfxy(10,0,"%5d %5d %5d %5d",s_VU1Stats[0][0],s_VU1Stats[0][1],s_VU1Stats[0][2],s_VU1Stats[0][3]);
    //x_printfxy(10,1,"%5d %5d %5d %5d",s_VU1Stats[1][0],s_VU1Stats[1][1],s_VU1Stats[1][2],s_VU1Stats[1][3]);

    // Clear draw's L2W

}

//=========================================================================

s32 eng_GetFrameIndex( void )
{
    return(s_FrameCount&0x01);
}

//=========================================================================

xbool eng_InBeginEnd(void)
{
    return s_InsideTask;
}

//=========================================================================

xbool eng_Begin( const char* pTaskName )
{
    ASSERT( !eng_InBeginEnd() );
	
    s_DListBegin = (byte*)DLIST.GetAddr();    
    s_InsideTask = TRUE;

    dmaman_NewTask( s_DListBegin, s_TaskName );
                    
    draw_ClearL2W();

    if( pTaskName ) x_strcpy(s_TaskName,pTaskName);
    else            x_strcpy(s_TaskName,"unknown");

    s_EngBeginVRAMStart = VRAM_BytesLoaded;
    s_EngBeginDListUsed = DLIST.GetLength();

    // Stall and setup mark register so we know what VIF1 is working on

    s_NTasks++;
    return TRUE;
}

//=========================================================================

void eng_End(void)
{
    ASSERT( eng_InBeginEnd() );
	//printf("End\n");
    dmaman_EndTask( VRAM_BytesLoaded - s_EngBeginVRAMStart,     // Number of VRAM bytes loaded
                    DLIST.GetLength() - s_EngBeginDListUsed);   // Length of the display list
    s_InsideTask = FALSE;
}

//=========================================================================

// Writes out current frame buffer into file
#define LINES_PER_CHUNK 32
static void DumpFrameBuffer( const char* pFileName )
{
    ASSERT(pFileName) ;
    s32     XRes = s_XRes;
    s32     YRes = s_YRes;
    X_FILE* fp;
    s32     i;

    // Try open file
    fp = x_fopen(pFileName,"wb");
    if (!fp)
        return;

    // Build tga header
    byte Header[18];
    x_memset(&Header,0,18);
    Header[16] = 32;
    Header[12] = (XRes>>0)&0xFF;
    Header[13] = (XRes>>8)&0xFF;
    Header[14] = (YRes>>0)&0xFF;
    Header[15] = (YRes>>8)&0xFF;
    Header[2]  = 2;
    Header[17] = 32;    // NOT flipped vertically.

    // Write out header data
    x_fwrite( Header, 18, 1, fp );

    // Allocate buffer 
    xcolor* pBuffer = (xcolor*)x_malloc( s_XRes*sizeof(xcolor)*LINES_PER_CHUNK );
    ASSERT(pBuffer);

    
    //
    // WAIT FOR PREVIOUS TASKS TO FINISH
    //
	dmaman_WaitForTasks();

    //
    // TURN OFF DMA HANDLERS
    //
    dmaman_HandlersOff();

    //
    // WAIT FOR VBLANK
    //
    sceGsSyncPath(0,0);      
    sceGsSyncV(0);


    // Build and execute image store
    sceGsStoreImage gsimage;
    s32 NLines = YRes;
    s32 Offset = 512 * (s_FrameCount & 0x01) ;
    while( NLines )
    {
        s32 N = MIN(NLines,LINES_PER_CHUNK);
        sceGsSetDefStoreImage(&gsimage, Offset*XRes/64, XRes / 64, SCE_GS_PSMCT32, 0, 0, XRes, N);

	    FlushCache(0);
	    sceGsExecStoreImage(&gsimage, (u_long128*)(pBuffer));
	    sceGsSyncPath(0, 0);

        NLines -= N;
        Offset += N;

        for (i=0;i<XRes*N;i++)
        {
            xcolor C = pBuffer[i];
            pBuffer[i].R = C.B;
            pBuffer[i].G = C.G;
            pBuffer[i].B = C.R;
            pBuffer[i].A = C.A;
        }
        x_fwrite(pBuffer,XRes*sizeof(xcolor)*N,1,fp);
    }

    x_fclose(fp);

    // free buffer
    x_free(pBuffer);

    // Turn handlers back on
    dmaman_HandlersOn();
}

//=========================================================================

// Returns current shot name
static void GetScreenShotSubPicFName( char* ShotName )
{
    // Locals
    char    Drive   [X_MAX_DRIVE];
    char    Dir     [X_MAX_DIR];
    char    FName   [X_MAX_FNAME];
    char    Ext     [X_MAX_EXT];

    // Create current shot name in the form:  ps2shot000_4x4_000.tga
    x_splitpath(s_ScreenShotName, Drive, Dir, FName, Ext) ;
    x_sprintf(ShotName, "%s%s%s_%dx%d_%03d%s", 
              Drive, Dir, FName,                        // ps2shot000
              s_ScreenShotSize, s_ScreenShotSize,       // 4x4
              s_ScreenShotNumber,                       // 000
              Ext) ;                                    // .tga
}

// Writes out next multi part screen shot
static void ProcessNextScreenShot( void )
{
    // Locals
    char    ShotName[X_MAX_PATH];

    // If just requested screen shot, then start the sequence -
    // next time round we'll be ready to capture
    if (s_ScreenShotRequest)
    {
        // Flag we are now active
        s_ScreenShotRequest = FALSE ;
        s_ScreenShotActive  = TRUE ;
        return ;
    }

    // Should only come here if active
    ASSERT(s_ScreenShotActive) ;

    // Create current shot name in the form:  ps2shot000_4x4_000.tga
    GetScreenShotSubPicFName(ShotName) ;
    
    // Goto next screen shot
    s_ScreenShotNumber++ ;
    
    // Write out current shot
    DumpFrameBuffer(ShotName) ;

    // Next screen across?
    if (++s_ScreenShotX == s_ScreenShotSize)
    {
        // Next screen down
        s_ScreenShotX = 0 ;

        // Done?
        if (++s_ScreenShotY == s_ScreenShotSize)
        {
            // End the screen shot
            s_ScreenShotActive = FALSE ;
            s_ScreenShotSize   = 1 ;
            s_ScreenShotX      = 0 ;
            s_ScreenShotY      = 0 ;
        }
    }
}

//=========================================================================

void eng_ScreenShot( const char* pFileName /*= NULL*/, s32 Size /*= 1*/  )
{
    // If already shooting, exit so that screen number does not increase!
    if ((s_ScreenShotRequest) || (s_ScreenShotActive))
        return ;

    // Request a screen shot now - this syncs with the page flip
    s_ScreenShotRequest = TRUE ;
    
    // Start at top left
    s_ScreenShotSize    = Size ;
    s_ScreenShotNumber  = 0 ;
    s_ScreenShotX       = 0 ;
    s_ScreenShotY       = 0 ;

    // File specified?
    if (pFileName)
        x_strcpy(s_ScreenShotName, pFileName) ;
    else
    {
        // Auto name
        s32     AutoName = 0 ;
        X_FILE* pFile    = NULL ;
        char    SubPicName[X_MAX_PATH] ;
        do
        {
            // Goto next auto name
            x_sprintf(s_ScreenShotName, "ps2sshot%03d.tga", AutoName++) ;
            
            // Try big file / single screen shot name
            pFile = x_fopen(s_ScreenShotName, "rb") ;
            if (!pFile)
            {
                // Try sub pic name
                GetScreenShotSubPicFName( SubPicName ) ;
                pFile = x_fopen(SubPicName, "rb") ;
            }

            // Just close the file if was found
            if (pFile)
                x_fclose(pFile) ;
        }
        while(pFile) ;  // Keep going until we get to a file that does not exist
    }

    // Easy case of just single frame buffer?
    if (Size == 1)
    {
        // Write it out
        DumpFrameBuffer( s_ScreenShotName ) ;
        return ;
    }
}

//=========================================================================

s32 eng_ScreenShotActive( void )
{
    return s_ScreenShotActive ;
}

//=========================================================================

s32 eng_ScreenShotSize( void )
{
    return s_ScreenShotSize ;
}

//=========================================================================

s32 eng_ScreenShotX( void )
{
    return s_ScreenShotX ;
}

//=========================================================================

s32 eng_ScreenShotY( void )
{
    return s_ScreenShotY ;
}

//=========================================================================

void ps2_8BitShot( const char* pFileName = NULL )
{
    static s32 NShots=0;
    char    AutoName[64];

    // Decide on a file name
    if( pFileName == NULL )
    {
        NShots++;
        x_strcpy(AutoName,xfs("ps2sshot%03d.tga",NShots));
        pFileName = AutoName;
    }

    // Allocate buffer 
    xcolor* pBuffer = (xcolor*)x_malloc( s_XRes*s_YRes*sizeof(xcolor) );
    byte* pByteBuffer = (byte*)x_malloc( s_XRes*s_YRes*sizeof(byte) );
    ASSERT(pBuffer);

    //
    // WAIT FOR PREVIOUS TASKS TO FINISH
    //
	dmaman_WaitForTasks();

    //
    // TURN OFF DMA HANDLERS
    //
    dmaman_HandlersOff();

    //
    // WAIT FOR VBLANK
    //
    sceGsSyncPath(0,0);      
    sceGsSyncV(0);

    // Build and execute image store
    sceGsStoreImage gs_simage;
    //sceGsSetDefStoreImage(&gs_simage, 0, s_XRes / 64, SCE_GS_PSMCT32, 0, 0, s_XRes, s_YRes);
    s32 NLines = s_YRes;
    s32 Offset = 0;
    while( NLines )
    {
        s32 N = MIN(NLines,128);
        sceGsSetDefStoreImage(&gs_simage, Offset*s_XRes/64, s_XRes / 64, SCE_GS_PSMT8H, 0, 0, s_XRes, N);
	    FlushCache(0);
	    sceGsExecStoreImage(&gs_simage, (u_long128*)(pByteBuffer+Offset*s_XRes));
	    sceGsSyncPath(0, 0);
        NLines -= N;
        Offset += N;
    }

    // Convert pixel colors
    s32 j=0;
    for( s32 i=0; i<s_XRes*s_YRes; i++ )
    {
        byte B = pByteBuffer[j];
        pBuffer[j].R = B;
        pBuffer[j].G = B;
        pBuffer[j].B = B;
        pBuffer[j].A = B;
        j++;
    }

    // Build tga header
    byte Header[18];
    x_memset(&Header,0,18);
    Header[16] = 32;
    Header[12] = (s_XRes>>0)&0xFF;
    Header[13] = (s_XRes>>8)&0xFF;
    Header[14] = (s_YRes>>0)&0xFF;
    Header[15] = (s_YRes>>8)&0xFF;
    Header[2]  = 2;
    //Header[17] = 8;
    Header[17] = 32;    // NOT flipped vertically.


    // Write out data
    X_FILE* fp;
    fp = x_fopen(pFileName,"wb");
    if( fp )
    {
        x_fwrite( Header, 18, 1, fp );

        // Write out scanlines in reverse order
        for( s32 i=0; i<s_YRes; i++ )
            x_fwrite( pBuffer+(i)*s_XRes, s_XRes*sizeof(xcolor), 1, fp );

        x_fclose( fp );
    }

    // free buffer
    x_free(pBuffer);
    x_free(pByteBuffer);

    // Turn handlers back on
    dmaman_HandlersOn();
}

//=============================================================================
//=============================================================================
//=============================================================================
//  VIEW MANAGEMENT
//=============================================================================
//=============================================================================
//=============================================================================

void eng_SetView ( const view& View, s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );
    s_View[ViewPaletteID] = View;;

    // Tell the view about a multi-part screen shot!
    s_View[ViewPaletteID].SetSubShot( s_ScreenShotX, s_ScreenShotY, s_ScreenShotSize ) ;
}

//=============================================================================

void eng_ActivateView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    // This view is already active
    if( s_ViewMask & (1<<ViewPaletteID) )
        return ;

    // Activate the view
    ASSERT( s_nActiveViews < ENG_MAX_VIEWS );

    s_ActiveView[ s_nActiveViews ] = &s_View[ ViewPaletteID ];
    s_nActiveViews++;
    s_ViewMask |= (1<<ViewPaletteID);
}

//=============================================================================

void eng_DeactivateView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    // The view is already not active
    if( (s_ViewMask & (1<<ViewPaletteID)) == 0 )
        return ;

    // Find the active view and remove it
    s32 i;
    for( i=0; i<ENG_MAX_VIEWS; i++ )
    {
        if( s_ActiveView[ i ] == &s_View[ ViewPaletteID ] )
        {
            s_ActiveView[ i ] = &s_View[ ViewPaletteID ];

            s_nActiveViews--;
            ASSERT( s_nActiveViews >= 0 );
            x_memmove( &s_ActiveView[ i ],  &s_ActiveView[ i+1 ], s_nActiveViews-i );            
            s_ViewMask &= ~(1<<ViewPaletteID);
            break;
        }
    }

    ASSERT( i < ENG_MAX_VIEWS );
}

//=============================================================================

u32 eng_GetActiveViewMask   ( void )
{
    return s_ViewMask;
}

//=============================================================================

const view* eng_GetView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    return &s_View[ ViewPaletteID ];
}

//=============================================================================

s32 eng_GetNActiveViews( void )
{
    return s_nActiveViews;
}

//=============================================================================

const view* eng_GetActiveView( s32 ActiveViewListID )
{
    ASSERT( ActiveViewListID >= 0  );
    ASSERT( ActiveViewListID < eng_GetNActiveViews() );

    return s_ActiveView[ ActiveViewListID ];
}

//=========================================================================
//=========================================================================
//=========================================================================
// MICROCODE
//=========================================================================
//=========================================================================
//=========================================================================

s32 eng_RegisterMicrocode   ( char* Name, void* CodeDMAAddr, s32 NBytes )
{

    ASSERT( ((u32)DLIST.GetAddr()) == (u32)ALIGN_16((u32)DLIST.GetAddr()) );
    ASSERT( s_NMCodes < MAX_MICROCODES );

    microcode* pM = &s_MCode[ s_NMCodes ];
    s_NMCodes++;

    x_strcpy( pM->Name, Name );

    pM->NBytes = ALIGN_16(NBytes);

    /*
    pM->pCode = (byte*)x_malloc( pM->NBytes );
    ASSERT( pM->pCode );
    x_memset( pM->pCode, 0, pM->NBytes );
    x_memcpy( pM->pCode, CodeDMAAddr, NBytes );
    */
    pM->pCode = (byte*)CodeDMAAddr;

    ASSERT( ((u32)pM->pCode) == (u32)ALIGN_8(((u32)pM->pCode)) );

    //x_printf("MICROCODE REGISTERED: %1s\n",Name);
    //x_printf("%08X %1d\n",(u32)pM->pCode,pM->NBytes);

    return s_NMCodes-1;
}

//=========================================================================

void eng_ActivateMicrocode   ( s32 ID )
{
    //ASSERT(eng_InsideTask());

    ASSERT( ((u32)DLIST.GetAddr()) == (u32)ALIGN_16((u32)DLIST.GetAddr()) );
    ASSERT( (ID>=0) && (ID<s_NMCodes) );
    
    if( ID == s_ActiveMCode )
        return;

    microcode* pM = &s_MCode[ ID ];

    s32 Size = pM->NBytes; 
    s32 Offset = 0;
    while( Size>0 )
    {
        s32 NBytes = MIN(Size,2032);
        dmatag* pDMA = DLStruct(dmatag);

        pDMA->SetRef( NBytes, (u32)(pM->pCode+Offset) );
        pDMA->PAD[0] = SCE_VIF1_SET_FLUSHE( 0 );
        pDMA->PAD[1] = SCE_VIF1_SET_MPG( (Offset/8), (NBytes/8), 0);

        Size -= NBytes;
        Offset += NBytes;
    }

    // Remember that this is the active microcode
    s_ActiveMCode = ID;

    ASSERT( ((u32)DLIST.GetAddr()) == (u32)ALIGN_16((u32)DLIST.GetAddr()) );
    // Display loading
    //x_printf("MICROCODE (%1s) Loaded. %1d\n",s_MCode[ID].Name);
}

//=========================================================================

s32 eng_GetActiveMicrocode  ( void )
{
    return s_ActiveMCode;
}


//=============================================================================
//=============================================================================
//=========================================================================
// MISC
//=============================================================================
//=============================================================================
//=============================================================================
/*
static
void SetAlphaBlending( void )
{
	struct alpha
	{
		dmatag   DMA;
		giftag   GIF;
		u64      AD[2];
	};

    alpha* pHeader = DLStruct(alpha);
    
    // Build the dma command
	pHeader->DMA.SetCont( sizeof(alpha) - sizeof(dmatag) );
	pHeader->DMA.MakeDirect();

    pHeader->GIF.BuildRegLoad(1,TRUE);

    // Set the alpha
    pHeader->AD[0] = SCE_GS_SET_ALPHA(0,1,0,1,1);
    pHeader->AD[1] = SCE_GS_ALPHA_1;
    //pHeader->AD[2] = 0;
    //pHeader->AD[3] = 0;
}
*/
//=========================================================================

void    eng_GetStats(s32 &Count, f32 &CPU, f32 &GS, f32 &INT, f32 &FPS)
{
    Count=0;
	//Count = (s32)TIME_SecSince(s_StartTime) ;
    CPU   = STAT_CPUMS;
    GS    = STAT_GSMS;
    INT   = STAT_IMS;
    FPS   = 1000.0f / (STAT_CPUMS+STAT_IMS);
}

//=========================================================================

void    eng_PrintStats      ( void )
{
    s32 Count ;
    f32 CPU, GS, INT, FPS ;
    
    eng_GetStats(Count, CPU, GS, INT, FPS) ;

    x_DebugMsg("================ Engine Stats ================\n");
	x_DebugMsg("%1d CPU:%4.1f  GS:%4.1f  INT:%4.1f  FPS:%4.1f\n", Count, CPU, GS, INT, FPS) ;
    dmaman_PrintTaskStats();

    {
        vector3 Pos;
        radian Pitch;
        radian Yaw;

        Pos = s_View[0].GetPosition();
        s_View[0].GetPitchYaw(Pitch,Yaw);

        x_DebugMsg("View0 Location: (%1.4f,%1.4f,%1.4f) P:%1.4f Y:%1.4f\n",
            Pos.X,Pos.Y,Pos.Z,Pitch,Yaw);
    }


    //for( s32 i=0; i<5; i++ )
        //printf("%4.1f ",STAT_INTERNAL[i]);
    //printf("\n");
}

//=========================================================================

xbool   eng_InsideTask      ( void )
{
    return s_InsideTask;
}

//=========================================================================

void eng_GetW2S( matrix4& pM )
{
    pM = s_MWorldToScreen;
}

//=========================================================================

void eng_GetW2SC( matrix4& pM )
{
    pM = s_MWorldToScreenClip ;
}

//=========================================================================

void eng_GetW2V( matrix4& pM )
{
    pM = s_MWorldToView;
}
//=========================================================================

void eng_GetW2C( matrix4& pM )
{
    pM = s_MWorldToClip;
}

//=========================================================================

void eng_GetL2S( matrix4& pM )
{
    if( !s_LocalToScreenComputed )
    {
        s_MLocalToScreen = s_MWorldToScreen * s_MLocalToWorld;
        s_LocalToScreenComputed = TRUE;
    }

    pM = s_MLocalToScreen;
}
//=========================================================================

void eng_GetL2C( matrix4& pM )
{
    if( !s_LocalToClipComputed )
    {
        s_MLocalToClip = s_MWorldToClip * s_MLocalToWorld;
        s_LocalToClipComputed = TRUE;
    }

    pM = s_MLocalToClip;
}

//=========================================================================

void eng_GetC2S( matrix4& pM )
{
    pM = s_MClipToScreen;
}

//=========================================================================

void    eng_GetL2W( matrix4& M )
{
    M = s_MLocalToWorld;
}

//=========================================================================

void    eng_SetL2W( const matrix4& M )
{
    s_MLocalToWorld = M;
    s_LocalToScreenComputed = FALSE;
    s_LocalToClipComputed = FALSE;
}

//=========================================================================
/*
void eng_SetActiveView( const view& View )
{
    ASSERT(!eng_InsideTask());

    matrix4 MW2V;
    matrix4 MV2C;
    matrix4 MC2S;
    matrix4 MC2SC ;

    radian  XFOV,YFOV;
    s32     L,T,R,B;
    f32     W,H;
    f32     ZN,ZF;                     
    f32     VS = 1.0f;
#ifdef BIT16
    f32     ZScale = (f32)((s32)1<<14);
#else
    f32     ZScale = (f32)((s32)1<<19);
#endif

    //    
    // Copy view into active view and get ingredients
    //
    x_memcpy( &s_View, &View, sizeof(view));


    //
    // Get viewport bounds and other info
    //
    s_View.GetViewport  ( L, T, R, B );
    s_View.GetZLimits   ( ZN, ZF );
    s_View.GetXFOV      ( XFOV );
    s_View.GetYFOV      ( YFOV );

    //
    // Build world to view matrix
    //
    s_View.GetW2VMatrix ( MW2V );

    //
    // Build view to clip matrix.  All components map
    // into (-1,+1) cube after W division
    //

    H = 1.0f / x_tan( YFOV*0.5f );
    W = 1.0f / x_tan( XFOV*0.5f );

    MV2C.Identity();
    MV2C.M[0][0] = -(1/VS)*W;
    MV2C.M[1][1] =  (1/VS)*H;
    MV2C.M[2][2] = (ZF+ZN)/(ZF-ZN);
    MV2C.M[3][2] = (-2*ZN*ZF)/(ZF-ZN);
    MV2C.M[2][3] = 1.0f;
    MV2C.M[3][3] = 0.0f;

    //    
    // Build clip to screen matrix.  Takes (-1,+1) cube
    // and maps to viewport.  Z gets mapped NZ=1.0, FZ=0.0
    //

    W = (R-L)*0.5f;
    H = (B-T)*0.5f;
    
    MC2S.Identity();
    MC2S.M[0][0] =  (VS)*W;
    MC2S.M[1][1] = -(VS)*H;
    MC2S.M[3][3] = 1.0f;
    MC2S.M[3][0] = W + L + (f32)WINDOW_LEFT;
    MC2S.M[3][1] = H + T + (f32)WINDOW_TOP;
    MC2S.M[2][2] = -ZScale/2;
    MC2S.M[3][2] =  ZScale/2;
  
    //
    // Same as the clip to screen matrix, but instead being relative to the
    // center of the screen, it's relative to 2048,2048 ready for screen space
    // clip testing.
    //

    MC2SC = MC2S ;
    MC2SC.Translate(vector3(-2048.0f, -2048.0f, -2048.0f)) ;    // Full screen clipping
    //MC2SC.Translate(vector3(-2048.0f, -2048.0f, -150.0f)) ; // Temp for 150 square clip test

    //  
    // Build final matrices
    //
    
    s_MWorldToView       = MW2V;
    s_MWorldToClip       = MV2C * s_MWorldToView;
    s_MClipToScreen      = MC2S;
    s_MWorldToScreen     = s_MClipToScreen * s_MWorldToClip;
    s_MWorldToScreenClip = MC2SC * s_MWorldToClip;

    //
    // Clear L2W
    //
    s_MLocalToWorld.Identity();
    s_LocalToScreenComputed=FALSE;
    s_LocalToClipComputed=FALSE;
}
*/
//=========================================================================

u64     eng_GetFRAMEReg     ( void )
{
    return FB_GetFRAMEReg(s_pAAbuff,  ((s_FrameCount)&0x01));
}

//=========================================================================

// 0=back buffer, 1=front buffer
u64     eng_GetFRAMEReg     ( s32 Buffer )
{
    ASSERT((Buffer == 0) || (Buffer == 1)) ;
    return FB_GetFRAMEReg(s_pAAbuff,  ((s_FrameCount+Buffer)&0x01));
}

//=========================================================================

u32     eng_GetFrameBufferAddr ( s32 Buffer )
{
    ASSERT((Buffer == 0) || (Buffer == 1)) ;
    return FB_GetFrameBufferAddr(s_pAAbuff,  (s_FrameCount + Buffer) & 0x01) ;
}

//=========================================================================

void    eng_PushGSContext   ( s32 Context )
{
    ASSERT( (Context==0) || (Context==1) );
    ASSERT( s_ContextStackIndex < 7 );
    s_ContextStackIndex++;
    s_ContextStack[s_ContextStackIndex] = Context;
    s_Context = Context;
}

//=========================================================================

void    eng_PopGSContext    ( void )
{
    if( s_ContextStackIndex==0 ) return;
    s_ContextStackIndex--;
    s_Context = s_ContextStack[s_ContextStackIndex];
}

//=========================================================================

s32     eng_GetGSContext    ( void )
{
    return s_Context;
}

//=========================================================================

void text_BeginRender( void )
{
    font_BeginRender();
}

//=========================================================================

void text_RenderStr( char* pStr, s32 NChars, xcolor Color, s32 PixelX, s32 PixelY )
{
    font_Render( pStr, NChars, Color, PixelX, PixelY );
}

//=========================================================================

void text_EndRender( void )
{
    font_EndRender();
}

//=============================================================================
//=============================================================================
//=========================================================================
// STARTUP
//=============================================================================
//=============================================================================
//=============================================================================

void PS2_SetFileMode( s32 Mode, char* pPrefix );
char ENGArgv0[256];
void ps2_GetFreeMem(s32 *pFree,s32 *pLargest,s32 *pFragments);

//=============================================================================

void ps2eng_Begin( s32 argc, char* argv[] )
{
    s32 Free,Largest,Fragments,Used;
	u16 mediatype;

    argc=0;
#ifdef TARGET_PS2_DEV
    xbool CDROM=FALSE;
#else
    xbool CDROM=TRUE;
#endif

    #ifdef VENDOR_MW
	mwInit();    /* To initialize the C++ runtime */
    #endif


    //
    // Watch for booting from a cdrom on a non-cd build
    //
#ifndef TARGET_PS2_DVD
    if( argc && 
        (argv[0][0] == 'c') &&
        (argv[0][1] == 'd') &&
        (argv[0][2] == 'r') &&
        (argv[0][3] == 'o') &&
        (argv[0][4] == 'm') )
#endif
    {
        CDROM = TRUE;
    }

    if( CDROM )
    {
        sceSifInitRpc(0);
        sceCdInit(SCECdINIT);
        mediatype = SCE_DEMO_MEDIATYPE_CD;

#ifdef DEMO_DISK_HARNESS
        u16 language,aspect,playmode,inactive,gameplay,mastervol,dirsector;

        language=aspect=playmode=inactive=gameplay=mastervol=dirsector = 0;

        sceDemoStart(argc,argv,&language,&aspect,&playmode,&inactive,&gameplay,&mediatype,&mastervol,&dirsector);
#endif

        if (mediatype==SCE_DEMO_MEDIATYPE_DVD)
        {
            sceCdMmode(SCECdDVD);
        }
        else
        {
            sceCdMmode(SCECdCD);
        }

        scePrintf("IOP rebooting...\n");
#ifdef DEMO_DISK_HARNESS
        while(!sceSifRebootIop("cdrom:\\TRIBES\\MODULES\\IOPRP243.IMG;1"));
#else
        while(!sceSifRebootIop("cdrom:\\MODULES\\IOPRP243.IMG;1"));
#endif
        
        scePrintf("IOP syncing...\n");
        while(!sceSifSyncIop());

        sceSifInitRpc(0);
        sceCdInit(SCECdINIT);

		if (mediatype==SCE_DEMO_MEDIATYPE_DVD)
		{
			sceCdMmode(SCECdDVD);
		}
		else
		{
			sceCdMmode(SCECdCD);
		}
    }
    else
    {
	    sceSifInitRpc(0);
#ifdef bwatson
        sceCdInit(SCECdINIT);
        sceCdMmode(SCECdCD);
#endif
    }

//#ifdef TARGET_PS2_DVD
	ShowSplash();
//#endif

    x_Init();

    // Do low-level init
    // 02/14/02 - x_Init now performed by the threading system
    //x_Init();
    ps2_GetFreeMem(&Free,&Largest,&Fragments);
    Used = Free;

#if defined(TARGET_PS2_DVD) || defined(TARGET_PS2_CLIENT)
#if !defined(RELEASE_CANDIDATE)
	// BW - 6/27/02, only enable the exception catcher on a teststation. On a devkit, this
	// will mean that we will drop straight in to the debugger should a crash occur.
	u32 sp;
	asm volatile("move %0,$29":"=r"(sp));
	if ( sp < 32 * 1048576)
	{
		except_Init();    
	}
	else
	{
		scePrintf("WARNING: Exception catcher was not installed as we're on a devkit\n");
	}

#endif
#else
    // The exception library, for some unforsaken reason, takes 170K. 
    // Probably that 128K scratch area.
    x_malloc(170*1024);
#endif

    // Install CDFS if booted from CDROM
    if( CDROM )
    {
        cdfs_Init( "cdrom:\\TRIBES\\FILES.DAT;1" );
//        cdfs_Init( "host:FILES.DAT" );
    }
    else
    {
        if (!cdfs_Init( "host:FILES.DAT" ))
        {
            // If the initialization fails on a host build (i.e. no files.dat) then
            // allocate some memory to compensate for the amount that would have been
            // used by cdfs_Init()
            x_malloc(310*1024);
        };
    }

    ps2_GetFreeMem(&Free,&Largest,&Fragments);
    Used -= Free;
    scePrintf("cdfs_Init: %d free, %d largest, %d fragments, %d used.\n",Free,Largest,Fragments,Used);
    //TIME_Init();

    // Update PS2 fileio with "correct" prefix and mode

    //
    // Copy argv[0] for future reference
    //
    if (argc)
    {
        x_strcpy(ENGArgv0,argv[0]);

        // display argv[0]
        //fclose(fopen(argv[0],"rb"));

        //
        // Watch for being a debug station
        //
        s32 i=0;
        while( argv[0][i] && (argv[0][i]!=',')) i++;
        if( argv[0][i] == ',' )
        {
            argv[0][i+0] = ',';
            argv[0][i+1] = 0;
            //PS2_SetFileMode(1,argv[0]);
        }
    }
    else
    {
        *ENGArgv0=0x0;
    }



    // Init iop
    iop_Init(4096);
#ifdef TARGET_PS2_CLIENT
    eng_Init();
    x_printfxy(3,6,"Waiting for network subsystem");
    x_printfxy(4,7,"to complete Initialization");
    eng_PageFlip();
    eng_PageFlip();
    net_Init();

    s32 status;

    status = -1;
    while (1)
    {
        status = net_SetConfiguration("mc0:BWNETCNF/BWNETCNF",0);
        if (status >= 0)
            break;
        x_printfxy(3,6,"Waiting for configuration to");
        x_printfxy(4,7,"activate.");
        eng_PageFlip();
        eng_PageFlip();

    }
    net_ActivateConfig(TRUE);
    netfs_Init();
#endif

    //
    // Let us know if we are running on cdrom
    //
    //if( CDROM )
//        PS2_SetFileMode(2,NULL);
}

//=============================================================================

void ps2eng_End( void )
{
    // Kill CDFS - it is safe to call if it was not installed
    cdfs_Kill();

#ifdef TARGET_PS2_CLIENT
    netfs_Kill();
    net_Kill();
#endif
    iop_Kill();
    // Do low-level kill
    //TIME_Kill();
    x_Kill();
    
    #ifdef VENDOR_MW
	mwExit();    /* Clean up, destroy constructed global objects */
    #endif
#ifdef DEMO_DISK_HARNESS
    sceDemoEnd(SCE_DEMO_ENDREASON_PLAYABLE_QUIT);
#endif
}

//=============================================================================

void eng_SetViewport( const view& View )
{
    (void)View;
}

//=============================================================================

void eng_PushDList( const dlist& DList )
{
    s_DListStackIndex++;
    ASSERT( s_DListStackIndex < DLIST_STACK_SIZE );
    s_DListStack[s_DListStackIndex] = DLIST;
    DLIST = DList;
}

//=============================================================================

void eng_PopDList( void )
{
    ASSERT( s_DListStackIndex >= 0 );
    DLIST = s_DListStack[s_DListStackIndex];
    s_DListStackIndex--;
}

//=============================================================================

f32 eng_GetTVRefreshRate( void )
{
	if (s_PalMode)
	{
		return 50.0f;
	}
	else
	{
		return 60.0f ;  // NTSC
	}
}

//=============================================================================


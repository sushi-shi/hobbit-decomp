#include <xCore/x_files/x_files.hpp>
#include <xCore/x_files/x_threads.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include <xCore/Entropy/Common/e_cdfs.hpp>
#include <stdio.h>

#if !defined(TARGET_LINUX)
#error This file is for a linux build only. Please check your exclusion rules for this project.
#endif

#include <signal.h>
//-----------------------------------------------
// Perform initial engine startup required for linux
//-----------------------------------------------
	xbool bmp_Load(xbitmap&, const char*);
	xbool tga_Load(xbitmap&, const char*);
	xbool png_Load(xbitmap&, const char*);
	xbool psd_Load(xbitmap&, const char*);

void linux_eng_Begin( s32 argc, char* argv[] )
{
	(void)argc;
	(void)argv;

	printf("linux_eng_Begin: Entered Application\n");
    x_Init();
	cdfs_Init( "files.dat" );


}

//-----------------------------------------------
// Perform post-execution shutdown required for linux
//-----------------------------------------------
void linux_eng_End( void )
{
	printf("linux_eng_End: Shutting down application\n");
    // Kill CDFS - it is safe to call if it was not installed
    cdfs_Kill();
    x_Kill();
}


//=========================================================================
//
// E_ENGINE.CPP
//
//=========================================================================

#include <xCore/Entropy/e_Engine.hpp>
       s32              s_FrameCount;
static s32              s_SMem      = (450*1024);

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


void draw_Init( void );
void draw_Kill( void );
void eng_Init(void)
{
    smem_Init(s_SMem);
    RoundTrip.Start();
	// Ignore the sigpipe signal which is caused when no data is
	// available on a socket that has been force-closed by the receiving
	// side.
	signal(SIGPIPE, SIG_IGN);
}

//=========================================================================

void eng_Kill(void)
{
    smem_Kill();
}

//=========================================================================

void eng_GetRes( s32& XRes, s32& YRes )
{
    XRes = 640;
    YRes = 480;
}

//=========================================================================

void eng_MaximizeViewport( view& View )
{
}

//=========================================================================

void eng_SetBackColor( xcolor Color )
{
}

//=========================================================================

void eng_Sync( void )
{
}

//=========================================================================
void eng_PageFlip(void)
{
 	x_WatchdogReset();
    //
    // Be sure 30ms is used up (40ms for pal)
    //
    while( RoundTrip.ReadMs() < 30.0f)
    {
        x_DelayThread( 1 );
    }
    RoundTrip.Reset();
    RoundTrip.Start();

    //
    // TOGGLE SYSTEMS
    //
    smem_Toggle();

    //
    // CHECK INPUT
    //
    // bw 7/22/01 input_CheckDevices() no longer needed since it's done in a seperate thread

    //
    // INCREMENT NUMBER OF FRAME PROCESSED
    //
    s_FrameCount++;

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
    return TRUE;
}

//=========================================================================

void eng_End(void)
{
}

//=========================================================================


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

void    eng_GetStats(s32 &Count, f32 &CPU, f32 &GS, f32 &INT, f32 &FPS)
{
}

//=========================================================================

void    eng_PrintStats      ( void )
{
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

void text_BeginRender( void )
{
}

//=========================================================================

void text_RenderStr( char* pStr, s32 NChars, xcolor Color, s32 PixelX, s32 PixelY )
{
}

//=========================================================================

void text_EndRender( void )
{
}


//=============================================================================

void eng_SetViewport( const view& View )
{
    (void)View;
}

//=============================================================================

//=============================================================================

f32 eng_GetTVRefreshRate( void )
{
	{
		return 60.0f ;  // NTSC
	}
}

//=============================================================================

//=============================================================================

void vram_Init( void )
{
}

//=============================================================================

void vram_Kill( void )
{
}

//=============================================================================

//=============================================================================

s32 vram_LoadTexture( const char* pFileName )
{
	return 0;
}

//=============================================================================

void vram_Activate( void )
{
}

//=============================================================================

void vram_Activate( s32 VRAM_ID )
{
    // Note: VRAM_ID == 0 means bitmap not registered!
}

//=============================================================================

s32 vram_Register( const xbitmap& Bitmap )
{
	return 0;
}

//=============================================================================

void vram_Unregister( s32 VRAM_ID )
{
}

//=============================================================================

void vram_Unregister( const xbitmap& Bitmap  )
{
}

//=============================================================================

void vram_Activate( const xbitmap& Bitmap  )
{
}

//=============================================================================

xbool vram_IsActive( const xbitmap& Bitmap )
{
	return TRUE;
}

//=============================================================================

void vram_Flush( void )
{
}

//=============================================================================

s32 vram_GetNRegistered( void )
{
    return 0;
}

//=============================================================================

s32 vram_GetRegistered( s32 ID )
{
    return -1;
}
//=============================================================================

void vram_SanityCheck( void )
{

}

xbool eng_ScreenShotActive(void)
{
	return FALSE;
}

void input_Feedback(float,float,s32)
{
}

xbool input_UpdateState(void)
{
	return FALSE;
}

xbool input_IsPressed(input_gadget,s32)
{
	return FALSE;
}

xbool input_WasPressed(input_gadget,s32)
{
	return FALSE;
}

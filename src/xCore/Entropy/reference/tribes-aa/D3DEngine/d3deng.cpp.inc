
///////////////////////////////////////////////////////////////////////////
// INCLUDES 
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <xCore/Entropy/e_Engine.hpp>
#include <xCore/x_files/x_threads.hpp>
#include <xCore/Entropy/Common/e_cdfs.hpp>

///////////////////////////////////////////////////////////////////////////
// INCLUDE ADITIONAL LIBRARIES
///////////////////////////////////////////////////////////////////////////

#pragma comment( lib, "winmm" )

///////////////////////////////////////////////////////////////////////////
// MAKE SUER THAT WE HAVE THE RIGHT VERSION
///////////////////////////////////////////////////////////////////////////

#ifndef DIRECT3D_VERSION
#error ------- COMPILING VERSION ? OF D3D ------- 
#endif

#if(DIRECT3D_VERSION < 0x0600)
#error ------- COMPILING VERSION 5 OF D3D ------- 
#endif

#if(DIRECT3D_VERSION < 0x0700)
#error ------- COMPILING VERSION 6 OF D3D ------- 
#endif

#if(DIRECT3D_VERSION < 0x0800)
#error ------- COMPILING VERSION 7 OF D3D ------- 
#endif

///////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////

#define ENG_FONT_SIZEX      7
#define ENG_FONT_SIZEY      12
#define SCRACH_MEM_SIZE     (2*1024*1024)
#define MAX_VERTEX_SHADERS  32
#define MAX_PIXEL_SHADERS   32

#define FULL_SCREEN_STYLE   (WS_POPUP)
//#define WINDOW_STYLE        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WINDOW_STYLE        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)


///////////////////////////////////////////////////////////////////////////
// EXTERNAL DEPENDENCIES
///////////////////////////////////////////////////////////////////////////

xbool d3deng_InitInput  ( HWND Window );

void  vram_Init         ( void );
void  vram_Kill         ( void );

void  smem_Init         ( s32 NBytes );
void  smem_Kill         ( void );

void  draw_Init         ( void );
void  draw_Kill         ( void );

// Finds closest mode on current adapter and device
s32 d3deng_FindBestMode( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed ) ;

void RestoreScreen( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed ) ;



///////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
///////////////////////////////////////////////////////////////////////////

IDirect3DDevice8*   g_pd3dDevice          = NULL;

xtimer rst;
s32 rstct;


///////////////////////////////////////////////////////////////////////////
// STRUCTURES
///////////////////////////////////////////////////////////////////////////

struct vertex_shader
{
    char            Name[32] ;  // Name
    const DWORD*    pDecl ;     // Declaration
    const DWORD*    pFunc ;     // Function data
          DWORD     Usage ;     // Usage
          DWORD     FVF ;       // Flexible vertex format flags (incase of fail)

          DWORD     Handle ;    // Current handle
} ;

struct pixel_shader
{
    char            Name[32] ;  // Name
    const DWORD*    pFunc ;     // Function data

          DWORD     Handle ;    // Current handle
} ;

// D3D mode
struct d3d_mode
{
    // Useful vars
    char            Name[32] ;              // Name of mode
    s32             BitDepth ;              // Bits per pixel
    xbool           bWindowed ;             // TRUE if it's a window mode

    // D3D description
    D3DDISPLAYMODE  DisplayMode ;           // Width, height + pixel format
    DWORD           dwBehavior;             // Hardware / Software / Mixed vertex processing
    D3DFORMAT       DepthStencilFormat;     // Which depth/stencil format to use with this mode
} ;

// D3D device
struct d3d_device
{
    // Useful vars
    char                Name[32] ;          // Name of device
    xarray<d3d_mode>    Modes ;             // List of modes

    // D3D Vars
    D3DDEVTYPE          Type ;              // D3D Type
    D3DCAPS8            Caps ;              // D3D capabilities
} ;

// D3D adapter                         
struct d3d_adapter
{
    // Useful vars
    char                    Name[MAX_DEVICE_IDENTIFIER_STRING] ;    // Name of adapter
    xarray<d3d_device>      Devices ;                               // List of device

    // D3D vars
    UINT                    D3DAdapter ;                            // D3D index
    D3DDISPLAYMODE          d3ddmDesktop ;                          // Desktop mode
} ;                         


///////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
///////////////////////////////////////////////////////////////////////////

static struct eng_locals
{
    eng_locals( void ) { memset( this, 0, sizeof(eng_locals) ); }

    //
    // general variables
    //
    HINSTANCE       hInst; 
    HWND            hWnd;  
    IDirect3D8*     pD3D;
    LPD3DXFONT      pFont;
    xcolor          TextColor;
    u32             Mode;
    xcolor          BackColor;
    xbool           bBeginScene;
    xbool           bD3DBeginScene;
    DWORD           dwMainThreadID ;    // ID of main thread (where game window was created)

    //
    // View variables
    //
    view            View[ ENG_MAX_VIEWS ];
    view*           ActiveView[ ENG_MAX_VIEWS ];
    s32             nActiveViews;
    s32             nViews;
    u32             ViewMask;

    //
    // Window/screen related varaibles
    //
    xbool                   bActive;
    xbool                   bReady;      
    RECT                    rcWindowRect;
    D3DPRESENT_PARAMETERS   d3dpp ; 
    xarray<d3d_adapter>     Adapters ;
    
    // Current video format
    s32                     VideoAdapter ;
    s32                     VideoDevice ;
    xbool                   VideoWindowed;   
    s32                     VideoMode ;
    s32                     VideoWidth ;
    s32                     VideoHeight ;
    s32                     VideoBitDepth ;
    d3deng_change_res_fn*   ChangeResCallback ;

    //
    // Input related variables
    //
    MSG             Msg;
    s32             LastPressedKey;
    f32             MouseX;
    f32             MouseY;
    xbool           bMouseLeftButton;
    xbool           bMouseRightButton;
    xbool           bMouseMiddleButton;
    f32             MouseWheelAbs;
    f32             MouseWheelRel;
    xbool           bMouseDelta;
    mouse_mode      MouseMode;
    POINT           MouseLastAbsPos ;

    //
    // FPS variables
    //
    xtick           FPSFrameTime[8];
    xtick           FPSLastTime;
    s32             FPSIndex;
    xtimer          CPUTIMER;
    f32             CPUMS;
    f32             IMS;

    // Vertex shader management
    s32             NVertexShaders ;
    vertex_shader   VertexShaders[MAX_VERTEX_SHADERS] ;
    s32             ActiveVertexShader ;

    // Pixel shader management
    s32             NPixelShaders ;
    pixel_shader    PixelShaders[MAX_PIXEL_SHADERS] ;
    s32             ActivePixelShader ;
} s;



//=========================================================================
// Adapter functions
//=========================================================================

// Returns adapter count
s32 d3deng_GetAdapterCount( void )
{
    return s.Adapters.GetCount() ;
}

// Returns adapter name
const char* d3deng_GetAdapterName( s32 Adapter )
{
    return s.Adapters[Adapter].Name ;
}

//=========================================================================
// Device functions
//=========================================================================

// Returns number of devices on adapter
s32 d3deng_GetDeviceCount( s32 Adapter )
{
    return s.Adapters[Adapter].Devices.GetCount() ;
}

// Returns name of device
const char* d3deng_GetDeviceName( s32 Adapter, s32 Device )
{
    return s.Adapters[Adapter].Devices[Device].Name ;
}

//=========================================================================
// Mode functions
//=========================================================================

// Returns number of modes on adapter device
s32 d3deng_GetModeCount( s32 Adapter, s32 Device )
{
    return s.Adapters[Adapter].Devices[Device].Modes.GetCount() ;
}

//=========================================================================

// Returns name of mode
const char* d3deng_GetModeName( s32 Adapter, s32 Device, s32 Mode )
{
    return s.Adapters[Adapter].Devices[Device].Modes[Mode].Name ;
}

//=========================================================================

// Returns width of mode
s32 d3deng_GetModeWidth( s32 Adapter, s32 Device, s32 Mode )
{
    return s.Adapters[Adapter].Devices[Device].Modes[Mode].DisplayMode.Width ;
}

//=========================================================================

// Returns height of mode
s32 d3deng_GetModeHeight( s32 Adapter, s32 Device, s32 Mode )
{
    return s.Adapters[Adapter].Devices[Device].Modes[Mode].DisplayMode.Height ;
}

//=========================================================================

// Returns bit depth of mode
s32 d3deng_GetModeBitDepth( s32 Adapter, s32 Device, s32 Mode )
{
    return s.Adapters[Adapter].Devices[Device].Modes[Mode].BitDepth ;
}

//=========================================================================

// Sets current screen mode
s32 d3deng_SetMode( s32 Adapter, s32 Device, s32 Mode )
{
    // Keep new info
    s.VideoAdapter = Adapter ;
    s.VideoDevice  = Device ;
    s.VideoMode    = Mode ;

    // Lookup info
    s32   Width    = s.Adapters[Adapter].Devices[Device].Modes[Mode].DisplayMode.Width ;
    s32   Height   = s.Adapters[Adapter].Devices[Device].Modes[Mode].DisplayMode.Height ;
    s32   BitDepth = s.Adapters[Adapter].Devices[Device].Modes[Mode].BitDepth ;
    xbool Windowed = s.Adapters[Adapter].Devices[Device].Modes[Mode].bWindowed ;

    // Goto new res!
    RestoreScreen(Width, Height, BitDepth, Windowed) ;

    return TRUE ;
}

//=========================================================================

// Call this to hookup a function that will get called when the resolution changes
void d3deng_SetChangeResCallback( d3deng_change_res_fn* Callback )
{
    s.ChangeResCallback = Callback ;
}

//=========================================================================

// Returns current mode info
void d3deng_GetMode( s32& Adapter, s32& Device, s32& Mode )
{
    s.VideoAdapter = Adapter ;
    s.VideoDevice  = Device ;
    s.VideoMode    = Mode ;
}

//=========================================================================

// Finds closest mode on current adapter and device
s32 d3deng_FindBestMode( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed )
{
    // Get current adapter and device
    d3d_adapter& Adapter = s.Adapters[s.VideoAdapter] ;
    d3d_device&  Device  = Adapter.Devices[s.VideoDevice] ;

    // Reset to no mode found
    s32 Best      = -1 ;
    s32 BestScore = S32_MAX ;

    s32 i, Score ;

    // If we want a window, just choose the closest resolution and ignore bit depth
    if (bWindowed)
    {
        // Check all modes
        for (i = 0 ; i < Device.Modes.GetCount() ; i++)
        {
            // Get mode
            d3d_mode& Mode = Device.Modes[i] ;
            if (Mode.bWindowed)
            {
                // Best score?
                Score = SQR(Mode.DisplayMode.Width - Width) + SQR(Mode.DisplayMode.Height - Height) ;
                if (Score < BestScore)
                {
                    // Keep
                    BestScore = Score ;
                    Best      = i ;
                }
            }
        }
    }

    // Attempt1 - Find the closest mode at this bit depth
    if (Best == -1)
    {
        // Check all modes
        for (i = 0 ; i < Device.Modes.GetCount() ; i++)
        {
            d3d_mode& Mode = Device.Modes[i] ;

            // Match?
            if (    (Mode.BitDepth  == BitDepth)
                 && (Mode.bWindowed == bWindowed) )
            {
                // Best score?
                Score = SQR(Mode.DisplayMode.Width - Width) + SQR(Mode.DisplayMode.Height - Height) ;
                if (Score < BestScore)
                {
                    // Keep
                    BestScore = Score ;
                    Best      = i ;
                }
            }
        }
    }

    // Attempt2 - Find the closest resolution mode match
    if (Best == -1)
    {
        // Check all modes
        for (i = 0 ; i < Device.Modes.GetCount() ; i++)
        {
            d3d_mode& Mode = Device.Modes[i] ;

            // Match?
            if (Mode.bWindowed == bWindowed)
            {
                // Best score?
                Score = SQR(Mode.DisplayMode.Width - Width) + SQR(Mode.DisplayMode.Height - Height) ;
                if (Score < BestScore)
                {
                    // Keep
                    BestScore = Score ;
                    Best      = i ;
                }
            }
        }
    }

    // Attempt3 - Find the closest resolution match
    if (Best == -1)
    {
        // Check all modes
        for (i = 0 ; i < Device.Modes.GetCount() ; i++)
        {
            d3d_mode& Mode = Device.Modes[i] ;

            // Best score?
            Score = SQR(Mode.DisplayMode.Width - Width) + SQR(Mode.DisplayMode.Height - Height) ;
            if (Score < BestScore)
            {
                // Keep
                BestScore = Score ;
                Best      = i ;
            }
        }
    }

    // Return the best found or -1 if none
    return Best ;
}

//=========================================================================

// Chooses closest mode on current adapter and device
s32 d3deng_SetMode( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed )
{
    // Easy!
    RestoreScreen(Width, Height, BitDepth, bWindowed) ;

    return TRUE ;
}

//=========================================================================

// Adds mode to adapters device if it's valid
static
void AddMode( d3d_adapter& Adapter, 
              d3d_device&  Device,
              UINT         cMode,
              xbool        bWindowed)
{
    dxerr   HR ;
    s32     i ;

    // Init mode
    d3d_mode Mode ;
    memset(&Mode, 0, sizeof(Mode)) ;

    // Setup mode info
    Mode.bWindowed  = bWindowed ;
    HR = s.pD3D->EnumAdapterModes(Adapter.D3DAdapter, cMode, &Mode.DisplayMode) ;
    if (HR != D3D_OK)
        return ;

    // Skip resolutions below 640x480
    if ((Mode.DisplayMode.Width < 640) || (Mode.DisplayMode.Height < 480))
        return ;

    // Windowed special cases
    if (Mode.bWindowed)
    {
        // Use desktop format
        Mode.DisplayMode.Format = Adapter.d3ddmDesktop.Format ;

        // If bigger than desktop, skip
        if (        (Mode.DisplayMode.Width  >= Adapter.d3ddmDesktop.Width)
                ||  (Mode.DisplayMode.Height >= Adapter.d3ddmDesktop.Height) )
        {
            return ;
        }
    }

    // Only allow 16bit or 32bit formats
    switch(Mode.DisplayMode.Format)
    {
        // Skip
        default:
            return ;

        case D3DFMT_X1R5G5B5:
        case D3DFMT_R5G6B5:
            Mode.BitDepth = 16 ;
            break ;

        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8R8G8B8:
            Mode.BitDepth = 32 ;
            break ;
    }

    // Skip if the device doesn't support this mode (could be due to monitor not supporting it)
    if (s.pD3D->CheckDeviceType(Adapter.D3DAdapter, 
                                Device.Type, 
                                Mode.DisplayMode.Format, 
                                Mode.DisplayMode.Format, 
                                Mode.bWindowed) != D3D_OK)
    {
        return ;
    }

    // Is this mode already present?
    for (i = 0 ; i < Device.Modes.GetCount() ; i++)
    {
        // Get mode
        d3d_mode& SearchMode = Device.Modes[i] ;

        // Match?
        if (       (SearchMode.DisplayMode.Width  == Mode.DisplayMode.Width)
                && (SearchMode.DisplayMode.Height == Mode.DisplayMode.Height)
                && (SearchMode.DisplayMode.Format == Mode.DisplayMode.Format)
                && (SearchMode.bWindowed          == Mode.bWindowed) )
        {
            // If the refresh rate of new mode is higher, just keep it
            if (SearchMode.DisplayMode.RefreshRate < Mode.DisplayMode.RefreshRate)
                SearchMode.DisplayMode.RefreshRate = Mode.DisplayMode.RefreshRate ;

            // Do not add
            return ;
        }
    }

    // We now have enough info to setup the name
    if (Mode.bWindowed)
        x_sprintf(Mode.Name, "%dx%dx%d (Windowed)", Mode.DisplayMode.Width, Mode.DisplayMode.Height, Mode.BitDepth) ;
    else
        x_sprintf(Mode.Name, "%dx%dx%d (FullScreen)", Mode.DisplayMode.Width, Mode.DisplayMode.Height, Mode.BitDepth) ;

    // TO DO - dwBehavior and DepthStencilFormat

    // Finally, success! Add it
    Device.Modes.Append(Mode) ;
}

//=========================================================================

// Adds device to adapter
static 
void AddDevice ( d3d_adapter& Adapter, const D3DDEVTYPE Type, const D3DCAPS8& Caps )
{
    // Create device
    d3d_device& Device = Adapter.Devices.Append() ;

    // Setup name
    switch(Type)
    {
        case D3DDEVTYPE_HAL:    x_strcpy(Device.Name, "HAL") ;  break ;
        case D3DDEVTYPE_REF:    x_strcpy(Device.Name, "REF") ;  break ;
        default:                x_strcpy(Device.Name, "ERROR") ; return ;
    }

    // Setup type and caps
    Device.Type = Type ;
    Device.Caps = Caps ;

    // Loop through windowed/full screen mode
    for (xbool cWindowed = FALSE ; cWindowed <= TRUE ; cWindowed++)
    {
        // Loop through all modes for this adapter
        for (UINT cMode = 0 ; cMode < s.pD3D->GetAdapterModeCount(Adapter.D3DAdapter) ; cMode++)
            AddMode(Adapter, Device, cMode, cWindowed) ;
    }
}

//=========================================================================

// Adds adapter to list
static
void AddAdapter (       UINT                    cAdapter, 
                  const D3DADAPTER_IDENTIFIER8& ID,
                  const D3DDISPLAYMODE&         d3ddmDesktop )
{
    D3DCAPS8    DeviceCaps ;
    D3DDEVTYPE  DeviceType ;

    // Create and setup adapter
    d3d_adapter& Adapter = s.Adapters.Append() ;
    Adapter.d3ddmDesktop = d3ddmDesktop ;
    memcpy(Adapter.Name, ID.Description, sizeof(Adapter.Name)) ;
    Adapter.D3DAdapter = cAdapter ;

    // Loop through all device types (support D3DDEVTYPE_HAL and D3DDEVTYPE_REF)
    for (s32 cDevice = 0 ; cDevice < 2 ; cDevice++)
    {
        // Setup device type
        if (cDevice == 0)
            DeviceType = D3DDEVTYPE_HAL ;
        else
            DeviceType = D3DDEVTYPE_REF ;

        // Get device caps and add device to list
        if (s.pD3D->GetDeviceCaps(cAdapter, DeviceType, &DeviceCaps) == D3D_OK)
            AddDevice( Adapter, DeviceType, DeviceCaps ) ; 
    }
}

//=========================================================================

static 
void BuildModeList( void )
{
    D3DADAPTER_IDENTIFIER8  AdapterID ;
    D3DDISPLAYMODE          d3ddmDesktop ;

    // Clear all adapters
    s.Adapters.Clear() ;

    // Loop through all adapters on the system
    for (UINT cAdapter = 0 ; cAdapter < s.pD3D->GetAdapterCount() ; cAdapter++)
    {
        // Lookup D3D info
        if (s.pD3D->GetAdapterIdentifier(cAdapter, D3DENUM_NO_WHQL_LEVEL, &AdapterID) == D3D_OK)
        {
            // Setup desktop display mode
            if (s.pD3D->GetAdapterDisplayMode(cAdapter, &d3ddmDesktop) == D3D_OK)
                AddAdapter (cAdapter, AdapterID, d3ddmDesktop) ;
        }
    }

    // Print all available modes
    x_DebugMsg("Available video modes:\n") ;
    for (s32 i = 0 ; i < d3deng_GetAdapterCount() ; i++)
    {
        for (s32 j = 0 ; j < d3deng_GetDeviceCount(i) ; j++)
        {
            for (s32 k = 0 ; k < d3deng_GetModeCount(i,j) ; k++)
            {
                x_DebugMsg("    Adapter:%s Device:%s Mode:%s\n", 
                           d3deng_GetAdapterName(i),
                           d3deng_GetDeviceName(i,j),
                           d3deng_GetModeName(i,j,k)) ;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////

//=========================================================================
static
void ConvertComandLine( s32* pargc, char* argv[], LPSTR lpCmdLine )
{
    s32 argc = 1;

    if( *lpCmdLine )
    {
        argv[1] = lpCmdLine;
        argc = 2;

        do
        {
            if( *lpCmdLine == ' ' )
            {
                do
                {
                    *lpCmdLine = 0;
                    lpCmdLine++;

                } while( *lpCmdLine == ' ' );

                if( *lpCmdLine == 0 ) break;
                argv[argc++] = lpCmdLine;
            }

            lpCmdLine++;

        } while( *lpCmdLine );
    }

    *pargc = argc;
}

//=========================================================================

static
void SetupDefaultD3DSettings ( void )
{                     
    // Do not reference any shaders!
    g_pd3dDevice->SetVertexShader( D3DFVF_DIFFUSE|D3DFVF_XYZ|D3DFVF_TEX1 ) ;
    g_pd3dDevice->SetPixelShader(NULL) ;

    // Set dither in case the use selects 16bits
    g_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE );

    // Make sure that we are dealing with the Z buffer
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

    // Set some color conbiner functions
    g_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );

    // Right handed mode
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );

    // We can do alpha from the begining
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    // Turn off the stupid specular lighting
    g_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, TRUE );

    // Set bilinear mode
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );

    // Turn on the trilinear blending
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );

    // Set the Initial wraping behavier
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );

    // Make sure that we turng off the lighting as the default
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    // Set the ambient light
    xcolor Color = 0xfff8f8f8 ; 
    D3DMATERIAL8 mtrl;

    //
    // What we do here is to create a material that will be use for 
    // all the render objects. why we need to do this? We need to do 
    // this to describe to D3D how we want the light to reflected 
    // from our objects. Here you can fin more info about materials. 
    // We set the color base of the material in this case just white. 
    // Then we set the contribution for the ambient lighting, in this 
    // case 0.3f. 
    //
    ZeroMemory( &mtrl, sizeof(mtrl) );

    // Color of the material
    mtrl.Diffuse.r  = mtrl.Diffuse.g = mtrl.Diffuse.b   = 1.0f; 
    mtrl.Specular.r = mtrl.Specular.g = mtrl.Specular.b = 0.5f;
    mtrl.Power      = 50;


    // ambient light
    mtrl.Ambient.r   = (Color>>16)*(1/255.0f); 
    mtrl.Ambient.g   = (Color>> 8)*(1/255.0f);
    mtrl.Ambient.b   = (Color>> 0)*(1/255.0f);
    mtrl.Ambient.a   = (Color>>24)*(1/255.0f);

    //
    // Finally we activate the material
    //
    g_pd3dDevice->SetMaterial( &mtrl );

    //
    // This function will set the ambient color. In this case white.
    // R=G=B=A=255. which is like saying 0xffffffff. Because the color
    // is describe in 32bits. One each of the bytes in those 32bits
    // describe a color component. You can also use a macro provided 
    // by d3d to build the color.
    //
    g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, Color );
}

//=========================================================================

static
void CreateFont(void)
{
    dxerr   Error;

    //
    // load the font
    //
    HFONT Font;

    /*Font = CreateFont(
        -ENG_FONT_SIZEY, 0, 0, 0, 
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DRAFT_QUALITY,
        FIXED_PITCH,
        NULL );//system" ); 
	*/
	Font = CreateFont(
        ENG_FONT_SIZEY, 0, 0, 0, 
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DRAFT_QUALITY,
        VARIABLE_PITCH,
        NULL );
    Error = D3DXCreateFont( g_pd3dDevice, Font, &s.pFont );
    ASSERT( Error == 0 );
}

//=========================================================================

static
void DestroyFont(void)
{
    // Destroy the font
    if( s.pFont )
    {
        s.pFont->Release();
        s.pFont = NULL ;
    }
}

//=============================================================================

// Kills vertex and pixel shaders
static
void eng_KillShaders( void )
{
    s32 i  ;
    dxerr   HR ;

	/* Not needed for dedicated server */
	return;
    // Do not reference any shaders!
    g_pd3dDevice->SetVertexShader( D3DFVF_DIFFUSE|D3DFVF_XYZ|D3DFVF_TEX1 ) ;
    g_pd3dDevice->SetPixelShader(NULL) ;

    // Kill vertex shaders
    for (i = 0 ; i < s.NVertexShaders ; i++)
    {
        // Shader created?
        if (s.VertexShaders[i].Handle != -1)
        {
            HR = g_pd3dDevice->DeleteVertexShader(s.VertexShaders[i].Handle) ;
            ASSERT(HR == D3D_OK) ;
        }
    }

    // Kill pixel shaders
    for (i = 0 ; i < s.NPixelShaders ; i++)
    {
        // Shader created?
        if (s.PixelShaders[i].Handle != -1)
        {
            HR = g_pd3dDevice->DeletePixelShader(s.PixelShaders[i].Handle) ;
            ASSERT(HR == D3D_OK) ;
        }
    }
}

//=============================================================================

// Restores shaders after losing device
static
void eng_RestoreShaders( void )
{
    s32     i  ;

    // Create vertex shaders
    for (i = 0 ; i < s.NVertexShaders ; i++)
    {
        vertex_shader& VShader = s.VertexShaders[i] ;

        // Try create the vertex shader
        VShader.Handle = -1 ;
        g_pd3dDevice->CreateVertexShader(VShader.pDecl, VShader.pFunc, &VShader.Handle, VShader.Usage) ;
    }

    // Create pixel shaders
    for (i = 0 ; i < s.NPixelShaders ; i++)
    {
        pixel_shader& PShader = s.PixelShaders[i] ;

        // Try create the pixel shader
        PShader.Handle = -1 ;
        g_pd3dDevice->CreatePixelShader(PShader.pFunc, &PShader.Handle) ;
    }
}

//=============================================================================

static
void SetMode ( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed )
{
    // Keep old res info for callback
    s32 OldResX     = s.VideoWidth ;
    s32 OldResY     = s.VideoHeight ;
    s32 OldBitDepth = s.VideoBitDepth ;

    // Find the best full screen match
    s.VideoMode = d3deng_FindBestMode( Width, Height, BitDepth, bWindowed) ;
    ASSERT(s.VideoMode != -1) ;

    // Get mode info
    d3d_mode& Mode  = s.Adapters[s.VideoAdapter].Devices[s.VideoDevice].Modes[s.VideoMode] ;
    s.VideoWindowed = Mode.bWindowed ;
    s.VideoWidth    = Mode.DisplayMode.Width ;
    s.VideoHeight   = Mode.DisplayMode.Height ;
    s.VideoBitDepth = Mode.BitDepth ;

    // Call the engine change res callback?
    if (s.ChangeResCallback)
    {
        // Did the video format change?
        if ((OldResX != s.VideoWidth) || (OldResY != s.VideoHeight) || (OldBitDepth != s.VideoBitDepth))
            s.ChangeResCallback(OldResX, OldResY, OldBitDepth, s.VideoWidth, s.VideoHeight, s.VideoBitDepth) ;
    }

    // Setup present parameters
    ZeroMemory( &s.d3dpp, sizeof(s.d3dpp) );

    // Windowed mode?
    if (s.VideoWindowed)
    {
        // fill the struct
        s.d3dpp.Windowed                  = TRUE;
        s.d3dpp.SwapEffect                = D3DSWAPEFFECT_COPY;
        s.d3dpp.BackBufferFormat          = s.Adapters[s.VideoAdapter].d3ddmDesktop.Format;
        s.d3dpp.BackBufferCount           = 1;
        s.d3dpp.BackBufferWidth  = s.VideoWidth ;
        s.d3dpp.BackBufferHeight = s.VideoHeight ;
    }
    else
    {
        // Full screen mode
        s.d3dpp.Windowed                  = FALSE;
        s.d3dpp.BackBufferWidth           = s.VideoWidth;
        s.d3dpp.BackBufferHeight          = s.VideoHeight;
        s.d3dpp.BackBufferFormat          = s.Adapters[s.VideoAdapter].Devices[s.VideoDevice].Modes[s.VideoMode].DisplayMode.Format ;
        s.d3dpp.BackBufferCount           = 1;
        s.d3dpp.SwapEffect                = D3DSWAPEFFECT_COPY_VSYNC;

        s.d3dpp.FullScreen_RefreshRateInHz      = D3DPRESENT_RATE_DEFAULT ; //D3DPRESENT_RATE_UNLIMITED ;
        s.d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE ;
    }

    // Turn off stencil?
    if( (s.Mode & ENG_ACT_STENCILOFF) == 0 )
    {
        ASSERT( (s.Mode&ENG_ACT_16_BPP) == 0 );
        s.d3dpp.EnableAutoDepthStencil    = TRUE;
        s.d3dpp.AutoDepthStencilFormat    = D3DFMT_D24S8;
    }                

    // Need a back buffer that can be locked (for screen shots)
    if( s.Mode & ENG_ACT_BACKBUFFER_LOCK )
        s.d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    // Must setup these for viewport
    s.rcWindowRect.left   = 0 ;
    s.rcWindowRect.right  = s.VideoWidth ;
    s.rcWindowRect.top    = 0 ;
    s.rcWindowRect.bottom = s.VideoHeight ;
}

//=============================================================================

static
void ShowWindow ( void )
{
    // Setup correct position

    // Windowed mode?
    if (s.VideoWindowed)
    {
        // Set windowed-mode style
        SetWindowLong( s.hWnd, GWL_STYLE, WINDOW_STYLE);

        // Setup the actual window size so that the requested client size is inside of it
        RECT WindowRect = s.rcWindowRect ;
        ::AdjustWindowRect(&WindowRect, WINDOW_STYLE, FALSE) ;

        // Center on desktop
        s32 Width  = WindowRect.right  - WindowRect.left ;
        s32 Height = WindowRect.bottom - WindowRect.top ;
        s32 X      = (s.Adapters[s.VideoAdapter].d3ddmDesktop.Width/2)  - (Width/2) ;
        s32 Y      = (s.Adapters[s.VideoAdapter].d3ddmDesktop.Height/2) - (Height/2) ;

        // Setup as none top most window and setup size
        SetWindowPos(s.hWnd,            // handle to window
                     HWND_NOTOPMOST,    // placement-order handle
                     X,                 // horizontal position
                     Y,                 // vertical position
                     Width,             // width
                     Height,            // height
                     SWP_NOCOPYBITS|SWP_NOREPOSITION) ; // window-positioning options
    }
    else
    {
        // Set fullscreen-mode style
        SetWindowLong( s.hWnd, GWL_STYLE, FULL_SCREEN_STYLE ) ;

        // Setup to full screen size
        SetWindowPos(s.hWnd,            // handle to window
                     HWND_TOPMOST,      // placement-order handle
                     0,                 // horizontal position
                     0,                 // vertical position
                     s.VideoWidth,      // width
                     s.VideoHeight,     // height
                     SWP_NOCOPYBITS|SWP_NOREPOSITION) ; // window-positioning options

    }

    // Update all windows otherwise you'll get a black rectangle in the top left hand
    // corner of the screen when switching from full screen to windowed mode
    InvalidateRect(NULL, NULL, TRUE) ;

    // Finally, show the window
    ShowWindow( s.hWnd, SW_SHOWNORMAL );
    UpdateWindow( s.hWnd );
    SetActiveWindow ( s.hWnd );
}

//=============================================================================

void RestoreScreen( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed )
{
    dxerr HR ;

    // Stop rendering
    s.bReady = FALSE ;

    // Free resources
    DestroyFont() ;
    eng_KillShaders() ;
    draw_Kill() ;

    // Switch to best mode
    SetMode(Width, Height, BitDepth, bWindowed) ;

    // Try get back the device using the last present parameters
    HR = g_pd3dDevice->Reset(&s.d3dpp) ; 
    ASSERT(HR == D3D_OK) ;

    // Re-create resources
    CreateFont() ;
    eng_RestoreShaders() ;
    draw_Init() ;

    // Setup default settings
    SetupDefaultD3DSettings() ;

    // Clear the screen
    HR = g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, s.BackColor, 1.0f, 0L ); 
    ASSERT(HR == D3D_OK) ;

    // Finally, show the window
    ShowWindow() ;

    // All is good again
    s.bReady = TRUE ;
}

//=============================================================================

static
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_ACTIVATE:
            {
                s.bActive = LOWORD(wParam) != WA_INACTIVE;
                if( s.bActive )
                {
                    d3deng_ComputeMousePos();
                    d3deng_ComputeMousePos();
                }
            }
            break;

        case WM_SIZE:
            // Check to see if we are losing or gaining our window. Set the
            // active flag to match.
            if( SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam )
                s.bActive = FALSE;
            else s.bActive = TRUE;

#if 0
            // A new window size will require a new backbuffer size. The
            // easiest way to achieve this is to release and re-create
            // everything. Note: if the window gets too big, we may run out
            // of video memory and need to exit. This simple app exits
            // without displaying error messages, but a real app would behave
            // itself much better.
            if( s.bActive && s.bReady )
            {
                //
                // Get the new size of the window
                //
                GetClientRect(  hWnd, &s.rcWindowRect );

                //
                // Resize the view port
                //
                DWORD dwRenderWidth  = s.rcWindowRect.right - s.rcWindowRect.left;
                DWORD dwRenderHeight = s.rcWindowRect.bottom - s.rcWindowRect.top;

                //dwRenderWidth  = MIN( (DWORD)s.MaxXRes, dwRenderWidth );
                //dwRenderHeight = MIN( (DWORD)s.MaxYRes, dwRenderHeight );

                D3DVIEWPORT8 vp = { 0, 0, dwRenderWidth, dwRenderHeight, 0.0f, 1.0f };
                g_pd3dDevice->SetViewport( &vp );

                s.bReady = TRUE;
            }
#endif
            break;

        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 100;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 100;
            break;

        case WM_SETCURSOR:
            // We turn the cursor off for fullscreen modes
            if( s.bActive && s.bReady && (!s.VideoWindowed) )
            {
                SetCursor(NULL);
                return TRUE;
            }
            break;

        case WM_SYSKEYUP:
            // Toggle full screen/windowed
            if( wParam == VK_RETURN )
                RestoreScreen(s.VideoWidth, s.VideoHeight, s.VideoBitDepth, s.VideoWindowed ^ TRUE) ;
            break;

        case WM_CLOSE:
            // Make sure we are in window mode
            if (s.VideoWindowed == FALSE)
                RestoreScreen(s.VideoWidth, s.VideoHeight, s.VideoBitDepth, TRUE) ;

            DestroyWindow( hWnd );
            return 0;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0L;

        case WM_MOUSEMOVE:
            s.MouseX = LOWORD(lParam);  // horizontal position of cursor 
            s.MouseY = HIWORD(lParam);  // vertical position of cursor 
            return 0;
         case WM_LBUTTONDOWN:
             s.bMouseLeftButton = true;
            return 0;
         case WM_MBUTTONDOWN:
             s.bMouseMiddleButton = true;
            return 0;
         case WM_RBUTTONDOWN:
             s.bMouseRightButton = true;
            return 0;
         case WM_LBUTTONUP:
             s.bMouseLeftButton = false;
            return 0;
         case WM_MBUTTONUP:
             s.bMouseMiddleButton = false;
            return 0;
         case WM_RBUTTONUP:
             s.bMouseRightButton = false;
            return 0;
         case WM_MOUSEWHEEL:
            {
                 f32 Wheel = (f32)((s16)(wParam >> 16)) / 120.0f ;
                 s.MouseWheelAbs += Wheel ;
                 s.MouseWheelRel = Wheel ;
            }
            return 0;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

//=========================================================================
static
HWND CreateWin( s32 Width, s32 Height )
{
    // Register the window class
    WNDCLASS wndClass = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, s.hInst,
                          NULL,
                          LoadCursor(NULL, IDC_ARROW), 
                          (HBRUSH)GetStockObject(WHITE_BRUSH), NULL,
                          TEXT("Render Window") };

    RegisterClass( &wndClass );

    // Create our main window
    if (s.VideoWindowed)
    {
        // Setup the actual window size so that the requested client size is inside of it
        RECT WindowRect ;
        WindowRect.left   = 0 ;
        WindowRect.top    = 0 ;
        WindowRect.right  = Width ;
        WindowRect.bottom = Height ;
        ::AdjustWindowRect(&WindowRect, WINDOW_STYLE, FALSE) ;
        Width  = WindowRect.right  - WindowRect.left ;
        Height = WindowRect.bottom - WindowRect.top ;

        // Position window in center of screen
        s32 X = (GetSystemMetrics(SM_CXSCREEN) - Width) >> 1 ;
        s32 Y = (GetSystemMetrics(SM_CYSCREEN) - Height) >> 1 ;

        // Create normal window
        return CreateWindow( TEXT("Render Window"),
                             TEXT("Inevitable PC Engine"),
                             WINDOW_STYLE, 
                             X,Y, Width, Height, 
                             0L, 0L, s.hInst, 0L );
    }
    else
    {
        // Create top level window
        return CreateWindow( TEXT("Render Window"),
                             TEXT("Inevitable PC Engine"),
                             FULL_SCREEN_STYLE, CW_USEDEFAULT,
                             CW_USEDEFAULT, Width, Height, 0L, 0L, s.hInst, 0L );
    }
}

//=========================================================================

void d3deng_EntryPoint( s32& argc, char**& argv, HINSTANCE h1, HINSTANCE h2, LPSTR str1, INT i1 )
{
    static char* ArgvBuff[256] = { NULL };
    argv = ArgvBuff;
    ConvertComandLine( &argc, argv, str1 );
    s.hInst = h1;

    x_Init();

#if 1
    cdfs_Init( "FILES.DAT" );
#endif

    s.FPSFrameTime[0] = 0;
    s.FPSFrameTime[1] = 0;
    s.FPSFrameTime[2] = 0;
    s.FPSFrameTime[3] = 0;
    s.FPSFrameTime[4] = 0;
    s.FPSFrameTime[5] = 0;
    s.FPSFrameTime[6] = 0;
    s.FPSFrameTime[7] = 0;

    s.FPSLastTime = x_GetTime();
}

//=========================================================================

void eng_Kill( void )
{
    // Free all adapters
    s.Adapters.Clear() ;

    // Destroy shaders
    eng_KillShaders() ;

    //
    // Free sub modules
    //
    //draw_Kill();
    //vram_Kill();
    smem_Kill();
//    text_Kill();

    //
    // Stop the d3d engine
    //
    if( g_pd3dDevice != NULL) 
        g_pd3dDevice->Release();

    if( s.pD3D != NULL)
        s.pD3D->Release();

    DestroyFont() ;

    UnregisterClass( "Render Window", s.hInst );
}

//=========================================================================

s32 d3deng_ExitPoint( void )
{
//  eng_Kill();

#if 0
    cdfs_Kill();
#endif

    x_Kill();
    return 0;
}

//=========================================================================

void d3deng_SetPresets( u32 Mode )
{
    s.Mode = Mode;
}

//=============================================================================
static
void UpdateFPS( void )
{
    xtick CurrentTime;

    CurrentTime                     = x_GetTime();
    s.FPSFrameTime[ s.FPSIndex ]    = CurrentTime - s.FPSLastTime;
    s.FPSLastTime                   = CurrentTime;
    s.FPSIndex                     += 1;
    s.FPSIndex                     &= 0x07;
}

//=============================================================================

f32 eng_GetFPS( void )
{
    xtick Sum = s.FPSFrameTime[0] +
                s.FPSFrameTime[1] +
                s.FPSFrameTime[2] +
                s.FPSFrameTime[3] +
                s.FPSFrameTime[4] +
                s.FPSFrameTime[5] +
                s.FPSFrameTime[6] +
                s.FPSFrameTime[7];

    return( (f32)(s32)(((8.0f / x_TicksToMs( Sum )) * 1000.0f) + 0.5f) );
}       

//=========================================================================

static
void InitializeD3D(void)
{
    dxerr Error;

    // TEMP
    //s.Mode |= ENG_ACT_FULLSCREEN ;
    //s.Mode |= ENG_ACT_SOFTWARE ;
    s.VideoWindowed = (s.Mode & ENG_ACT_FULLSCREEN) != ENG_ACT_FULLSCREEN;

    // Keep main thread is for restoring d3d device
    s.dwMainThreadID = ::GetCurrentThreadId() ;
    
    //
    // Create the D3D object, which is needed to create the D3DDevice.
    //
    s.pD3D = Direct3DCreate8( D3D_SDK_VERSION );
    ASSERT( s.pD3D );

    // Bulid available modes
    BuildModeList() ;

    // Setup default wideo
    s.VideoAdapter  = 0 ;
    s.VideoDevice   = 0 ;
    s.VideoMode     = 0 ;
    s.VideoWidth    = 640 ;
    s.VideoHeight   = 480 ;
    s.VideoBitDepth = ( s.Mode & ENG_ACT_16_BPP ) ? 16 : 32 ;
    s.VideoWindowed = TRUE ;
    SetMode(s.VideoWidth, s.VideoHeight, s.VideoBitDepth, s.VideoWindowed) ;

    // Initialize the window
    //
    s.hWnd = CreateWin( s.VideoWidth, s.VideoHeight ) ;

    // Put mouse in middle of window
    RECT    Rect ;
    GetWindowRect( s.hWnd, &Rect );
    s32 CenterX = (Rect.right + Rect.left) >> 1 ;
    s32 CenterY = (Rect.bottom + Rect.top) >> 1 ;
    s.MouseLastAbsPos.x = CenterX ;
    s.MouseLastAbsPos.y = CenterY ;
	SetCursorPos( CenterX, CenterY ) ;

    // Create the device
    Error = s.pD3D->CreateDevice(   
        D3DADAPTER_DEFAULT,    
        (s.Mode & ENG_ACT_SOFTWARE) ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL, 
        s.hWnd,
        //((s.Mode & ENG_ACT_SHADERS_IN_SOFTWARE) || (s.Mode & ENG_ACT_SOFTWARE) ) ?
        //     D3DCREATE_SOFTWARE_VERTEXPROCESSING : D3DCREATE_HARDWARE_VERTEXPROCESSING,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &s.d3dpp, 
        &g_pd3dDevice );
	if(Error != 0)
	{
		MessageBox(d3deng_GetWindowHandle(), xfs("Error creating device: %d", Error), "Device Error", MB_OK);
	}
    ASSERT( Error == 0 );

    //
    // If we are doing a window mode set the viewport to show 
    // only the portion that we are interested
    //
    if( s.VideoWindowed )
    {
        D3DVIEWPORT8 vp = { 0, 0, s.VideoWidth, s.VideoHeight, 0.0f, 1.0f };
        g_pd3dDevice->SetViewport( &vp );         
        //RECT Window;
        //GetWindowRect   ( s.hWnd, &Window );
        //GetClientRect   ( s.hWnd, &s.rcWindowRect         );
        //s32 W = (Window.right   - Window.left)*2 - (s.rcWindowRect.right  - s.rcWindowRect.left) ;
        //s32 H = (Window.bottom  - Window.top)*2  - (s.rcWindowRect.bottom - s.rcWindowRect.top ) ;
        //MoveWindow      ( s.hWnd,  0, 0, W, H, true );
        //GetClientRect   ( s.hWnd, &s.rcWindowRect );
    }
    else
    {
        // Must setup these for viewport
        s.rcWindowRect.left   = 0 ;
        s.rcWindowRect.right  = s.VideoWidth ;
        s.rcWindowRect.top    = 0 ;
        s.rcWindowRect.bottom = s.VideoHeight ;
    }

    // Setup default settings
    SetupDefaultD3DSettings() ;

    // Finally, show the window
    ShowWindow() ;
}

//=========================================================================

void text_BeginRender( void )
{
    //
    // Make sure that the direct 3d has started
    //
    if( s.bD3DBeginScene == FALSE )
    {
        eng_Begin();
        eng_End();
    }
}

//=========================================================================

void text_RenderStr( char* pStr, s32 NChars, xcolor Color, s32 PixelX, s32 PixelY )
{
	dxerr                   Error;
    RECT                    Rect;

    Rect.left   = PixelX;
    Rect.top    = PixelY;
    Rect.bottom = PixelY + ENG_FONT_SIZEY;
    Rect.right  = Rect.left + (NChars*ENG_FONT_SIZEX);
                    
    rst.Start();
	//rstct+=NChars;
    Error = s.pFont->DrawTextA( pStr, NChars, &Rect, DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP | DT_END_ELLIPSIS | DT_CALCRECT, Color );//s.TextColor );
	Error = s.pFont->DrawTextA( pStr, NChars, &Rect, DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP | DT_END_ELLIPSIS, Color );//s.TextColor );
	if(Error != D3D_OK) rstct = Error;
	rst.Stop();
	
}

//=========================================================================

void text_EndRender( void )
{
}

//=========================================================================

void ENG_TextColor( const xcolor& Color )
{
    s.TextColor = Color;
}

//=============================================================================

void ENG_SetLight( s32 LightID, const vector3& Dir, const xcolor& Color )
{
    D3DLIGHT8 Light;

    //
    // Here we fill up the structure for D3D. The first thing we say 
    // is the type of light that we want. In this case DIRECTIONAL. 
    // Which basically means that it doesn't have an origin. The 
    // second thing that we fill is the diffuse lighting. Basically 
    // is the color of the light. Finally we fill up the Direction. 
    // Note how we negate to compensate for the D3D way of thinking.
    //

    ZeroMemory( &Light, sizeof(Light) );   // Clear the hold structure
    Light.Type          = D3DLIGHT_DIRECTIONAL;

    Light.Diffuse.r   = (Color>>16)*(1/255.0f); 
    Light.Diffuse.g   = (Color>> 8)*(1/255.0f);
    Light.Diffuse.b   = (Color>> 0)*(1/255.0f);
    Light.Diffuse.a   = (Color>>24)*(1/255.0f);

    Light.Specular    = Light.Diffuse;

    Light.Direction.x = -Dir.X;   // Set the direction of
    Light.Direction.y = -Dir.Y;   // the light and compensate
    Light.Direction.z = -Dir.Z;   // for DX way of thinking.

    //
    // Here we set the light number zero to be the light which we just
    // describe. What is light 0? Light 0 is one of the register that 
    // D3D have for lighting. You can overwrite registers at any time. 
    // Only lights that are set in registers are use in the rendering 
    // of the scene.
    //
    g_pd3dDevice->SetLight( LightID, &Light );

    //
    // Here we enable out register 0. That way what ever we render 
    // from now on it will use register 0. The other registers are by 
    // default turn off.
    //
    g_pd3dDevice->LightEnable( 0, TRUE );
}

//=========================================================================

void eng_Init( void )
{
    //
    // Set the scrach memory
    //
    smem_Init( SCRACH_MEM_SIZE );

    //
    // Initialize dinput
    //
    VERIFY( d3deng_InitInput( s.hWnd ) == TRUE );

    /* DEDICATED_SERVER

    // Setup the D3D device
    InitializeD3D() ;

    //
    // Init text
    //
    text_Init();
    text_SetParams( s.VideoWidth, s.VideoHeight, 0, 0,
                    ENG_FONT_SIZEX, ENG_FONT_SIZEY,
                    8 );
    text_ClearBuffers();


    //
    // load the font
    CreateFont() ;

    ENG_TextColor( 0xffffffff ); // xcolor(255,255,125,255)

    //
    // Initialize dinput
    //
    VERIFY( d3deng_InitInput( s.hWnd ) == TRUE );

    //
    // Initialize vram
    //
    vram_Init();

    //
    // Initialize draw
    //
    draw_Init();

    DEDICATED_SERVER */

    //
    // Indicate teh the engine is ready
    //
    s.bReady = TRUE;
}

//=============================================================================

xbool eng_Begin( const char* pTaskName )
{
    ASSERT( s.bBeginScene == FALSE );

    if( s.bD3DBeginScene == FALSE )
    {
        // Mark the d3dbeginscene as active
        //
        s.bD3DBeginScene = TRUE;

        //if( FAILED( g_pd3dDevice->TestCooperativeLevel() ) )
            //return TRUE ;

        // Check the cooperative level before rendering
        //if( FAILED( g_pd3dDevice->TestCooperativeLevel() ) )
            //return FALSE;

        //if( FAILED( g_pd3dDevice->BeginScene() ) ) 
            //return FALSE;

        // Alway call begin scene
        g_pd3dDevice->BeginScene() ;

        // Do not reference any shaders!
        g_pd3dDevice->SetVertexShader( D3DFVF_DIFFUSE|D3DFVF_XYZ|D3DFVF_TEX1 ) ;
        g_pd3dDevice->SetPixelShader(NULL) ;

        //
    }

    s.bBeginScene = TRUE;

    // Clear draw's L2W
    draw_ClearL2W();

    return TRUE;
}

//=============================================================================

xbool eng_InBeginEnd( void )
{
    return s.bBeginScene;
}

//=============================================================================

void eng_End( void )
{
    ASSERT( s.bBeginScene );
    s.bBeginScene = FALSE;
}

//=============================================================================

void eng_PrintStats( void )
{
    x_DebugMsg("CPU:%1.1f  Pageflip:%1.1f  FPS:%1.1f\n",s.CPUMS,s.IMS,1000.0f/(s.CPUMS+s.IMS));
}

//=============================================================================

void eng_PageFlip()
{
    dxerr   HR ;

    // TEMP
    //HR = g_pd3dDevice->TestCooperativeLevel() ;
    //if (HR != D3D_OK)
    //{
        //while(1) ;
    //}

    xtimer ARHTimer;
	ARHTimer.Reset();
    s.CPUTIMER.Stop();
    s.CPUMS = s.CPUTIMER.ReadMs();
    xtimer InternalTime;
    InternalTime.Start();

    const xbool bPrintFPS = FALSE;

    //
    // If the user have to call eng_beging scene force one call
    //
    if( s.bD3DBeginScene == FALSE )
    {
        eng_Begin();
        eng_End();
    }

    //
    // Handle all the buffered text
    //
	rst.Reset();
	rstct=0;
	ARHTimer.Start();
    text_Render();
	ARHTimer.Stop();
	text_ClearBuffers();
	//x_printfxy(0, 43, "A1: %7.3f", ARHTimer.ReadMs());
	//x_printfxy(0, 44, "rst: %7.3f %d", rst.ReadMs(),rstct);
    //
    // Handle the FPS
    //
    if( bPrintFPS ) 
        x_printfxy( 0,0, "FPS: %4.2f", eng_GetFPS() );

    //
    // End the scene. and page flip
    //
    if (g_pd3dDevice->TestCooperativeLevel() == D3D_OK)
    {
        ASSERT( s.bD3DBeginScene );
        g_pd3dDevice->EndScene();

        if( s.VideoWindowed )
        {
            RECT SrcRect;

            SrcRect.top    = 0;
            SrcRect.left   = 0;
            SrcRect.right  = s.rcWindowRect.right  - s.rcWindowRect.left;
            SrcRect.bottom = s.rcWindowRect.bottom - s.rcWindowRect.top;

            //SrcRect.right  = MIN( s.MaxXRes, SrcRect.right  );
            //SrcRect.bottom = MIN( s.MaxYRes, SrcRect.bottom );

            HR = g_pd3dDevice->Present( &SrcRect, NULL, NULL, NULL );
            // THIS FIRES OFF WHEN QUITING!
            //ASSERT(HR != D3DERR_INVALIDCALL) ;
        }
        else        
        {
            HR = g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
            ASSERT(HR != D3DERR_INVALIDCALL) ;
        }
    }
        s.bD3DBeginScene = FALSE;
    
    // Can only check for restoring on the same thread that created the d3d device
    if (::GetCurrentThreadId() == s.dwMainThreadID)
    {
        ASSERT(g_pd3dDevice) ;
        do
        {
            // Get device status
            HR = g_pd3dDevice->TestCooperativeLevel() ;

            // Is device lost (eg. alt+tab pressed) and can be restored?
            if (HR != D3D_OK)
            {
                // Flag not ready
                s.bReady = FALSE ;

                // We must process messages otherwise the device never gets to the DEVICENORESET state
                MSG  msg;
                while(PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }

                // Restore screen as a window when we can reset
                if (HR == D3DERR_DEVICENOTRESET)
                {
                    // If we are in windowed mode, then the user has changed the screen
                    // settings, so read them again
                    if (s.VideoWindowed)
                    {
                        HR = s.pD3D->GetAdapterDisplayMode( s.Adapters[s.VideoAdapter].D3DAdapter,
                                                            &s.Adapters[s.VideoAdapter].d3ddmDesktop ) ;
                        ASSERT( HR == D3D_OK );
                    }

                    // Rebuild the mode list
                    BuildModeList() ;

                    // Restore everything in windowed mode
                    RestoreScreen(s.VideoWidth, s.VideoHeight, s.VideoBitDepth, TRUE) ;
                }
            }
        }
        while(HR != D3D_OK) ;   // Wait until we restore the device
    }

    //
    // Taggle the scrach memory
    //
    smem_Toggle();

    //
    // Clear all the active views
    //
    //s.ViewMask     = 0;
    //s.nActiveViews = 0;

    //
    // Clear the backgournd
    //
    dxerr Error;
    s32 Width  = s.rcWindowRect.right-s.rcWindowRect.left ;
    s32 Height = s.rcWindowRect.bottom-s.rcWindowRect.top ;
    D3DVIEWPORT8 vp = { 0, 0, Width, Height, 0.0f, 1.0f };
    Error = g_pd3dDevice->SetViewport( &vp );
    ASSERT( Error == 0 );
   
    Error = g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, s.BackColor, 1.0f, 0L ); 
    ASSERT( Error == 0 );

    //
    // Update fps
    //
    UpdateFPS();

    InternalTime.Stop();
    s.IMS = InternalTime.ReadMs();
    s.CPUTIMER.Reset();
    s.CPUTIMER.Start();
}

//=============================================================================

void eng_GetRes( s32& XRes, s32& YRes )
{
    XRes = s.VideoWidth ;
    YRes = s.VideoHeight ;
}

//=============================================================================

void eng_MaximizeViewport( view& View )
{
    s32 Width  = s.rcWindowRect.right-s.rcWindowRect.left ;
    s32 Height = s.rcWindowRect.bottom-s.rcWindowRect.top ;

    View.SetViewport( 0, 0, Width, Height );
}

//=============================================================================

void eng_SetBackColor( xcolor Color )
{
    s.BackColor = Color;
}

//=============================================================================

void eng_SetView ( const view& View, s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );
    s.View[ViewPaletteID] = View;
}

//=============================================================================

void eng_ActivateView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    // This view is already active
    if( s.ViewMask & (1<<ViewPaletteID) )
        return ;

    // Activate the view
    ASSERT( s.nActiveViews < ENG_MAX_VIEWS );

    s.ActiveView[ s.nActiveViews ] = &s.View[ ViewPaletteID ];
    s.nActiveViews++;
    s.ViewMask |= (1<<ViewPaletteID);
}

//=============================================================================

void eng_DeactivateView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    // The view is already not active
    if( (s.ViewMask & (1<<ViewPaletteID)) == 0 )
        return ;

    // Find the active view and remove it
    s32 i;
    for( i=0; i<ENG_MAX_VIEWS; i++ )
    {
        if( s.ActiveView[ i ] == &s.View[ ViewPaletteID ] )
        {
            s.ActiveView[ i ] = &s.View[ ViewPaletteID ];

            s.nActiveViews--;
            ASSERT( s.nActiveViews >= 0 );
            memmove( &s.ActiveView[ i ],  &s.ActiveView[ i+1 ], s.nActiveViews-i );            
            s.ViewMask &= ~(1<<ViewPaletteID);
            break;
        }
    }

    ASSERT( i < ENG_MAX_VIEWS );
}

//=============================================================================

u32 eng_GetActiveViewMask   ( void )
{
    return s.ViewMask;
}

//=============================================================================

const view* eng_GetView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    return &s.View[ ViewPaletteID ];
}

//=============================================================================

s32 eng_GetNActiveViews( void )
{
    return s.nActiveViews;
}

//=============================================================================

const view* eng_GetActiveView( s32 ActiveViewListID )
{
    ASSERT( ActiveViewListID >= 0  );
    ASSERT( ActiveViewListID < eng_GetNActiveViews() );

    return s.ActiveView[ ActiveViewListID ];
}

//=============================================================================

void eng_ScreenShot( const char* pFileName /*= NULL*/, s32 Size /*= 1*/ )
{
    (void)pFileName ;
    (void)Size ;
    ASSERTS( 0, "Press Print screen for now" );
}

//=============================================================================

xbool eng_ScreenShotActive( void )
{
    // Not yet implemented
    return FALSE ;
}

//=============================================================================

s32 eng_ScreenShotSize( void )
{
    // Not yet implemented
    return 1 ;
}

//=============================================================================

s32 eng_ScreenShotX( void )
{
    // Not yet implemented
    return 0 ;
}

//=============================================================================

s32 eng_ScreenShotY( void )
{
    // Not yet implemented
    return 0 ;
}

//=============================================================================

void eng_Sync ( void )
{

}

//=============================================================================

void DebugMessage( const char* FormatStr, ... )
{
    va_list Args; 
    va_start( Args, FormatStr );

    OutputDebugString( xvfs( FormatStr, Args) );

    va_end( Args );
}

//=============================================================================

void eng_SetViewport( const view& View )
{
    dxerr           Error;
    D3DVIEWPORT8    vp;
    s32             L, T, R, B;

    View.GetViewport( L, T, R, B );

    vp.X      = L;
    vp.Y      = T;
    vp.Width  = R-L-1;
    vp.Height = B-T-1;
    vp.MinZ   = 0.0f;
    vp.MaxZ   = 1.0f;

    vp.Width  = MIN( (u32)s.VideoWidth, vp.Width );
    vp.Height = MIN( (u32)s.VideoHeight, vp.Height );

    Error = g_pd3dDevice->SetViewport( &vp );
    ASSERT( Error == 0 );
}


//=========================================================================
void  d3deng_SetMouseMode( mouse_mode Mode )
{
    // Going to a new mode?
    if (s.MouseMode != Mode)
    {
        // If we are going to a absolute mode, restore the last absolute position
	    if (Mode == MOUSE_MODE_ABSOLUTE)
        {
            // Goto last valid absolute value
            SetCursorPos(s.MouseLastAbsPos.x, s.MouseLastAbsPos.y) ;
        }
        else
        {
            // Save absolute value
            GetCursorPos ( &s.MouseLastAbsPos );
        }

        // Keep new mouse
        s.MouseMode = Mode;

        // If going to a relative mode, clear the mouse
	    if(Mode != MOUSE_MODE_ABSOLUTE)
	    {
		    s.MouseX = 0 ;
		    s.MouseY = 0 ;

            RECT    Rect ;
            GetClientRect( s.hWnd, &Rect );
            ClientToScreen( s.hWnd, (POINT*)&Rect.left) ;
            ClientToScreen( s.hWnd, (POINT*)&Rect.right) ;
            s32 CenterX = (Rect.right + Rect.left) >> 1;
            s32 CenterY = (Rect.bottom + Rect.top) >> 1;
		    SetCursorPos( CenterX, CenterY );
	    }
    }
}

//=========================================================================

void d3deng_ComputeMousePos( void )
{
    xbool CheckMouse = FALSE;
    static xbool WasActive = FALSE;
    xbool  LastTimeActive;
    s.MouseX = 0;
    s.MouseY = 0;

    LastTimeActive = WasActive;
    WasActive      = FALSE;

    if( s.bActive == FALSE )
        return;

    if( s.MouseMode == MOUSE_MODE_NEVER )
        return;

    if( s.MouseMode == MOUSE_MODE_BUTTONS )
    {
        if( s.bMouseLeftButton == 0 && s.bMouseRightButton == 0 && s.bMouseMiddleButton == 0 )
            return;
    }

	
    WasActive = TRUE;


    RECT    Rect ;
    POINT   MousePos;
    GetCursorPos ( &MousePos );
    
    GetClientRect( s.hWnd, &Rect );
    ClientToScreen( s.hWnd, (POINT*)&Rect.left) ;
    ClientToScreen( s.hWnd, (POINT*)&Rect.right) ;


    s32 CenterX = (Rect.right + Rect.left) >> 1;
    s32 CenterY = (Rect.bottom + Rect.top) >> 1;

	if(s.MouseMode != MOUSE_MODE_ABSOLUTE)
    {
		SetCursorPos( CenterX, CenterY );
	}

    // Show mouse if it's outside the client area, otherwise hide it
    //ShowCursor(!PtInRect(&Rect, MousePos)) ;

    if( LastTimeActive == FALSE )        
        return;
	if(s.MouseMode != MOUSE_MODE_ABSOLUTE)
	{
		s.MouseX = (f32)(MousePos.x - CenterX);
		s.MouseY = (f32)(MousePos.y - CenterY);
	}
	else
	{
		s.MouseX = (f32)(MousePos.x - Rect.left);
		s.MouseY = (f32)(MousePos.y - Rect.top);
	}
}

//=========================================================================

f32 d3deng_GetMouseX( void )
{
    return s.MouseX;
}

//=========================================================================

f32 d3deng_GetMouseY( void )
{
    return s.MouseY;
}

//=============================================================================

f32 d3deng_GetMouseWheel( void )
{
    // TEMP FOR NOW...
    f32 Value = s.MouseWheelRel ;
    s.MouseWheelRel = 0 ;

    return Value ;
}

//=============================================================================

xbool d3deng_MouseGetLButton( void )
{
    return s.bMouseLeftButton;
}

//=============================================================================

xbool d3deng_MouseGetRButton( void )
{
    return s.bMouseRightButton;
}

//=============================================================================

xbool d3deng_MouseGetMButton( void )
{
    return s.bMouseMiddleButton;
}

//=============================================================================

HINSTANCE d3deng_GetInstace( void )
{
    return s.hInst;
}

//=============================================================================

xbool d3deng_IsActive( void )
{
    return s.bActive ;
}

//=============================================================================

HWND d3deng_GetWindowHandle( void )
{
    return s.hWnd;
}

//=============================================================================

// Registers and creates a vertex shader
s32  d3deng_RegisterVertexShader( const char*  Name,    // Name
                                  const DWORD* pDecl,   // Declaration data
                                  const DWORD* pFunc,   // Function data
                                        DWORD  Usage,   // Usage flag
                                        DWORD  FVF)     // Flexible vertex format flags (incase of fail)
{
    // Max reached?
    ASSERT(s.NVertexShaders < MAX_VERTEX_SHADERS) ;

    // Setup next vertex shader
    vertex_shader& VShader = s.VertexShaders[s.NVertexShaders] ;
    x_memcpy(VShader.Name, Name, sizeof(VShader.Name)) ;
    VShader.Name[sizeof(VShader.Name)-1] = 0 ;
    VShader.pDecl = pDecl ;
    VShader.pFunc = pFunc ;
    VShader.Usage = Usage ;
    VShader.FVF   = FVF ;
    VShader.Handle = -1 ;
    
    // Try create the vertex shader
    g_pd3dDevice->CreateVertexShader(VShader.pDecl, VShader.pFunc, &VShader.Handle, VShader.Usage) ;

    // Return ID and increment for next register
    return ++s.NVertexShaders ;
}

//=============================================================================

// Returns TRUE if vertex shader was created successfully
xbool d3deng_IsValidVertexShader( s32 ID )
{
    // Must be valid ID
    ASSERT(ID > 0) ;
    ASSERT(ID <= MAX_VERTEX_SHADERS) ;

    // Starts from 1
    ID-- ;

    // Handle created?
    return (s.VertexShaders[ID].Handle != -1) ;
}

//=============================================================================


// Activates vertex shader
void d3deng_ActivateVertexShader( s32 ID )
{
    dxerr HR ;

    ASSERT(ID >= 0) ;
    ASSERT(ID <= MAX_VERTEX_SHADERS) ;

    // Aready active?
    //if (s.ActiveVertexShader == ID)
        //return ;

    // Exit if device is lost, because this crashes D3D!
    if (g_pd3dDevice->TestCooperativeLevel() != D3D_OK)
        return ;

    // Keep active ID
    s.ActiveVertexShader = ID ;

    // Use default shader?
    if (ID == 0)
        g_pd3dDevice->SetVertexShader( D3DFVF_DIFFUSE|D3DFVF_XYZ|D3DFVF_TEX1 );
    else
    {
        ID-- ;
        if (s.VertexShaders[ID].Handle != -1)
        {
            HR = g_pd3dDevice->SetVertexShader(s.VertexShaders[ID].Handle) ;
            ASSERT(HR == D3D_OK) ;
        }
        else
            g_pd3dDevice->SetVertexShader( s.VertexShaders[ID].FVF ) ;

    }
}

//=============================================================================

// Registers and creates a pixel shader
s32  d3deng_RegisterPixelShader( const char*  Name,    // Name
                                 const DWORD* pFunc )  // Function data
{
    // Max reached?
    ASSERT(s.NPixelShaders < MAX_PIXEL_SHADERS) ;

    // Setup next pixel shader
    pixel_shader& PShader = s.PixelShaders[s.NPixelShaders] ;
    x_memcpy(PShader.Name, Name, sizeof(PShader.Name)) ;
    PShader.Name[sizeof(PShader.Name)-1] = 0 ;
    PShader.pFunc  = pFunc ;
    PShader.Handle = -1 ;

    // Try create the pixel shader
    g_pd3dDevice->CreatePixelShader(PShader.pFunc, &PShader.Handle) ;

    // Return ID and increment for next register
    return ++s.NPixelShaders ;
}

//=============================================================================

// Returns TRUE if pixel shader was created successfully
xbool d3deng_IsValidPixelShader( s32 ID )
{
    // Must be valid ID
    ASSERT(ID > 0) ;
    ASSERT(ID <= MAX_PIXEL_SHADERS) ;

    // Starts from 1
    ID-- ;

    // Handle created?
    return (s.PixelShaders[ID].Handle != -1) ;
}

//=============================================================================

// Activates pixel shader
void d3deng_ActivatePixelShader( s32 ID )
{
    dxerr HR ;

    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_VERTEX_SHADERS) ;

    // Aready active?
    //if (s.ActivePixelShader == ID)
        //return ;

    // Exit if device is lost, because this crashes D3D!
    if (g_pd3dDevice->TestCooperativeLevel() != D3D_OK)
        return ;

    // Keep active ID
    s.ActivePixelShader = ID ;

    // Use default shader?
    if (ID == 0)
        g_pd3dDevice->SetPixelShader(NULL) ;
    else
    {
        ID-- ;
        if (s.PixelShaders[ID].Handle != -1)
        {
            HR = g_pd3dDevice->SetPixelShader(s.PixelShaders[ID].Handle) ;
            ASSERT(HR == D3D_OK) ;
        }
        else
            g_pd3dDevice->SetPixelShader(NULL) ;
    }        
}

//=============================================================================

f32 eng_GetTVRefreshRate( void )
{
    // This is a test to emulate PAL clients connecting to NTSC PS2s
    return 50.0f ;
}

//=============================================================================





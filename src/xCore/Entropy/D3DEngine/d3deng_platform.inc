// Provisional original Tribes D3D8 bodies using fields available in the
// complete Hobbit state. Unlabelled behavior is cross-build source evidence;
// version-specific originals remain in d3deng_tribes.inc. RestoreScreen also
// remains version-gated because it requires the original old shader arrays.

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

s32 d3deng_SetMode( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed );
static
void ConvertComandLine( s32* pargc, char* argv[], LPSTR lpCmdLine );
static
void SetupDefaultD3DSettings ( void );
static
void CreateFont(void);
static
void DestroyFont(void);
void RestoreScreen( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed );
void d3deng_EntryPoint( s32& argc, char**& argv, HINSTANCE h1, HINSTANCE h2, LPSTR str1, INT i1 );
s32 d3deng_ExitPoint( void );
void d3deng_SetPresets( u32 Mode );
static
void UpdateFPS( void );
f32 eng_GetFPS( void );
void ENG_TextColor( const xcolor& Color );
void ENG_SetLight( s32 LightID, const vector3& Dir, const xcolor& Color );
xbool eng_Begin( const char* pTaskName );
xbool eng_InBeginEnd( void );
void eng_End( void );
void eng_PrintStats( void );
void eng_MaximizeViewport( view& View );
void eng_SetBackColor( xcolor Color );
void eng_SetView ( const view& View, s32 ViewPaletteID );
void eng_ActivateView( s32 ViewPaletteID );
void eng_DeactivateView( s32 ViewPaletteID );
u32 eng_GetActiveViewMask   ( void );
const view* eng_GetView( s32 ViewPaletteID );
s32 eng_GetNActiveViews( void );
const view* eng_GetActiveView( s32 ActiveViewListID );
void eng_ScreenShot( const char* pFileName  , s32 Size   );
xbool eng_ScreenShotActive( void );
s32 eng_ScreenShotSize( void );
s32 eng_ScreenShotX( void );
s32 eng_ScreenShotY( void );
void eng_Sync ( void );
void d3deng_ComputeMousePos( void );
f32 d3deng_GetMouseX( void );
f32 d3deng_GetMouseY( void );
f32 d3deng_GetMouseWheel( void );
xbool d3deng_MouseGetLButton( void );
xbool d3deng_MouseGetRButton( void );
xbool d3deng_MouseGetMButton( void );
HINSTANCE d3deng_GetInstace( void );
xbool d3deng_IsActive( void );
HWND d3deng_GetWindowHandle( void );
f32 eng_GetTVRefreshRate( void );
s32 d3deng_GetAdapterCount( void );
const char* d3deng_GetAdapterName( s32 Adapter );
s32 d3deng_GetDeviceCount( s32 Adapter );
const char* d3deng_GetDeviceName( s32 Adapter, s32 Device );
s32 d3deng_GetModeCount( s32 Adapter, s32 Device );
const char* d3deng_GetModeName( s32 Adapter, s32 Device, s32 Mode );
s32 d3deng_GetModeWidth( s32 Adapter, s32 Device, s32 Mode );
s32 d3deng_GetModeHeight( s32 Adapter, s32 Device, s32 Mode );
s32 d3deng_GetModeBitDepth( s32 Adapter, s32 Device, s32 Mode );
s32 d3deng_SetMode( s32 Adapter, s32 Device, s32 Mode );
void d3deng_SetChangeResCallback( d3deng_change_res_fn* Callback );
void d3deng_GetMode( s32& Adapter, s32& Device, s32& Mode );
s32 d3deng_FindBestMode( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed );
static
void AddMode( d3d_adapter& Adapter, 
              d3d_device&  Device,
              UINT         cMode,
              xbool        bWindowed);
static 
void AddDevice ( d3d_adapter& Adapter, const D3DDEVTYPE Type, const D3DCAPS8& Caps );
static
void AddAdapter (       UINT                    cAdapter, 
                  const D3DADAPTER_IDENTIFIER8& ID,
                  const D3DDISPLAYMODE&         d3ddmDesktop );
static 
void BuildModeList( void );
static
void eng_KillShaders( void );
static
void eng_RestoreShaders( void );
static
void SetMode ( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed );
static
void ShowWindow ( void );
static
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
static
HWND CreateWin( s32 Width, s32 Height );
void eng_Kill( void );
static
void InitializeD3D(void);
void eng_Init( void );
void eng_PageFlip();
void eng_GetRes( s32& XRes, s32& YRes );
void eng_SetViewport( const view& View );
void  d3deng_SetMouseMode( mouse_mode Mode );
s32  d3deng_RegisterVertexShader( const char*  Name,     
                                  const DWORD* pDecl,    
                                  const DWORD* pFunc,    
                                        DWORD  Usage,    
                                        DWORD  FVF);
xbool d3deng_IsValidVertexShader( s32 ID );
void d3deng_ActivateVertexShader( s32 ID );
s32  d3deng_RegisterPixelShader( const char*  Name,     
                                 const DWORD* pFunc );
xbool d3deng_IsValidPixelShader( s32 ID );
void d3deng_ActivatePixelShader( s32 ID );

s32 d3deng_SetMode( s32 Width, s32 Height, s32 BitDepth, xbool bWindowed )
{
    // Easy!
    RestoreScreen(Width, Height, BitDepth, bWindowed) ;

    return TRUE ;
}

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

void d3deng_EntryPoint( s32& argc, char**& argv, HINSTANCE h1, HINSTANCE h2, LPSTR str1, INT i1 )
{
    static char* ArgvBuff[256] = { NULL };
    argv = ArgvBuff;
    ConvertComandLine( &argc, argv, str1 );
    s.hInst = h1;

    x_Init();

#if 0
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

RVA(0x0025c170, 0x8)
s32 d3deng_ExitPoint( void )
{
//  eng_Kill();

#if 0
    cdfs_Kill();
#endif

    x_Kill();
    return 0;
}

RVA(0x0025c180, 0xa)
void d3deng_SetPresets( u32 Mode )
{
    s.Mode = Mode;
}

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

RVA(0x0025c9a0, 0x28)
void ENG_TextColor( const xcolor& Color )
{
    s.TextColor = Color;
}

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

RVA(0x0025f420, 0x6)
xbool eng_InBeginEnd( void )
{
    return s.bBeginScene;
}

RVA(0x0025f430, 0xb)
void eng_End( void )
{
    ASSERT( s.bBeginScene );
    s.bBeginScene = FALSE;
}

void eng_PrintStats( void )
{
    x_DebugMsg("CPU:%1.1f  Pageflip:%1.1f  FPS:%1.1f\n",s.CPUMS,s.IMS,1000.0f/(s.CPUMS+s.IMS));
}

RVA(0x0025f8a0, 0x66)
void eng_GetRes(s32& XRes, s32& YRes)
{
    if (s.ResetRequested)
    {
        XRes = s.d3dpp.BackBufferWidth;
        YRes = s.d3dpp.BackBufferHeight;
        return;
    }
    if (g_CurrentPresentParameters.BackBufferWidth)
    {
        XRes = g_CurrentPresentParameters.BackBufferWidth;
        YRes = g_CurrentPresentParameters.BackBufferHeight;
        return;
    }
    GetClientRect(s.hWnd, &s.rcWindowRect);
    XRes = s.rcWindowRect.right;
    YRes = s.rcWindowRect.bottom;
}

RVA(0x0025f910, 0x30)
void eng_MaximizeViewport( view& View )
{
    s32 Width, Height;
    eng_GetRes(Width, Height);
    View.SetViewport(0, 0, Width, Height);
}

RVA(0x0025f940, 0x20)
void eng_SetBackColor( xcolor Color )
{
    s.BackColor = Color;
    s.BackColor.A = 0;
}

void eng_SetView ( const view& View, s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );
    s.View[ViewPaletteID] = View;
}

RVA(0x0025fc10, 0x4a)
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

RVA(0x0025fc60, 0x6e)
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

RVA(0x0025fcd0, 0x6)
u32 eng_GetActiveViewMask   ( void )
{
    return s.ViewMask;
}

RVA(0x0025fce0, 0x10)
const view* eng_GetView( s32 ViewPaletteID )
{
    ASSERT( ViewPaletteID >= 0  );
    ASSERT( ViewPaletteID < ENG_MAX_VIEWS );

    return &s.View[ ViewPaletteID ];
}

RVA(0x0025fcf0, 0x6)
s32 eng_GetNActiveViews( void )
{
    return s.nActiveViews;
}

RVA(0x0025fd00, 0xc)
const view* eng_GetActiveView( s32 ActiveViewListID )
{
    ASSERT( ActiveViewListID >= 0  );
    ASSERT( ActiveViewListID < eng_GetNActiveViews() );

    return s.ActiveView[ ActiveViewListID ];
}

void eng_ScreenShot( const char* pFileName /*= NULL*/, s32 Size /*= 1*/ )
{
    (void)pFileName ;
    (void)Size ;
    ASSERTS( 0, "Press Print screen for now" );
}

xbool eng_ScreenShotActive( void )
{
    // Not yet implemented
    return FALSE ;
}

s32 eng_ScreenShotSize( void )
{
    // Not yet implemented
    return 1 ;
}

s32 eng_ScreenShotX( void )
{
    // Not yet implemented
    return 0 ;
}

s32 eng_ScreenShotY( void )
{
    // Not yet implemented
    return 0 ;
}

void eng_Sync ( void )
{

}

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

RVA(0x00260510, 0x17)
f32 d3deng_GetMouseX( void )
{
    if (s.MouseMode == MOUSE_MODE_ABSOLUTE)
        return s.ABSMouseX;
    return s.MouseX;
}

RVA(0x00260530, 0x17)
f32 d3deng_GetMouseY( void )
{
    if (s.MouseMode == MOUSE_MODE_ABSOLUTE)
        return s.ABSMouseY;
    return s.MouseY;
}

RVA(0x00260550, 0x11)
f32 d3deng_GetMouseWheel( void )
{
    // TEMP FOR NOW...
    f32 Value = s.MouseWheelRel ;
    s.MouseWheelRel = 0 ;

    return Value ;
}

RVA(0x00260570, 0x6)
xbool d3deng_MouseGetLButton( void )
{
    return s.bMouseLeftButton;
}

RVA(0x00260580, 0x6)
xbool d3deng_MouseGetRButton( void )
{
    return s.bMouseRightButton;
}

RVA(0x00260590, 0x6)
xbool d3deng_MouseGetMButton( void )
{
    return s.bMouseMiddleButton;
}

RVA(0x002605a0, 0x6)
HINSTANCE d3deng_GetInstace( void )
{
    return s.hInst;
}

xbool d3deng_IsActive( void )
{
    return s.bActive ;
}

RVA(0x002605b0, 0x6)
HWND d3deng_GetWindowHandle( void )
{
    return s.hWnd;
}

f32 eng_GetTVRefreshRate( void )
{
    // This is a test to emulate PAL clients connecting to NTSC PS2s
    return 50.0f ;
}

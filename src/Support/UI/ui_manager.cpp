// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_manager.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_manager.cpp
//
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <Support/UI/ui_manager.hpp>

// Genuine sibling provider; Xbox UI:ui_manager.obj confirms qualified owner.
// PC global address remains unclaimed.
ui_manager* g_UiMgr = NULL;
#include <Support/UI/ui_win.hpp>
#include <Support/UI/ui_font.hpp>
#include <Support/UI/ui_dialog.hpp>
#include <Support/UI/ui_button.hpp>
#include <Support/UI/ui_frame.hpp>
#include <Support/UI/ui_combo.hpp>
#include <Support/UI/ui_radio.hpp>
#include <Support/UI/ui_check.hpp>
#include <Support/UI/ui_edit.hpp>
#include <Support/UI/ui_text.hpp>
#include <Support/UI/ui_slider.hpp>
#include <Support/UI/ui_listbox.hpp>
#include <Support/UI/ui_textbox.hpp>
#include <Support/UI/ui_dlg_vkeyboard.hpp>
#include <Support/UI/ui_tabbed_dialog.hpp>
#include <Support/UI/ui_dlg_list.hpp>

#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>

//=========================================================================
//  Defines
//=========================================================================

#define ENABLE_SCREENSHOTS  0

//=========================================================================
//  Structs
//=========================================================================

//=========================================================================
//  Data
//=========================================================================

s32 g_uiLastSelectController = 0;

//=========================================================================
//  Helpers
//=========================================================================

void ui_manager::UpdateButton( ui_manager::button& Button, xbool State, f32 DeltaTime )
{
    // Clear number of presses, repeats and releases
    Button.nPresses  = 0;
    Button.nRepeats  = 0;
    Button.nReleases = 0;

    if( m_EnableUserInput )
    {
        // Check for press
        if( !Button.State && State )
        {
            Button.nPresses++;
            Button.RepeatTimer = Button.RepeatDelay;
        }

        // Check for repeat
        if( Button.State && State )
        {
            // If repeat interval is 0 then repeat is disabled
            if( Button.RepeatInterval > 0.0f )
            {
                Button.RepeatTimer -= DeltaTime;
                while( Button.RepeatTimer < 0.0f )
                {
                    Button.nRepeats++;
                    Button.RepeatTimer += Button.RepeatInterval;
                }
            }
        }

        // Check for release
        if( Button.State && !State )
        {
            Button.nReleases++;
        }
    }
    else
    {
        State = 0;
    }

    // Set new state
    Button.State = State;
}

//=========================================================================

void ui_manager::UpdateAnalog( ui_manager::button& Button, f32 Value, f32 DeltaTime )
{
    xbool State = Button.State;

    ASSERT( Button.AnalogDisengage < 1.0f );

    // Clear number of presses, repeats and releases
    Button.nPresses  = 0;
    Button.nRepeats  = 0;
    Button.nReleases = 0;

    if( m_EnableUserInput )
    {
        // Do Analog mapping to a pressed button
        Value *= Button.AnalogScaler;
        if( Value > Button.AnalogEngage )
            State = TRUE;
        if( Value < Button.AnalogDisengage )
            State = FALSE;

        // Make time scaler from Value
        Value = 1.0f + (Value - Button.AnalogDisengage) / (1.0f - Button.AnalogDisengage) * 3;

        // Check for press
        if( !Button.State && State )
        {
            Button.nPresses++;
            Button.RepeatTimer = Button.RepeatDelay;
        }

        // Check for repeat
        if( Button.State && State )
        {
            // If repeat interval is 0 then repeat is disabled
            if( Button.RepeatInterval > 0.0f )
            {
                Button.RepeatTimer -= DeltaTime * Value;
                while( Button.RepeatTimer < 0.0f )
                {
                    Button.nRepeats++;
                    Button.RepeatTimer += Button.RepeatInterval;
                }
            }
        }

        // Check for release
        if( Button.State && !State )
        {
            Button.nReleases++;
        }
    }
    else
    {
        State = 0;
    }

    // Set new state
    Button.State = State;
}

//=========================================================================
//  ui_manager
//=========================================================================

ui_manager::ui_manager( void )
{
    m_AlphaTime = 0.0f;
}

//=========================================================================

ui_manager::~ui_manager( void )
{
    Kill();
}

//=========================================================================

void ui_manager::Init( void )
{
    // Register the default window classes
    RegisterWinClass( "button",  &ui_button_factory  );
    RegisterWinClass( "frame",   &ui_frame_factory   );
    RegisterWinClass( "frame1",  &ui_frame_factory   );
    RegisterWinClass( "combo",   &ui_combo_factory   );
    RegisterWinClass( "radio",   &ui_radio_factory   );
    RegisterWinClass( "check",   &ui_check_factory   );
    RegisterWinClass( "edit",    &ui_edit_factory    );
    RegisterWinClass( "listbox", &ui_listbox_factory );
    RegisterWinClass( "text",    &ui_text_factory    );
    RegisterWinClass( "slider",  &ui_slider_factory  );
    RegisterWinClass( "textbox", &ui_textbox_factory );

    // Register the default dialog classes
//    RegisterDialogClass( "ui_combolist",     (dialog_tem*)0, &ui_dlg_combolist_factory );
    RegisterDialogClass( "ui_vkeyboard",     (dialog_tem*)0, &ui_dlg_vkeyboard_factory );
    RegisterDialogClass( "ui_tabbed_dialog", (dialog_tem*)0, &ui_tabbed_dialog_factory );
    ui_dlg_list_register( this );

#ifdef TARGET_PC
#ifdef T2_DESIGNER_BUILD
    ShowCursor(TRUE);
#else
    ShowCursor(FALSE);
#endif
    m_Mouse.Load( "data/ui/pc/ui_cursor.xbmp");
    
    VERIFY( vram_Register( m_Mouse ) );

    m_MouseColor.Set(ARGB(224,224,224,224));
    //d3deng_SetMouseMode( MOUSE_MODE_ALWAYS );
    d3deng_SetMouseMode( MOUSE_MODE_ABSOLUTE ) ;
#endif
    // Allow processing of user input
    m_EnableUserInput = TRUE;
}

//=========================================================================

void ui_manager::Kill( void )
{
    // Destroy Users
    while( m_Users.GetCount() > 0 )
    {
        user*   pUser = m_Users[0];

        // Destroy Dialog Stack
        while( pUser->DialogStack.GetCount() > 0 )
        {
            ui_dialog* pDialog = pUser->DialogStack[0];

            pDialog->Destroy();
            delete pDialog;
            pUser->DialogStack.Delete( 0 );
        }

        // Destroy User
        delete pUser;
        m_Users.Delete( 0 );
    }

    // Destroy Fonts
    while( m_Fonts.GetCount() > 0 )
    {
        m_Fonts[0]->pFont->Kill();
        delete m_Fonts[0]->pFont;
        delete m_Fonts[0];
        m_Fonts.Delete( 0 );
    }

    // Destroy Elements
    while( m_Elements.GetCount() > 0 )
    {
        vram_Unregister( m_Elements[0]->Bitmap );
        m_Elements[0]->Bitmap.Kill();
        delete m_Elements[0];
        m_Elements.Delete( 0 );
    }

    // Destroy Backgrounds
    while( m_Backgrounds.GetCount() > 0 )
    {
        vram_Unregister( m_Backgrounds[0]->Bitmap );
        m_Backgrounds[0]->Bitmap.Kill();
        delete m_Backgrounds[0];
        m_Backgrounds.Delete( 0 );
    }

    // Destroy Window Classes
    m_WindowClasses.Delete( 0, m_WindowClasses.GetCount() );

    // Destroy Dialog Classes
    m_DialogClasses.Delete( 0, m_DialogClasses.GetCount() );
}

//=========================================================================

s32 ui_manager::LoadBackground( const char* pName, const char* pPathName )
{
    // Check if background already exists
    {
        s32 ID = FindBackground( pName );
        if( ID != -1 )
            return ID;
    }

    // Create new background
    background* pBackground = new background;
    ASSERT( pBackground );

    // Set data
    pBackground->Name = pName;

    // Load the bitmap
    VERIFY( pBackground->Bitmap.Load( pPathName ) );

    // Register for VRAM
    vram_Register( pBackground->Bitmap );

    // Add background to array and return ID
    m_Backgrounds.Append() = pBackground;
    return m_Backgrounds.GetCount()-1;
}

//=========================================================================

s32 ui_manager::FindBackground( const char* pName )
{
    s32 iFound = -1;
    s32 i;

    for( i=0 ; i<m_Backgrounds.GetCount() ; i++ )
    {
        if( m_Backgrounds[i]->Name == pName )
        {
            iFound = i;
            break;
        }
    }

    return iFound;
}

//=========================================================================

void ui_manager::RenderBackground( s32 iBackground ) const
{
    background* pBackground = m_Backgrounds[iBackground];
    ASSERT( pBackground );
    s32 Width, Height;
    eng_GetRes(Width, Height);

    draw_Begin( DRAW_SPRITES, DRAW_2D|DRAW_TEXTURED|DRAW_NO_ZBUFFER );
    draw_SetTexture( pBackground->Bitmap );
    draw_Sprite( vector3(0.0f,0.0f,0.0f),vector2((f32)Width, (f32)Height), XCOLOR_WHITE );
//    draw_Sprite( vector3(0.0f,0.0f,0.0f),vector2((f32)pBackground->Bitmap.GetWidth(), (f32)pBackground->Bitmap.GetHeight()), XCOLOR_WHITE );
    draw_End();
}

//=========================================================================

s32 ui_manager::LoadElement( const char* pName, const char* pPathName, s32 nStates, s32 cx, s32 cy )
{
    element*    pElement;
    xarray<s32> x;
    xarray<s32> y;
    s32         i;
    s32         ix;
    s32         iy;
    xcolor      RegColor;

    // Check if element already exists
    {
        s32 ID = FindElement( pName );
        if( ID != -1 )
            return ID;
    }

    // Create new element
    pElement = new element;
    ASSERT( pElement );

    // Set data
    pElement->Name      = pName;
    pElement->nStates   = nStates;
    pElement->cx        = cx;
    pElement->cy        = cy;

    // Load the bitmap
    VERIFY( pElement->Bitmap.Load( pPathName ) );

    if( (nStates > 0) && (cx > 0) && (cy > 0) )
    {
        ASSERT( (cx==1) || (cx==3) );
        ASSERT( (cy==1) || (cy==3) );

        // Pick out registration mark color
        RegColor = pElement->Bitmap.GetPixelColor( 0, 0 );

        // Find the registration markers
        x.SetCapacity( cx+1 );
        y.SetCapacity( (cy*nStates)+1 );
        for( i=0 ; i<pElement->Bitmap.GetWidth() ; i++ )
        {
            if( pElement->Bitmap.GetPixelColor( i, 0 ) == RegColor )
                x.Append() = i;
        }
        for( i=0 ; i<pElement->Bitmap.GetHeight() ; i++ )
        {
            if( pElement->Bitmap.GetPixelColor( 0, i ) == RegColor )
                y.Append() = i;
        }
        ASSERT( x.GetCount() == (cx+1) );
        ASSERT( y.GetCount() == ((cy*nStates)+1) );

        // Setup the rectangles
        pElement->r.SetCapacity( cx*cy*nStates );
        for( iy=0 ; iy<(cy*nStates) ; iy++ )
        {
            for( ix=0 ; ix<cx ; ix++ )
            {
                pElement->r.Append().Set( x[ix]+1, y[iy]+1, x[ix+1], y[iy+1] );
            }
        }
    }

    // Register the bitmap for VRAM
    vram_Register( pElement->Bitmap );

    // Add element to array and return ID
    m_Elements.Append() = pElement;
    return m_Elements.GetCount()-1;
}

//=========================================================================

s32 ui_manager::FindElement( const char* pName ) const
{
    s32 i;

    for( i=0 ; i<m_Elements.GetCount() ; i++ )
    {
        if( m_Elements[i]->Name == pName )
        {
            return i;
        }
    }

    return -1;
}

//=========================================================================

void ui_manager::RenderElement( s32 iElement, const irect& Position, s32 State, const xcolor& Color, xbool IsAdditive ) const
{
    xbool       ScaleX = FALSE;
    xbool       ScaleY = FALSE;
    s32         ix;
    s32         iy;
    s32         ie;
    vector3     p(0.0f, 0.0f, 0.5f );
    vector2     wh;
    vector2     uv0;
    vector2     uv1;
    element*    pElement;

    ASSERT( (iElement >= 0) && (iElement < m_Elements.GetCount()) );

    // Get Element pointer
    pElement = m_Elements[iElement];

    // Validate arguments
    ASSERT( (State >= 0) && (State < pElement->nStates) );

    // Determine what type we are, scaled horizontal, vertical, or both
    if( Position.GetWidth()  != 0 ) ScaleX = TRUE;
    if( Position.GetHeight() != 0 ) ScaleY = TRUE;

    // Being drawing
    draw_Begin( DRAW_SPRITES, DRAW_2D|DRAW_TEXTURED|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
    draw_SetTexture( pElement->Bitmap );
    draw_DisableBilinear();

    // Set Additive Mode
    if( IsAdditive )
    {
#ifdef TARGET_PC
        g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
#endif
#ifdef TARGET_PS2
        gsreg_Begin();
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
        gsreg_End();
#endif
    }

    // Render all the parts of the element
    ie      = State * (pElement->cx*pElement->cy);
    p.Y     = (f32)Position.t;

    // Loop on y
    for( iy=0 ; iy<pElement->cy ; iy++ )
    {
        // Reset x position
        p.X = (f32)Position.l;

        // Set Height
        if( (pElement->cy == 3) && (iy == 1) )
        {
            wh.Y = (f32)Position.GetHeight() - ((f32)pElement->r[2*pElement->cx].GetHeight() + (f32)pElement->r[0].GetHeight());
        }
        else
        {
            wh.Y = (f32)pElement->r[ie].GetHeight();
        }


        // Loop on x
        for( ix=0 ; ix<pElement->cx ; ix++ )
        {
            // Calculate UVs
            uv0.X = ((f32)pElement->r[ie].l + 0.5f) / pElement->Bitmap.GetWidth();
            uv0.Y = ((f32)pElement->r[ie].t + 0.5f) / pElement->Bitmap.GetHeight();
            uv1.X = ((f32)pElement->r[ie].r - 0.0f) / pElement->Bitmap.GetWidth();
            uv1.Y = ((f32)pElement->r[ie].b - 0.0f) / pElement->Bitmap.GetHeight();

            // Set Width
            if( (pElement->cx == 3) && (ix == 1) )
            {
                wh.X = (f32)Position.GetWidth() - ((f32)pElement->r[2].GetWidth() + (f32)pElement->r[0].GetWidth());
            }
            else
            {
                wh.X = (f32)pElement->r[ie].GetWidth();
            }

            // Draw sprite
            draw_SpriteUV( p, wh, uv0, uv1, Color );

            // Advance position on x
            p.X += wh.X;

            // Advance index to element
            ie++;
        }

        // Advance position on y
        p.Y += wh.Y;
    }

    // End drawing
    draw_End();
}

//=========================================================================

void ui_manager::RenderElementUV( s32 iElement, const irect& Position, const irect& UV, const xcolor& Color, xbool IsAdditive ) const
{
    vector3     p(0.0f, 0.0f, 0.5f );
    vector2     wh;
    vector2     uv0;
    vector2     uv1;
    element*    pElement;

    ASSERT( (iElement >= 0) && (iElement < m_Elements.GetCount()) );

    // Get Element pointer
    pElement = m_Elements[iElement];

    // Being drawing
    draw_Begin( DRAW_SPRITES, DRAW_2D|DRAW_TEXTURED|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
    draw_SetTexture( pElement->Bitmap );
    draw_DisableBilinear();

    // Set Additive Mode
    if( IsAdditive )
    {
#ifdef TARGET_PC
        g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
#endif
#ifdef TARGET_PS2
        gsreg_Begin();
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
        gsreg_End();
#endif
    }

    // Calculate Position and Size
    p. X = (f32)Position.l;
    p. Y = (f32)Position.t;
    wh.X = (f32)Position.GetWidth();
    wh.Y = (f32)Position.GetHeight();

    // Calculate UVs
    uv0.X = (UV.l + 0.5f) / pElement->Bitmap.GetWidth();
    uv0.Y = (UV.t + 0.5f) / pElement->Bitmap.GetHeight();
    uv1.X = (UV.r + 0.5f) / pElement->Bitmap.GetWidth();
    uv1.Y = (UV.b + 0.5f) / pElement->Bitmap.GetHeight();

    // Draw sprite
    draw_SpriteUV( p, wh, uv0, uv1, Color );

    // End drawing
    draw_End();
}

//=========================================================================

const ui_manager::element* ui_manager::GetElement( s32 iElement ) const
{
    ASSERT( (iElement >= 0) && (iElement < m_Elements.GetCount()) );

    return m_Elements[iElement];
}

//=========================================================================

s32 ui_manager::LoadFont( const char* pName, const char* pPathName )
{
    // Check if font already exists
    {
        s32 ID = FindFont( pName );
        if( ID != -1 )
            return ID;
    }

    // Create font record
    font* pFont = new font;
    ASSERT( pFont );

    // Setup Font
    pFont->Name = pName;

    // Create and load font
    pFont->pFont = new ui_font;
    ASSERT( pFont->pFont );
    VERIFY( pFont->pFont->Load( pPathName ) );

    // Add to array of fonts
    m_Fonts.Append() = pFont;

    // Return Font ID
    return m_Fonts.GetCount()-1;
}

//=========================================================================

RVA(0x29fa60, 0x54)
s32 ui_manager::FindFont( const char* pName ) const
{
    s32 iFound = -1;
    s32 i;

    for( i=0 ; i<m_Fonts.GetCount() ; i++ )
    {
        if( m_Fonts[i]->Name == pName )
        {
            iFound = i;
            break;
        }
    }

    return( iFound );
}

//=========================================================================

void ui_manager::RenderText( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const char* pString ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    pFont->RenderText( Position, Flags, Color, pString );
}

//=========================================================================
/*
void ui_manager::RenderText( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const char* pString ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    pFont->RenderText( Position, Flags, Color, pString );
}
*/
//=========================================================================

void ui_manager::RenderText( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const xwchar* pString ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    pFont->RenderText( Position, Flags, Color, pString );
}

//=========================================================================
/*
void ui_manager::RenderTextFormatted( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const xwchar* pString ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    pFont->DrawFormattedText( Position, Flags, Color, pString );
}
*/
//=========================================================================

void ui_manager::RenderText( s32 iFont, const irect& Position, s32 Flags, s32 Alpha, const xwchar* pString ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    pFont->RenderText( Position, Flags, Alpha, pString );
}

//=========================================================================

RVA(0x29fc80, 0x27)
void ui_manager::TextSize( s32 iFont, irect& Rect, const xwchar* pString, s32 Count ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    pFont->TextSize( Rect, pString, Count );
}

//=========================================================================

s32 ui_manager::GetLineHeight( s32 iFont ) const
{
    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );

    ui_font* pFont = m_Fonts[iFont]->pFont;

    return pFont->GetLineHeight();
}

//=========================================================================

void ui_manager::RenderRect( const irect& r, const xcolor& Color, xbool IsWire ) const
{
    draw_Rect( r, Color, IsWire );
}

//=========================================================================

void ui_manager::RenderGouraudRect( const irect& r, const xcolor& c1, const xcolor& c2, const xcolor& c3, const xcolor& c4, xbool IsWire ) const
{
    draw_GouraudRect( r, c1, c2, c3, c4, IsWire );
}

//=========================================================================

RVA(0x29fdd0, 0x6e)
xbool ui_manager::RegisterWinClass ( const char* ClassName, ui_pfn_winfact pFactory )
{
    xbool   Success = FALSE;
    s32     iFound = -1;
    s32     i;

    // Find the winclass entry
    for( i=0 ; i<m_WindowClasses.GetCount() ; i++ )
    {
        if( m_WindowClasses[i].ClassName == ClassName )
        {
            iFound = i;
        }
    }

    // If not found then add a new one
    if( iFound == -1 )
    {
        winclass& wc = m_WindowClasses.Append();
        wc.ClassName = ClassName;
        wc.pFactory  = pFactory;
        Success = TRUE;
    }

    // Return success code
    return Success;
}

//=========================================================================

RVA(0x29fe40, 0x6b)
ui_win* ui_manager::CreateWin( s32 UserID, const char* ClassName, const irect& Position, ui_win* pParent, s32 Flags )
{
    ui_win*         pWin        = NULL;
    ui_pfn_winfact  pFactory    = NULL;
    s32             i;

    // Find the winclass entry
    for( i=0 ; i<m_WindowClasses.GetCount() ; i++ )
    {
        if( m_WindowClasses[i].ClassName == ClassName )
        {
            pFactory = m_WindowClasses[i].pFactory;
        }
    }

    // If we have a factory function then continue
    if( pFactory )
    {
        pWin = pFactory( UserID, this, Position, pParent, Flags );
    }

    // Return pointer to new window
    return pWin;
}

//=========================================================================

s32 ui_manager::CreateUser( s32 ControllerID, const irect& Bounds, s32 Data )
{
    // Create new user struct
    user*   pUser = new user;
    ASSERT( pUser );
    if( pUser )
    {
        s32     i;

        // Fill out the struct
        x_memset( pUser, 0, sizeof(user) );
        pUser->Enabled              = TRUE;
        pUser->ControllerID         = ControllerID;
        pUser->CursorX              = Bounds.GetWidth()/2  + Bounds.l;
        pUser->CursorY              = Bounds.GetHeight()/2 + Bounds.t;
        pUser->CursorVisible        = TRUE;
        pUser->iBackground          = -1;
        pUser->Bounds               = Bounds;
        pUser->Data                 = Data;
        pUser->iHighlightElement    = FindElement( "highlight" );

        ASSERT( pUser->iHighlightElement );

        // Set Analog Scalers
        for( i=0 ; i<2 ; i++ )
        {
            pUser->DPadUp[i]       .SetupRepeat( 0.200f, 0.060f );
            pUser->DPadDown[i]     .SetupRepeat( 0.200f, 0.060f );
            pUser->DPadLeft[i]     .SetupRepeat( 0.200f, 0.060f );
            pUser->DPadRight[i]    .SetupRepeat( 0.200f, 0.060f );
            pUser->PadSelect[i]    .SetupRepeat( 0.200f, 0.060f );
            pUser->PadBack[i]      .SetupRepeat( 0.200f, 0.060f );
            pUser->PadHelp[i]      .SetupRepeat( 0.200f, 0.060f );
            pUser->PadShoulderL[i] .SetupRepeat( 0.200f, 0.060f );
            pUser->PadShoulderR[i] .SetupRepeat( 0.200f, 0.060f );
            pUser->PadShoulderL2[i].SetupRepeat( 0.200f, 0.060f );
            pUser->PadShoulderR2[i].SetupRepeat( 0.200f, 0.060f );
            pUser->LStickUp[i]     .SetupRepeat( 0.600f, 0.300f );
            pUser->LStickDown[i]   .SetupRepeat( 0.600f, 0.300f );
            pUser->LStickLeft[i]   .SetupRepeat( 0.600f, 0.300f );
            pUser->LStickRight[i]  .SetupRepeat( 0.600f, 0.300f );
            pUser->LStickUp[i]     .SetupAnalog(  1.0f, 0.15f, 0.2f );
            pUser->LStickDown[i]   .SetupAnalog( -1.0f, 0.15f, 0.2f );
            pUser->LStickLeft[i]   .SetupAnalog( -1.0f, 0.15f, 0.2f );
            pUser->LStickRight[i]  .SetupAnalog(  1.0f, 0.15f, 0.2f );
        }

        // Add an entry to the users list
        m_Users.Append() = pUser;
    }

    return (s32)pUser;
}

//=========================================================================

void ui_manager::DeleteAllUsers( void )
{
    while( m_Users.GetCount() > 0 )
    {
        DeleteUser( (s32)m_Users[0] );
    }
}

//=========================================================================

void ui_manager::DeleteUser( s32 UserID )
{
    s32     Index;

    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    // Find the users index
    Index = m_Users.Find( (user*)UserID );
    ASSERT( Index != -1 );

    // Close all the dialog that may be open
    while( m_Users[Index]->DialogStack.GetCount() > 0 )
    {
        s32 i = m_Users[Index]->DialogStack.GetCount()-1;
        m_Users[Index]->DialogStack[i]->Destroy();
        m_Users[Index]->DialogStack.Delete( i );
    }

    // Delete the user
    delete m_Users[Index];
    m_Users.Delete( Index );
}

//=========================================================================

RVA(0x2a0380, 0x7)
ui_manager::user* ui_manager::GetUser( s32 UserID ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    return pUser;
}

//=========================================================================

s32 ui_manager::GetUserData( s32 UserID ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    return pUser->Data;
}

//=========================================================================
ui_win* ui_manager::GetWindowUnderCursor( s32 UserID ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    return pUser->pLastWindowUnderCursor;
}

//=========================================================================

void ui_manager::SetCursorVisible( s32 UserID, xbool State )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    pUser->CursorVisible = State;
}

//=========================================================================

xbool ui_manager::GetCursorVisible( s32 UserID ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    return pUser->CursorVisible;
}

//=========================================================================

RVA(0x2a03c0, 0x43)
void ui_manager::SetCursorPos( s32 UserID, s32 x, s32 y )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    pUser->CursorX = x;
    pUser->CursorY = y;

    ui_win* pWin = GetWindowAtXY( pUser, x, y );

    // Has window under cursor changed?
    if( pWin != pUser->pLastWindowUnderCursor )
    {
        // Call exit function if there was a window under the cursor
        if( pUser->pLastWindowUnderCursor )
        {
            pUser->pLastWindowUnderCursor->OnCursorExit( pUser->pLastWindowUnderCursor );
        }

        // Set new window under cursor and call enter function
        pUser->pLastWindowUnderCursor = pWin;
        if( pUser->pLastWindowUnderCursor )
        {
            pUser->pLastWindowUnderCursor->OnCursorEnter( pUser->pLastWindowUnderCursor );
        }
    }

}

//=========================================================================

void ui_manager::GetCursorPos( s32 UserID, s32& x, s32& y ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    x = pUser->CursorX;
    y = pUser->CursorY;
}

//=========================================================================

ui_win* ui_manager::SetCapture( s32 UserID, ui_win* pWin )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    ui_win* pOldCaptureWin = pUser->pCaptureWindow;
    pUser->pCaptureWindow = pWin;

    return pOldCaptureWin;
}

//=========================================================================

void ui_manager::ReleaseCapture( s32 UserID )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    pUser->pCaptureWindow = NULL;
}

//=========================================================================

RVA(0x2a0410, 0xe)
void ui_manager::SetUserBackground( s32 UserID, s32 iBackground )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    pUser->iBackground = iBackground;
}

//=========================================================================

const irect& ui_manager::GetUserBounds( s32 UserID ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    return pUser->Bounds;
}

//=========================================================================

RVA(0x2a0420, 0xd)
void ui_manager::EnableUser( s32 UserID, xbool State )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    pUser->Enabled = State;
}

//=========================================================================

xbool ui_manager::IsUserEnabled( s32 UserID ) const
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;
    return pUser->Enabled;
}

//=========================================================================

RVA(0x2a0430, 0x5f)
void ui_manager::AddHighlight( s32 UserID, irect& r, xbool Flash )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );

    user*   pUser = (user*)UserID;

    highlight& h = m_Highlights.Append();
    h.r = r;
    h.r.Inflate( 10, 11 );
    h.iElement = pUser->iHighlightElement;
    h.Flash    = Flash;
}

//=========================================================================

RVA(0x2a0490, 0x3f)
xbool ui_manager::ProcessInput( f32 DeltaTime )
{
    s32     i;
    xbool   Continue = TRUE;

    // Loop through each user
    for( i=0 ; i<m_Users.GetCount() ; i++ )
    {
        user* pUser = m_Users[i];
        ASSERT( pUser );

        // Only process input for enabled users
        if( pUser->Enabled )
        {
            Continue &= ProcessInput( DeltaTime, (s32)pUser );
        }
    }

    return Continue;
}

//=========================================================================

RVA(0x2a04d0, 0x83)
ui_win* ui_manager::GetWindowAtXY( user* pUser, s32 x, s32 y )
{
    ui_win* pWindow = NULL;

    // Check if anything on dialog stack
    if( pUser->DialogStack.GetCount() > 0 )
    {
        s32     i = pUser->DialogStack.GetCount()-1;

        // Yes search from topmost dialog back
        while( (pWindow == NULL) && (i >= 0) )
        {
            ui_dialog* pDialog = pUser->DialogStack[i];
            pDialog->ScreenToLocal( x, y );
            pWindow = pDialog->GetWindowAtXY( x, y );

            // Don't select a disabled window
            if( pWindow && (pWindow->GetFlags() & ui_win::WF_DISABLED) )
                pWindow = pDialog;

            // If modal then exit, otherwise step back to next dialog
            if( pDialog->GetFlags() & ui_win::WF_INPUTMODAL )
            {
                if( pWindow == NULL )
                    pWindow = pDialog;
                break;
            }
            else
            {
                i--;
            }
        }
    }
    return pWindow;
}

//=========================================================================
extern xbool DO_SCREEN_SHOTS;

xbool ui_manager::ProcessInput( f32 DeltaTime, s32 UserID )
{
    xbool   Iterate;
    s32     StartController = 0;
    s32     EndController   = 0;

    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );
    user*   pUser = (user*)UserID;

    do
    {
        // Don't iterate unless set later
        Iterate = FALSE;

        // Get pointer to window to receive input
        ui_win* pWin = pUser->pCaptureWindow;

        // Update mouse cursor position
        //pUser->CursorX += (s32)input_GetValue(INPUT_MOUSE_X_REL);
        //pUser->CursorY += (s32)input_GetValue(INPUT_MOUSE_Y_REL);
#ifdef TARGET_PC
#ifndef T2_DESIGNER_BUILD
        pUser->CursorX = (s32)input_GetValue(INPUT_MOUSE_X_ABS);
        pUser->CursorY = (s32)input_GetValue(INPUT_MOUSE_Y_ABS);
#endif
#endif
        pUser->CursorX = MAX( pUser->CursorX, 0 );
        pUser->CursorX = MIN( pUser->CursorX, (pUser->Bounds.GetWidth()-1) );
        pUser->CursorY = MAX( pUser->CursorY, 0 );
        pUser->CursorY = MIN( pUser->CursorY, (pUser->Bounds.GetHeight()-1) );

        // Determine which window cursor is now over and call appropriate EXIT/ENTER functions
        if( pWin == NULL )
        {
            ui_win* pWindowUnderCursor = GetWindowAtXY( pUser, pUser->CursorX, pUser->CursorY );

            // Has window under cursor changed?
            if( pWindowUnderCursor != pUser->pLastWindowUnderCursor )
            {
                // Call exit function if there was a window under the cursor
                if( pUser->pLastWindowUnderCursor )
                {
                    pUser->pLastWindowUnderCursor->OnCursorExit( pUser->pLastWindowUnderCursor );
                }

                // Set new window under cursor and call enter function
                pUser->pLastWindowUnderCursor = pWindowUnderCursor;
                if( pUser->pLastWindowUnderCursor )
                {
                    pUser->pLastWindowUnderCursor->OnCursorEnter( pUser->pLastWindowUnderCursor );
                }
            }

            // Set pointer to window to receive input
            pWin = pUser->pLastWindowUnderCursor;
        }

#ifdef TARGET_PC
        // Update mouse buttons
        UpdateButton( pUser->ButtonLB, input_IsPressed( INPUT_MOUSE_BTN_L ), DeltaTime );
        UpdateButton( pUser->ButtonMB, input_IsPressed( INPUT_MOUSE_BTN_C ), DeltaTime );
        UpdateButton( pUser->ButtonRB, input_IsPressed( INPUT_MOUSE_BTN_R ), DeltaTime );

        // Update pad buttons
        UpdateButton( pUser->DPadUp[0],       input_IsPressed( INPUT_KBD_UP        ), DeltaTime );
        UpdateButton( pUser->DPadDown[0],     input_IsPressed( INPUT_KBD_DOWN      ), DeltaTime );
        UpdateButton( pUser->DPadLeft[0],     input_IsPressed( INPUT_KBD_LEFT      ), DeltaTime );
        UpdateButton( pUser->DPadRight[0],    input_IsPressed( INPUT_KBD_RIGHT     ), DeltaTime );
        UpdateButton( pUser->PadSelect[0],    input_IsPressed( INPUT_KBD_RETURN    ), DeltaTime );
        UpdateButton( pUser->PadBack[0],      input_IsPressed( INPUT_KBD_BACK      ), DeltaTime );
        UpdateButton( pUser->PadShoulderL[0], input_IsPressed( INPUT_KBD_S         ), DeltaTime );
        UpdateButton( pUser->PadShoulderR[0], input_IsPressed( INPUT_KBD_F         ), DeltaTime );
#endif

#ifdef TARGET_PS2

        if( DO_SCREEN_SHOTS && input_WasPressed( INPUT_PS2_BTN_L_STICK, 0 ) )
        {
            eng_ScreenShot();
        }

        // Update d-pad buttons
        StartController = pUser->ControllerID;
        EndController   = StartController;
        if( StartController == -1 )
        {
            StartController = 0;
            EndController   = 1;
        }
        {
            s32 i;

            for( i=StartController ; i<=EndController ; i++ )
            {
                UpdateButton( pUser->DPadUp[i],        input_IsPressed( INPUT_PS2_BTN_L_UP,     i ), DeltaTime );
                UpdateButton( pUser->DPadDown[i],      input_IsPressed( INPUT_PS2_BTN_L_DOWN,   i ), DeltaTime );
                UpdateButton( pUser->DPadLeft[i],      input_IsPressed( INPUT_PS2_BTN_L_LEFT,   i ), DeltaTime );
                UpdateButton( pUser->DPadRight[i],     input_IsPressed( INPUT_PS2_BTN_L_RIGHT,  i ), DeltaTime );
                UpdateButton( pUser->PadSelect[i],     input_IsPressed( INPUT_PS2_BTN_CROSS,    i ), DeltaTime );
                UpdateButton( pUser->PadBack[i],       input_IsPressed( INPUT_PS2_BTN_TRIANGLE, i ), DeltaTime );
                UpdateButton( pUser->PadDelete[i],     input_IsPressed( INPUT_PS2_BTN_SQUARE,   i ), DeltaTime );
                UpdateButton( pUser->PadShoulderL[i],  input_IsPressed( INPUT_PS2_BTN_L1,       i ), DeltaTime );
                UpdateButton( pUser->PadShoulderR[i],  input_IsPressed( INPUT_PS2_BTN_R1,       i ), DeltaTime );
                UpdateButton( pUser->PadShoulderL2[i], input_IsPressed( INPUT_PS2_BTN_L2,       i ), DeltaTime );
                UpdateButton( pUser->PadShoulderR2[i], input_IsPressed( INPUT_PS2_BTN_R2,       i ), DeltaTime );
/*
                UpdateAnalog( pUser->LStickUp[i],       input_GetValue( INPUT_PS2_STICK_LEFT_Y, i ), DeltaTime );
                UpdateAnalog( pUser->LStickDown[i],     input_GetValue( INPUT_PS2_STICK_LEFT_Y, i ), DeltaTime );
                UpdateAnalog( pUser->LStickLeft[i],     input_GetValue( INPUT_PS2_STICK_LEFT_X, i ), DeltaTime );
                UpdateAnalog( pUser->LStickRight[i],    input_GetValue( INPUT_PS2_STICK_LEFT_X, i ), DeltaTime );
*/

                // Keep index of last controller that pressed a select button so we can hack
                // the controller number into the players controller for 1 player games
                if( pUser->PadSelect[i].nPresses > 0 )
                {
                    g_uiLastSelectController = i;
                }
            }
        }
#endif

        // Only do this if there is a target window
        if( pWin )
        {
            // Issue window calls for mouse
            if( (pUser->LastCursorX != pUser->CursorX) || (pUser->LastCursorY != pUser->CursorY) )
            {
                pWin->OnCursorMove( pWin, pUser->CursorX, pUser->CursorY );
            }
            if( pUser->ButtonLB.nPresses  ) pWin->OnLBDown( pWin );
            if( pUser->ButtonLB.nReleases ) pWin->OnLBUp  ( pWin );
            if( pUser->ButtonMB.nPresses  ) pWin->OnMBDown( pWin );
            if( pUser->ButtonMB.nReleases ) pWin->OnMBUp  ( pWin );
            if( pUser->ButtonRB.nPresses  ) pWin->OnRBDown( pWin );
            if( pUser->ButtonRB.nReleases ) pWin->OnRBUp  ( pWin );

#ifdef TARGET_PC
/*          // Can't do this anymore, talk to SULTAN to inquire why.
            for( s32 i = INPUT_KBD_ESCAPE; i <= INPUT_KBD_DELETE; i++)
            {   
                if( input_IsPressed( (input_gadget)i ) )
                    pWin->OnKeyDown( pWin, i );
                else
                    pWin->OnKeyUp( pWin, i );
            }
*/
#endif

            // Sum up button presses
            s32 pDPadUp         = 0;
            s32 pDPadDown       = 0;
            s32 pDPadLeft       = 0;
            s32 pDPadRight      = 0;
            s32 rDPadUp         = 0;
            s32 rDPadDown       = 0;
            s32 rDPadLeft       = 0;
            s32 rDPadRight      = 0;
            s32 tDPadUp         = 0;
            s32 tDPadDown       = 0;
            s32 tDPadLeft       = 0;
            s32 tDPadRight      = 0;
            s32 PadSelect       = 0;
            s32 PadBack         = 0;
            s32 PadDelete       = 0;
            s32 PadShoulderL    = 0;
            s32 PadShoulderR    = 0;
            s32 PadShoulderL2   = 0;
            s32 PadShoulderR2   = 0;
            {
                s32 i;
                for( i=StartController ; i<=EndController ; i++ )
                {
                    pDPadUp         += pUser->DPadUp[i].nPresses;
                    pDPadDown       += pUser->DPadDown[i].nPresses;
                    pDPadLeft       += pUser->DPadLeft[i].nPresses;
                    pDPadRight      += pUser->DPadRight[i].nPresses;
                    rDPadUp         += pUser->DPadUp[i].nRepeats;
                    rDPadDown       += pUser->DPadDown[i].nRepeats;
                    rDPadLeft       += pUser->DPadLeft[i].nRepeats;
                    rDPadRight      += pUser->DPadRight[i].nRepeats;
                    tDPadUp         += pUser->DPadUp[i].nPresses       + pUser->DPadUp[i].nRepeats;
                    tDPadDown       += pUser->DPadDown[i].nPresses     + pUser->DPadDown[i].nRepeats;
                    tDPadLeft       += pUser->DPadLeft[i].nPresses     + pUser->DPadLeft[i].nRepeats;
                    tDPadRight      += pUser->DPadRight[i].nPresses    + pUser->DPadRight[i].nRepeats;
                    PadSelect       += pUser->PadSelect[i].nPresses;
                    PadBack         += pUser->PadBack[i].nPresses;
                    PadDelete       += pUser->PadDelete[i].nPresses;
                    PadShoulderL    += pUser->PadShoulderL[i].nPresses + pUser->PadShoulderL[i].nRepeats;
                    PadShoulderR    += pUser->PadShoulderR[i].nPresses + pUser->PadShoulderR[i].nRepeats;
                    PadShoulderL2   += pUser->PadShoulderL2[i].nPresses + pUser->PadShoulderL2[i].nRepeats;
                    PadShoulderR2   += pUser->PadShoulderR2[i].nPresses + pUser->PadShoulderR2[i].nRepeats;

                    pDPadUp         += pUser->LStickUp[i].nPresses;
                    pDPadDown       += pUser->LStickDown[i].nPresses;
                    pDPadLeft       += pUser->LStickLeft[i].nPresses;
                    pDPadRight      += pUser->LStickRight[i].nPresses;
                    rDPadUp         += pUser->LStickUp[i].nRepeats;
                    rDPadDown       += pUser->LStickDown[i].nRepeats;
                    rDPadLeft       += pUser->LStickLeft[i].nRepeats;
                    rDPadRight      += pUser->LStickRight[i].nRepeats;
                    tDPadUp         += pUser->LStickUp[i].nPresses    + pUser->LStickUp[i].nRepeats;
                    tDPadDown       += pUser->LStickDown[i].nPresses  + pUser->LStickDown[i].nRepeats;
                    tDPadLeft       += pUser->LStickLeft[i].nPresses  + pUser->LStickLeft[i].nRepeats;
                    tDPadRight      += pUser->LStickRight[i].nPresses + pUser->LStickRight[i].nRepeats;
                }
            }

            // Issue window calls for pad navigation
            if( tDPadUp    ) { Iterate = TRUE; pWin->OnPadNavigate( pWin, NAV_UP,    pDPadUp,    rDPadUp    ); };
            if( tDPadDown  ) { Iterate = TRUE; pWin->OnPadNavigate( pWin, NAV_DOWN,  pDPadDown,  rDPadDown  ); };
            if( tDPadLeft  ) { Iterate = TRUE; pWin->OnPadNavigate( pWin, NAV_LEFT,  pDPadLeft,  rDPadLeft  ); };
            if( tDPadRight ) { Iterate = TRUE; pWin->OnPadNavigate( pWin, NAV_RIGHT, pDPadRight, rDPadRight ); };

            // Issue window calls for pad select / back / help
            if( PadSelect ) { Iterate = TRUE; pWin->OnPadSelect( pWin ); };
            if( PadBack   ) { Iterate = TRUE; pWin->OnPadBack  ( pWin ); };
            if( PadDelete ) { Iterate = TRUE; pWin->OnPadDelete( pWin ); };

            // Issue window calls for pad shoulders
            if( PadShoulderL ) { pWin->OnPadShoulder ( pWin, -1 ); };
            if( PadShoulderR ) { pWin->OnPadShoulder ( pWin,  1 ); };
            if( PadShoulderL2) { pWin->OnPadShoulder2( pWin, -1 ); };
            if( PadShoulderR2) { pWin->OnPadShoulder2( pWin,  1 ); };
        }

        // Save Last Cursor Position for next time around
        pUser->LastCursorX = pUser->CursorX;
        pUser->LastCursorY = pUser->CursorY;

        // Clear DeltaTime in case of next iteration
        DeltaTime = 0.0f;
    } while( Iterate );

    // Do Global inputs
#ifdef TARGET_PC
    if( input_IsPressed( INPUT_MSG_EXIT ) )
        return FALSE;
#endif

#if 0
    // We want to wait till all the input calls have been made before releasing the dialog stack.
    while ( m_KillDialogStack.GetCount() > 0 )
    {
        // Get dialog pointer
        ui_dialog* pDialog = m_KillDialogStack[0];
        pDialog->Destroy();
        delete pDialog;
        m_KillDialogStack.Delete( 0 );
    }
#endif

    // Return TRUE if not exiting
    return TRUE;
}

//=========================================================================

void ui_manager::EnableUserInput( void )
{
    m_EnableUserInput = TRUE;
}

//=========================================================================

RVA(0x2a0f10, 0x13b)
void ui_manager::DisableUserInput( void )
{
    m_EnableUserInput = FALSE;

    for( s32 i=0 ; i<m_Users.GetCount() ; i++ )
    {
        user* pUser = m_Users[i];
        ASSERT( pUser );

        for( s32 j=0 ; j<2 ; j++ )
        {
            pUser->DPadDown[j]    .Clear();
            pUser->DPadLeft[j]    .Clear();
            pUser->DPadRight[j]   .Clear();
            pUser->DPadUp[j]      .Clear();
            pUser->LStickDown[j]  .Clear();
            pUser->LStickLeft[j]  .Clear();
            pUser->LStickRight[j] .Clear();
            pUser->LStickUp[j]    .Clear();
            pUser->PadSelect[j]   .Clear();
            pUser->PadBack[j]     .Clear();
            pUser->PadShoulderL[j].Clear();
            pUser->PadShoulderR[j].Clear();
        }
    }
}

//=========================================================================

void ui_manager::Update( f32 DeltaTime )
{
    // Update AlphaTime
    m_AlphaTime += DeltaTime;
    m_AlphaTime = x_fmod( m_AlphaTime, 1.0f );

    // Loop through each user
    for( s32 i=0 ; i<m_Users.GetCount() ; i++ )
    {
        user* pUser = m_Users[i];
        ASSERT( pUser );

        // Only update enabled users
        if( pUser->Enabled )
        {
            // Update all Dialogs on Stack
            for( s32 j=0 ; j<pUser->DialogStack.GetCount() ; j++ )
            {
                pUser->DialogStack[j]->OnUpdate( pUser->DialogStack[j], DeltaTime );
            }
        }
    }
}

//=========================================================================

void ui_manager::Render( void )
{
    s32 i;

    eng_Begin( "UI" );

#ifdef TARGET_PC

    SetRes();
    xbool RenderCursor = FALSE;

#endif

    // Loop through each user to render
    for( i=0 ; i<m_Users.GetCount() ; i++ )
    {
        user* pUser = m_Users[i];
        ASSERT( pUser );

        // Only render enabled users
        if( pUser->Enabled )
        {
#ifdef TARGET_PC
            // If there are visible dialogs, render the stack
            if (pUser->DialogStack.GetCount())
                RenderCursor = TRUE;
#endif            
            // Render Background
            if( pUser->iBackground != -1 )
            {
                RenderBackground( pUser->iBackground );
            }

            // Find Topmost Render Modal Dialog
            s32 j = pUser->DialogStack.GetCount()-1;
            while( (j > 0) && !(pUser->DialogStack[j]->GetFlags() & ui_win::WF_RENDERMODAL) )
                j--;

            // Make sure we start with a legal dialog
            if( j < 0 ) j = 0;

            // Render all Dialogs from the Render Modal one
            for( ; j<pUser->DialogStack.GetCount() ; j++ )
            {
                pUser->DialogStack[j]->Render( pUser->Bounds.l, pUser->Bounds.t );
            }

/*
            // Render cursor
            if( pUser->CursorVisible )
            {
                rect r;
                r.Set( (f32)(pUser->CursorX), (f32)(pUser->CursorY), (f32)(pUser->CursorX+1), (f32)(pUser->CursorY+1) );
                draw_Rect( r, XCOLOR_GREEN, FALSE );
            }
*/
        }
    }

//#ifndef TARGET_PC
    // Render Highlights
    f32 Alpha = (1.0f + x_sin( R_360 * m_AlphaTime )) * 0.5f;
    for( i=0 ; i<m_Highlights.GetCount() ; i++ )
    {
        highlight& h = m_Highlights[i];
        if( h.Flash )
            RenderElement( h.iElement, h.r, 0, xcolor(255,255,255,(s32)(32+Alpha*128)), TRUE );
        else
            RenderElement( h.iElement, h.r, 0, xcolor(255,255,255,(s32)(32+128)      ), TRUE );
    }
//#endif

    // Clear highlights list
    m_Highlights.Clear();

#ifdef TARGET_PC
    
    // Only render the curosr if the dialogs are visible.
    if( RenderCursor )
    {
        irect r;
        POINT   Pos;

        // Get the last user.
        user* pUser = m_Users[m_Users.GetCount()-1];

        Pos.x = pUser->CursorX;
        Pos.y = pUser->CursorY;
//        x_printfxy(10,10,"X:%d, Y:%d", Pos.x, Pos.y);
        
        // Set position to draw the sprite.
        r.Set( Pos.x, Pos.y, Pos.x+m_Mouse.GetWidth(), Pos.y+m_Mouse.GetHeight() );

        draw_Begin( DRAW_SPRITES, DRAW_USE_ALPHA|DRAW_TEXTURED|DRAW_2D|DRAW_NO_ZBUFFER );

        draw_SetTexture( m_Mouse );

        draw_DisableBilinear();
        draw_Sprite( vector3((f32)r.l,(f32)r.t, 0.0f), vector2((f32)m_Mouse.GetWidth(),(f32)m_Mouse.GetHeight()), m_MouseColor );
        draw_End( );
    }
#endif

    eng_End();
}

//=========================================================================

RVA(0x2a13b0, 0x75)
xbool ui_manager::RegisterDialogClass( const char* ClassName, dialog_tem* pDialogTem, ui_pfn_dlgfact pFactory )
{
    xbool   Success = FALSE;
    s32     iFound = -1;
    s32     i;

    // Find the winclass entry
    for( i=0 ; i<m_DialogClasses.GetCount() ; i++ )
    {
        if( m_DialogClasses[i].ClassName == ClassName )
        {
            iFound = i;
        }
    }

    // If not found then add a new one
    if( iFound == -1 )
    {
        dialogclass& dc = m_DialogClasses.Append();
        dc.ClassName  = ClassName;
        dc.pDialogTem = pDialogTem;
        dc.pFactory   = pFactory;
        Success = TRUE;
    }

    // Return success code
    return Success;
}

//=========================================================================

ui_dialog* ui_manager::OpenDialog( s32 UserID, const char* ClassName, irect Position, ui_win* pParent, s32 Flags, void* pUserData )
{
    s32             i;
    ui_dialog*      pDialog     = NULL;
    ui_pfn_dlgfact  pFactory    = NULL;
    dialog_tem*     pDialogTem  = NULL;

    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );
    user* pUser = (user*)UserID;

    // Find the dialogclass entry
    for( i=0 ; i<m_DialogClasses.GetCount() ; i++ )
    {
        if( m_DialogClasses[i].ClassName == ClassName )
        {
            pFactory   = m_DialogClasses[i].pFactory;
            pDialogTem = m_DialogClasses[i].pDialogTem;
        }
    }

    // If Found
    if( pFactory )
    {
        // Check for centering the dialog
        if( Flags & ui_win::WF_DLG_CENTER )
        {
#ifdef TARGET_PC
            irect    b(0,0,512,448);
            Position.Translate( b.l + (b.GetWidth ()-Position.GetWidth ())/2 - Position.l,
                                b.t + (b.GetHeight()-Position.GetHeight())/2 - Position.t );
#else
            irect b = GetUserBounds( UserID );
            b.Translate( -b.l, -b.t );
            Position.Translate( b.l + (b.GetWidth ()-Position.GetWidth ())/2 - Position.l,
                                b.t + (b.GetHeight()-Position.GetHeight())/2 - Position.t );
#endif
        }


        irect CreatePosition = Position;
        s32 XRes, YRes;
        eng_GetRes( XRes, YRes );
                
#ifdef TARGET_PC
        // Adjust the position of the dialogs according to the resolution.
        if( (pParent == NULL)  )
        {
            s32 midX = XRes>>1;
            s32 midY = YRes>>1;

            s32 dx = midX - 256;
            s32 dy = midY - 256;

            Position.Translate( dx, dy );
        }
#endif
                                    
        // Create the Dialog Window
        pDialog = (ui_dialog*)pFactory( UserID, this, pDialogTem, Position, pParent, Flags, pUserData );
        ASSERT( pDialog );

        pDialog->m_CreatePosition = CreatePosition;
        pDialog->m_XRes = XRes;
        pDialog->m_YRes = YRes;

        // If this is not a TAB dialog page
        if( !(pDialog->GetFlags() & ui_win::WF_TAB) )
        {
            // Add to the Dialog Stack
            if( pParent == NULL )
                pUser->DialogStack.Append() = pDialog;

            // Activate the dialog if it has controls
            if( !(Flags & ui_win::WF_NO_ACTIVATE) && (pDialog->m_Children.GetCount() > 0) )
                pDialog->GotoControl( );
        }
    }

    // Return pointer to new dialog
    return pDialog;
}

//=========================================================================

RVA(0x002a16c0, 0x9c)
void ui_manager::EndDialog( s32 UserID, xbool ResetCursor )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );
    user*   pUser = (user*)UserID;
    s32     Count = pUser->DialogStack.GetCount();

    // Check if there are any dialogs to end
    if( Count > 0 )
    {
        // Store the dialog to a kill stack, which will get released after the input has been processed.
//        m_KillDialogStack.Append() = pUser->DialogStack[Count-1];

        // Get dialog pointer
        ui_dialog* pDialog = pUser->DialogStack[Count-1];

        // Reset the cursor
        if( ResetCursor )
        {
            pUser->CursorX = pDialog->m_OldCursorX;
            pUser->CursorY = pDialog->m_OldCursorY;
        }

        // Clear LastWindow under cursor if it was part of this dialog
        if (pUser->pLastWindowUnderCursor)
        {
            if( (pUser->pLastWindowUnderCursor == (ui_win*)pDialog) ||
                (pUser->pLastWindowUnderCursor->IsChildOf( pDialog )) )
            {
                pUser->pLastWindowUnderCursor = NULL;
            }
        }
        // End the dialog
        pUser->DialogStack.Delete( Count-1 );
        pDialog->Destroy();
        delete pDialog;
    }
}

//=========================================================================

RVA(0x2a1760, 0x2d)
void ui_manager::EndUsersDialogs( s32 UserID )
{
    // Loop until all dialogs gone
    while( GetNumUserDialogs( UserID ) > 0 )
    {
        // End last dialog on stack
        EndDialog( UserID );
    }
}

//=========================================================================

RVA(0x2a1790, 0xd)
s32 ui_manager::GetNumUserDialogs( s32 UserID )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );
    user*   pUser = (user*)UserID;

    // Return the number of stacked dialogs
    return pUser->DialogStack.GetCount();
}

//=========================================================================

RVA(0x2a17a0, 0x20)
ui_dialog* ui_manager::GetTopmostDialog( s32 UserID )
{
    ASSERT( (m_Users.Find( (user*)UserID )) != -1 );
    user*   pUser = (user*)UserID;

    if( pUser->DialogStack.GetCount() > 0 )
        return pUser->DialogStack.GetAt( pUser->DialogStack.GetCount()-1 );
    else
        return NULL;
}

//=========================================================================

RVA(0x2a17c0, 0x1cb)
void ui_manager::PushClipWindow( const irect &r )
{
    s32 X0,Y0,X1,Y1;

    // Read View
    view v = *eng_GetView(0);

    // Save current viewport
    cliprecord& cr = m_ClipStack.Append();
    v.GetViewport( X0, Y0, X1, Y1 );
    cr.r.Set( X0, Y0, X1, Y1 );

#ifdef TARGET_PC
    // Set new viewport & activate
    X0 = r.l;
    Y0 = r.t;
    X1 = r.r;
    Y1 = r.b;
    v.SetViewport( X0,Y0,X1,Y1 );
    eng_SetView( v, 0 );
    eng_ActivateView( 0 );
#endif
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetScissor(r.l,r.t,r.r,r.b);
    eng_PushGSContext(1);
    gsreg_SetScissor(r.l,r.t,r.r,r.b);
    eng_PopGSContext();
    gsreg_End();
#endif
}

//=========================================================================

RVA(0x2a1990, 0xf6)
void ui_manager::PopClipWindow( void )
{
    ASSERT( m_ClipStack.GetCount() > 0 );

    // Read View
    view v = *eng_GetView(0);

    // Read previous viewport from stack
    irect& r = m_ClipStack[m_ClipStack.GetCount()-1].r;

#ifdef TARGET_PC
    // Set new viewport & activate
    s32 X0 = r.l;
    s32 Y0 = r.t;
    s32 X1 = r.r;
    s32 Y1 = r.b;
    v.SetViewport( X0,Y0,X1,Y1 );
    eng_SetView( v, 0 );
    eng_ActivateView( 0 );
#endif
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetScissor(r.l,r.t,r.r,r.b);
    eng_PushGSContext(1);
    gsreg_SetScissor(r.l,r.t,r.r,r.b);
    eng_PopGSContext();
    gsreg_End();
#endif

    // Delete from stack
    m_ClipStack.Delete( m_ClipStack.GetCount()-1 );
}

//=========================================================================

const xwstring& ui_manager::WordWrapString( s32 iFont, const irect& r, const char* pString )
{
    static xwstring s;

    s32 i;
    s32 x           = 0;
    s32 iString     = 0;
    s32 iLineStart  = 0;
    s32 iStringWrap = -1;
    s32 cPrev       = 0;
    s32 c;
    s32 w;

    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );
    ui_font* pFont = m_Fonts[iFont]->pFont;

    // Clear the string
    s.Clear();

    // Word Wrap Text
    while( pString[iString] )
    {
        // Get Character
        c = pString[iString++];

        // Check for end of word
        if( x_isspace(c) && !x_isspace(cPrev) )
        {
            iStringWrap = iString-1;
        }

        // Update previous character
        cPrev = c;

        // Advance cursor before checking wrap
        w = pFont->GetCharacter(c).W;
        x += w+1;

        // Check for NewLine
        if( c == '\n' )
        {
            // Copy String up to wrap point
            for( i=iLineStart ; i<iString ; i++ )
            {
                s += pString[i];
            }
            
            iLineStart  = iString;
            iStringWrap = -1;
            x           = 0;
        }
        else if( x > r.GetWidth() )
        {
            ASSERT( iStringWrap != -1 );

            // Copy String up to wrap point
            for( i=iLineStart ; i<iStringWrap ; i++ )
            {
                s += pString[i];
            }
            s += '\n';

            // Skip Space
            while( x_isspace(pString[i]) )
                i++;

            // Reset line scanner
            iLineStart  = i;
            iString     = i;
            iStringWrap = -1;
            x           = 0;
        }
    }

    // Output last line
    while( iLineStart < iString )
        s += pString[iLineStart++];

    // Return the string
    return s;
}

//=========================================================================

const xwstring& ui_manager::WordWrapString( s32 iFont, const irect& r, const xwstring& String )
{
    static xwstring s;

    s32 i;
    s32 x           = 0;
    s32 iString     = 0;
    s32 iLineStart  = 0;
    s32 iStringWrap = -1;
    s32 cPrev       = 0;
    s32 c;
    s32 w;

    ASSERT( (iFont >= 0) && (iFont < m_Fonts.GetCount()) );
    ui_font* pFont = m_Fonts[iFont]->pFont;

    // Clear the string
    s.Clear();

    // Word Wrap Text
    while( String[iString] )
    {
        // Get Character
        c = String[iString++];

        // Skip Color Codes
        if( (c & 0xff00) == 0xff00 )
        {
            iString++;
        }
        else
        {
            // Check for end of word
            if( x_isspace(c) && !x_isspace(cPrev) )
            {
                iStringWrap = iString-1;
            }

            // Update previous character
            cPrev = c;

            // Advance cursor before checking wrap
            w = pFont->GetCharacter(c).W;
            x += w+1;

            // Check for NewLine
            if( c == '\n' )
            {
                // Copy String up to wrap point
                for( i=iLineStart ; i<iString ; i++ )
                {
                    s += String[i];
                }
            
                iLineStart  = iString;
                iStringWrap = -1;
                x           = 0;
            }
            else if( x > r.GetWidth() )
            {
                ASSERT( iStringWrap != -1 );

                // Copy String up to wrap point
                for( i=iLineStart ; i<iStringWrap ; i++ )
                {
                    s += String[i];
                }
                s += '\n';

                // Skip Space
                while( x_isspace(String[i]) )
                    i++;

                // Reset line scanner
                iLineStart  = i;
                iString     = i;
                iStringWrap = -1;
                x           = 0;
            }
        }
    }

    // Output last line
    while( iLineStart < iString )
        s += String[iLineStart++];

    // Return the string
    return s;
}

//=========================================================================

void ui_manager::SetRes( void )
{
    // Adjust the position of the dialogs according to the resolution.
    s32 XRes, YRes;
    eng_GetRes( XRes, YRes );
    s32 midX = XRes>>1;
    s32 midY = YRes>>1;

    s32 dx = midX - 256;
    s32 dy = midY - 256;

    for( s32 i = 0; i < m_Users.GetCount(); i++)
    {
        user* pUser = m_Users[i];
        
        // Render all Dialogs from the Render Modal one
        for( s32 j = 0; j<pUser->DialogStack.GetCount(); j++)
        {
            // If the resolution of the Dialog don't then we have to reposition them.
            if( pUser->DialogStack[j]->m_XRes != XRes && pUser->DialogStack[j]->m_YRes != YRes )
            {
                pUser->DialogStack[j]->m_Position = pUser->DialogStack[j]->m_CreatePosition;
                pUser->DialogStack[j]->m_Position.Translate( dx, dy );
                pUser->DialogStack[j]->m_XRes = XRes;
                pUser->DialogStack[j]->m_YRes = YRes;
            }
        }        
    }
}

//=========================================================================

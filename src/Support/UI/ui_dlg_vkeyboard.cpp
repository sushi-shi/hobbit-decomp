// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_dlg_vkeyboard.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_dlg_vkeyboard.cpp
//
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <Support/AudioMgr/tribes-aa/audio.hpp>
#include <Support/LabelSets/reference/tribes-aa/Tribes2Types.hpp>

#include <Support/UI/ui_dlg_vkeyboard.hpp>
#include <Support/UI/ui_manager.hpp>
#include <Support/UI/ui_control.hpp>
#include <Support/UI/ui_font.hpp>
#include <Support/UI/ui_frame.hpp>

#include <Support/StringMgr/StringMgr.hpp>

#include <Support/reference/tribes-aa/Demo1/fe_colors.hpp>

#include <Support/reference/tribes-aa/Demo1/Data/UI/ui_strings.h>

//=========================================================================
//  Defines
//=========================================================================

#define KEYW    14
#define KEYH    14
#define NCOLS   17
#define NROWS   5

enum notifications
{
    WN_CHARACTER    = ui_win::WN_USER,
    WN_REFRESH,
};

static u8 Keys[17*5] =
{
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '!',
    '$',
    '^',
    '#',
    '/',
    ':',
    '\\',

    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    'g',
    'h',
    'i',
    'j',
    'k',
    'l',
    'm',
    '@',
    '(',
    '+',
    ')',

    'n',
    'o',
    'p',
    'q',
    'r',
    's',
    't',
    'u',
    'v',
    'w',
    'x',
    'y',
    'z',
    '?',
    '<',
    '=',
    '>',

    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'L',
    'M',
    '~',
    '[',
    '-',
    ']',

    'N',
    'O',
    'P',
    'Q',
    'R',
    'S',
    'T',
    'U',
    'V',
    'W',
    'X',
    'Y',
    'Z',
    '.',
    '{',
    '|',
    '}'
};

//=========================================================================
//  Structs
//=========================================================================

//=========================================================================
//  Data
//=========================================================================

//=========================================================================
//  Virtual Keyboard Dialog
//=========================================================================

enum controls
{
    IDC_CANCEL,
    IDC_ACCEPT,
    IDC_FRAME
};

ui_manager::control_tem vkeyboardControls[] =
{
    { IDC_CANCEL,   IDS_CANCEL, "button",     9, 138+22, 100, 24, 0, 8, 9, 1, ui_win::WF_VISIBLE },
    { IDC_ACCEPT,   IDS_OK,     "button",   162, 138+22, 100, 24, 9, 8, 8, 1, ui_win::WF_VISIBLE },

    { IDC_FRAME,    IDS_NULL,   "frame",      8,  36+22, 254, 98, 0, 0, 0, 0, ui_win::WF_VISIBLE|ui_win::WF_STATIC },
};

ui_manager::dialog_tem vkeyboardDialog =
{
    IDS_VIRTUAL_KEYBOARD_TITLE,
    17, 9,
    sizeof(vkeyboardControls)/sizeof(ui_manager::control_tem),
    &vkeyboardControls[0],
    0
};

//=========================================================================
//  vkey Window
//=========================================================================

class ui_vkey : public ui_control
{
public:
    virtual void    Render              ( s32 ox=0, s32 oy=0 );
    virtual void    OnPadSelect         ( ui_win* pWin );
    virtual void    OnCursorEnter       ( ui_win* pWin );
protected:
};

//=========================================================================

void ui_vkey::Render( s32 ox, s32 oy )
{
    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
        // Render placeholder rectangle
        xcolor  Color       = XCOLOR_BLACK;
        xcolor  TextColor1  = XCOLOR_WHITE;
        xcolor  TextColor2  = XCOLOR_BLACK;
        xbool   ForceRect   = FALSE;

        // Calculate rectangle
        irect    r;
        r.Set( (m_Position.l+ox), (m_Position.t+oy), (m_Position.r+ox), (m_Position.b+oy) );

        xwstring Label;
        Label = m_Label;

        switch( Label[0] )
        {
        case '\010':
//            ForceRect = TRUE;
            Label = g_StringMgr( "ui", IDS_BACKSPACE );
            break;
        case '\040':
//            ForceRect = TRUE;
            Label = g_StringMgr( "ui", IDS_SPACE );
            break;
        }

        // Render appropriate state
        if( m_Flags & WF_DISABLED )
        {
            Color      = xcolor(0,0,0,0);
            TextColor1 = XCOLOR_GREY;
            TextColor2 = xcolor(0,0,0,0);
        }
        else if( m_Flags & (WF_HIGHLIGHT|WF_SELECTED) )
        {
            Color      = xcolor( 121, 199, 213 );
            TextColor1 = XCOLOR_BLACK;
            TextColor2 = XCOLOR_WHITE;
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;

            m_pManager->AddHighlight( m_UserID, r );
        }
        else
        {
            Color      = xcolor(0,0,0,0);
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;
        }

        if( ForceRect )
            Color = xcolor( 121, 199, 213 );

        r.Inflate( 1, 1 );
        m_pManager->RenderRect( r, Color, TRUE );

        r.Translate(  2, -2 );
        m_pManager->RenderText( m_Font, r, ui_font::h_center|ui_font::v_center, TextColor2, Label );
        r.Translate( -1, -1 );
        m_pManager->RenderText( m_Font, r, ui_font::h_center|ui_font::v_center, TextColor1, Label );
    }
}

//=========================================================================

void ui_vkey::OnPadSelect( ui_win* pWin )
{
    (void)pWin;
    m_pParent->OnNotify( m_pParent, this, WN_CHARACTER, &m_Label );
}

//=========================================================================

void ui_vkey::OnCursorEnter( ui_win* pWin )
{
    (void)pWin;
    m_Flags |= WF_HIGHLIGHT;

    audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
}

//=========================================================================

//=========================================================================
//  vkString Window
//=========================================================================

class ui_vkString : public ui_control
{
public:
    xbool           Create              ( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );
    virtual void    Render              ( s32 ox=0, s32 oy=0 );
    virtual void    OnPadNavigate       ( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats );
    virtual void    OnPadShoulder       ( ui_win* pWin, s32 Direction );
    virtual void    OnPadDelete         ( ui_win* pWin );

    void            Backspace           ( void );
    void            Character           ( const xstring& String );
    s32             GetCursorPos        ( void );
    void            SetCursorPos        ( s32 Pos );

protected:
    s32             m_iElement;
    s32             m_Cursor;
};

//=========================================================================

xbool ui_vkString::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success;

    Success = ui_control::Create( UserID, pManager, Position, pParent, Flags );

    // Initialize Data
    m_iElement = m_pManager->FindElement( "frame" );
    ASSERT( m_iElement != -1 );
    
    m_Cursor = 0;

    return Success;
}

//=========================================================================

static s32 s_CursorFrame = 0;

void ui_vkString::Render( s32 ox, s32 oy )
{
    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
        // Render placeholder rectangle
        xcolor  Color       = XCOLOR_BLACK;
        xcolor  TextColor1  = XCOLOR_WHITE;
        xcolor  TextColor2  = XCOLOR_BLACK;

        // Calculate rectangle
        irect    r;
        r.Set( (m_Position.l+ox), (m_Position.t+oy), (m_Position.r+ox), (m_Position.b+oy) );

        // Render appropriate state
        if( m_Flags & WF_DISABLED )
        {
            Color      = xcolor(0,0,0,0);
            TextColor1 = XCOLOR_GREY;
            TextColor2 = xcolor(0,0,0,0);
        }
        else if( m_Flags & (WF_HIGHLIGHT|WF_SELECTED) )
        {
            Color      = xcolor( 121, 199, 213 );
            TextColor1 = XCOLOR_BLACK;
            TextColor2 = XCOLOR_WHITE;
        }
        else
        {
            Color      = xcolor(25,79,103,255);
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;
        }

        // Render color rectangle and frame
        irect   rb = r;
        rb.Deflate( 1, 1 );
        m_pManager->RenderRect( rb, Color, FALSE );
        m_pManager->RenderElement( m_iElement, r, 0 );

        // Set a clip window to render the text
        rb.Deflate( 4, 1 );
        m_pManager->PushClipWindow( rb );

        // Render Text
        irect rt = rb;
        rt.l += 1;
        rt.r -= 3;
        rt.Translate(  3, -2 );
        m_pManager->RenderText( m_Font, rt, ui_font::h_left|ui_font::v_center|ui_font::clip_r_justify, TextColor2, m_Label );
        rt.Translate( -1, -1 );
        m_pManager->RenderText( m_Font, rt, ui_font::h_left|ui_font::v_center|ui_font::clip_r_justify, TextColor1, m_Label );

        // Render Cursor
        irect Rect;
        m_pManager->TextSize( m_Font, Rect, m_Label, m_Cursor );
        rb.l = rt.l + Rect.r - 1;
        rb.r = rb.l + 1;
        rb.Deflate( 0, 2 );

        if( !(s_CursorFrame & 0x10) )
            m_pManager->RenderRect( rb, TextColor1, TRUE );
        s_CursorFrame++;

        // Clear the clip window
        m_pManager->PopClipWindow();
    }
}

//=========================================================================

void ui_vkString::OnPadNavigate( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats )
{
    // Which way are we moving
    switch( Code )
    {
    case ui_manager::NAV_LEFT:
        if( m_Cursor > 0 )
        {
            m_Cursor--;
            s_CursorFrame = 0;
        }
        else
            audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
        break;
    case ui_manager::NAV_RIGHT:
        if( m_Cursor < m_Label.GetLength() )
        {
            m_Cursor++;
            s_CursorFrame = 0;
        }
        else
            audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
        break;
    default:
        ui_control::OnPadNavigate( pWin, Code, Presses, Repeats );
    }
}

//=========================================================================

void ui_vkString::OnPadShoulder( ui_win* pWin, s32 Direction )
{
    // Which way are we moving
    switch( Direction )
    {
    case -1:
        if( m_Cursor > 0 )
        {
            m_Cursor--;
            s_CursorFrame = 0;
        }
        else
            audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
        break;
    case 1:
        if( m_Cursor < m_Label.GetLength() )
        {
            m_Cursor++;
            s_CursorFrame = 0;
        }
        else
            audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
        break;
    default:
        ui_control::OnPadShoulder( pWin, Direction );
    }
}

//=========================================================================

void ui_vkString::OnPadDelete( ui_win* pWin )
{
    (void)pWin;
    if( m_Cursor > 0 )
    {
        Backspace();
        m_pParent->OnNotify( m_pParent, this, WN_REFRESH, NULL );
    }
}

//=========================================================================

void ui_vkString::Backspace( void )
{
    ASSERT( m_Cursor > 0 );

    xstring NewString;
    NewString = m_Label.Left( m_Cursor-1 );
    NewString += m_Label.Right( m_Label.GetLength()-m_Cursor );
    m_Label = NewString;
    m_Cursor--;
    s_CursorFrame = 0;
}

void ui_vkString::Character( const xstring& String )
{
    xstring NewString;
    NewString = m_Label.Left( m_Cursor );
    NewString += String;
    NewString += m_Label.Right( m_Label.GetLength()-m_Cursor );
    m_Label = NewString;
    m_Cursor++;
    s_CursorFrame = 0;
}

s32 ui_vkString::GetCursorPos( void )
{
    return m_Cursor;
}

void ui_vkString::SetCursorPos( s32 Pos )
{
    if( Pos < 0 )
        Pos = 0;
    if( Pos > m_Label.GetLength() )
        Pos = m_Label.GetLength();
    m_Cursor = Pos;
}

//=========================================================================
//  Factory function
//=========================================================================

ui_win* ui_dlg_vkeyboard_factory( s32 UserID, ui_manager* pManager, ui_manager::dialog_tem* pDialogTem, const irect& Position, ui_win* pParent, s32 Flags, void* pUserData )
{
    ui_dlg_vkeyboard* pDialog = new ui_dlg_vkeyboard;
    pDialog->Create( UserID, pManager, pDialogTem, Position, pParent, Flags, pUserData );

    return (ui_win*)pDialog;
}

//=========================================================================
//  ui_dlg_vkeyboard
//=========================================================================

ui_dlg_vkeyboard::ui_dlg_vkeyboard( void )
{
    m_pResultDone = NULL;
    m_pResultOk   = NULL;
}

//=========================================================================

ui_dlg_vkeyboard::~ui_dlg_vkeyboard( void )
{
    Destroy();
}

//=========================================================================

xbool ui_dlg_vkeyboard::Create( s32                        UserID,
                                ui_manager*                pManager,
                                ui_manager::dialog_tem*    pDialogTem,
                                const irect&               Position,
                                ui_win*                    pParent,
                                s32                        Flags,
                                void*                      pUserData )
{
    xbool   Success = FALSE;
    s32     x,y;


    (void)pDialogTem;
    (void)pUserData;

    ASSERT( pManager );

    // Set Size of window
    irect r( 0, 0, 270, 170+22 );
    r.Translate( Position.GetWidth()/2-r.GetWidth()/2, Position.GetHeight()/2-r.GetHeight()/2 );

    // Do dialog creation
    Success = ui_dialog::Create( UserID, pManager, &vkeyboardDialog, r, pParent, Flags );

    // Setup Frame
    ((ui_frame*)m_Children[2])->SetBackgroundColor( xcolor(25,79,103,255) );

    // Add custom controls to Dialog
    {
        s32 i = 0;
        for( y=0 ; y<NROWS ; y++ )
        {
            for( x=0 ; x<NCOLS ; x++ )
            {
                // Create each control
                irect Pos;
                Pos.Set( 16+x*KEYW, 22+42+y*KEYH, 16+x*KEYW+KEYW, 22+42+y*KEYH+KEYH );
                ui_vkey* pVKey = new ui_vkey;
                ASSERT( pVKey );
                pVKey->Create( m_UserID, m_pManager, Pos, this, ui_win::WF_VISIBLE );

                // Configure the control
                pVKey->SetLabel( xwstring(xfs("%c",Keys[i])) );
                pVKey->SetNavPos( irect( x, y+1, x+1, y+2 ) );
                m_NavGraph[x+(y+1)*NCOLS] = pVKey;
                i++;
            }
        }
    }

    // Add backspace
    {
        s32 x=0;
        s32 y=5;
        irect Pos;
        Pos.Set( 16+x*KEYW, 22+4+42+y*KEYH, 16+(x+6)*KEYW+KEYW, 22+4+42+y*KEYH+KEYH );
        ui_vkey* pVKey = new ui_vkey;
        ASSERT( pVKey );
        pVKey->Create( m_UserID, m_pManager, Pos, this, ui_win::WF_VISIBLE );
        pVKey->SetLabel( "\010" );
        pVKey->SetNavPos( irect( x, y+1, x+9, y+2 ) );
        for( x=0 ; x<9 ; x++ )
            m_NavGraph[x+(y+1)*NCOLS] = pVKey;
    }

    // Add space
    {
        s32 x=10;
        s32 y=5;
        irect Pos;
        Pos.Set( 16+x*KEYW, 22+4+42+y*KEYH, 16+(x+6)*KEYW+KEYW, 22+4+42+y*KEYH+KEYH );
        ui_vkey* pVKey = new ui_vkey;
        ASSERT( pVKey );
        pVKey->Create( m_UserID, m_pManager, Pos, this, ui_win::WF_VISIBLE );
        pVKey->SetLabel( "\040" );
        pVKey->SetNavPos( irect( x, y+1, x+8, y+2 ) );
        for( x=9 ; x<(9+8) ; x++ )
            m_NavGraph[x+(y+1)*NCOLS] = pVKey;
    }

    // Add String control to dialog
    m_pStringCtrl = new ui_vkString;
    ASSERT( m_pStringCtrl );
    irect Pos( 8, 22+8, 8+254, 22+8+24 );
    m_pStringCtrl->Create( m_UserID, m_pManager, Pos, this, ui_win::WF_VISIBLE );
    for( x=0 ; x<m_NavW ; x++ )
        m_NavGraph[x] = m_pStringCtrl;

    // Initialize Data
    m_iElement = m_pManager->FindElement( "frame" );
    ASSERT( m_iElement != -1 );
    m_BackgroundColor   = FECOL_DIALOG2;
    m_pString       = NULL;
    m_MaxCharacters = -1;

    // Return success code
    return Success;
}

//=========================================================================

void ui_dlg_vkeyboard::Render( s32 ox, s32 oy )
{
    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
        xcolor  Color       = XCOLOR_WHITE;

        // Set color if highlighted or selected or disabled
        if( m_Flags & WF_DISABLED )
            Color = XCOLOR_GREY;
        if( m_Flags & (WF_HIGHLIGHT|WF_SELECTED) )
            Color = XCOLOR_RED;

        // Get window rectangle
        irect   r;
        r.Set( m_Position.l+ox, m_Position.t+oy, m_Position.r+ox, m_Position.b+oy );

        // Render background color
        if( m_BackgroundColor.A > 0 )
        {
            irect   rb = r;
            rb.Deflate( 1, 1 );
            m_pManager->RenderRect( rb, m_BackgroundColor, FALSE );
/*
            irect   r1 = rb;
            irect   r2 = rb;
            irect   r3 = rb;
            s32     s = MIN( 128, r1.GetHeight()/2 );
            r1.b = r1.t + s;
            r2.t = r1.b;
            r3.t = r3.b - s;
            r2.b = r3.t;
            xcolor  c1 = m_BackgroundColor;
            xcolor  c2 = m_BackgroundColor;
            c1.A = 224;
            c2.A = 128;
            m_pManager->RenderGouraudRect( r1, c1,c2,c2,c1, FALSE );
            m_pManager->RenderGouraudRect( r2, c2,c2,c2,c2, FALSE );
            m_pManager->RenderGouraudRect( r3, c2,c1,c1,c2, FALSE );
*/
        }

        // Render Title
        irect rect = r;
        rect.Deflate( 1, 1 );
        rect.SetHeight( 22 );
        xcolor c1 = FECOL_TITLE1;
        xcolor c2 = FECOL_TITLE2;
        m_pManager->RenderGouraudRect( rect, c1, c1, c2, c2, FALSE );

        rect.Deflate( 8, 0 );
        rect.Translate( 1, -1 );
        m_pManager->RenderText( 0, rect, ui_font::h_left|ui_font::v_center, XCOLOR_BLACK, m_Label );
        rect.Translate( -1, -1 );
        m_pManager->RenderText( 0, rect, ui_font::h_left|ui_font::v_center, XCOLOR_WHITE, m_Label );

        // Render frame
        m_pManager->RenderElement( m_iElement, r, 0 );

        // Render children
        for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
        {
            m_Children[i]->Render( m_Position.l+ox, m_Position.t+oy );
        }
    }
}

//=========================================================================

void ui_dlg_vkeyboard::OnPadNavigate( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats )
{
    ui_dialog::OnPadNavigate( pWin, Code, Presses, Repeats );
}

//=========================================================================

void ui_dlg_vkeyboard::OnPadShoulder( ui_win* pWin, s32 Direction )
{
    m_pStringCtrl->OnPadShoulder( pWin, Direction );
}

//=========================================================================

void ui_dlg_vkeyboard::OnPadDelete( ui_win* pWin )
{
    m_pStringCtrl->OnPadDelete( pWin );
}

//=========================================================================

void ui_dlg_vkeyboard::OnPadSelect( ui_win* pWin )
{
    s32 iChild = m_Children.Find( pWin );
    ASSERT( iChild != -1 );

    // Cancel
    if( iChild == 0 )
    {
        // Undo changes
        if( m_pString )
            *m_pString = m_BackupString;

        if( m_pResultOk )
            *m_pResultOk   = FALSE;
        if( m_pResultDone )
            *m_pResultDone = TRUE;

        // Close dialog
        m_pManager->EndDialog( m_UserID, TRUE );
    }

    // Accept
    if( iChild == 1 )
    {
        if( m_pResultOk )
            *m_pResultOk   = TRUE;
        if( m_pResultDone )
            *m_pResultDone = TRUE;

        // Close dialog
        m_pManager->EndDialog( m_UserID, TRUE );
    }
}

//=========================================================================

void ui_dlg_vkeyboard::OnPadBack( ui_win* pWin )
{
    (void)pWin;

    // Undo changes
    if( m_pString )
        *m_pString = m_BackupString;

    // Close dialog
    m_pManager->EndDialog( m_UserID, TRUE );
}

//=========================================================================

void ui_dlg_vkeyboard::OnNotify( ui_win* pWin, ui_win* pSender, s32 Command, void* pData )
{
    (void)pWin;
    (void)pSender;

    switch( Command )
    {
    case WN_CHARACTER:
        {
            ASSERT( pData );

            // Update String
            xstring*    pString = (xstring*)pData;
            if( pString->GetAt(0) == '\010' )
            {
                if( m_pStringCtrl->GetCursorPos() > 0 )
                    m_pStringCtrl->Backspace();
                else
                    audio_Play( SFX_FRONTEND_ERROR,AUDFLAG_CHANNELSAVER );
            }
            else
            {
                if( (m_MaxCharacters == -1) || (m_pStringCtrl->GetLabel().GetLength() < m_MaxCharacters) )
                {
//                    m_pStringCtrl->SetLabel( m_pStringCtrl->GetLabel() + *pString );
                    m_pStringCtrl->Character( *pString );
                }
                else
                    audio_Play( SFX_FRONTEND_ERROR,AUDFLAG_CHANNELSAVER );
            }

            // Update connected string
            if( m_pString )
                *m_pString = m_pStringCtrl->GetLabel();
        }
        break;
    case WN_REFRESH:
        {
            // Update connected string
            if( m_pString )
                *m_pString = m_pStringCtrl->GetLabel();
        }
    };
}

//=========================================================================

void ui_dlg_vkeyboard::ConnectString( xwstring* pString, s32 MaxCharacters )
{
    ASSERT( pString );

    // Initialize all strings and pointers
    m_pString = pString;
    m_BackupString = *pString;
    m_pStringCtrl->SetLabel( *pString );
    m_pStringCtrl->SetCursorPos( pString->GetLength() );
    m_MaxCharacters = MaxCharacters;
}

//=========================================================================

void ui_dlg_vkeyboard::SetReturn( xbool* pDone, xbool* pOk )
{
    m_pResultDone = pDone;
    m_pResultOk   = pOk;
}

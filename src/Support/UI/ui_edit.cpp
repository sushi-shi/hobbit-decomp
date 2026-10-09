// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_edit.cpp; complete Area51 variant available in external entropy-src donor.
//=========================================================================
//
//  ui_edit.cpp
//
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <Support/UI/ui_edit.hpp>
#include <Support/UI/ui_manager.hpp>
#include <Support/UI/ui_font.hpp>
#include <Support/UI/ui_dlg_vkeyboard.hpp>

//=========================================================================
//  Defines
//=========================================================================

//=========================================================================
//  Structs
//=========================================================================

//=========================================================================
//  Data
//=========================================================================

//=========================================================================
//  Factory function
//=========================================================================

ui_win* ui_edit_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    ui_edit* pedit = new ui_edit;
    pedit->Create( UserID, pManager, Position, pParent, Flags );

    return (ui_win*)pedit;
}

//=========================================================================
//  ui_edit
//=========================================================================

ui_edit::ui_edit( void )
{
}

//=========================================================================

ui_edit::~ui_edit( void )
{
    Destroy();
}

//=========================================================================

xbool ui_edit::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success;

    Success = ui_control::Create( UserID, pManager, Position, pParent, Flags );

    // Initialize data
    m_iElement1 = m_pManager->FindElement( "button_edit" );
    m_iElement2 = m_pManager->FindElement( "button_edit" );
    ASSERT( m_iElement1 != -1 );
    ASSERT( m_iElement2 != -1 );
    m_LabelWidth    = 0;
    m_MaxCharacters = -1;

#ifdef TARGET_PC
    m_CursorPosition = m_Label.GetLength();
    KeyFlags = 0;
    // Usually the numlock is on.
    KeyFlags |= NUMLOCK;
#endif
    return Success;
}

//=========================================================================

void ui_edit::Render( s32 ox, s32 oy )
{
    s32     State = ui_manager::CS_NORMAL;

    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
//        xcolor  LabelColor1 = XCOLOR_WHITE;
//        xcolor  LabelColor2 = XCOLOR_BLACK;
        xcolor  TextColor1  = XCOLOR_WHITE;
        xcolor  TextColor2  = XCOLOR_BLACK;

        // Calculate rectangle
        irect    r, r2;
        r.Set( (m_Position.l+ox), (m_Position.t+oy), (m_Position.r+ox), (m_Position.b+oy) );
        r2 = r;
//        r.r = r.l + m_LabelWidth;
//        r2.l = r.r;

        // Render appropriate state
        if( m_Flags & WF_DISABLED )
        {
            State = ui_manager::CS_DISABLED;
            TextColor1 = XCOLOR_GREY;
            TextColor2 = xcolor(0,0,0,0);
        }
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == WF_HIGHLIGHT )
        {
            State = ui_manager::CS_HIGHLIGHT;
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;
        }
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == WF_SELECTED )
        {
            State = ui_manager::CS_SELECTED;
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;
        }
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == (WF_HIGHLIGHT|WF_SELECTED) )
        {
            State = ui_manager::CS_HIGHLIGHT_SELECTED;
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;
        }
        else
        {
            State = ui_manager::CS_NORMAL;
            TextColor1 = XCOLOR_WHITE;
            TextColor2 = XCOLOR_BLACK;
        }
        m_pManager->RenderElement( m_iElement2, r2, State );

        // Add Highlight to list
        if( m_Flags & WF_HIGHLIGHT )
            m_pManager->AddHighlight( m_UserID, r );

        // Render Label Text
//        r.Translate( 1-3, -2 );
//        m_pManager->RenderText( m_Font, r, ui_font::h_center|ui_font::v_center, LabelColor2, m_Label );
//        r.Translate( -1, -1 );
//        m_pManager->RenderText( m_Font, r, ui_font::h_center|ui_font::v_center, LabelColor1, m_Label );

/*        // Render Edit Text
        {
            r2.Deflate( 4, 0 );
            r2.Translate( 1, -1 );
            m_pManager->RenderText( m_Font, r2, ui_font::h_center|ui_font::v_center|ui_font::clip_character|ui_font::clip_l_justify, TextColor2, m_Label );
            r2.Translate( -1, -1 );
            m_pManager->RenderText( m_Font, r2, ui_font::h_center|ui_font::v_center|ui_font::clip_character|ui_font::clip_l_justify, TextColor1, m_Label );
        }
*/
        // Set a clip window to render the text
        r2.Deflate( 4, 1 );
        m_pManager->PushClipWindow( r2 );

        // Render Text
        irect rt = r2;
        rt.l += 1;
        rt.r -= 3;
        rt.Translate(  3, -2 );
        m_pManager->RenderText( m_Font, rt, ui_font::h_center|ui_font::v_center|ui_font::clip_ellipsis|ui_font::clip_l_justify, TextColor2, m_Label );
        rt.Translate( -1, -1 );
        m_pManager->RenderText( m_Font, rt, ui_font::h_center|ui_font::v_center|ui_font::clip_ellipsis|ui_font::clip_l_justify, TextColor1, m_Label );

        // Clear the clip window
        m_pManager->PopClipWindow();

#ifdef TARGET_PC
#if 0
        s32 tx;
        s32 Width = m_Label.GetLength();
        // Adjust lateral position for alignment flags.
        tx = r.l + (r.GetWidth() - Width) / 2;

        irect rect = r;
        rect.SetHeight( m_pManager->GetLineHeight( m_Font ) );
        rect.l = tx;
        rect.SetWidth( 1 );
        rect.Translate(-20, 0 );
        rect.Translate( (m_CursorPosition)*6 , 6 );
        m_pManager->RenderRect( rect, xcolor(224,224,224,224), FALSE );
#endif
#endif
        // Render children
        for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
        {
            m_Children[i]->Render( m_Position.l+ox, m_Position.t+oy );
        }
    }
}

//=========================================================================

void ui_edit::OnPadSelect( ui_win* pWin )
{
    (void)pWin;


//#ifndef TARGET_PC
    irect   r = m_pManager->GetUserBounds( m_UserID );

    // Open virtual keyboard dialog and connect it to the edit string
    ui_dlg_vkeyboard* pVKeyboard = (ui_dlg_vkeyboard*)m_pManager->OpenDialog( m_UserID, "ui_vkeyboard", r, NULL, ui_win::WF_VISIBLE|ui_win::WF_INPUTMODAL );
    pVKeyboard->ConnectString( &m_Label, m_MaxCharacters );
    pVKeyboard->SetLabel( m_VirtualKeyboardTitle );
//#else
//    m_CursorPosition = m_Label.GetLength()-1;
//#endif
}

//=========================================================================

void ui_edit::SetLabelWidth( s32 Width )
{
    m_LabelWidth = Width;
}

//=========================================================================

void ui_edit::SetMaxCharacters( s32 MaxCharacters )
{
    m_MaxCharacters = MaxCharacters;
}

//=========================================================================

void ui_edit::SetVirtualKeyboardTitle( const xwstring& Title )
{
    m_VirtualKeyboardTitle = Title;
}

/*
//=========================================================================

void ui_edit::SetText( const xstring& Text )
{
    m_Text = Text;
}

//=========================================================================

void ui_edit::SetText( const char* Text )
{
    m_Text = Text;
}

//=========================================================================

const xstring& ui_edit::GetText( void ) const
{
    return m_Text;
}
*/

//=========================================================================

void ui_edit::OnKeyDown ( ui_win* pWin, s32 Key )
{
    (void)pWin;
    (void)Key;
#ifdef TARGET_PC
    
    char type = ConvertGadgetIDToChar( Key );
    
    if( (type != -1) && (m_Label.GetLength() < m_MaxCharacters) )
    {
        m_CursorPosition++;
        
        // To get upper case letter just change subtract the diffrence between lower case and upper case letters.
        if( (type >= 97) && (type <= 122) && ((KeyFlags & CAPSLOCK) || (KeyFlags & LSHIFT) || (KeyFlags & RSHIFT)) )
            type -= 32;
        m_Label += type;
    }

#endif
}

//=========================================================================

void ui_edit::OnKeyUp ( ui_win* pWin, s32 Key )
{
    (void)pWin;
    (void)Key;

#ifdef TARGET_PC
    
    if( m_LastKey == Key )
        m_LastKey = -1;
    
    if( (KeyFlags & LSHIFT) || (KeyFlags & RSHIFT) )
    {
        switch ( Key )
        {
            case  INPUT_KBD_LSHIFT:
                KeyFlags &= ~LSHIFT;
            break;
            case  INPUT_KBD_RSHIFT:
                KeyFlags &= ~RSHIFT;
            break;
        }
    }

#endif

}

//=========================================================================

void ui_edit::OnCursorEnter ( ui_win* pWin )
{
    (void)pWin;
    ui_win::OnCursorEnter( pWin );
#ifdef TARGET_PC    
    m_CursorPosition = m_Label.GetLength();
#endif
}

//=========================================================================
char ui_edit::ConvertGadgetIDToChar ( s32 Key )
{
    (void)Key;

#ifdef TARGET_PC
    
    // Don't include the shif key.
    if( (Key != INPUT_KBD_LSHIFT) && (Key != INPUT_KBD_RSHIFT) )
    {
    
        if( m_LastKey == Key )
        {
            return -1;
        }
        else
        { 
            m_LastKey = Key;
        }
    }

    // Use the brute force method.
    switch ( Key )
    {
        case  INPUT_KBD_ESCAPE:
            return -1;
        break;
        case  INPUT_KBD_1:
            return '1';
        break;
        case  INPUT_KBD_2:
            return '2';
        break;
        case  INPUT_KBD_3:
            return '3';
        break;
        case  INPUT_KBD_4:
            return '4';
        break;
        case  INPUT_KBD_5:
            return '5';
        break;
        case  INPUT_KBD_6:
            return '6';
        break;
        case  INPUT_KBD_7:
            return '7';
        break;
        case  INPUT_KBD_8:
            return '8';
        break;
        case  INPUT_KBD_9:
            return '9';
        break;
        case  INPUT_KBD_0:
            return '0';
        break;
        case  INPUT_KBD_MINUS:
            return '-';
        break;    // - on main keyboard 
        case  INPUT_KBD_EQUALS:
            return '=';
        break; 
        case  INPUT_KBD_BACK:
            return 127;
        break;     // backspace 
        case  INPUT_KBD_TAB:
            return -1;
        break;
        case  INPUT_KBD_Q:
            return 'q';
        break;
        case  INPUT_KBD_W:
            return 'w';
        break;
        case  INPUT_KBD_E:
            return 'e';
        break;
        case  INPUT_KBD_R:
            return 'r';
        break;
        case  INPUT_KBD_T:
            return 't';
        break;
        case  INPUT_KBD_Y:
            return 'y';
        break;
        case  INPUT_KBD_U:
            return 'u';
        break;
        case  INPUT_KBD_I:
            return 'i';
        break;
        case  INPUT_KBD_O:
            return 'o';
        break;
        case  INPUT_KBD_P:
            return 'p';
        break;
        case  INPUT_KBD_LBRACKET:
            return '[';
        break;
        case  INPUT_KBD_RBRACKET:
            return ']';
        break;
        case  INPUT_KBD_RETURN:
            return -1;
        break;    // Enter on main keyboard 
        case  INPUT_KBD_LCONTROL:
            return -1;
        break;
        case  INPUT_KBD_A:
            return 'a';
        break;
        case  INPUT_KBD_S:
            return 's';
        break;
        case  INPUT_KBD_D:
            return 'd';
        break;
        case  INPUT_KBD_F:
            return 'f';
        break;
        case  INPUT_KBD_G:
            return 'g';
        break;
        case  INPUT_KBD_H:
            return 'h';
        break;
        case  INPUT_KBD_J:
            return 'j';
        break;
        case  INPUT_KBD_K:
            return 'k';
        break;
        case  INPUT_KBD_L:
            return 'l';
        break;
        case  INPUT_KBD_SEMICOLON:
            return ';';
        break;
        case  INPUT_KBD_APOSTROPHE:
            return 96;
        break;
        case  INPUT_KBD_GRAVE:
            return 44;
        break;     // accent grave 
        case  INPUT_KBD_LSHIFT:
            KeyFlags |= LSHIFT;
            return -1;
        break;
        case  INPUT_KBD_BACKSLASH:
            return 92;
        break;
        case  INPUT_KBD_Z:
            return 'z';
        break;
        case  INPUT_KBD_X:
            return 'x';
        break;
        case  INPUT_KBD_C:
            return 'c';
        break;
        case  INPUT_KBD_V:
            return 'v';
        break;
        case  INPUT_KBD_B:
            return 'b';
        break;
        case  INPUT_KBD_N:
            return 'n';
        break;
        case  INPUT_KBD_M:
            return 'm';
        break;
        case  INPUT_KBD_COMMA:
            return ',';
        break;
        case  INPUT_KBD_PERIOD:
            return '.';
        break;     // . on main keyboard 
        case  INPUT_KBD_SLASH:
            return '/';
        break;     // / on main keyboard 
        case  INPUT_KBD_RSHIFT:
            KeyFlags |= RSHIFT;
            return -1;
        break;
        case  INPUT_KBD_MULTIPLY:
            return '*';
        break;     // * on numeric keypad 
        case  INPUT_KBD_SPACE:
            return 32;    
        break;
        case  INPUT_KBD_CAPITAL:
            if( KeyFlags & CAPSLOCK )
                KeyFlags &= ~ CAPSLOCK;
            else
                KeyFlags |= CAPSLOCK;

            return -1;
        break;
        case  INPUT_KBD_NUMLOCK:
            if( KeyFlags & NUMLOCK )
                KeyFlags &= ~ NUMLOCK;
            else
                KeyFlags |= NUMLOCK;
            return -1;
        break;
        case  INPUT_KBD_NUMPAD7:
            if( KeyFlags & NUMLOCK )
                return '7';
        break;
        case  INPUT_KBD_NUMPAD8:
            if( KeyFlags & NUMLOCK )
                return '8';
        break;
        case  INPUT_KBD_NUMPAD9:
            if( KeyFlags & NUMLOCK )
                return '9';
        break;
        case  INPUT_KBD_SUBTRACT:
            if( KeyFlags & NUMLOCK )
                return '-';
        break;     // - on numeric keypad 
        case  INPUT_KBD_NUMPAD4:
            if( KeyFlags & NUMLOCK )
                return '4';
        break;
        case  INPUT_KBD_NUMPAD5:
            if( KeyFlags & NUMLOCK )
                return '5';
        break;
        case  INPUT_KBD_NUMPAD6:
            if( KeyFlags & NUMLOCK )
                return '6';
        break;
        case  INPUT_KBD_ADD:
            if( KeyFlags & NUMLOCK )
                return '+';
        break;     // + on numeric keypad 
        case  INPUT_KBD_NUMPAD1:
            if( KeyFlags & NUMLOCK )
                return '1';
        break;
        case  INPUT_KBD_NUMPAD2:
            if( KeyFlags & NUMLOCK )
                return '2';
        break;
        case  INPUT_KBD_NUMPAD3:
            if( KeyFlags & NUMLOCK )
                return '3';
        break;
        case  INPUT_KBD_NUMPAD0:
            if( KeyFlags & NUMLOCK )
                return '0';
        break;
        case  INPUT_KBD_DECIMAL:
            if( KeyFlags & NUMLOCK )
                return '.';
        break;     // . on numeric keypad 
        case  INPUT_KBD_HOME:
            m_CursorPosition = 0;
            return -1;
        break;     // Home on arrow keypad 
        case  INPUT_KBD_LEFT:
            m_CursorPosition--;

            // Check the bounds for the cursor.
            if( m_CursorPosition > m_Label.GetLength()-1 )
                m_CursorPosition = m_Label.GetLength()-1;
            else if( m_CursorPosition < 0 )
                m_CursorPosition = 0;

            return -1;                        
        break;     // LeftArrow on arrow keypad 
        case  INPUT_KBD_RIGHT:
            m_CursorPosition++;

            // Check the bounds for the cursor.
            if( m_CursorPosition > m_Label.GetLength()-1 )
                m_CursorPosition = m_Label.GetLength()-1;
            else if( m_CursorPosition < 0 )
                m_CursorPosition = 0;

            return -1;
        break;     // RightArrow on arrow keypad 
        case  INPUT_KBD_END:
            
            return -1;
        break;     // End on arrow keypad 
        case  INPUT_KBD_DELETE:

            m_CursorPosition--;
            
            // Check the bounds for the cursor.
            if( m_CursorPosition > m_Label.GetLength() )
            {
                m_CursorPosition = m_Label.GetLength()-1;
            }
            if( m_Label.GetLength() == 0 )
            {
                m_CursorPosition = 0;
                return -1;
            }
            if( m_CursorPosition < 0 )
            {
                m_CursorPosition = 0;
            }

            m_Label.Delete( m_CursorPosition );

            return -1;
        break;     // Delete on arrow keypad.
        default:
            return -1;
        break;
    }

    return -1;
#else
    return -1;
#endif
}

//=========================================================================
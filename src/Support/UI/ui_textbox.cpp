// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_textbox.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_textbox.cpp
//
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <Support/AudioMgr/tribes-aa/audio.hpp>
#include <Support/LabelSets/reference/tribes-aa/Tribes2Types.hpp>

#include <Support/UI/ui_textbox.hpp>
#include <Support/UI/ui_manager.hpp>
#include <Support/UI/ui_font.hpp>
//#include "ui_dlg_combolist.hpp"

//=========================================================================
//  Defines
//=========================================================================

#define SPACE_TOP       4
#define SPACE_BOTTOM    4
#define TEXT_OFFSET     -2

//=========================================================================
//  Structs
//=========================================================================

//=========================================================================
//  Data
//=========================================================================

//=========================================================================
//  Factory function
//=========================================================================

ui_win* ui_textbox_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    ui_textbox* pcombo = new ui_textbox;
    pcombo->Create( UserID, pManager, Position, pParent, Flags );

    return (ui_win*)pcombo;
}

//=========================================================================
//  ui_textbox
//=========================================================================

ui_textbox::ui_textbox( void )
{
}

//=========================================================================

ui_textbox::~ui_textbox( void )
{
    Destroy();
}

//=========================================================================

xbool ui_textbox::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success;

    Success = ui_control::Create( UserID, pManager, Position, pParent, Flags );

    // Set default text flags
    m_LabelFlags = ui_font::h_center|ui_font::v_center|ui_font::clip_character|ui_font::clip_l_justify;
    m_BackgroundColor = xcolor(0,0,0,0);

    // Initialize data
    m_iElementFrame         = m_pManager->FindElement( "frame" );
    m_iElement_sb_arrowdown = m_pManager->FindElement( "sb_arrowdown" );
    m_iElement_sb_arrowup   = m_pManager->FindElement( "sb_arrowup" );
    m_iElement_sb_container = m_pManager->FindElement( "sb_container" );
    m_iElement_sb_thumb     = m_pManager->FindElement( "sb_thumb" );
    ASSERT( m_iElementFrame != -1 );
    ASSERT( m_iElement_sb_arrowdown != -1 );
    ASSERT( m_iElement_sb_arrowup   != -1 );
    ASSERT( m_iElement_sb_container != -1 );
    ASSERT( m_iElement_sb_thumb     != -1 );

    m_LineHeight        = m_pManager->GetLineHeight( 0 );
    m_iFirstVisibleLine = 0;
    m_nLines            = 0;
    m_ShowBorders       = TRUE;
    m_nVisibleLines     = (m_Position.GetHeight()-SPACE_TOP-SPACE_BOTTOM) / m_LineHeight;

#ifdef TARGET_PC
    m_ScrollTime = 0.0f;
    m_MouseDown = FALSE;
    m_ScrollDown = FALSE;
#endif

    return Success;
}

//=========================================================================

void ui_textbox::SetLabel( const xwstring& Text )
{
    // Create wordwrap string
    irect r = m_Position;
    r.r -= 19;
    r.Deflate( 4, 4 );
    xwstring Wrapped = m_pManager->WordWrapString( 0, r, Text );

    m_Label  = Wrapped;
    m_pManager->TextSize( 0, m_TextRect, (const xwchar*)m_Label, -1 );
    m_nLines = m_TextRect.GetHeight() / m_LineHeight;
    m_iFirstVisibleLine = 0;
}

//=========================================================================

void ui_textbox::Render( s32 ox, s32 oy )
{
    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
        xcolor  c1  = XCOLOR_WHITE;
        xcolor  c2  = XCOLOR_BLACK;
        s32     State       = ui_manager::CS_NORMAL;

        // Calculate rectangle
        irect   br;
        irect   r;
        irect   r2;
        br.Set( (m_Position.l+ox), (m_Position.t+oy), (m_Position.r+ox), (m_Position.b+oy) );
        r = br;
        r2 = r;
        r.r -= 19;
        r2.l = r.r;

        // Render appropriate state
        if( m_Flags & WF_DISABLED )
        {
            State = ui_manager::CS_DISABLED;
            c1  = XCOLOR_GREY;
            c2  = xcolor(0,0,0,0);
        }
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == WF_HIGHLIGHT )
        {
            State = ui_manager::CS_HIGHLIGHT;
            c1  = XCOLOR_WHITE;
            c2  = XCOLOR_BLACK;
        }
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == WF_SELECTED )
        {
            
            State = ui_manager::CS_SELECTED;
            c1  = XCOLOR_WHITE;
            c2  = XCOLOR_BLACK;
        }
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == (WF_HIGHLIGHT|WF_SELECTED) )
        {
            State = ui_manager::CS_HIGHLIGHT_SELECTED;
            c1  = XCOLOR_WHITE;
            c2  = XCOLOR_BLACK;
        }
        else
        {
            State = ui_manager::CS_NORMAL;
            c1  = XCOLOR_WHITE;
            c2  = XCOLOR_BLACK;
        }

        // Add Highlight to list
        if( m_Flags & WF_HIGHLIGHT )
            m_pManager->AddHighlight( m_UserID, br, !(m_Flags & WF_SELECTED) );

        // Render background color
        irect   rb = r;
        rb.Deflate( 1, 1 );
        m_pManager->RenderRect( rb, m_BackgroundColor, FALSE );

        // Render text offset by first visible line
        irect rt = r;
        rt.t += SPACE_TOP;
        rt.b -= SPACE_BOTTOM;
        rt.Deflate( 4, 0 );

        m_pManager->PushClipWindow( rt );

        rt.t -= m_LineHeight*m_iFirstVisibleLine;
//      rt.Translate( 1, 1 );
//      m_pManager->RenderText( 0, rt, m_LabelFlags, c2, (const xwchar*)m_Label );
//      rt.Translate( -1, -1 );
        m_pManager->RenderText( 0, rt, m_LabelFlags, 191, (const xwchar*)m_Label );

        m_pManager->PopClipWindow();

        if (m_ShowBorders)
        {
            // Render Frame
            m_pManager->RenderElement( m_iElementFrame, r, 0 );
            irect r3 = r2;
            irect r4 = r2;
            r3.b = r3.t + 22;
            r4.t = r4.b - 22;
            r2.t = r3.b;
            r2.b = r4.t;

#ifdef TARGET_PC
            m_UpArrow = r3;
            m_DownArrow = r4;
#endif
            m_pManager->RenderElement( m_iElement_sb_container, r2, State );
            m_pManager->RenderElement( m_iElement_sb_arrowup,   r3, State );
            m_pManager->RenderElement( m_iElement_sb_arrowdown, r4, State );

            // Render Thumb
            r2.Deflate( 2, 2 );
            if( m_nLines > 0 )
            {
                f32 t = (f32)m_iFirstVisibleLine / m_nLines;
                f32 b = (f32)(m_iFirstVisibleLine + m_nVisibleLines) / m_nLines;
                if( t < 0.0f ) t = 0.0f;
                if( b > 1.0f ) b = 1.0f;
                r2.Set( r2.l, r2.t + (s32)(r2.GetHeight() * t), r2.r, r2.t + (s32)(r2.GetHeight() * b) );
            }
            if( r2.GetHeight() > 16 )
                m_pManager->RenderElement( m_iElement_sb_thumb,     r2, State );

#ifdef TARGET_PC
            m_ScrollBar = r2;
#endif
        }
        
        if( m_Flags & WF_SELECTED )
            m_pManager->AddHighlight( m_UserID, r2 );

        // Render children
        for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
        {
            m_Children[i]->Render( m_Position.l+ox, m_Position.t+oy );
        }
    }
}

//=========================================================================

void ui_textbox::SetPosition( const irect& Position )
{
    m_Position      = Position;
    m_nVisibleLines = (m_Position.GetHeight()-SPACE_TOP-SPACE_BOTTOM) / m_LineHeight;
}

//=========================================================================

void ui_textbox::OnPadNavigate( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats )
{
    xbool       Processed = FALSE;
    s32         dy = 0;

    // Determine movement required
    switch( Code )
    {
    case ui_manager::NAV_UP:
        dy = -1;
        break;
    case ui_manager::NAV_DOWN:
        dy =  1;
    }

    // Apply movement
    if( m_Flags & WF_SELECTED )
    {
        if( dy != 0 )
        {
            xbool   Limited      = FALSE;
            s32     FirstVisible = m_iFirstVisibleLine;
            
            FirstVisible += dy;
            if( FirstVisible > (m_nLines-m_nVisibleLines) )
            {
                FirstVisible = (m_nLines-m_nVisibleLines);
                Limited = TRUE;
            }
            if( FirstVisible < 0 )
            {
                FirstVisible = 0;
                Limited = TRUE;
            }

            // Check for error at top and bottom of scroll range
            if( Limited && (Presses > 0) )
                audio_Play( SFX_FRONTEND_ERROR,AUDFLAG_CHANNELSAVER );

            // Set new position back into first visible
            if( FirstVisible != m_iFirstVisibleLine )
            {
                audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
                m_iFirstVisibleLine = FirstVisible;
            }
        }

        Processed = TRUE;
    }

    // Pass up chain if not processed
    if( !Processed )
    {
        if( m_pParent )
            m_pParent->OnPadNavigate( pWin, Code, Presses, Repeats );
    }
}

//=========================================================================

void ui_textbox::OnPadSelect( ui_win* pWin )
{
    (void)pWin;

    // Toggle Selected
    m_Flags ^= WF_SELECTED;

    if( m_Flags & WF_SELECTED )
    {
        audio_Play( SFX_FRONTEND_SELECT_02,AUDFLAG_CHANNELSAVER );
    }
    else
    {
        audio_Play( SFX_FRONTEND_CANCEL_02,AUDFLAG_CHANNELSAVER );
    }
}

//=========================================================================

void ui_textbox::OnPadBack( ui_win* pWin )
{
    (void)pWin;

    if( m_Flags & WF_SELECTED )
    {
        // Clear selected
        m_Flags &= ~WF_SELECTED;
        audio_Play( SFX_FRONTEND_CANCEL_02,AUDFLAG_CHANNELSAVER );
    }
    else
    {
        if( m_pParent )
            m_pParent->OnPadBack( pWin );
    }
}

//=========================================================================

void ui_textbox::SetBackgroundColor( xcolor Color )
{
    m_BackgroundColor = Color;
}

//=========================================================================

xcolor ui_textbox::GetBackgroundColor( void ) const
{
    return m_BackgroundColor;
}

//=========================================================================

void ui_textbox::OnCursorMove ( ui_win* pWin, s32 x, s32 y )
{
    (void)pWin;
    (void)x;
    (void)y;

#ifndef TARGET_PC
    return;
#else
    
    if( m_ScrollDown )
    {    
        if( m_ScrollBar.PointInRect( m_CursorX, m_CursorY ) )
        {
            
            s32 FirstVisible = m_iFirstVisibleLine;
            s32 diff = (y - m_CursorY);

            if( diff > 0 )
            {
                diff = (s32)((f32)diff/4);
                FirstVisible += diff;
        
                if( FirstVisible > (m_nLines-m_nVisibleLines) )
                    FirstVisible = (m_nLines-m_nVisibleLines);

                // Set new position back into first visible
                if( FirstVisible != m_iFirstVisibleLine )
                {
                    audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
                    m_iFirstVisibleLine = FirstVisible;
                }
            }
            else if( diff < 0 )
            {
                diff = (s32)((f32)diff/4);
                FirstVisible += diff;

                if( FirstVisible < 0 )
                    FirstVisible = 0;

                // Set new position back into first visible
                if( FirstVisible != m_iFirstVisibleLine )
                {
                    audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
                    m_iFirstVisibleLine = FirstVisible;
                }
            }
        }
    }

    if( m_MouseDown )
    {
        // Just move the selected item down one.
        if( m_DownArrow.PointInRect( m_CursorX, m_CursorY ) )
        {
            m_MouseDown = TRUE;
        }
        // Just move the selected item up one.
        else if( m_UpArrow.PointInRect( m_CursorX, m_CursorY ) )
        {
            m_MouseDown = TRUE;
        }    
        else
        {
            m_MouseDown = FALSE;
        }
    }   

    // Store the latest mouse position.
    m_CursorX = x;
    m_CursorY = y;
       
#endif

}

//=========================================================================

void ui_textbox::OnLBDown ( ui_win* pWin )
{
    (void)pWin;

#ifdef TARGET_PC
    s32 FirstVisible = m_iFirstVisibleLine;

    // Just move the selected item down one.
    if( m_DownArrow.PointInRect( m_CursorX, m_CursorY ) )
    {
        FirstVisible++;
        
        if( FirstVisible > (m_nLines-m_nVisibleLines) )
            FirstVisible = (m_nLines-m_nVisibleLines);
        m_MouseDown = TRUE;
    }

    // Just move the selected item up one.
    else if( m_UpArrow.PointInRect( m_CursorX, m_CursorY ) )
    {
        FirstVisible--;
        if( FirstVisible < 0 )
            FirstVisible = 0;
        m_MouseDown = TRUE;
    }
    // Did the mouse click on the scroll bar.    
    else if( m_ScrollBar.PointInRect( m_CursorX, m_CursorY ) )
        m_ScrollDown = TRUE;

    if( m_MouseDown )
    {
        // Set new position back into first visible
        if( FirstVisible != m_iFirstVisibleLine )
        {
            audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
            m_iFirstVisibleLine = FirstVisible;
        }
    }

#endif
}

//=========================================================================

void ui_textbox::OnLBUp ( ui_win* pWin )
{
    (void)pWin;

#ifdef TARGET_PC    
    m_MouseDown = FALSE;
    m_ScrollDown = FALSE;
#endif
}

//=========================================================================

void ui_textbox::OnUpdate ( ui_win* pWin, f32 DeltaTime )
{
    (void)pWin;
    (void)DeltaTime;

#ifdef TARGET_PC
    
    if( m_MouseDown )
    {    
        m_ScrollTime += DeltaTime;
    
        // Set a delay to scroll.
        if( m_ScrollTime < 0.3f )
            return;
        else
            m_ScrollTime = 0.0f;

        s32 FirstVisible = m_iFirstVisibleLine;

        // Just move the selected item down one.
        if( m_DownArrow.PointInRect( m_CursorX, m_CursorY ) )
        {
            FirstVisible++;
        
            if( FirstVisible > (m_nLines-m_nVisibleLines) )
                FirstVisible = (m_nLines-m_nVisibleLines);
        }

        // Just move the selected item up one.
        else if( m_UpArrow.PointInRect( m_CursorX, m_CursorY ) )
        {
            FirstVisible--;
            if( FirstVisible < 0 )
                FirstVisible = 0;
        }
        
        // Set new position back into first visible
        if( FirstVisible != m_iFirstVisibleLine )
        {
            audio_Play( SFX_FRONTEND_CURSOR_MOVE_02 ,AUDFLAG_CHANNELSAVER);
            m_iFirstVisibleLine = FirstVisible;
        }
    }
#endif

}

//=========================================================================

void ui_textbox::OnCursorExit ( ui_win* pWin )
{
    (void)pWin;

#ifdef TARGET_PC
    m_MouseDown = FALSE;
    m_ScrollDown = FALSE;
#endif
    ui_win::OnCursorExit( pWin );

}

//=========================================================================
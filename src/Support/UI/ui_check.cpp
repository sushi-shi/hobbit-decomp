// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_check.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_check.cpp
//
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <Support/AudioMgr/tribes-aa/audio.hpp>
#include <Support/LabelSets/reference/tribes-aa/Tribes2Types.hpp>

#include <Support/UI/ui_check.hpp>
#include <Support/UI/ui_manager.hpp>
#include <Support/UI/ui_font.hpp>

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

ui_win* ui_check_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    ui_check* pcheck = new ui_check;
    pcheck->Create( UserID, pManager, Position, pParent, Flags );

    return (ui_win*)pcheck;
}

//=========================================================================
//  ui_check
//=========================================================================

ui_check::ui_check( void )
{
}

//=========================================================================

ui_check::~ui_check( void )
{
    Destroy();
}

//=========================================================================

xbool ui_check::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success;

    Success = ui_control::Create( UserID, pManager, Position, pParent, Flags );

    // Initialize Data
    m_iElement = m_pManager->FindElement( "button_check" );
    ASSERT( m_iElement != -1 );

    return Success;
}

//=========================================================================

void ui_check::Render( s32 ox, s32 oy )
{
    s32     State = ui_manager::CS_NORMAL;

    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
        xcolor  TextColor1 = XCOLOR_WHITE;
        xcolor  TextColor2 = XCOLOR_BLACK;

        // Calculate rectangle
        irect    r;
        r.Set( (m_Position.l+ox), (m_Position.t+oy), (m_Position.r+ox), (m_Position.b+oy) );

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
        m_pManager->RenderElement( m_iElement, r, State );

        // Add Highlight to list
        if( m_Flags & WF_HIGHLIGHT )
            m_pManager->AddHighlight( m_UserID, r );

        // Render Text
        r.r -= 19;
        r.Deflate( 4, 3 );
        r.Translate( 1, -1 );
        m_pManager->RenderText( m_Font, r, m_LabelFlags, TextColor2, m_Label );
        r.Translate( -1, -1 );
        m_pManager->RenderText( m_Font, r, m_LabelFlags, TextColor1, m_Label );

        // Render children
        for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
        {
            m_Children[i]->Render( m_Position.l+ox, m_Position.t+oy );
        }
    }
}

//=========================================================================

void ui_check::OnPadSelect( ui_win* pWin )
{
    if( pWin == (ui_win*)this )
    {
        m_Flags ^= WF_SELECTED;

        // Notify Parent
        if( m_pParent )
            m_pParent->OnNotify( m_pParent, this, WN_CHECK_CHANGE, (void*)(m_Flags & WF_SELECTED) );
        audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
    }
}

//=========================================================================

void ui_check::SetSelected( xbool State )
{
    if( State )
        m_Flags |= WF_SELECTED;
    else
        m_Flags &= ~WF_SELECTED;
}

//=========================================================================

xbool ui_check::GetSelected( void ) const
{
    return m_Flags & WF_SELECTED;
}

//=========================================================================

void ui_check::OnLBDown ( ui_win* pWin )
{
    (void)pWin;

#ifndef TARGET_PC
    return;
#endif

    if( pWin == (ui_win*)this )
    {
        m_Flags ^= WF_SELECTED;

        // Notify Parent
        if( m_pParent )
            m_pParent->OnNotify( m_pParent, this, WN_CHECK_CHANGE, (void*)(m_Flags & WF_SELECTED) );
        audio_Play( SFX_FRONTEND_CURSOR_MOVE_02,AUDFLAG_CHANNELSAVER );
    }
}
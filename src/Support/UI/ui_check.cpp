// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_check.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_check.cpp
//
//=========================================================================

#include <rva.h>

#include <xCore/Entropy/Entropy.hpp>
#include <xCore/Entropy/e_Audio.hpp>
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

RVA(0x2a5460, 0x75) ui_win* ui_check_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    ui_check* pcheck = new ui_check;
    pcheck->Create( UserID, pManager, Position, pParent, Flags );

    return (ui_win*)pcheck;
}

//=========================================================================
//  ui_check
//=========================================================================

RVA(0x2a54e0, 0x12) ui_check::ui_check( void )
{
}

//=========================================================================

RVA(0x2a5520, 0x4f) ui_check::~ui_check( void )
{
    Destroy();
}

//=========================================================================

RVA(0x2a5570, 0x40) xbool ui_check::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success;

    Success = ui_control::Create( UserID, pManager, Position, pParent, Flags );

    // Initialize Data
    m_iElement = m_pManager->FindElement( "button_check" );
    ASSERT( m_iElement != -1 );

    return Success;
}

//=========================================================================

RVA(0x2a55b0, 0x2c5)
void ui_check::Render( s32 ox, s32 oy )
{
    x_mem_owner __owner__("ui_check::Render");
    s32 State = ui_manager::CS_NORMAL;
    if( m_Flags & WF_VISIBLE )
    {
        xcolor TextColor1 = XCOLOR_WHITE;
        xcolor TextColor2 = XCOLOR_BLACK;
        irect r;
        r.Set( m_Position.l+ox, m_Position.t+oy, m_Position.r+ox, m_Position.b+oy );
        if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == WF_SELECTED )
            State = ui_manager::CS_SELECTED;
        else if( (m_Flags & (WF_HIGHLIGHT|WF_SELECTED)) == (WF_HIGHLIGHT|WF_SELECTED) )
            State = ui_manager::CS_HIGHLIGHT_SELECTED;
        m_pManager->RenderElement( m_iElement, r, State );
        if( m_Flags & WF_HIGHLIGHT )
            m_pManager->AddHighlight( m_UserID, r );
        if( m_Flags & WF_HIGHLIGHT )
        {
            r.r -= 19;
            r.Deflate( 4, 3 );
            r.Translate( 2, -2 );
            m_pManager->RenderText( g_UiMgr->FindFont("large"), r, m_LabelFlags, TextColor2, m_Label );
            r.Translate( -2, -2 );
            m_pManager->RenderText( g_UiMgr->FindFont("large"), r, m_LabelFlags, TextColor1, m_Label );
        }
        else
        {
            r.r -= 19;
            r.Deflate( 4, 3 );
            r.Translate( 1, -1 );
            m_pManager->RenderText( g_UiMgr->FindFont("large"), r, m_LabelFlags, TextColor2, m_Label );
            r.Translate( -1, -1 );
            m_pManager->RenderText( g_UiMgr->FindFont("large"), r, m_LabelFlags, xcolor(150,150,150,255), m_Label );
        }
        for( s32 i=0; i<m_Children.GetCount(); i++ )
            m_Children[i]->Render( m_Position.l+ox, m_Position.t+oy );
    }
}

//=========================================================================

RVA(0x2a5880, 0x43)
void ui_check::OnPadSelect( ui_win* pWin )
{
    if( pWin == (ui_win*)this )
    {
        m_Flags ^= WF_SELECTED;

        // Notify Parent
        if( m_pParent )
            m_pParent->OnNotify( m_pParent, this, WN_CHECK_CHANGE, (void*)(m_Flags & WF_SELECTED) );
        g_AudioMgr.Play("OptionSelect");
    }
}

//=========================================================================

RVA(0x2a58d0, 0x21)
void ui_check::SetSelected( xbool State )
{
    if( State )
        m_Flags |= WF_SELECTED;
    else
        m_Flags &= ~WF_SELECTED;
}

//=========================================================================

RVA(0x2a5900, 0xa)
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
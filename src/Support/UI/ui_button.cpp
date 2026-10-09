// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_button.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_button.cpp
//
//=========================================================================

#include <rva.h>

#include <xCore/Entropy/Entropy.hpp>
#include <Support/UI/ui_button.hpp>
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

RVA(0x2a7870, 0x75) ui_win* ui_button_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    ui_button* pButton = new ui_button;
    pButton->Create( UserID, pManager, Position, pParent, Flags );

    return (ui_win*)pButton;
}

//=========================================================================
//  ui_button
//=========================================================================

RVA(0x2a78f0, 0x12) ui_button::ui_button( void )
{
}

//=========================================================================

RVA(0x2a7930, 0x4f) ui_button::~ui_button( void )
{
    Destroy();
}

//=========================================================================

RVA(0x2a7980, 0x40) xbool ui_button::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success;

    Success = ui_control::Create( UserID, pManager, Position, pParent, Flags );

    // Initialize Data
    m_iElement = m_pManager->FindElement( "button" );
    ASSERT( m_iElement != -1 );

    return Success;
}

//=========================================================================

RVA(0x2a79c0, 0x245)
void ui_button::Render( s32 ox, s32 oy )
{
    x_mem_owner __owner__("ui_button::Render");
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
        r.Translate( 1, -1 );
        m_pManager->RenderText( m_Font, r, ui_font::h_center|ui_font::v_center, TextColor2, m_Label );
        r.Translate( -1, -1 );
        m_pManager->RenderText( m_Font, r, ui_font::h_center|ui_font::v_center, TextColor1, m_Label );

        // Render children
        for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
        {
            m_Children[i]->Render( m_Position.l+ox, m_Position.t+oy );
        }
    }
}

//=========================================================================

void ui_button::OnUpdate( f32 DeltaTime )
{
    (void)DeltaTime;
}

//=========================================================================

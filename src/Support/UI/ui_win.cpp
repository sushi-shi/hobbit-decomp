// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_win.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_win.cpp
//
//=========================================================================

#include <rva.h>
#include <xCore/Entropy/Entropy.hpp>
#ifndef TARGET_PC
#include "../AudioMgr/audio.hpp"
#endif
#ifndef TARGET_PC
#include "../LabelSets/Tribes2Types.hpp"
#endif

#include <Support/UI/ui_win.hpp>
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
//  ui_win
//=========================================================================

ui_win::ui_win( void )
{
    m_ID    = -1;
    m_Flags = 0;
    m_Font  = 0;
    m_LabelColor = XCOLOR_WHITE;
    m_FontName = "large";
}

//=========================================================================

ui_win::~ui_win( void )
{
    Destroy();
}

//=========================================================================

RVA(0x002a2f20, 0x12d)
xbool ui_win::Create( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags )
{
    xbool   Success = TRUE;

    m_UserID         = UserID;
    m_pManager       = pManager;
    m_pParent        = pParent;
    m_CreatePosition = Position;
    m_Position       = Position;
    m_Flags          = Flags;
    m_LabelFlags     = ui_font::h_center|ui_font::v_center;
    m_LabelColor     = XCOLOR_WHITE;

    // Add child entry to parent window if we have a parent
    if( m_pParent )
        m_pParent->m_Children.Append() = this;

    return Success;
}

//=========================================================================

RVA(0x002a3050, 0x84)
void ui_win::Destroy( void )
{
    s32     i;
    s32     iFound = -1;

    // Kill Children
    while( m_Children.GetCount() > 0 )
    {
        ui_win* pChild = m_Children[0];
        delete pChild;
    }

    if( m_pParent )
    {
        // Find in parents child list and remove
        for( i=0 ; i<m_pParent->m_Children.GetCount() ; i++ )
        {
            if( m_pParent->m_Children[i] == this )
                iFound = i;
        }
        if( iFound != -1 )
        {
            m_pParent->m_Children.Delete( iFound );
        }

        // Clear Parent pointer
        m_pParent = NULL;
    }
}

//=========================================================================

RVA(0x002a30e0, 0x7f)
void ui_win::Render( s32 ox, s32 oy )
{
    x_mem_owner __owner__("ui_win::Render");
    // Only render is visible
    if( m_Flags & WF_VISIBLE )
    {
        // Render children
        for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
        {
            m_Children[i]->Render( ox, oy );
        }
    }
}

//=========================================================================

RVA(0x002a3160, 0x20)
void ui_win::SetPosition( const irect& Position )
{
    m_Position = Position;
}

//=========================================================================

const irect& ui_win::GetPosition( void ) const
{
    return m_Position;
}

//=========================================================================

const irect& ui_win::GetCreatePosition( void ) const
{
    return m_Position;
}

//=========================================================================

RVA(0x002a3180, 0x7)
s32 ui_win::GetWidth( void ) const
{
    return m_Position.GetWidth();
}

//=========================================================================

RVA(0x002a3190, 0x7)
s32 ui_win::GetHeight( void ) const
{
    return m_Position.GetHeight();
}

//=========================================================================

RVA(0x2a31a0, 0x80)
ui_win* ui_win::GetWindowAtXY( s32 x, s32 y ) const
{
    ui_win* pFound = NULL;

    // Don't process for STATIC or INVISIBLE windows
    if( !(m_Flags & WF_STATIC) && (m_Flags & WF_VISIBLE) )
    {
        // Check if the coordinates hit our rectangle
        if( ((x >= 0) && (x < m_Position.GetWidth())) &&
            ((y >= 0) && (y < m_Position.GetHeight())) )
        {
            // Loop through all children testing
            for( s32 i=0 ; (i<m_Children.GetCount()) && !pFound ; i++ )
            {
                irect r = m_Children[i]->GetPosition();
                pFound = m_Children[i]->GetWindowAtXY( x - r.l, y - r.t );
            }

            // If no child found then return this window
            if( pFound == NULL )
                pFound = (ui_win*)this;
        }
    }

    // Return window found
    return pFound;
}

//=========================================================================

RVA(0x002a3220, 0xa)
void ui_win::SetFlags( s32 Flags )
{
    m_Flags = Flags;
}

//=========================================================================

RVA(0x002a3230, 0x4)
s32 ui_win::GetFlags( void ) const
{
    return m_Flags;
}

//=========================================================================

RVA(0x002a3240, 0x28)
void ui_win::SetFlag( s32 Flag, s32 State )
{
    if( State )
        m_Flags |= Flag;
    else
        m_Flags &= ~Flag;
}

//=========================================================================

RVA(0x002a3270, 0xa)
s32 ui_win::GetFlags( s32 Flag )
{
    return m_Flags & Flag;
}

//=========================================================================

RVA(0x2a3280, 0x8)
void ui_win::SetLabel( const xwstring& Text )
{
    m_Label = Text;
}

//=========================================================================

RVA(0x2a3290, 0x8)
void ui_win::SetLabel( const xwchar* Text )
{
    m_Label = Text;
}

//=========================================================================

RVA(0x002a32a0, 0x4)
const xwstring& ui_win::GetLabel( void ) const
{
    return m_Label;
}

//=========================================================================

RVA(0x002a32b0, 0xd)
void ui_win::SetLabelFlags( u32 Flags )
{
    m_LabelFlags = Flags;
}

//=========================================================================

RVA(0x002a32c0, 0xa)
void ui_win::SetControlID( s32 ID )
{
    m_ID = ID;
}

//=========================================================================

RVA(0x002a32d0, 0x4)
s32 ui_win::GetControlID( void ) const
{
    return m_ID;
}

//=========================================================================
RVA(0x002a32e0, 0x2a)
void ui_win::SetLabelColor(const xcolor& color)
{
    m_LabelColor = color;
}

//=========================================================================
RVA(0x002a3310, 0x7)
const xcolor& ui_win::GetLabelColor(void) const
{
    return m_LabelColor;
}

/*
//=========================================================================

void ui_win::SetText( const xstring& Text )
{
    m_Label = Text;
}

//=========================================================================

void ui_win::SetText( const char* Text )
{
    m_Label = Text;
}

//=========================================================================

const xstring& ui_win::GetText( void ) const
{
    return m_Label;
}

//=========================================================================

void ui_win::SetTextFlags( u32 Flags )
{
    m_LabelFlags = Flags;
}

*/
//=========================================================================

void ui_win::SetParent( ui_win* pParent )
{
    s32     i;
    s32     iFound = -1;

    // Remove from previous parent
    if( m_pParent )
    {
        // Find in parents child list and remove
        for( i=0 ; i<m_pParent->m_Children.GetCount() ; i++ )
        {
            if( m_pParent->m_Children[i] == this )
                iFound = i;
        }
        if( iFound != -1 )
        {
            m_pParent->m_Children.Delete( iFound );
        }
    }

    // Add to new parent
    m_pParent = pParent;
    m_pParent->m_Children.Append() = this;
}

//=========================================================================

ui_win* ui_win::GetParent( void ) const
{
    return m_pParent;
}

/*
//=========================================================================

ui_win* ui_win::FindChildByLabel( const xstring& Label ) const
{
    for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
    {
        ui_win* pChild = m_Children[i];
        if( pChild->GetLabel() == Label )
            return pChild;
    }

    return NULL;
}

//=========================================================================

ui_win* ui_win::FindChildByLabel( const char* Label ) const
{
    return FindChildByLabel( xstring( Label ) );
}
*/
//=========================================================================

RVA(0x002a3320, 0x28) ui_win* ui_win::FindChildByID( s32 ID ) const
{
    for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
    {
        ui_win* pChild = m_Children[i];
        if( pChild->m_ID == ID )
            return pChild;
    }

    // Failed to find the child
    ASSERT( 0 );

    return NULL;
}

//=========================================================================

RVA(0x002a3350, 0x28)
xbool ui_win::IsChildOf( ui_win* pParent ) const
{
    if( m_pParent )
    {
        if( m_pParent == pParent )
            return TRUE;
        else
            return m_pParent->IsChildOf( pParent );
    }

    return FALSE;
}

//=========================================================================

RVA(0x002a3380, 0x35)
void ui_win::LocalToScreen( s32& x, s32& y ) const
{
    x += m_Position.l;
    y += m_Position.t;
    if( m_pParent )
    {
        m_pParent->LocalToScreen( x, y );
    }
}

//=========================================================================

RVA(0x002a33c0, 0x35)
void ui_win::ScreenToLocal( s32& x, s32& y ) const
{
    x -= m_Position.l;
    y -= m_Position.t;
    if( m_pParent )
    {
        m_pParent->ScreenToLocal( x, y );
    }
}

//=========================================================================

RVA(0x002a3400, 0x43)
void ui_win::LocalToScreen( irect& r ) const
{
    r.l += m_Position.l;
    r.r += m_Position.l;
    r.t += m_Position.t;
    r.b += m_Position.t;
    if( m_pParent )
    {
        m_pParent->LocalToScreen( r );
    }
}

//=========================================================================

RVA(0x002a3450, 0x43)
void ui_win::ScreenToLocal( irect& r ) const
{
    r.l -= m_Position.l;
    r.r -= m_Position.l;
    r.t -= m_Position.t;
    r.b -= m_Position.t;
    if( m_pParent )
    {
        m_pParent->ScreenToLocal( r );
    }
}

//=========================================================================

void ui_win::LocalToScreenCreate( irect& r ) const
{
    irect r2 = GetCreatePosition();
    r.l += r2.l;
    r.r += r2.l;
    r.t += r2.t;
    r.b += r2.t;
    if( m_pParent )
    {
        m_pParent->LocalToScreenCreate( r );
    }
}

//=========================================================================

void ui_win::ScreenToLocalCreate( irect& r ) const
{
    irect r2 = GetCreatePosition();
    r.l -= r2.l;
    r.r -= r2.l;
    r.t -= r2.t;
    r.b -= r2.t;
    if( m_pParent )
    {
        m_pParent->ScreenToLocalCreate( r );
    }
}

//=========================================================================
//=========================================================================
//  Message Handler Functions
//=========================================================================
//=========================================================================

RVA(0x002a34a0, 0x2d)
void ui_win::OnUpdate( ui_win* pWin, f32 DeltaTime )
{
    (void)pWin;
    (void)DeltaTime;

    // Render children
    for( s32 i=0 ; i<m_Children.GetCount() ; i++ )
    {
        m_Children[i]->OnUpdate( m_Children[i], DeltaTime );
    }
}

//=========================================================================

RVA(0x002a34d0, 0xf)
void ui_win::OnNotify( ui_win* pWin, ui_win* pSender, s32 Command, void* pData )
{
    (void)pWin;
    (void)pSender;
    (void)Command;
    (void)pData;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnNotify( pWin, pSender, Command, pData );
}

//=========================================================================

void ui_win::OnLBDown( ui_win* pWin )
{
    (void)pWin;
    
    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadSelect( pWin );
}

//=========================================================================

void ui_win::OnLBUp( ui_win* pWin )
{
    (void)pWin;
}

//=========================================================================

void ui_win::OnMBDown( ui_win* pWin )
{
    (void)pWin;
}

//=========================================================================

void ui_win::OnMBUp( ui_win* pWin )
{
    (void)pWin;
}

//=========================================================================

void ui_win::OnRBDown( ui_win* pWin )
{
    (void)pWin;
}

//=========================================================================

void ui_win::OnRBUp( ui_win* pWin )
{
    (void)pWin;
}

//=========================================================================

void ui_win::OnCursorMove( ui_win* pWin, s32 x, s32 y )
{
    (void)pWin;
    (void)x;
    (void)y;
}

//=========================================================================

RVA(0x002a3540, 0xa)
void ui_win::OnCursorEnter( ui_win* pWin )
{
    (void)pWin;
    m_Flags |= WF_HIGHLIGHT;

#ifndef TARGET_PC
    audio_Play( SFX_FRONTEND_CURSOR_MOVE_01,AUDFLAG_CHANNELSAVER );
#endif
}

//=========================================================================

RVA(0x002a3550, 0xa)
void ui_win::OnCursorExit( ui_win* pWin )
{
    (void)pWin;
    m_Flags &= ~WF_HIGHLIGHT;
}

//=========================================================================

RVA(0x002a3560, 0xf)
void ui_win::OnKeyDown( ui_win* pWin, s32 Key )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnKeyDown( pWin, Key );
}

//=========================================================================

RVA(0x002a3570, 0xf)
void ui_win::OnKeyUp( ui_win* pWin, s32 Key )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnKeyUp( pWin, Key );
}

//=========================================================================

RVA(0x002a3580, 0xf)
void ui_win::OnPadNavigate( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadNavigate( pWin, Code, Presses, Repeats );
}

//=========================================================================

void ui_win::OnPadSelect( ui_win* pWin )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadSelect( pWin );
}

//=========================================================================

RVA(0x002a3590, 0xf)
void ui_win::OnPadBack( ui_win* pWin )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadBack( pWin );
}

//=========================================================================

RVA(0x002a35a0, 0xf)
void ui_win::OnPadDelete( ui_win* pWin )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadDelete( pWin );
}

//=========================================================================

RVA(0x002a35b0, 0xf)
void ui_win::OnPadHelp( ui_win* pWin )
{
    (void)pWin;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadHelp( pWin );
}

//=========================================================================

RVA(0x002a35c0, 0x12)
void ui_win::OnPadShoulder( ui_win* pWin, s32 Direction )
{
    (void)pWin;
    (void)Direction;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadShoulder( pWin, Direction );
}

//=========================================================================

RVA(0x002a35e0, 0x12)
void ui_win::OnPadShoulder2( ui_win* pWin, s32 Direction )
{
    (void)pWin;
    (void)Direction;

    // Pass up chain to parent
    if( m_pParent )
        m_pParent->OnPadShoulder2( pWin, Direction );
}

//=========================================================================

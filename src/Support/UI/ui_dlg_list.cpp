// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_dlg_list.cpp; complete Area51 variant retained as reference.
//=========================================================================
//
//  ui_dlg_list.cpp
//
//=========================================================================

#include <rva.h>

#include <xCore/Entropy/Entropy.hpp>
#include <Support/AudioMgr/tribes-aa/audio.hpp>
#include <Support/LabelSets/reference/tribes-aa/Tribes2Types.hpp>

#include <Support/UI/ui_dlg_list.hpp>
#include <Support/UI/ui_manager.hpp>
#include <Support/UI/ui_control.hpp>
#include <Support/UI/ui_font.hpp>
#include <Support/UI/ui_listbox.hpp>

#include <Support/reference/tribes-aa/Demo1/fe_colors.hpp>

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
//  List Selection Dialog
//=========================================================================

enum controls
{
    IDC_LIST
};

DATA(0x355d9c) ui_manager::control_tem ListControls[] =
{
    { IDC_LIST, 0, "listbox", 0, 0, 0, 0, 0, 0, 1, 1, ui_win::WF_VISIBLE },
};

DATA(0x355dcc) ui_manager::dialog_tem ListDialog =
{
    0,
    1, 1,
    sizeof(ListControls)/sizeof(ui_manager::control_tem),
    &ListControls[0],
    0
};

//=========================================================================
//  Factory function
//=========================================================================

RVA(0x2a6130, 0x19) void ui_dlg_list_register( ui_manager* pManager )
{
    pManager->RegisterDialogClass( "ui_list", &ListDialog, &ui_dlg_list_factory );
}

//=========================================================================
//  Factory function
//=========================================================================

RVA(0x2a6150, 0x7f) ui_win* ui_dlg_list_factory( s32 UserID, ui_manager* pManager, ui_manager::dialog_tem* pDialogTem, const irect& Position, ui_win* pParent, s32 Flags, void* pUserData )
{
    ui_dlg_list* pDialog = new ui_dlg_list;
    pDialog->Create( UserID, pManager, pDialogTem, Position, pParent, Flags, pUserData );

    return (ui_win*)pDialog;
}

//=========================================================================
//  ui_dlg_list
//=========================================================================

RVA(0x2a61d0, 0x12) ui_dlg_list::ui_dlg_list( void )
{
}

//=========================================================================

RVA(0x2a6210, 0x4f) ui_dlg_list::~ui_dlg_list( void )
{
    Destroy();
}

//=========================================================================

RVA(0x2a6260, 0xfa) xbool ui_dlg_list::Create( s32                        UserID,
                           ui_manager*                pManager,
                           ui_manager::dialog_tem*    pDialogTem,
                           const irect&               Position,
                           ui_win*                    pParent,
                           s32                        Flags,
                           void*                      pUserData )
{
    xbool   Success = FALSE;

    (void)pUserData;

    ASSERT( pManager );

    // Make it input modal
    Flags |= WF_INPUTMODAL;

    // Do dialog creation
    Success = ui_dialog::Create( UserID, pManager, pDialogTem, Position, pParent, Flags );

    m_pResultPtr        = 0;
    m_BackgroundColor   = xcolor(0,20,30,255);

    m_pList = (ui_listbox*)FindChildByID( IDC_LIST );

    irect r = Position;
    r.Translate( -r.l, -r.t );
    r.Deflate( 8, 8 );
    m_pList->SetPosition( r );

    m_pList->SetFlags( m_pList->GetFlags() | WF_SELECTED );

#ifdef TARGET_PC
    m_InsideListBox = TRUE;
    m_UserID        = UserID;
#endif

    // Return success code
    return Success;
}

//=========================================================================

RVA(0x2a6360, 0x115)
void ui_dlg_list::Render( s32 ox, s32 oy )
{
    x_mem_owner __owner__("ui_dlg_list::Render");
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
        irect   rb = r;
        rb.Deflate( 1, 1 );
        m_pManager->RenderRect( rb, m_BackgroundColor, FALSE );

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

ui_listbox* ui_dlg_list::GetListBox( void )
{
    return m_pList;
}

//=========================================================================

void ui_dlg_list::SetResultPtr( s32* pResultPtr )
{
    m_pResultPtr = pResultPtr;
}

//=========================================================================

RVA(0x2a6480, 0x70) void ui_dlg_list::OnNotify( ui_win* pWin, ui_win* pSender, s32 Command, void* pData )
{
    (void)pWin;
    (void)pSender;
    (void)Command;
    (void)pData;

    if( pSender == (ui_win*)m_pList )
    {
        if( Command == WN_LIST_ACCEPTED )
        {
            if( m_pResultPtr && (m_pList->GetSelection() != -1) )
                *m_pResultPtr = m_pList->GetSelectedItemData();
            m_pManager->EndDialog( m_UserID, TRUE );
        }
        else if( Command == WN_LIST_CANCELLED )
        {
            m_pManager->EndDialog( m_UserID, TRUE );
        }
    }
}

//=========================================================================

RVA(0x2a64f0, 0x1e) void ui_dlg_list::OnLBDown ( ui_win* pWin )
{
    (void)pWin;
#ifdef TARGET_PC
    
    if( !m_InsideListBox )
    {
        m_pManager->EndDialog( m_UserID );
    }
#endif
}

//=========================================================================

RVA(0x2a6510, 0x36) void ui_dlg_list::OnCursorMove ( ui_win* pWin, s32 x, s32 y )
{
    (void)pWin;
    (void)x;
    (void)y;
#ifdef TARGET_PC
    if( (x >= m_Position.l) && (x <= m_Position.r) &&
        (y >= m_Position.t) && (y <= m_Position.b) )
        m_InsideListBox = TRUE;
    else
        m_InsideListBox = FALSE;

#endif
}

//=========================================================================
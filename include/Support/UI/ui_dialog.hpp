// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_dialog.hpp; complete Area51 variant retained as reference.
//==============================================================================
//  
//  ui_dialog.hpp
//  
//==============================================================================

#ifndef UI_DIALOG_HPP
#define UI_DIALOG_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_win.hpp>
#include <Support/UI/ui_manager.hpp>

//==============================================================================
//  ui_dialog
//==============================================================================

extern ui_win* ui_dialog_factory( s32 UserID, ui_manager* pManager, ui_manager::dialog_tem* pDialogTem, const irect& Position, ui_win* pParent, s32 Flags, void* pUserData );

class ui_dialog : public ui_win
{
    friend ui_manager;

public:
                            ui_dialog           ( void );
    virtual                ~ui_dialog           ( void );
                            
    xbool                   Create              ( s32                       UserID,
                                                  ui_manager*               pManager,
                                                  ui_manager::dialog_tem*   pDialogTem,
                                                  const irect&              Position,
                                                  ui_win*                   pParent,
                                                  s32                       Flags,
                                                  void*                     pUserData = NULL );

    virtual void            Render              ( s32 ox=0, s32 oy=0 );

    virtual const irect&    GetCreatePosition   ( void ) const;

    virtual void            OnPadNavigate       ( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats );
                            
    void                    SetBackgroundColor  ( xcolor Color );
    xcolor                  GetBackgroundColor  ( void ) const;
                            
    xbool                   GotoControl         ( s32 iControl = 0 );
    xbool                   GotoControl         ( ui_control* pControl );
                            
    s32                     GetNumControls      ( void ) const;

    ui_manager::dialog_tem* GetTemplate ( void );

protected:
    s32                     m_iElement;
    ui_manager::dialog_tem* m_pDialogTem;
    s32                     m_NavW;
    s32                     m_NavH;
    s32                     m_NavX;
    s32                     m_NavY;
    xarray<ui_win*>         m_NavGraph;
    xcolor                  m_BackgroundColor;
    s32                     m_OldCursorX;
    s32                     m_OldCursorY;
    s32                     m_XRes,m_YRes;
    void*                   m_pUserData;
};

//==============================================================================
#endif // UI_DIALOG_HPP
//==============================================================================

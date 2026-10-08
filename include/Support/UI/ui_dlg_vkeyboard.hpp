// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_dlg_vkeyboard.hpp; complete Area51 variant retained as reference.
//==============================================================================
//  
//  ui_dlg_vkeyboard.hpp
//  
//==============================================================================

#ifndef UI_DLG_VKEYBOARD_HPP
#define UI_DLG_VKEYBOARD_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/UI/ui_dialog.hpp>

//==============================================================================
//  ui_dlg_vkeyboard
//==============================================================================

extern ui_win* ui_dlg_vkeyboard_factory( s32 UserID, ui_manager* pManager, ui_manager::dialog_tem* pDialogTem, const irect& Position, ui_win* pParent, s32 Flags, void* pUserData );

class ui_vkString;

class ui_dlg_vkeyboard : public ui_dialog
{
public:
                    ui_dlg_vkeyboard    ( void );
    virtual        ~ui_dlg_vkeyboard    ( void );

    xbool           Create              ( s32                       UserID,
                                          ui_manager*               pManager,
                                          ui_manager::dialog_tem*   pDialogTem,
                                          const irect&              Position,
                                          ui_win*                   pParent,
                                          s32                       Flags,
                                          void*                     pUserData );

    virtual void    Render              ( s32 ox=0, s32 oy=0 );

    virtual void    OnPadNavigate       ( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats );
    virtual void    OnPadShoulder       ( ui_win* pWin, s32 Direction );

    virtual void    OnPadSelect         ( ui_win* pWin );
    virtual void    OnPadBack           ( ui_win* pWin );
    virtual void    OnPadDelete         ( ui_win* pWin );

    virtual void    OnNotify            ( ui_win* pWin, ui_win* pSender, s32 Command, void* pData );

    void            ConnectString       ( xwstring* pString, s32 MaxCharacters );
    void            SetReturn           ( xbool* pDone, xbool* pOk );

protected:
    s32             m_iElement;
    s32             m_MaxCharacters;
    ui_vkString*    m_pStringCtrl;
    xwstring*       m_pString;
    xwstring        m_BackupString;
    xbool*          m_pResultDone;
    xbool*          m_pResultOk;
};

//==============================================================================
#endif // UI_DLG_VKEYBOARD_HPP
//==============================================================================

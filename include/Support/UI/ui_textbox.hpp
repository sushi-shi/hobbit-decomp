// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_textbox.hpp; complete Area51 variant retained as reference.
//==============================================================================
//  
//  ui_textbox.hpp
//  
//==============================================================================

#ifndef UI_TEXTBOX_HPP
#define UI_TEXTBOX_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_textbox
//==============================================================================

extern ui_win* ui_textbox_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_textbox : public ui_control
{
    struct item
    {
        xbool       Enabled;
        xwstring    Label;
        s32         Data;
        xcolor      Color;
    };

public:
                    ui_textbox              ( void );
    virtual        ~ui_textbox              ( void );

    xbool           Create                  ( s32           UserID,
                                              ui_manager*   pManager,
                                              const irect&  Position,
                                              ui_win*       pParent,
                                              s32           Flags );

    virtual void    SetLabel            ( const xwstring&   Text );

    virtual void    Render                  ( s32 ox=0, s32 oy=0 );

    virtual void    SetPosition             ( const irect& Position );

    virtual void    OnPadNavigate           ( ui_win* pWin, s32 Code, s32 Presses, s32 Repeats );
    virtual void    OnPadSelect             ( ui_win* pWin );
    virtual void    OnPadBack               ( ui_win* pWin );
    virtual void    OnCursorMove            ( ui_win* pWin, s32 x, s32 y );
    virtual void    OnLBDown                ( ui_win* pWin );
    virtual void    OnLBUp                  ( ui_win* pWin );
    virtual void    OnUpdate                ( ui_win* pWin, f32 DeltaTime );
    virtual void    OnCursorExit            ( ui_win* pWin );

    void            EnableBorders           ( void ) { m_ShowBorders = TRUE;};
    void            DisableBorders          ( void ) { m_ShowBorders = FALSE;};

    s32             GetLineCount            ( void ) const;

    void            EnsureVisible           ( s32 iLine );

    void            SetBackgroundColor      ( xcolor Color );
    xcolor          GetBackgroundColor      ( void ) const;

protected:
    xbool           m_ShowBorders;
    s32             m_iElementFrame;
    s32             m_iElement_sb_arrowdown;
    s32             m_iElement_sb_arrowup;
    s32             m_iElement_sb_container;
    s32             m_iElement_sb_thumb;

#ifdef TARGET_PC
    s32             m_CursorX;
    s32             m_CursorY;
    irect           m_UpArrow;
    irect           m_DownArrow;
    irect           m_ScrollBar;
    xbool           m_MouseDown;
    xbool           m_ScrollDown;
    f32             m_ScrollTime;
#endif

    irect           m_TextRect;
    s32             m_LineHeight;
    s32             m_nLines;
    s32             m_nVisibleLines;
    s32             m_iFirstVisibleLine;

    xcolor          m_BackgroundColor;
};

//==============================================================================
#endif // UI_TEXTBOX_HPP
//==============================================================================

// Genuine available sibling engine source import; PC mapping/version reconciliation pending.
// Source: Area51 pristine 431f72b9 Support/UI/ui_blankbox.hpp; see docs/imports/ui-meshutil.json.
//==============================================================================
//  
//  ui_blankbox.hpp
//  
//==============================================================================

#ifndef UI_BLANKBOX_HPP
#define UI_BLANKBOX_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_listbox
//==============================================================================

extern ui_win* ui_blankbox_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_blankbox : public ui_control
{

public:
                    ui_blankbox              ( void );
    virtual        ~ui_blankbox              ( void );

    xbool           Create                  ( s32           UserID,
                                              ui_manager*   pManager,
                                              const irect&  Position,
                                              ui_win*       pParent,
                                              s32           Flags );

    virtual void    Render                  ( s32 ox=0, s32 oy=0 );

    virtual void    OnUpdate                ( ui_win* pWin, f32 DeltaTime );

    void            SetHasTitleBar          ( xbool hasTitle );

    void            SetTitleBarColor        ( xcolor Color );
    xcolor          GetTitleBarColor        ( void ) const;

    void            SetBackgroundColor      ( xcolor Color );
    xcolor          GetBackgroundColor      ( void ) const;

    void            SetBitmap               ( s32 BitmapID );
    void            SetBitmap               ( s32 BitmapID, irect& Pos );

protected:
    xbool           m_HasTitleBar;
    xcolor          m_TitleBarColor;
    xcolor          m_BackgroundColor;
    s32             m_BitmapID;
    irect           m_BitmapPos;
};

//==============================================================================
#endif // UI_BLANKBOX_HPP
//==============================================================================

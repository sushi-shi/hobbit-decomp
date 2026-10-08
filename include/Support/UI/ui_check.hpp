// Reconstructed older UI family adaptation; PC qualification in docs/imports/ui-hobbit-family.md.
// Original Tribes-AA 4aab7137 support/ui/ui_check.hpp; complete Area51 variant retained as reference.
//==============================================================================
//  
//  ui_check.hpp
//  
//==============================================================================

#ifndef UI_CHECK_HPP
#define UI_CHECK_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_check
//==============================================================================

extern ui_win* ui_check_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_check : public ui_control
{
public:
                    ui_check            ( void );
    virtual        ~ui_check            ( void );

    xbool           Create              ( s32           UserID,
                                          ui_manager*   pManager,
                                          const irect&  Position,
                                          ui_win*       pParent,
                                          s32           Flags );

    virtual void    Render              ( s32 ox=0, s32 oy=0 );

    virtual void    OnPadSelect         ( ui_win* pWin );
    virtual void    OnLBDown            ( ui_win* pWin );

    void            SetSelected         ( xbool State );
    xbool           GetSelected         ( void ) const;

protected:
    s32             m_iElement;
};

//==============================================================================
#endif // UI_CHECK_HPP
//==============================================================================

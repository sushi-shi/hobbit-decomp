// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_button.hpp; complete Area51 variant available in external entropy-src donor.
//==============================================================================
//  
//  ui_button.hpp
//  
//==============================================================================

#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_button
//==============================================================================

extern ui_win* ui_button_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_button : public ui_control
{
public:
                    ui_button           ( void );
    virtual        ~ui_button           ( void );

    xbool           Create              ( s32           UserID,
                                          ui_manager*   pManager,
                                          const irect&  Position,
                                          ui_win*       pParent,
                                          s32           Flags );

    virtual void    Render              ( s32 ox=0, s32 oy=0 );
    virtual void    OnUpdate            ( f32 DeltaTime );

protected:
    s32             m_iElement;
};

//==============================================================================
#endif // UI_BUTTON_HPP
//==============================================================================

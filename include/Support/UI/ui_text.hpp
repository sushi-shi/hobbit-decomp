// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_text.hpp; complete Area51 variant available in external entropy-src donor.
//==============================================================================
//  
//  ui_text.hpp
//  
//==============================================================================

#ifndef UI_TEXT_HPP
#define UI_TEXT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_text
//==============================================================================

extern ui_win* ui_text_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_text : public ui_control
{
public:
                    ui_text             ( void );
    virtual        ~ui_text             ( void );

    xbool           Create              ( s32           UserID,
                                          ui_manager*   pManager,
                                          const irect&  Position,
                                          ui_win*       pParent,
                                          s32           Flags );

    virtual void    Render              ( s32 ox=0, s32 oy=0 );
    virtual void    OnUpdate            ( f32 DeltaTime );

protected:
};

//==============================================================================
#endif // UI_TEXT_HPP
//==============================================================================

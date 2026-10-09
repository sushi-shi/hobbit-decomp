// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_radio.hpp; complete Area51 variant available in external entropy-src donor.
//==============================================================================
//  
//  ui_radio.hpp
//  
//==============================================================================

#ifndef UI_RADIO_HPP
#define UI_RADIO_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_radio
//==============================================================================

extern ui_win* ui_radio_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_radio : public ui_control
{
public:
                    ui_radio            ( void );
    virtual        ~ui_radio            ( void );

    xbool           Create              ( s32           UserID,
                                          ui_manager*   pManager,
                                          const irect&  Position,
                                          ui_win*       pParent,
                                          s32           Flags );

    virtual void    Render              ( s32 ox=0, s32 oy=0 );

protected:
    s32             m_iElement;
};

//==============================================================================
#endif // UI_RADIO_HPP
//==============================================================================

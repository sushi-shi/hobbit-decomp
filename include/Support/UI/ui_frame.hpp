// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_frame.hpp; complete Area51 variant available in external entropy-src donor.
//==============================================================================
//  
//  ui_frame.hpp
//  
//==============================================================================

#ifndef UI_FRAME_HPP
#define UI_FRAME_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

//==============================================================================
//  ui_frame
//==============================================================================

extern ui_win* ui_frame_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_frame : public ui_control
{
public:
                    ui_frame            ( void );
    virtual        ~ui_frame            ( void );

    xbool           Create              ( s32           UserID,
                                          ui_manager*   pManager,
                                          const irect&  Position,
                                          ui_win*       pParent,
                                          s32           Flags );

    virtual void    Render              ( s32 ox=0, s32 oy=0 );

    void            SetBackgroundColor  ( xcolor Color );
    xcolor          GetBackgroundColor  ( void ) const ;
    void            EnableTitle         ( const xwstring&   Text, xbool BigTitle = FALSE );
    void            EnableTitle         ( const xwchar*     Text, xbool BigTitle = FALSE );
    void            EnableNewFrame      ( xbool bFrame, s32 TextWidth );   
    void            RenderNewFrame      ( irect& Position );
    void            ChangeElement       ( void );

protected:
    xwstring        m_Title;            // Frames Title, if the flag is enabled.
    s32             m_iElement;
    xcolor          m_BackgroundColor;
    xbool           m_NewFrame;
    s32             m_TextWidth;
	xbool			m_BigTitle;
};

//==============================================================================
#endif // UI_FRAME_HPP
//==============================================================================

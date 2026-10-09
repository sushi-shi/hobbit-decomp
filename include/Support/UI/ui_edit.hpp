// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_edit.hpp; complete Area51 variant available in external entropy-src donor.
//==============================================================================
//  
//  ui_edit.hpp
//  
//==============================================================================

#ifndef UI_EDIT_HPP
#define UI_EDIT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_control.hpp>

#ifdef TARGET_PC

enum{ KEYBOARD_LSHIFT, KEYBOARD_RSHIFT, KEYBOARD_CAPS, KEYBOARD_NUM };

#define LSHIFT       (1<<KEYBOARD_LSHIFT)
#define RSHIFT       (1<<KEYBOARD_RSHIFT)
#define CAPSLOCK    (1<<KEYBOARD_CAPS)
#define NUMLOCK     (1<<KEYBOARD_NUM)

#endif
//==============================================================================
//  ui_edit
//==============================================================================

extern ui_win* ui_edit_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_edit : public ui_control
{
public:
                            ui_edit                 ( void );
    virtual                ~ui_edit                 ( void );

    xbool                   Create                  ( s32           UserID,
                                                    ui_manager*   pManager,
                                                    const irect&  Position,
                                                    ui_win*       pParent,
                                                    s32           Flags );

    virtual void            Render                  ( s32 ox=0, s32 oy=0 );

    virtual void            OnPadSelect             ( ui_win* pWin );
    virtual void            OnKeyDown               ( ui_win* pWin, s32 Key );
    virtual void            OnKeyUp                 ( ui_win* pWin, s32 Key );
    virtual void            OnCursorEnter           ( ui_win* pWin );

    void                    SetLabelWidth           ( s32 Width );
    void                    SetMaxCharacters        ( s32 MaxCharacters );
    void                    SetVirtualKeyboardTitle ( const xwstring& Title );
    char                    ConvertGadgetIDToChar   ( s32 Key );

//    virtual void            SetText             ( const xstring& Text );
//    virtual void            SetText             ( const char*    Text );
//    virtual const xstring&  GetText             ( void ) const;

protected:
    s32             m_iElement1;
    s32             m_iElement2;
    s32             m_LabelWidth;
    s32             m_MaxCharacters;
    xwstring        m_VirtualKeyboardTitle;

#ifdef TARGET_PC
    s32             KeyFlags;
    s32             m_CursorPosition;
    s32             m_LastKey;
#endif
//    xstring         m_Text;
};

//==============================================================================
#endif // UI_EDIT_HPP
//==============================================================================

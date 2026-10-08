// Genuine available sibling engine source import; PC mapping/version reconciliation pending.
// Source: Area51 pristine 431f72b9 Support/UI/ui_joinlist.hpp; see docs/imports/ui-meshutil.json.
//==============================================================================
//  
//  ui_joinlist.hpp
//  
//==============================================================================

#ifndef UI_JOINLIST_HPP
#define UI_JOINLIST_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_listbox.hpp>

//==============================================================================
//  defines
//==============================================================================

enum mutation_icons
{
    ICON_MUTANT = 0,
    ICON_HUMAN,
    ICON_VS,
    ICON_CYCLE,
    NUM_MUTATION_ICONS,
};

//==============================================================================
//  ui_joinlist
//==============================================================================

extern ui_win* ui_joinlist_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_joinlist : public ui_listbox
{
public:
                    ui_joinlist             ( void );
    virtual        ~ui_joinlist             ( void );

    virtual void    Render                  ( s32 ox=0, s32 oy=0 );

    void            RenderString            ( irect r, u32 Flags, const xcolor& c1, const xcolor& c2, const char* pString );
    void            RenderString            ( irect r, u32 Flags, const xcolor& c1, const xcolor& c2, const xwchar* pString );
    void            RenderTitle             ( irect r, u32 Flags, const xwchar* pString );
    virtual void    RenderItem              ( irect r, const item& Item, const xcolor& c1, const xcolor& c2 );

private:
    s32             MutationIcon[NUM_MUTATION_ICONS];
};

//==============================================================================
#endif // UI_JOINLIST_HPP
//==============================================================================

// Genuine available sibling engine source import; PC mapping/version reconciliation pending.
// Source: Area51 pristine 431f72b9 Support/UI/ui_maplist.hpp; see docs/imports/ui-meshutil.json.
//==============================================================================
//  
//  ui_maplist.hpp
//  
//==============================================================================

#ifndef UI_MAP_LIST_HPP
#define UI_MAP_LIST_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include <Support/UI/ui_listbox.hpp>

//==============================================================================
//  ui_maplist
//==============================================================================

extern ui_win* ui_maplist_factory( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );

class ui_maplist : public ui_listbox
{
public:
                    ui_maplist             ( void );
    virtual        ~ui_maplist             ( void );

    virtual void    Render                  ( s32 ox=0, s32 oy=0 );

    void            RenderString            ( irect r, u32 Flags, const xcolor& c1, const xcolor& c2, const char* pString );
    void            RenderString            ( irect r, u32 Flags, const xcolor& c1, const xcolor& c2, const xwchar* pString );
    void            RenderTitle             ( irect r, u32 Flags, const xwchar* pString );
    virtual void    RenderItem              ( irect r, const item& Item, const xcolor& c1, const xcolor& c2 );

private:
};

//==============================================================================
#endif // UI_MAP_LIST_HPP
//==============================================================================

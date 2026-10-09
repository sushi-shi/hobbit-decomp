//==============================================================================
//
//  hud_Reticle.hpp
//
//  Copyright (c) 2002-2004 Inevitable Entertainment Inc.  All rights reserved.
//
//==============================================================================

#ifndef HUD_RETICLE_HPP
#define HUD_RETICLE_HPP

//==============================================================================
// INCLUDES
//==============================================================================

#include <Support/Obj_mgr/obj_mgr.hpp>
#include <xCore/x_files/x_bitmap.hpp>
#include <Support/Objects/Player.hpp>

#include <Support/Objects/hud_Renderable.hpp>

//==============================================================================
// CLASS
//==============================================================================

class hud_reticle : public hud_renderable
{
public:
                    hud_reticle     ( void );
    virtual        ~hud_reticle     ( void ) {};

    virtual void    OnRender        ( player*       pPlayer );
    virtual void    OnAdvanceLogic  ( player*       pPlayer, f32 DeltaTime );
    virtual xbool   OnProperty      ( prop_query&   rPropQuery );
    virtual void    OnEnumProp      ( prop_enum&    List );

    guid    m_LastTarget;
    f32     m_LockonTime;

};

#endif
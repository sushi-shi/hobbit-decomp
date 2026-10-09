//==============================================================================
//
//  Mortar.hpp
//
//==============================================================================

#ifndef MORTAR_HPP
#define MORTAR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Arcing.hpp"
#include "Particles/ParticleEffect.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class mortar : public arcing
{

//------------------------------------------------------------------------------
//  Public Functions

public:

        void            Render          ( void );
        void            RenderParticles ( void );
        void            OnAcceptInit    ( const bitstream& BitStream );

        void            Initialize      ( const vector3&   Position,
                                          const vector3&   Direction,  
                                                object::id OriginID,
                                                object::id ExcludeID,
                                                u32        TeamBits,
                                                s32        Type = 0 );

                                                // For Type: 0 = Mortar Gun
                                                //           1 = Mortar Turret
        
        void            OnAdd           ( void );

//------------------------------------------------------------------------------
//  Private Functions

protected:

        xbool           Impact          ( const collider::collision& Collision );        
        update_code     AdvanceLogic    ( f32 DeltaTime );

//------------------------------------------------------------------------------
//  Private Data

protected:
    
        f32             m_MaxAge;
        pfx_effect      m_SmokeEffect;
        xbool           m_HasExploded;
        s32             m_Type;             // For Type: 0 = Mortar Gun   
};                                          //           1 = Mortar Turret

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     MortarCreator( void );

//==============================================================================
#endif // MORTAR_HPP
//==============================================================================

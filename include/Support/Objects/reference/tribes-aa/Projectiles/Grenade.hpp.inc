//==============================================================================
//
//  Grenade.hpp
//
//==============================================================================

#ifndef GRENADE_HPP
#define GRENADE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Arcing.hpp"
#include "Particles/ParticleEffect.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class missile;

class grenade : public arcing
{

//------------------------------------------------------------------------------
//  Public Functions
public:
        enum type
        {
            TYPE_UNDEFINED,
            TYPE_LAUNCHED,
            TYPE_BASIC,
            TYPE_FLASH,
            TYPE_CONCUSSION,
            TYPE_FLARE
        };

public:

        void        Render          ( void );
        void        RenderFlare     ( void );
        void        RenderParticles ( void );


        void        Initialize      ( const vector3&        Position,
                                      const vector3&        Direction,  
                                      const vector3&        OriginVel,  
                                            object::id      OriginID,
                                            u32             TeamBits,
                                            f32             ExtraSpeed,
                                            type            Type );

        void        OnAdd           ( void );

        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnProvideInit   (       bitstream& BitStream );
        void        DestroyFlare    ( void );

        // so render knows which ones to render where
        type        GetType         ( void )    { return m_Type; }        

//------------------------------------------------------------------------------
//  Private Functions

protected:

        // White out screen for players when flash grenade
        void        FlashPlayers    ( void );

        xbool       Impact          ( const collider::collision& Collision ) ;

        void        Explode         ( const vector3& Normal );

        update_code AdvanceLogic    ( f32 DeltaTime );
        
//------------------------------------------------------------------------------
//  Private Data

protected:

        type            m_Type;    
        f32             m_MaxAge;
        pfx_effect      m_SmokeEffect;
        xbool           m_HasExploded;
        radian3         m_Rotation;
    
friend missile;

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     GrenadeCreator( void );

//==============================================================================
#endif // GRENADE_HPP
//==============================================================================

//==============================================================================
//
//  SatchelCharge.hpp
//
//==============================================================================

#ifndef SATCHEL_CHARGE_HPP
#define SATCHEL_CHARGE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Arcing.hpp"



//==============================================================================
//  TYPES
//==============================================================================

class satchel_charge : public arcing
{
//------------------------------------------------------------------------------
//  Defines
public:
        // States
        enum state
        {
            STATE_MOVING,
            STATE_ARMING,
            STATE_ARMED,
            STATE_DETONATE,

            STATE_START = STATE_MOVING,
            STATE_END   = STATE_DETONATE
        } ;

        // Explosion type
        enum explosion_type
        {
            EXPLOSION_BIG,
            EXPLOSION_SMALL,
            EXPLOSION_FIZZLE,
        } ;


//------------------------------------------------------------------------------
//  Public Functions

public:

        void            Render          ( void );
        
        void            OnProvideInit   (       bitstream& BitStream );
        void            OnAcceptInit    ( const bitstream& BitStream );

        update_code     OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
        void            OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );

        void            Initialize      ( const vector3&   Position,      
                                          const vector3&   Direction,     
                                                object::id OriginID,      
                                                u32        TeamBits,      
                                                f32        ExtraSpeed = 0 ) ;

        void            OnAdd           ( void ) ;

        xbool           Detonate        ( explosion_type   ExplosionType ) ;

        void            SetOriginID     ( object::id       OriginID ) ;
        xbool           IsArmed         ( void ) { return( m_State == STATE_ARMED ); }

//------------------------------------------------------------------------------
//  Private Functions

protected:

        void            OnPain          ( const pain& Pain );
        f32             GetPainRadius   ( void ) const;

        xbool           Impact          ( const collider::collision& Collision ) ;

        void            SetupState      ( state State ) ;

        update_code     AdvanceLogic    ( f32 DeltaTime );

//------------------------------------------------------------------------------
//  Private Data

protected:
    
        state           m_State ;           // Current state
        f32             m_StateTime ;       // Current state time
        explosion_type  m_ExplosionType ;   // Type of explosion to do

        object::id      m_SurfaceObjectID ; // Object ID of surface attached to
        radian3         m_SurfaceRot ;      // Rotation of surface satchel is on (world or local if on node)
        vector3         m_SurfacePos ;      // Position on surface (world or local if on node)
        s32             m_SurfaceCollID ;   // Collision ID to use
        s32             m_SurfaceNode ;     // Node of surface or -1 if none
} ;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     SatchelChargeCreator( void );

//==============================================================================
#endif // SATCHEL_CHARGE_HPP
//==============================================================================

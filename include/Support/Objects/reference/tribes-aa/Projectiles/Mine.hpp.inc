//==============================================================================
//
//  Mine.hpp
//
//==============================================================================

#ifndef MINE_HPP
#define MINE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Arcing.hpp"

//==============================================================================
//  ANNOUNCEMENTS
//==============================================================================

class game_mgr;

//==============================================================================
//  TYPES
//==============================================================================

class mine : public arcing
{

//------------------------------------------------------------------------------
//  Public Functions

public:
                    mine            ( void );
                    
        void        Trip            ( void );

        void        DebugRender     ( void );
        void        Render          ( void );

        void        Initialize      ( const vector3&   Position,
                                      const vector3&   Direction,  
                                      const vector3&   OriginVel,  
                                            object::id OriginID,
                                            u32        TeamBits,
                                            f32        ExtraSpeed );

        void        OnAdd           ( void );

        update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
        void        OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnProvideInit   (       bitstream& BitStream );

        void        OnPain          ( const pain& Pain );

        void        OnCollide       ( u32 AttrBits, collider& Collider );

        f32         GetPainRadius   ( void ) const;
        vector3     GetPainPoint    ( void ) const;

        void        UnbindOriginID  ( object::id OriginID );

//------------------------------------------------------------------------------
//  Private Functions

protected:

        update_code AdvanceLogic    ( f32 DeltaTime );

        xbool       Impact          ( const collider::collision& Collision ) ;

//------------------------------------------------------------------------------
//  Private Types

        enum state
        {
            FLYING, 
            SETTLING, 
            DRILLING, 
            ARMED, 
            TRIPPED,    // Only needed when shot.  Blows up on next logic.
            GONE        // Used on client until server kills it.
        };

//------------------------------------------------------------------------------
//  Private Data

protected:

        state           m_State;
        xbool           m_InvalidSurface;

        // m_SettleNormal is in ancestor.

//------------------------------------------------------------------------------
//  Internal Stuff

friend game_mgr;

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     MineCreator( void );

//==============================================================================
#endif // MINE_HPP
//==============================================================================

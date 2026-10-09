//==============================================================================
//
//  Flag.hpp
//
//==============================================================================

#ifndef FLAG_HPP
#define FLAG_HPP

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

class flag : public arcing
{

//------------------------------------------------------------------------------
//  Public Functions

public:

        void        Render          ( void );

        // Used for flag on stand during map loading.
        void        Initialize      ( const vector3&   Position,
                                            radian     Yaw,
                                            u32        TeamBits );

        // Used when flag is thrown from player.  (Value used by Hunter.)
        void        Initialize      ( const vector3&   Position,
                                            radian     Yaw, 
                                      const vector3&   Velocity,
                                            object::id OriginID,
                                            u32        TeamBits,
                                            s32        Value = 3 ); // 3 = WHITE for CTF

        void        CommonInit      ( const vector3&   Position,
                                            radian     Yaw,
                                      const vector3&   Velocity,
                                            object::id OriginID,
                                            u32        TeamBits,
                                            s32        Value ); 

        update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
        void        OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnProvideInit   (       bitstream& BitStream );

        void        OnAdd           ( void );
        void        OnRemove        ( void );
        void        OnPain          ( const pain& Pain );
        s32         GetValue        ( void ) const;

        radian      GetYaw          ( void ) const;
        f32         GetPainRadius   ( void ) const;
        vector3     GetPainPoint    ( void ) const;

        const xwchar* GetLabel      ( void ) const;

//------------------------------------------------------------------------------
//  Private Functions

protected:

        xbool       Impact          ( const collider::collision& Collision ) ;

        update_code AdvanceLogic    ( f32 DeltaTime );

//------------------------------------------------------------------------------
//  Private Data

protected:

        radian      m_Yaw;
        s32         m_Value;        // Value of flag (hunter), 0-3 only.
        xbool       m_TimeOut;      // Should this flag time out?
        f32         m_TimeOutAge;   // Age to return/vanish spontaneously.
        s32         m_PointLight;   // Handle to persistent point light.
        xwchar      m_Label[32];    // Label.

//------------------------------------------------------------------------------
//  Internal Stuff

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     FlagCreator( void );

// Returns model of flag to use in draw
model*      AnimateFlagShape( shape* FlagShape, s32 Frame, f32 DeltaTime ) ;

//==============================================================================
#endif // FLAG_HPP
//==============================================================================

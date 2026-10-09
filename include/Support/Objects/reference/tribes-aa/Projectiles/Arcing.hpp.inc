//==============================================================================
//
//  Arcing.hpp
//
//==============================================================================

#ifndef ARCING_HPP
#define ARCING_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "AudioMgr/Audio.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class arcing : public object
{

//------------------------------------------------------------------------------
//  Public Functions

public:
                    arcing          ( void );

        update_code OnAdvanceLogic  ( f32 DeltaTime );
        update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
        void        OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnProvideInit   (       bitstream& BitStream );
        void        OnAdd           ( void );
        void        OnRemove        ( void );
        void        OnCollide       ( u32 AttrBits, collider& Collider );

        void        Initialize      ( const vector3&   Position,
                                      const vector3&   StartPos,
                                      const vector3&   Velocity,
                                            object::id OriginID,
                                            object::id ExcludeID,
                                            u32        TeamBits );

        void        Initialize      ( const vector3&   Position,
                                      const vector3&   Velocity,
                                            object::id OriginID,
                                            object::id ExcludeID,
                                            u32        TeamBits );

        void        SetScoreID      ( const object::id ScoreID );

virtual void        SetOriginID     ( object::id       OriginID );
        object::id  GetOriginID     ( void ) { return( m_OriginID ); }

        vector3     GetVelocity     ( void ) const;
        void        SetVelocity     ( const vector3&   Velocity );
static  f32         GetGravity      ( void ) { return 25.0f; }

const   vector3&    GetRenderPos    ( void );    

//------------------------------------------------------------------------------
//  Private Functions

protected:

virtual update_code AdvanceLogic    ( f32 DeltaTime );
        update_code AdvancePhysics  ( f32 DeltaTime );
virtual xbool       Impact          ( const collider::collision& Collision ); 
        s32         RenderPrep      ( void );

//------------------------------------------------------------------------------
//  Private Types

    enum state
    {
        STATE_FALLING,
        STATE_SLIDING,
        STATE_SETTLED,
    };

//------------------------------------------------------------------------------
//  Private Data

protected:

    f32             m_Gravity;      // Overridable
    vector3         m_Velocity;     // Velocity in world
    vector3         m_StartPos;     // Creation point in world    
    f32             m_Age;          // Age in seconds
    f32             m_Update;       // Time to next update
    f32             m_Interval;     // Time between updates w/o collisions
    f32             m_Pending;      // Time not processed in Logic
    xbool           m_Hidden;       // True: client predicts removal
    xbool           m_Accurate;     // True: must send accurate Position
    state           m_State;        // Falling, Sliding, or Settled
    vector3         m_SettleNormal; // Settle AND slide surface normal

    object::id      m_OriginID;     // Object ID of source
    object::id      m_ScoreID;      // What object gets points if any
    object::id      m_ExcludeID;
    object::id      m_ObjectHitID;

    s32             m_SfxHandle;
    s32             m_SoundID;

    xbool           m_Blending;
    f32             m_BlendTimer;
    vector3         m_BlendVector;
    vector3         m_BlendPos;
    xbool           m_BlendReady;

    f32             m_RunFactor;
    f32             m_RiseFactor;
    f32             m_DragSpeed;
    f32             m_Drag;
    f32             m_SlowSlideTime;

    u32             m_CollisionAttr;

    shape_instance  m_Shape; 
    u32             m_ShapeDrawFlags;
};

//==============================================================================
#endif // ARCING_HPP
//==============================================================================

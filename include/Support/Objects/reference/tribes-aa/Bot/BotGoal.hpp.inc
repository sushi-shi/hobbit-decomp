//==============================================================================
//
//  BotGoal.hpp
//
//==============================================================================

#ifndef BOTGOAL_HPP
#define BOTGOAL_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "Objects/Player/PlayerObject.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  CLASS BOT_GOAL
//==============================================================================

class bot_goal
{

//------------------------------------------------------------------------------
//  Public Values
//------------------------------------------------------------------------------
public:
    typedef enum
    {
        ID_NO_GOAL = 0,
        
        ID_GO_TO_LOCATION,  // Navigate to a m_targetLocation
        ID_PURSUE_OBJECT,   // Navigate to m_targetObjectID's object
        ID_PURSUE_LOCATION, // Navigate to a changing location
        ID_PATROL,          // Patrol an area and fire on enemies
        ID_ENGAGE,          // Fight player id'd by m_targetObjectID
        ID_ROAM,
        ID_LOADOUT,
        ID_PICKUP,
        ID_MORTAR,
        ID_REPAIR,
        ID_DESTROY,
        ID_ANTAGONIZE,
        ID_DEPLOY,
        ID_SNIPE,
        
        ID_NUM_GOALS
    }  goal_id;

    typedef enum
    {
        STATE_NO_STATE = 0,
        
        STATE_ENGAGE_GOTO,
        STATE_ENGAGE_FIGHTING,
        
        STATE_NUM_STATES
    }  goal_state;
    
        
//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
public:
    				    bot_goal( void );
    goal_id             GetGoalID( void ) const;
    void                SetGoalID( goal_id id );
    goal_state          GetGoalState( void ) const;
    void                SetGoalState( goal_state state );
    f32                 GetUpdateTimeIncrement( void ) const { return m_UpdateTimeIncrements[m_GoalID]; };

    const vector3&      GetTargetLocation( void ) const;
    void                SetTargetLocation( const vector3& TargetLocation );
    const object*       GetTargetObject( void ) const;
    void                SetTargetObject( object::id TargetObjectID );
    const vector3&      GetAuxLocation( void ) const;
    void                SetAuxLocation( const vector3& AuxLocation );
    const vector3&      GetAuxDirection( void ) const;
    void                SetAuxDirection( const vector3& AuxDirection );
    void                SetAuxDirection( radian AuxDirection );
    object::type        GetObjectType( void ) const;
    void                SetObjectType( object::type ObjectType );
    xstring             GetGoalString( void ) const;
    
private:
    goal_id             m_GoalID;
    goal_state          m_GoalState;
    static f32          m_UpdateTimeIncrements[ID_NUM_GOALS];
    
    // Targets -- zero or more of these may be used depending on the GoalID
    vector3             m_TargetLocation;
    object::id          m_TargetObject;
    vector3             m_AuxLocation;
    vector3             m_AuxDirection;
    object::type        m_ObjectType;
};


//==============================================================================
#endif // ndef BOTGOAL_HPP
//==============================================================================

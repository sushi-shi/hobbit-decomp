//==============================================================================
//
//  BotGoal.cpp
//
//==============================================================================
#include "BotGoal.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "Objects/Player/PlayerObject.hpp"

//==============================================================================
//  INCLUDES
//==============================================================================

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================
f32 bot_goal::m_UpdateTimeIncrements[bot_goal::ID_NUM_GOALS]; // static member

vector3* g_DebugTargetLoc = NULL;
//==============================================================================
//  FUNCTIONS
//==============================================================================
bot_goal::bot_goal( void ) :
    m_GoalID( ID_NO_GOAL )
    , m_GoalState( STATE_NO_STATE )
    , m_TargetObject( obj_mgr::NullID )
{
    s32 i;

    // Init everything
    for ( i = 0; i < bot_goal::ID_NUM_GOALS; ++i )
    {
        m_UpdateTimeIncrements[i] = 1.0f;
    }

    // Adjust those that should differ from init value
    m_UpdateTimeIncrements[bot_goal::ID_GO_TO_LOCATION]     = 5.0f;

#if DEBUG_COST_ANALYSIS
    if (g_bCostAnalysisOn)
        g_DebugTargetLoc = &m_TargetLocation;
#endif

    m_AuxDirection.Zero();
}

bot_goal::goal_id bot_goal::GetGoalID( void ) const
{
    return m_GoalID;
}

void bot_goal::SetGoalID( goal_id ID )
{
    m_GoalID = ID;
    //x_printf( "New goal: %s\n", (const char *)GetGoalString() );
}

bot_goal::goal_state bot_goal::GetGoalState( void ) const
{
    return m_GoalState;
}

void bot_goal::SetGoalState( goal_state State )
{
    m_GoalState = State;
}

const vector3& bot_goal::GetTargetLocation( void ) const
{
    return m_TargetLocation;
}

void bot_goal::SetTargetLocation( const vector3& TargetLocation )
{
    m_TargetLocation = TargetLocation;
};

const object* bot_goal::GetTargetObject( void ) const
{
    return (ObjMgr.GetObjectFromID(m_TargetObject));
}

void bot_goal::SetTargetObject( object::id TargetObjectID )
{
    m_TargetObject = TargetObjectID;
}

const vector3& bot_goal::GetAuxLocation( void ) const
{
    return m_AuxLocation;
}

void bot_goal::SetAuxLocation( const vector3& AuxLocation )
{
    m_AuxLocation = AuxLocation;
};

const vector3& bot_goal::GetAuxDirection( void ) const
{
    return m_AuxDirection;
}

void bot_goal::SetAuxDirection( const vector3& AuxDirection )
{
    ASSERT(AuxDirection.LengthSquared() < 100.0f);
    m_AuxDirection = AuxDirection;
};

void bot_goal::SetAuxDirection( radian AuxDirection )
{
    m_AuxDirection.X = -1.0f;
    m_AuxDirection.Y = (f32)AuxDirection;
    m_AuxDirection.Z = -1.0f;
};

object::type bot_goal::GetObjectType( void ) const
{
    return m_ObjectType;
}

void bot_goal::SetObjectType( object::type ObjectType )
{
    m_ObjectType = ObjectType;
};

xstring bot_goal::GetGoalString( void ) const
{
    switch ( m_GoalID )
    {
    case ID_NO_GOAL:
        return "NO_GOAL";
        
    case ID_GO_TO_LOCATION:
        return "GO_TO_LOCATION";
        
    case ID_PURSUE_OBJECT:
        return "PURSUE_OBJECT";
        
    case ID_PURSUE_LOCATION:
        return "PURSUE_LOCATION";
        
    case ID_PATROL:
        return "PATROL";
        
    case ID_ENGAGE:
        return "ENGAGE";
        
    case ID_ROAM:
        return "ROAM";

    case ID_LOADOUT:
        return "LOADOUT";

    case ID_PICKUP:
        return "PICKUP";

    case ID_MORTAR:
        return "MORTAR";

    case ID_REPAIR:
        return "REPAIR";

    case ID_DESTROY:
        return "DESTROY";

    case ID_ANTAGONIZE:
        return "ANTAGONIZE";

    case ID_DEPLOY:
        return "DEPLOY";

    case ID_SNIPE:
        return "SNIPE";

    default:
        ASSERT( 0 );
        return "";
    }
}


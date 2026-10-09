//==============================================================================
//  
//  Task.hpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Task.hpp"
#define TASK_LIFETIME 50
//==============================================================================
//  TASK FUNCTIONS
//==============================================================================

bot_task::bot_task ( void )
{
    Initialize();
}

//==============================================================================

bot_task::~bot_task ( void )
{
}

//==============================================================================

void bot_task::Initialize( void )
{
    m_TaskID = TASK_TYPE_NONE;
    m_Priority = 0;
    m_BotAssigned = 0;
    m_WaveAssigned = 0;
    m_nMaxBotsToBeAssigned = 1;
    m_State = TASK_STATE_INACTIVE;
    m_TimeToReset = TASK_LIFETIME;
    m_TargetID = obj_mgr::NullID;
    m_TargetPos.Zero();
}

//==============================================================================

void bot_task::operator=   ( const bot_task& RHS )
{
//    m_TaskID                = RHS.m_TaskID;
    m_Priority              = RHS.m_Priority;
    m_State                 = RHS.m_State;
    m_BotAssigned           = RHS.m_BotAssigned;      
    m_WaveAssigned          = RHS.m_WaveAssigned;
    m_TargetID              = RHS.m_TargetID;
    m_TargetPos             = RHS.m_TargetPos;
    m_TimeToReset           = RHS.m_TimeToReset;
    m_nMaxBotsToBeAssigned  = RHS.m_nMaxBotsToBeAssigned;
}

//==============================================================================

bot_task::task_type bot_task::GetID ( void ) const
{
    return m_TaskID;
}

//==============================================================================

s32 bot_task::GetPriority( void ) const
{
    return m_Priority;
}

//==============================================================================

void bot_task::SetPriority( s32 Value )
{
    m_Priority = Value;
}

//==============================================================================

xbool bot_task::IsAssigned ( void ) const 
{
    return ( m_BotAssigned );
}

//==============================================================================

u32 bot_task::GetAssignedBot( void ) const
{
    return m_BotAssigned;
}

//==============================================================================

s32 bot_task::GetAssignedBotID( void ) const
{
    if (!m_BotAssigned)
        return -1;
    u32 Assigned = m_BotAssigned;
    for (s32 i = 0; i < 32; i++)
    {
        if (Assigned & 1)
            return i;
        else
            Assigned>>=1;
    }
    ASSERT(0);
    return -1;
}

//==============================================================================

void bot_task::SetAssignedBot( s32 BotID )
{
    ASSERT(BotID >= 0);
    m_BotAssigned |= (1 << BotID);
}

//==============================================================================

void bot_task::UnassignBot( s32 BotID )
{
    ASSERT(BotID >= 0);
    if (!(m_BotAssigned & (1 << BotID)))
        return;
    m_BotAssigned &= ~(1 << BotID);
}

//==============================================================================

const object* bot_task::GetTargetObject()
{
    return ObjMgr.GetObjectFromID(m_TargetID);
}

//==============================================================================

void bot_task::SetTargetObject( object* Target )
{
    if (Target)
    {
        m_TargetID = Target->GetObjectID();
    }
    else
    {
        m_TargetID = obj_mgr::NullID;
    }

}

//==============================================================================

char *bot_task::GetTaskName( void ) const
{

    switch (m_TaskID)
    {
        case TASK_TYPE_NONE:
            return "O:None        ";
        case TASK_TYPE_DEFEND_FLAG:
            return "O:H.Defend    ";
        break;
        case TASK_TYPE_LIGHT_DEFEND_FLAG:
            return "O:L.Defend  ";
        break;
        case TASK_TYPE_RETRIEVE_FLAG:
            return "-:Retrieve  ";
        break;
        case TASK_TYPE_CAPTURE_FLAG:
            return "X:Capture   ";
        break;
        case TASK_TYPE_ESCORT_FLAG_CARRIER:
            return "X:Escort    ";
        break;

        case TASK_TYPE_GOTO:
            return "-:Goto      ";
        break;

        case TASK_TYPE_DEPLOY_TURRET:
            return "O:Turret    ";
        break;
        case TASK_TYPE_DEPLOY_INVEN:
            return "O:Inven     ";
        break;
        case TASK_TYPE_DEPLOY_SENSOR:
            return "O:Sensor    ";
        break;
        case TASK_TYPE_REPAIR:
            return "O:Fix       ";
        break;
        case TASK_TYPE_DESTROY:
            return "X:Destroy   ";
        break;
        case TASK_TYPE_MORTAR:
            return "X:Mortar    " ;
        break;
        case TASK_TYPE_SNIPE:
            return "O:xSnipe     ";
        break;

        case TASK_TYPE_ENGAGE_ENEMY:
            return "O:Engage    ";
        case TASK_TYPE_ANTAGONIZE:
            return "O:Antagonize";
        break;

        default:
            ASSERT (m_TaskID >= TASK_TYPE_NONE && m_TaskID < TASK_TYPE_TOTAL);
            return NULL;
    }
}

//==============================================================================

char *bot_task::GetTaskStateName( void ) const
{
    switch ( m_State )
    {
    case TASK_STATE_INACTIVE:
        return "INACTIVE  ";

    case TASK_STATE_LOADOUT:
        return "LOADOUT   ";

    case TASK_STATE_GOTO_POINT:
        return "GOTO_POINT";

    case TASK_STATE_PURSUE:
        return "PURSUE    ";

    case TASK_STATE_ROAM_AREA:
        return "ROAM_AREA ";

    case TASK_STATE_ATTACK:
        return "ATTACK    ";

    case TASK_STATE_MORTAR:
        return "MORTAR    ";

    case TASK_STATE_SNIPE:
        return "SNIPE     ";

    case TASK_STATE_REPAIR:
        return "REPAIR    ";

    case TASK_STATE_DESTROY:
        return "DESTROY   ";

    case TASK_STATE_DEPLOY_TURRET:
        return "TURRET    ";

    case TASK_STATE_DEPLOY_INVEN:
        return "INVEN     ";

    case TASK_STATE_DEPLOY_SENSOR:
        return "SENSOR    ";

    case TASK_STATE_ANTAGONIZE:
        return "ANTAGONIZE";
    default:
        ASSERT( (m_State >= TASK_STATE_START) && (m_State < TASK_STATE_TOTAL) );
        return "";
    }
}

//==============================================================================

default_loadouts::type  bot_task::GetIdealLoadout ( void ) const
{
        default_loadouts::type Type;

    switch  (m_TaskID)
    {
        case (TASK_TYPE_DEFEND_FLAG):    
        case (TASK_TYPE_DESTROY):
        case (TASK_TYPE_MORTAR):
        case (TASK_TYPE_DEPLOY_TURRET):
        case (TASK_TYPE_DEPLOY_INVEN):
        case (TASK_TYPE_DEPLOY_SENSOR):
            Type = default_loadouts::STANDARD_BOT_HEAVY;
        break;

        case (TASK_TYPE_RETRIEVE_FLAG):    
        case (TASK_TYPE_CAPTURE_FLAG):    
        case (TASK_TYPE_ENGAGE_ENEMY):
        case (TASK_TYPE_ESCORT_FLAG_CARRIER):    
        case (TASK_TYPE_GOTO):    
            Type = default_loadouts::STANDARD_BOT_LIGHT;
        break;

        default:
            Type = default_loadouts::STANDARD_BOT_LIGHT;
        break;
            // To do:  add default loadout types for the following:
        /*
        case (TASK_TYPE_DEPLOY_TURRET):   
            break;

        case (TASK_TYPE_DEPLOY_MINES):
            break;
        
        case (TASK_TYPE_REPAIR):
            break;

        case (TASK_TYPE_SNIPE):
            break;
        */
    };

    return Type;

}
//==============================================================================

s32 bot_task::GetNumAssignedBots() const
{
    s32 i; 
    s32 Num = 0;
    s32 BotsAssigned = m_BotAssigned;
        ;
    for (i = 0; i < 16; i++)
    {
        if (BotsAssigned & 1)
            Num++;
        BotsAssigned >>= 1;
        if (!BotsAssigned)
            break;
    }
    return Num;
}

//==============================================================================

void bot_task::Update( void )
{
    if (m_TaskID == TASK_TYPE_SNIPE)
        return;
    // State gets set by Master AI.
    // Update target position & priority value.
    object* Target = ObjMgr.GetObjectFromID(m_TargetID);
    if (Target)
        m_TargetPos = Target->GetPosition();
    if (m_TimeToReset > 0)
        m_TimeToReset--;
}

//==============================================================================

xbool bot_task::IsImportant( void ) const
{
    return (m_Priority >= 100);
}

//==============================================================================

void bot_task::ResetTimer ( void )
{
    m_TimeToReset = TASK_LIFETIME;
}

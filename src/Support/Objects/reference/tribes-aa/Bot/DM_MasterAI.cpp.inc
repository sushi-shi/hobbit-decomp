//==============================================================================
//  
//  DM_MasterAI.cpp // Master AI for Deathmatch.
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "DM_MasterAI.hpp"

//==============================================================================
//  Storage
//==============================================================================
extern s32 TeamBitsFound;

//==============================================================================
//  DM MASTER AI FUNCTIONS
//==============================================================================

dm_master_ai::dm_master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
}

//==============================================================================

dm_master_ai::~dm_master_ai ( void )
{
    UnloadMission();
}

//==============================================================================


void dm_master_ai::UnloadMission( void )
{
    m_TeamBits = 0;
    m_EngageTask = NULL;

    m_MissionInitialized = FALSE;
}

//==============================================================================

void dm_master_ai::InitializeMission( void )
{
    ASSERT (!m_MissionInitialized);
    m_MissionInitialized = TRUE;

    // Set the team bit flag appropriately.
    UpdatePlayerPointers();

    m_nTasks = 1;

    m_GoalList[0].SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);

    m_EngageTask = &m_GoalList[0];
}
//==============================================================================

void dm_master_ai::UpdatePlayerPointers( void )
{
    // Set the team bit flag appropriately.

    m_PlayersChanged = FALSE;
    player_object* CurrPlayer;
    bot_object*    CurrBot;
    s32 i = 0;
    s32 j = 0;

    // Grab all the player object pointers.
    ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
    while ( (CurrPlayer = (player_object*)(ObjMgr.GetNextInType())) != NULL )
    {
        // All players have to be enemies, since this is a non-team game.
        // This guy plays for another team!
        ASSERT (j < 32);
        if (m_OtherTeamPlayer[j] != CurrPlayer)
        {
            m_PlayersChanged = TRUE;
            m_OtherTeamPlayer[j] = CurrPlayer;
        }
        j++;
    }
    ObjMgr.EndTypeLoop();

    // Grab all the bot object pointers.
    ObjMgr.StartTypeLoop( object::TYPE_BOT );
    while ( (CurrBot = (bot_object*)(ObjMgr.GetNextInType())) != NULL )
    {
        if (!m_TeamBits)
        {
            if ( CurrBot->GetTeamBits() & TeamBitsFound )
                // This guy is on another guy's team.
                continue;
            else 
            {
                // Set this bot's bits as our team bits.
                m_TeamBits = CurrBot->GetTeamBits();
                TeamBitsFound |= m_TeamBits;
            }
        }
        if ( CurrBot->GetTeamBits() & m_TeamBits )
        {
            ASSERT (i < 16);
            if (m_TeamBot[i] != (bot_object*)CurrBot)
            {
                m_PlayersChanged = TRUE;
                m_TeamBot[i] = (bot_object*)CurrBot;
            }
            i++;
        }
        else if ( CurrBot->GetTeamBits() & ~m_TeamBits )
        {

            ASSERT (j < 32);
            if (m_OtherTeamPlayer[j] != CurrBot)
            {
                m_PlayersChanged = TRUE;
                m_OtherTeamPlayer[j] = CurrBot;
            }
            j++;
        }
    }
    ObjMgr.EndTypeLoop();
   
    m_nTeamBots = i;
    m_nOtherTeamPlayers = j; 
}


//==============================================================================

void dm_master_ai::PollAllObjects ( void )
{
    ASSERT(m_MissionInitialized);

    // Is there anyone playing?
    if (m_nTeamBots == 0 || m_nOtherTeamPlayers == 0)
        return;

    // Everyone for themselves, right?
    ASSERT(m_nTeamBots == 1);

    f32 DistSq;
    s32 ClosestEnemy = FindClosestEnemy(m_TeamBot[0]->GetPosition(), DistSq);
   
    if (ClosestEnemy != -1)
    {
        m_EngageTask->SetTargetObject(m_OtherTeamPlayer[ClosestEnemy]);
        m_EngageTask->SetState(bot_task::TASK_STATE_ATTACK);
        if (DistSq < PRIORITY_RADIUS_SQ)
            m_EngageTask->SetPriority(100);
        else
            m_EngageTask->SetPriority(50);
    }
    else
    {
        // Everyone else must have died!  YOU KICK ASS, BOT!
        m_EngageTask->SetTargetObject(NULL);
        m_EngageTask->SetState(bot_task::TASK_STATE_INACTIVE);
    }
}

//==============================================================================

void dm_master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
    s32 i;

    for (i = 0; i < m_nTeamBots; i++)
    {
        if (!m_TeamBot[i]->HasTask())
            AssignBotTask(i, m_EngageTask);
        if (m_TeamBot[i]->GetTask()->GetPriority() == 100)
        {
            m_TeamBot[i]->DisableLoadout();
        }
        else
            m_TeamBot[i]->EnableLoadout();
    }
}


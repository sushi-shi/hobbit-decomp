//==============================================================================
//  
//  TDM_MasterAI.cpp // Master AI for Team Deathmatch.
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "TDM_MasterAI.hpp"

//==============================================================================
//  Storage
//==============================================================================
extern s32 TeamBitsFound;

//==============================================================================
//  TDM MASTER AI FUNCTIONS
//==============================================================================

tdm_master_ai::tdm_master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
}

//==============================================================================

tdm_master_ai::~tdm_master_ai ( void )
{
    UnloadMission();
}

//==============================================================================


void tdm_master_ai::UnloadMission( void )
{
    m_TeamBits = 0;
    x_memset(m_EngageTask, NULL, NUM_TDM_TASKS);

    m_MissionInitialized = FALSE;
}

//==============================================================================

void tdm_master_ai::InitializeMission( void )
{
    ASSERT (!m_MissionInitialized);
    m_MissionInitialized = TRUE;

    // Set the team bit flag appropriately.
    UpdatePlayerPointers();

    ASSERT(m_nTeamBots <= NUM_TDM_TASKS);

    ResetTaskList();

}

//==============================================================================

void tdm_master_ai::ResetTaskList( void )
{
    m_nTasks = m_nTeamBots;

    s32 i;
    for (i = 0;i < m_nTeamBots; i++)
    {
        m_GoalList[i].SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
        m_EngageTask[i] = &m_GoalList[i];
    }
}

//==============================================================================

void tdm_master_ai::PollAllObjects ( void )
{
    ASSERT(m_MissionInitialized);

    UpdatePlayerPointers();

    if (m_nTeamBots > m_nTasks)
    {
        ResetTaskList();
    }

    // Is there anyone playing?
    if (m_nTeamBots == 0 || m_nOtherTeamPlayers == 0)
        return;

    s32 i;
    f32 DistSq;
    for (i = 0; i < m_nTeamBots; i++)
    {
        s32 ClosestEnemy = FindClosestEnemy(m_TeamBot[i]->GetPosition(), DistSq);
   
        if (ClosestEnemy != -1)
        {
            m_EngageTask[i]->SetTargetObject(m_OtherTeamPlayer[ClosestEnemy]);
            m_EngageTask[i]->SetState(bot_task::TASK_STATE_ATTACK);

            if (DistSq < PRIORITY_RADIUS_SQ)
                m_EngageTask[i]->SetPriority(100);
            else
                m_EngageTask[i]->SetPriority(50);
        }
        else
        {
            // Everyone else is dead.  
            m_EngageTask[i]->SetTargetObject(NULL);
            m_EngageTask[i]->SetState(bot_task::TASK_STATE_INACTIVE);
        }
    }
}

//==============================================================================

void tdm_master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
    s32 i;

    for (i = 0; i < m_nTeamBots; i++)
    {
        AssignBotTask(i, m_EngageTask[i]);
        if (m_TeamBot[i]->GetTask()->GetPriority() == 100)
        {
            m_TeamBot[i]->DisableLoadout();
        }
        else
            m_TeamBot[i]->EnableLoadout();
    }
}


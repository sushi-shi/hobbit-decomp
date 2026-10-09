//==============================================================================
//  
//  SPM_MasterAI.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "SPM_MasterAI.hpp"

//==============================================================================
//  Storage
//==============================================================================
extern s32 TeamBitsFound;

//==============================================================================
//  SPM MASTER AI FUNCTIONS
//==============================================================================

spm_master_ai::spm_master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
}

//==============================================================================

spm_master_ai::~spm_master_ai ( void )
{
    UnloadMission();
}

//==============================================================================


void spm_master_ai::UnloadMission( void )
{
    m_TeamBits = 0;
    TeamBitsFound = 0;

    m_MissionInitialized = FALSE;
}

//==============================================================================

void spm_master_ai::InitializeMission( void )
{
    ASSERT (!m_MissionInitialized);
    m_MissionInitialized = TRUE;

    // Set the team bit flag appropriately.
    UpdatePlayerPointers();

    // Initialize the specific tasks.
    m_nTasks = NUM_SPM_TASKS;

    m_GoalList[0].SetID(bot_task::TASK_TYPE_GOTO);
    m_GotoTask = &m_GoalList[0];

    m_GoalList[1].SetID(bot_task::TASK_TYPE_REPAIR);
    m_RepairTask = &m_GoalList[1];

    m_GoalList[2].SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
    m_EngageTask = &m_GoalList[2];
    
    m_GoalList[3].SetID(bot_task::TASK_TYPE_DEFEND_FLAG);
    m_DefendTask = &m_GoalList[3];

    m_DefendTask->SetState( bot_task::TASK_STATE_ROAM_AREA );

    m_GoalList[4].SetID(bot_task::TASK_TYPE_GOTO);
    m_RoamTask = &m_GoalList[4];

    m_GoalList[5].SetID(bot_task::TASK_TYPE_MORTAR);
    m_MortarTask = &m_GoalList[5];

    m_GoalList[6].SetID(bot_task::TASK_TYPE_DESTROY);
    m_DestroyTask = &m_GoalList[6];

    m_GoalList[7].SetID(bot_task::TASK_TYPE_ANTAGONIZE);
    m_AntagonizeTask = &m_GoalList[7];

    m_bDeathMatchMode = FALSE;
    m_DeathMatchStartTask = 8;
    for (s32 i = m_DeathMatchStartTask; i < NUM_SPM_TASKS; i++)
    {
        m_GoalList[i].SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
        m_GoalList[i].SetMaxBots(1);
    }
}

//==============================================================================

void spm_master_ai::PollAllObjects ( void )
{
    ASSERT(m_MissionInitialized);

    UpdatePlayerPointers();
    m_EngageTask->SetMaxBots(m_nTeamBots);
    m_DefendTask->SetMaxBots(m_nTeamBots);   
    m_RoamTask->SetMaxBots(m_nTeamBots);
    m_DestroyTask->SetMaxBots(m_nTeamBots);
    m_MortarTask->SetMaxBots(m_nTeamBots);

    m_GotoTask->SetMaxBots(1);
    m_RepairTask->SetMaxBots(1);

    if (m_nTeamBots == 0)
        return;

    ResetAssignments();

}

//==============================================================================

void spm_master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
    s32 i;
    s32 ClosestBot;
    s32 nBotsToAssign;
    s32 CurrBot = 0;
    const object* Target;
    s32 Unassigned = m_nTeamBots;

    if (m_bDeathMatchMode)
    {
        f32 DistSq;
        for (i = m_DeathMatchStartTask; i < m_DeathMatchStartTask+m_nTeamBots; i++)
        {
            CurrBot = i - m_DeathMatchStartTask;

            if (!(m_GoalList[i].GetAssignedWave() & m_TeamBot[CurrBot]->GetWaveBit()))
            {
                // Don't assign this task if the wave bits don't match.
                continue;
            }
            ClosestBot = FindClosestEnemy(m_TeamBot[CurrBot]->GetPosition(), DistSq);
            if (ClosestBot != -1)
            {
                m_GoalList[i].SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
                m_GoalList[i].SetState(bot_task::TASK_STATE_ATTACK);
                m_GoalList[i].SetTargetObject(m_OtherTeamPlayer[ClosestBot]);

                if (DistSq < PRIORITY_RADIUS_SQ)
                    m_GoalList[i].SetPriority(100);
                else
                    m_GoalList[i].SetPriority(50);

                AssignBotTask(CurrBot, &m_GoalList[i]);
                Unassigned--;
            }
            else    // all bots must be dead.
            {
                m_GoalList[i].SetID(bot_task::TASK_TYPE_NONE);
                m_GoalList[i].SetState(bot_task::TASK_STATE_INACTIVE);
                m_GoalList[i].SetTargetObject(NULL);
            }
        }
    }
    f32 Temp;

    // Assign Special Forces:
    if (m_RepairTask->GetPriority())
    {
#if defined (acheng)
        x_printfxy(10, 5, "Target Object Health: %2.2f", m_RepairTask->GetTargetObject()->GetHealth());
#endif
        if (m_RepairTask->GetTargetObject()->GetHealth() == 1)
            m_RepairTask->SetPriority(0);
        else
        {
            if (m_RepairBot == -1)
            {
                // Find the closest bot to a repair pack.
                pickup* pPack = NULL;
                pickup* pClosestPack = NULL;
                f32 ClosestPackDistSQ = F32_MAX;

                ObjMgr.StartTypeLoop( object::TYPE_PICKUP );
                while ( (pPack = (pickup*)ObjMgr.GetNextInType()) != NULL )
                {
                    if (pPack->GetKind() != pickup::KIND_ARMOR_PACK_REPAIR) 
                    {
                        continue;
                    }
        
                    // see if it's closer
                    const f32 DistSQ = (pPack->GetPosition() - m_RepairTask->GetTargetPos()).LengthSquared();

                    if ( DistSQ < ClosestPackDistSQ )
                    {
                        // Best candidate so far
                        pClosestPack = pPack;
                        ClosestPackDistSQ = DistSQ;
                    }
                }

                ObjMgr.EndTypeLoop();

                vector3 PackPos;
                if ( pClosestPack )
                {
                    PackPos = pClosestPack->GetPosition();
                    ClosestBot = FindClosestAvailableBot(PackPos, Temp);
                }
                else ClosestBot = FindClosestAvailableBot(m_RepairTask, Temp);
            }
            else
                ClosestBot = m_RepairBot;
            if (ClosestBot != -1)
            {
                AssignBotTask(ClosestBot, m_RepairTask);
                m_TeamBot[ClosestBot]->DisableLoadout();
                Unassigned--;
            }
        }
    }

    if (m_GotoTask->GetPriority())
    {
        ClosestBot = FindClosestAvailableBot(m_GotoTask, Temp);
        if (ClosestBot != -1)
        {
            AssignBotTask(ClosestBot, m_GotoTask);
            m_TeamBot[ClosestBot]->DisableLoadout();
            Unassigned--;
        }
    }

    // Assign Offense:
    if (m_MortarTask->GetPriority())
    {
        Target = m_MortarTask->GetTargetObject();
        if (!Target || Target->GetHealth() <= 0)
        {
            m_MortarTask->SetID(bot_task::TASK_TYPE_MORTAR);
            m_MortarTask->SetState(bot_task::TASK_STATE_ANTAGONIZE);
            m_MortarTask->SetTargetObject(NULL);
        }

        // Only assign heavies.
        for (i = 0; i < m_nTeamBots && m_MortarTask->MaxBotsToBeAssigned() >
                                    m_MortarTask->GetNumAssignedBots(); i++)
        {
            if (m_TeamBot[i]->HasTask())    continue;
            if ( (m_MortarTask->GetAssignedWave() & m_TeamBot[i]->GetWaveBit())
                &&
                m_TeamBot[i]->GetArmorType() == player_object::ARMOR_TYPE_HEAVY)
            {
                AssignBotTask(i, m_MortarTask);
                Unassigned--;
                if (!m_TeamBot[i]->GetWeaponAmmoCount(player_object::INVENT_TYPE_WEAPON_MORTAR_GUN))
                {
                    player_object::loadout Loadout;
                    // Give the heavies without mortar ammo something to do.
                    default_loadouts::GetLoadout( m_TeamBot[i]->GetCharacterType(), default_loadouts::TASK_TYPE_MORTAR, Loadout );
                    m_TeamBot[i]->SetInventoryLoadout( Loadout );
                    m_TeamBot[i]->InventoryReload();
                }
                m_TeamBot[i]->DisableLoadout();
            }
        }

        /*// Add others to mortar task?
        nBotsToAssign = m_MortarTask->MaxBotsToBeAssigned() - 
                                    m_MortarTask->GetNumAssignedBots();
        for (i = 0; i < nBotsToAssign; i++)
        {
            ClosestBot = FindClosestAvailableBot(m_MortarTask);
            if (ClosestBot != -1)
                AssignBotTask(ClosestBot, m_MortarTask);
        }
        */
    }

    if (m_AntagonizeTask->GetPriority())
    {
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (m_TeamBot[i]->HasTask())    continue;
            if (m_AntagonizeTask->GetNumAssignedBots() < m_AntagonizeTask->MaxBotsToBeAssigned())
            {
                if (m_AntagonizeTask->GetAssignedWave() & m_TeamBot[i]->GetWaveBit())
                {
                    AssignBotTask(i, m_AntagonizeTask);
                    Unassigned--;
                    m_TeamBot[i]->DisableLoadout();
                }
            }
            else
                break;
        }
    }

    if (m_EngageTask->GetPriority())
    {
        Target = m_EngageTask->GetTargetObject();
        if (!Target || Target->GetHealth() <= 0)
        {
            m_EngageTask->SetID(bot_task::TASK_TYPE_NONE);
            m_EngageTask->SetState(bot_task::TASK_STATE_INACTIVE);
            m_EngageTask->SetTargetObject(NULL);
        }

        for (i = 0; i < m_nTeamBots; i++)
        {
            if (m_TeamBot[i]->HasTask())    continue;
            if (m_EngageTask->GetNumAssignedBots() < m_EngageTask->MaxBotsToBeAssigned())
            {
                if (m_EngageTask->GetAssignedWave() & m_TeamBot[i]->GetWaveBit())
                {
                    AssignBotTask(i, m_EngageTask);
                    Unassigned--;
                    m_TeamBot[i]->DisableLoadout();
                }
            }
            else
                break;
        }
    }

    if (m_RoamTask->GetPriority())
    {
        nBotsToAssign = m_RoamTask->MaxBotsToBeAssigned() - 
                            m_RoamTask->GetNumAssignedBots();
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (m_TeamBot[i]->HasTask())    continue;
            if (m_RoamTask->GetNumAssignedBots() < m_RoamTask->MaxBotsToBeAssigned())
            {
                if (m_RoamTask->GetAssignedWave() & m_TeamBot[i]->GetWaveBit())
                {
                    AssignBotTask(i, m_RoamTask);
                    Unassigned--;
                    m_TeamBot[i]->DisableLoadout();
                }
            }
            else
                break;
        }

        // Debug Test
#if defined (acheng)
        if (m_TeamBits == 1)
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (m_TeamBot[i]->GetTask() == NULL)
                ASSERT(0);
        }
#endif

    }


    f32 DistSq;
    if (m_DefendTask->GetPriority())
    {
        if(SetTaskDefend(m_DefendTask, m_DefendLocation, 50*50, 75*75, DistSq))
        {
            m_DefendTask->SetPriority(100);
        }
        else 
            m_DefendTask->SetPriority(50);

        // Assign Defense to the rest:
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (!m_TeamBot[i]->HasTask())
            {
                if (m_DefendTask->GetAssignedWave() & m_TeamBot[i]->GetWaveBit())
                {
                    AssignBotTask(i, m_DefendTask);
                    Unassigned--;
                    m_TeamBot[i]->DisableLoadout();
                }
            }
        }
    }
    if (!m_nOtherTeamPlayers)
    {
        m_AntagonizeTask->SetState(bot_task::TASK_STATE_ANTAGONIZE);
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (!m_TeamBot[i]->HasTask())
            {
                AssignBotTask(i, m_AntagonizeTask);
                Unassigned--;
            }
        }
    }

    static xbool NotCheckingUnassigned = 1;
    ASSERT (Unassigned == 0 || NotCheckingUnassigned);

}

//==============================================================================

void   spm_master_ai::Goto      ( const vector3& Location )                   
{
    m_GotoTask->SetPriority(100);
    m_GotoTask->SetMaxBots(1);
    m_GotoTask->SetState(bot_task::TASK_STATE_GOTO_POINT);
    m_GotoTask->SetTargetObject(NULL);
    m_GotoTask->SetTargetPos(Location);
}

//==============================================================================

void   spm_master_ai::Repair    ( object::id ObjID )                   
{
    m_RepairTask->SetPriority(100);
    m_RepairTask->SetMaxBots(1);
    m_RepairTask->SetState(bot_task::TASK_STATE_REPAIR);
    m_RepairTask->SetTargetObject(ObjMgr.GetObjectFromID(ObjID));
}

//==============================================================================

void   spm_master_ai::Roam      ( const vector3& Location, u32 WaveBit )                   
{
    ResetPriorities(WaveBit);
    m_RoamTask->SetPriority(100);
    m_RoamTask->SetAssignedWave(WaveBit);
    m_RoamTask->SetState(bot_task::TASK_STATE_ROAM_AREA);
    m_RoamTask->SetTargetObject(NULL);
    m_RoamTask->SetTargetPos(Location);
    m_RoamTask->SetMaxBots(m_nTeamBots);
}

//==============================================================================

void   spm_master_ai::Attack    ( object::id ObjID, u32 WaveBit )                   
{
    ResetPriorities(WaveBit);
    UpdatePlayerPointers();    
    m_EngageTask->SetPriority(100); 
    m_EngageTask->SetAssignedWave(WaveBit);
    m_EngageTask->SetState(bot_task::TASK_STATE_ATTACK);
    m_EngageTask->SetTargetObject(ObjMgr.GetObjectFromID(ObjID));
    m_EngageTask->SetMaxBots(m_nTeamBots);
}

//==============================================================================

void   spm_master_ai::Defend    ( const vector3& Location, u32 WaveBit )                   
{
    m_DefendLocation = Location;
    ResetPriorities(WaveBit);
    m_DefendTask->SetPriority(100);
    m_DefendTask->SetAssignedWave(WaveBit);
    m_DefendTask->SetMaxBots(m_nTeamBots);
}

//==============================================================================

void spm_master_ai::Mortar              ( object::id ObjID, u32 WaveBit)
{
    ResetPriorities(WaveBit);
    UpdatePlayerPointers();    
    m_MortarTask->SetPriority(100); 
    m_MortarTask->SetAssignedWave(WaveBit);
    m_MortarTask->SetState(bot_task::TASK_STATE_MORTAR);
    m_MortarTask->SetTargetObject(ObjMgr.GetObjectFromID(ObjID));
    m_MortarTask->SetMaxBots(m_nTeamBots);
}

//==============================================================================

void spm_master_ai::Destroy             ( object::id ObjID, u32 WaveBit)
{
    ResetPriorities(WaveBit);
    m_EngageTask->SetPriority(100); 
    m_EngageTask->SetAssignedWave(WaveBit);
    m_EngageTask->SetState(bot_task::TASK_STATE_DESTROY);
    m_EngageTask->SetTargetObject(ObjMgr.GetObjectFromID(ObjID));
    m_EngageTask->SetMaxBots(m_nTeamBots);
}

//==============================================================================

void spm_master_ai::DeathMatch          ( u32 WaveBit )
{
    ResetPriorities(WaveBit);
    UpdatePlayerPointers();
    s32 i;
    m_bDeathMatchMode = TRUE;

    for (i = m_DeathMatchStartTask; i < m_DeathMatchStartTask+MAX_BOTS_PER_TEAM; i++)
    {
        m_GoalList[i].SetAssignedWave(WaveBit);
    }
}

//==============================================================================

void spm_master_ai::Antagonize          ( u32 WaveBit )
{
    ResetPriorities(WaveBit);
    UpdatePlayerPointers();
    m_AntagonizeTask->SetPriority(100);
    m_AntagonizeTask->SetAssignedWave(WaveBit);
    m_AntagonizeTask->SetState(bot_task::TASK_STATE_ANTAGONIZE);
    m_AntagonizeTask->SetMaxBots(m_nTeamBots);
}

//==============================================================================

void spm_master_ai::ResetPriorities     ( u32 WaveBit )
{
    s32 i;
    for (i = 2; i < m_DeathMatchStartTask; i++)
    {
        if (m_GoalList[i].GetAssignedWave() & WaveBit)
        {
            m_GoalList[i].UnassignWave(WaveBit);

            // Disable this task from our list if no one is assigned to it.
            if (m_GoalList[i].GetAssignedWave() == 0)
            {
                m_GoalList[i].SetPriority(0);
                m_GoalList[i].SetState(bot_task::TASK_STATE_INACTIVE);
            }
        }
    }

    xbool DeathMatchBeingUsed = FALSE;
    if (m_bDeathMatchMode)
        for (i = m_DeathMatchStartTask; i < NUM_SPM_TASKS; i++)
        {
            if (m_GoalList[i].GetAssignedWave() & WaveBit)
            {
                m_GoalList[i].UnassignWave(WaveBit);
                if (m_GoalList[i].GetAssignedWave() != 0)
                    DeathMatchBeingUsed = TRUE;
            }
        }

    if (!DeathMatchBeingUsed)
        m_bDeathMatchMode = FALSE;
}

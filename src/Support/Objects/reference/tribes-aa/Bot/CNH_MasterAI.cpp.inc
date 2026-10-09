//==============================================================================
//  
//  CNH_MasterAI.cpp    // Capture and Hold Master AI
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "CNH_MasterAI.hpp"
#include "Support\GameMgr\GameMgr.hpp"
#include "Support\Objects\Projectiles\FlipFlop.hpp"

#define ENGAGE_RADIUS 50
#define RETREAT_RADIUS 75

#define ENGAGE_RADIUS_SQ  (ENGAGE_RADIUS * ENGAGE_RADIUS)
#define RETREAT_RADIUS_SQ (RETREAT_RADIUS * RETREAT_RADIUS)

//==============================================================================
//  Storage
//==============================================================================
extern s32 TeamBitsFound;
extern bot_object::bot_debug g_BotDebug;

//==============================================================================
//  CNH MASTER AI FUNCTIONS
//==============================================================================

cnh_master_ai::cnh_master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
    x_memset(m_Switch, 0, MAX_CNH_SWITCHES * sizeof(switch_data));
}

//==============================================================================

cnh_master_ai::~cnh_master_ai ( void )
{
    UnloadMission();
}

//==============================================================================

void cnh_master_ai::UnloadMission( void )
{
    m_TeamBits = 0;
    m_MissionInitialized = FALSE;
    TeamBitsFound = 0;
}

//==============================================================================

void cnh_master_ai::UpdatePlayerPointers( void )
{
    master_ai::UpdatePlayerPointers();

    if (m_PlayersChanged && m_MissionInitialized)
    {
        s32 i;
        for (i = 0; i < m_nSwitches; i++)
        {
            // Make sure we've already been initialized.
            ASSERT( m_Switch[i].ptDeploy
                &&  m_Switch[i].ptCapture
                &&  m_Switch[i].ptHeavyHold
                &&  m_Switch[i].ptHold
                &&  m_Switch[i].ptSnipe );
                
            m_Switch[i].ptDeploy->ResetAssignments();
            m_Switch[i].ptCapture->ResetAssignments();
            m_Switch[i].ptHeavyHold->ResetAssignments();
            m_Switch[i].ptHold->ResetAssignments();
            m_Switch[i].ptSnipe->ResetAssignments();
            m_Switch[i].iDeploying = -1;
        }

        for (i = 0; i < m_nTeamBots; i++ )
        {
            m_TeamBot[i]->SetTask(NULL);
        }

        m_iRepairing = -1;
    }
}

//==============================================================================

void cnh_master_ai::InitializeMission( void )
{
    // Set the team bit flag appropriately.
    UpdatePlayerPointers();

    ASSERT (!m_MissionInitialized);
    m_MissionInitialized = TRUE;

    // Grab all the necessary object pointers.
    InitializeSwitchData();

    s32 i,TaskCtr = 0;

    for (i = 0; i < MAX_BOTS; i++)
        BotSwitch[i] = -1;

    for (i = 0; i < m_nSwitches; i++)
    {
        // Give the task pointers something to point to.
        m_Switch[i].ptCapture       = &m_GoalList[TaskCtr++];
        m_Switch[i].ptHold          = &m_GoalList[TaskCtr++];
        m_Switch[i].ptHeavyHold     = &m_GoalList[TaskCtr++];
        m_Switch[i].ptDeploy        = &m_GoalList[TaskCtr++];
        m_Switch[i].ptSnipe         = &m_GoalList[TaskCtr++];
        ASSERT(TaskCtr < MAX_TASKS && "Need to increase master_ai::MAX_TASKS!");

        // Set the task id's
        m_Switch[i].ptCapture  ->SetID(bot_task::TASK_TYPE_CAPTURE_FLAG);
        m_Switch[i].ptHold     ->SetID(bot_task::TASK_TYPE_DEFEND_FLAG);
        m_Switch[i].ptHeavyHold->SetID(bot_task::TASK_TYPE_DEFEND_FLAG);
        m_Switch[i].ptDeploy   ->SetID(bot_task::TASK_TYPE_DEPLOY_TURRET);
        m_Switch[i].ptSnipe    ->SetID(bot_task::TASK_TYPE_SNIPE);

        // Set the task states
        m_Switch[i].ptCapture  ->SetState(bot_task::TASK_STATE_GOTO_POINT);
        m_Switch[i].ptHold     ->SetState(bot_task::TASK_STATE_ROAM_AREA);
        m_Switch[i].ptHeavyHold->SetState(bot_task::TASK_STATE_ROAM_AREA);
        m_Switch[i].ptDeploy   ->SetState(bot_task::TASK_STATE_DEPLOY_TURRET);
        m_Switch[i].ptSnipe    ->SetState(bot_task::TASK_STATE_SNIPE);

        // Set task target locations
        m_Switch[i].ptCapture  ->SetTargetPos(m_Switch[i].Position);
        m_Switch[i].ptHold     ->SetTargetPos(m_Switch[i].Position);
        m_Switch[i].ptHeavyHold->SetTargetPos(m_Switch[i].Position);

        m_Switch[i].ptCapture  ->SetMaxBots(MAX(2, m_nTeamBots/m_nSwitches));
        m_Switch[i].ptHold     ->SetMaxBots(3);
        m_Switch[i].ptHeavyHold->SetMaxBots(3);
        m_Switch[i].ptDeploy   ->SetMaxBots(1);
        m_Switch[i].ptSnipe    ->SetMaxBots(1);

        m_Switch[i].iDeploying = -1;
    }
    m_RepairTask = &m_GoalList[TaskCtr++];
    m_RepairTask->SetID(bot_task::TASK_TYPE_REPAIR);
    m_RepairTask->SetState(bot_task::TASK_STATE_REPAIR);
    m_RepairTask->SetMaxBots(1);
    m_iRepairing = -1;
    m_ForceReset = TRUE;
    m_HeldSwitchBits = 0;
    m_LastSwitchBits = 0;
    m_DeployBot = -1;
        /*
        m_GoalList[i].SetID(bot_task::TASK_TYPE_GOTO);
        m_CaptureTask[i] = &m_GoalList[i];
        m_CaptureTask[i]->SetState(bot_task::TASK_STATE_GOTO_POINT);
        m_CaptureTask[i]->SetMaxBots(MAX(2, m_nTeamBots/m_nSwitches));
    }
    for (j = m_nSwitches; j < 2*m_nSwitches; j++)
    {
        m_GoalList[j].SetID(bot_task::TASK_TYPE_DEFEND_FLAG);
        m_HoldTask[j-m_nSwitches] = &m_GoalList[j];
        m_HoldTask[j-m_nSwitches]->SetState(bot_task::TASK_STATE_ROAM_AREA);
        m_HoldTask[j-m_nSwitches]->SetMaxBots(MAX(2, m_nTeamBots/m_nSwitches));
        m_HoldTask[j-m_nSwitches]->SetTargetObject(NULL);
        m_HoldTask[j-m_nSwitches]->SetTargetPos(m_CaptureTask[j-m_nSwitches]->GetTargetPos());
    }
    for (j = 2*m_nSwitches; j < 3*m_nSwitches; j++)
    {
        m_GoalList[j].SetID(bot_task::TASK_TYPE_DEPLOY_TURRET);
        m_DeployTask[j-2*m_nSwitches] = &m_GoalList[j];
        m_DeployTask[j-2*m_nSwitches]->SetState(bot_task::TASK_STATE_DEPLOY_TURRET);
        m_DeployTask[j-2*m_nSwitches]->SetMaxBots(1);
        m_DeployTask[j-2*m_nSwitches]->SetTargetObject(NULL);
        m_iDeploying[j-2*m_nSwitches] = -1;
    }
    for (j = 3*m_nSwitches; j < 4*m_nSwitches; j++)
    {
        m_GoalList[j].SetID(bot_task::TASK_TYPE_REPAIR);
        m_RepairTask[j-3*m_nSwitches] = &m_GoalList[j];
        m_RepairTask[j-3*m_nSwitches]->SetState(bot_task::TASK_STATE_REPAIR);
        m_RepairTask[j-3*m_nSwitches]->SetMaxBots(1);
        m_RepairTask[j-3*m_nSwitches]->SetTargetObject(NULL);
        m_iRepairing[j-3*m_nSwitches] = -1;
    }
    for (j = 4*m_nSwitches; j < 5*m_nSwitches; j++)
    {
        m_GoalList[j].SetID(bot_task::TASK_TYPE_REPAIR);
        m_RepairTask[j-3*m_nSwitches] = &m_GoalList[j];
        m_RepairTask[j-3*m_nSwitches]->SetState(bot_task::TASK_STATE_REPAIR);
        m_RepairTask[j-3*m_nSwitches]->SetMaxBots(1);
        m_RepairTask[j-3*m_nSwitches]->SetTargetObject(NULL);
        m_iRepairing[j-3*m_nSwitches] = -1;
    }
    */
}

//==============================================================================

void cnh_master_ai::InitializeSwitchData( void )
{
    flipflop* pFlipFlop;
    s32 ctr = 0;
    ObjMgr.StartTypeLoop( object::TYPE_FLIPFLOP );
    while ( (pFlipFlop = (flipflop*)(ObjMgr.GetNextInType())) != NULL )
    {
        m_Switch[ctr].SwitchID = pFlipFlop->GetObjectID();
        m_Switch[ctr].bHeld = FALSE;
        m_Switch[ctr].Position = pFlipFlop->GetPosition();

        ctr++;
    }
    ObjMgr.EndTypeLoop();

    m_nSwitches = ctr;
    m_nDeployAssets = m_nSwitches;
}

//==============================================================================
void cnh_master_ai::Update ( void )
{
    UpdatePlayerPointers();

    // Set the new values for the collected data.
    PollAllObjects();

    if (m_ForceReset || m_LastSwitchBits != m_HeldSwitchBits)
    {
        m_ForceReset = FALSE;
        ResetAssignments();
    }

    if (m_nTeamBots)
    {
        // Determine priority values for each task.
        PrioritizeTasks     ( );

        // (Re)assign tasks based on bot availability.
        AssignAllTasks      ( );

        // Make sure each task state is current.
        UpdateAllTaskStates ( );
    }
}
//==============================================================================

void cnh_master_ai::QuerySwitchStatus( void )
{
    // Reset the value of held switches since it may have changed.
    m_nHeldSwitches = 0;
    flipflop* pSwitch;
    s32 i;
    u32 SwitchBits;
    f32 DistSq;
    m_LastSwitchBits = m_HeldSwitchBits;
    // Recount the switches in our possession.
    for (i = 0; i < m_nSwitches; i++)
    {
        pSwitch = (flipflop*)(ObjMgr.GetObjectFromID(m_Switch[i].SwitchID));
        SwitchBits = pSwitch->GetTeamBits();
        // Make sure we have this one, and no one else does.
        if (SwitchBits & m_TeamBits && (~m_TeamBits & SwitchBits) == 0)
        {
            m_HeldSwitchBits |= (1 << i);
            m_Switch[i].bHeld = TRUE;
            m_nHeldSwitches++;

            m_Switch[i].bUnderAttack = SetTaskDefend(m_Switch[i].ptHeavyHold, 
                pSwitch->GetPosition(), 
                ENGAGE_RADIUS_SQ, RETREAT_RADIUS_SQ, DistSq);

            *(m_Switch[i].ptHold) = *(m_Switch[i].ptHeavyHold);

            if (m_Switch[i].bUnderAttack)
            {
                // No loading out when there are invaders present.
                m_Switch[i].ptHold->SetPriority(100);
                m_Switch[i].ptHeavyHold->SetPriority(100);
                m_Switch[i].ptDeploy->SetPriority(0);
            }
            else
            {
                m_Switch[i].ptHold->SetPriority(50);
                m_Switch[i].ptHeavyHold->SetPriority(50);
                m_Switch[i].ptDeploy->SetPriority(100);
            }
            m_Switch[i].ptCapture->SetPriority(0);
        }
        else
        {
            m_HeldSwitchBits &= ~(1 << i);

            m_Switch[i].bHeld = FALSE;
            m_Switch[i].bUnderAttack = TRUE;
            m_Switch[i].ptCapture->SetPriority(100);
            m_Switch[i].ptHold->SetPriority(0);
            m_Switch[i].ptHeavyHold->SetPriority(0);
            m_Switch[i].ptDeploy->SetPriority(0);
        }
    }
}
//==============================================================================

void cnh_master_ai::PollAllObjects ( void )
{
    ASSERT(m_MissionInitialized);

    if (m_nTeamBots == 0)
        return;

    QuerySwitchStatus();

    // Check if any repairs need to be made & get a count of deployed assets.
    if (ScanAssets(NULL, m_RepairTask))
    {
        s32 i;
        for (i = 0; i < m_nSwitches; i++)
        {
            if (m_CurrentDeployAsset[i] == NULL
                && m_Switch[i].ptCapture->GetAssignedBot())
            {
                if (m_Switch[i].iDeploying != -1)
                    m_TeamBot[m_Switch[i].iDeploying]->m_CurTask = NULL;
                
                m_Switch[i].ptDeploy->ResetAssignments();
                m_Switch[i].iDeploying = -1;
            }
        }
    }
    #if defined (acheng)
        static const char TEAM[2][15] = {"Storm", "Inferno",};
        x_printfxy(5, 10+m_TeamBits, "%s Turrets: %d", TEAM[m_TeamBits -1], 
                                                               m_nDeployedTurrets);
    #endif
}

//==============================================================================

void cnh_master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
    s32 i;
    s32 ClosestBot;
    s32 Uncaptured = -1;
    s32 Unassigned = m_nTeamBots;

    u32 AssignedSwitches =  0;
    s32 ClosestSwitch    = -1;

    // deploy task integrity test:
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying != -1)
        {
            // If no one is actually assigned, then reset the value.
            if(!m_Switch[i].ptDeploy->GetAssignedBot())
                m_Switch[i].iDeploying = -1;
        }
        else
        {
            ASSERT(!m_Switch[i].ptDeploy->GetAssignedBot());
        }

        if (m_Switch[i].ptCapture->GetAssignedBot()
            || m_Switch[i].ptHold->GetAssignedBot()
            || m_Switch[i].ptHeavyHold->GetAssignedBot()
            || m_Switch[i].ptDeploy->GetAssignedBot()
            || m_Switch[i].ptSnipe->GetAssignedBot())
        {
            AssignedSwitches |= (1<<i);
        }
    }

    // Less bots than unheld switches?
    if (m_nTeamBots < m_nSwitches - m_nHeldSwitches )
    {
        // Assign each bot to the closest switch.
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (m_TeamBot[i]->HasTask())
            {
                Unassigned--;
                continue;
            }
            ClosestSwitch = FindClosestSwitch(i, AssignedSwitches, TRUE);
            ASSERT(ClosestSwitch != -1);
            AssignBotTask(i, m_Switch[ClosestSwitch].ptCapture);
            BotSwitch[i] = ClosestSwitch;
            Unassigned--;
        }

        ASSERT(Unassigned == 0);
        return;
    }   
#if defined (acheng)
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying != -1)
        {
            ASSERT(m_Switch[i].ptDeploy->GetAssignedBot());
        }
        else
        {
            ASSERT(!m_Switch[i].ptDeploy->GetAssignedBot());
        }

    }
#endif

    // Assign one bot to each unheld switch:
    for (i = 0; i < m_nSwitches; i++)
    {
        // If deploying with turret on back, let him finish.
        if (m_Switch[i].ptDeploy->GetAssignedBot() && 
            (m_TeamBot[m_Switch[i].iDeploying]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR]
            || (m_TeamBot[m_Switch[i].iDeploying]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR]) 
            || (m_TeamBot[m_Switch[i].iDeploying]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION]) ))
        {
            ASSERT(m_Switch[i].iDeploying != -1);
            // Make sure we have a valid deploy location.
            if (!m_CurrentDeployAsset[i])
            {
                if (m_TeamBot[m_Switch[i].iDeploying]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR]
                && !m_TurretFull)
                    ChooseTurretSpot(m_Switch[i].SwitchID, i);
                else if 
                    (m_TeamBot[m_Switch[i].iDeploying]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR]
                && !m_SensorFull)
                    ChooseSensorSpot(m_Switch[i].SwitchID, i);
                else if 
                    (m_TeamBot[m_Switch[i].iDeploying]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION]
                && !m_InvenFull)
                    ChooseInvenSpot(m_Switch[i].SwitchID, i);
            }

            // Set him on deploy and let him be on his way.
            if (m_CurrentDeployAsset[i])
            {
                SetUpDeployTask(i);
                Unassigned--;
                continue;
            }
        }

        // Assign capturers
        if (!m_Switch[i].bHeld)
        {
            Uncaptured = i;
            f32 temp;
            ClosestBot = FindClosestAvailableBot(m_Switch[i].ptCapture, temp);

            // Assign bot to capture
            if (ClosestBot != -1)
            {
                AssignBotTask(ClosestBot, m_Switch[i].ptCapture);
                BotSwitch[ClosestBot] = i;
                Unassigned--;
                AssignedSwitches |= (1<<i);
                if (Unassigned <= 0)
                    return;
            }
        }
        else
        {
            // We have this switch. Time to deploy turrets?
            if (!m_Switch[i].bUnderAttack)
            {
                // Do we already have a deploying bot?
                if (m_Switch[i].ptDeploy->GetAssignedBot())
                {
                    ASSERT(m_Switch[i].iDeploying != -1);
                    // Confirm we have a valid deploy location.
                    if (!m_CurrentDeployAsset[i])
                    {
                        ChooseAssetSpotFn(m_Switch[i].SwitchID, i);
                    }
    
                    if (m_CurrentDeployAsset[i])
                    /* && (m_TeamBot[m_Switch[i].iDeploying]->GetPosition() -
                        m_Switch[i].ptCapture->GetTargetPos()).LengthSquared() < ENGAGE_RADIUS_SQ)*/
                    {
                        SetUpDeployTask(i);
                        Unassigned--;

#if defined (acheng)
    static const char TEAM[2][15] = {"Storm", "Inferno",};
                        x_printfxy(5, 30+4*m_TeamBits+i, "%s Deploying on Switch %d",
                            TEAM[m_TeamBits-1], i);
#endif
                    }
                    else
                    {
                        if(m_Switch[i].ptDeploy->GetAssignedBot())
                        {
                            ASSERT(m_Switch[i].iDeploying != -1);
                            m_TeamBot[m_Switch[i].iDeploying]->SetTask(NULL);
                            m_Switch[i].ptDeploy->ResetAssignments();
                        }
                        m_Switch[i].iDeploying = -1;
                            
                    }
                }
                else
                {   // See if we have a bot available to deploy stuff.
                    f32 DistSq;
                    ClosestBot = FindClosestAvailableBot(m_Switch[i].ptCapture->GetTargetPos(), DistSq);
                    if (ClosestBot != -1) // && DistSq < ENGAGE_RADIUS_SQ)  
                    {
                        // Confirm we have a valid deploy location.
                        if (!m_CurrentDeployAsset[i])
                        {
                            if (ChooseAssetSpotFn(m_Switch[i].SwitchID, i))
                            {
                                m_Switch[i].iDeploying = ClosestBot;
                                AssignBotTask(ClosestBot, m_Switch[i].ptDeploy);
                                SetUpDeployTask(i);
                                Unassigned--;
                            }
                        }

                        if (!m_CurrentDeployAsset[i])
                        {
                            if(m_Switch[i].ptDeploy->GetAssignedBot())
                            {
                                ASSERT(m_Switch[i].iDeploying != -1);
                                m_TeamBot[m_Switch[i].iDeploying]->SetTask(NULL);
                                m_Switch[i].ptDeploy->ResetAssignments();
                            }
                            m_Switch[i].iDeploying = -1;
                        }
                        else
                        {
                            m_Switch[i].iDeploying = ClosestBot;
                            AssignBotTask(ClosestBot, m_Switch[i].ptDeploy);
                            BotSwitch[ClosestBot] = i;
                            SetUpDeployTask(i);
                            Unassigned--;
                            ASSERT(m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.GetAuxDirection().LengthSquared() < 100.0f);
                        }
                    }

                    else
                    {
                        if(m_Switch[i].ptDeploy->GetAssignedBot())
                        {
                            ASSERT(m_Switch[i].iDeploying != -1);
                            m_TeamBot[m_Switch[i].iDeploying]->SetTask(NULL);
                            m_Switch[i].ptDeploy->ResetAssignments();
                        }
                        m_Switch[i].iDeploying = -1;
                    }
                }
            }
        }
    }
#if defined (acheng)
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying != -1)
        {
            ASSERT(m_Switch[i].ptDeploy->GetAssignedBot());
        }
        else
        {
            ASSERT(!m_Switch[i].ptDeploy->GetAssignedBot());
        }
    }
#endif

    // Assign remaining bots to closest building.
    for (i = 0; i < m_nTeamBots; i++)
    {
        // Refresh enabled loadouts.
        m_TeamBot[i]->EnableLoadout();

        // Only assign bots that aren't already assigned.
        if (!m_TeamBot[i]->HasTask())
        {
            // Determine the closest switch
            ClosestSwitch = FindClosestSwitch(i, AssignedSwitches);
            if (m_Switch[ClosestSwitch].bHeld == FALSE)
            {
                // Assign if there aren't too many already assigned to this.
                if (m_Switch[ClosestSwitch].ptCapture->GetNumAssignedBots()
                    < m_Switch[ClosestSwitch].ptCapture->MaxBotsToBeAssigned())
                {
                    AssignBotTask(i, m_Switch[ClosestSwitch].ptCapture);
                    BotSwitch[i] = ClosestSwitch;
                    Unassigned--;
                    AssignedSwitches |= (1<<ClosestSwitch);
                }
            }
            else
            {
                // Assign if there aren't too many already assigned to this.
                if (m_Switch[ClosestSwitch].ptHold->GetNumAssignedBots()
                    < m_Switch[ClosestSwitch].ptHold->MaxBotsToBeAssigned())
                {
                    AssignBotTask(i, m_Switch[ClosestSwitch].ptHold);
                    BotSwitch[i] = ClosestSwitch;
                    AssignedSwitches |= (1<<ClosestSwitch);
                    Unassigned--;
                }
                else
                {
                    // Find another building to defend or something
                    if (Uncaptured != -1)
                    {
                        AssignBotTask(i, m_Switch[Uncaptured].ptCapture);
                        BotSwitch[i] = Uncaptured;
                        AssignedSwitches |= (1<<Uncaptured);
                        Unassigned--;
                    }
                    else
                    {
                        AssignBotTask(i, m_Switch[ClosestSwitch].ptHold);
                        BotSwitch[i] = ClosestSwitch;
                        AssignedSwitches |= (1<<ClosestSwitch);
                        Unassigned--;
                    }
                    // to do.. figure out what to do with him.
                }
            }
        }
    }
#if defined (acheng)
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying != -1)
        {
            ASSERT(m_Switch[i].ptDeploy->GetAssignedBot());
        }
        else
        {
            ASSERT(!m_Switch[i].ptDeploy->GetAssignedBot());
        }
    }
#endif

    s32 ClosestEnemy = 0;
    f32 DistSq = 0;
    // Now go through all defenders, setting em on light & heavy hold.
    for (i = 0; i < m_nTeamBots; i++)
    {
        if (!m_TeamBot[i]->m_CurTask)
        {
            // Set to defend the closest switch.
            u32 Garbage = 0;
            ClosestSwitch = FindClosestSwitch(i,Garbage);
            AssignBotTask(i, m_Switch[ClosestSwitch].ptHold);
            BotSwitch[i] = ClosestSwitch;
            Unassigned--;
        }

        if (m_TeamBot[i]->m_CurTask->GetID() == bot_task::TASK_TYPE_DEFEND_FLAG)
        {
            ClosestEnemy = FindClosestEnemy(m_Switch[BotSwitch[i]].Position, DistSq, 900);
            if (ClosestEnemy != -1) // Disable loadouts if under attack.
            {
                m_TeamBot[i]->DisableLoadout();
                
                ASSERT(BotSwitch[i] != -1);

                // Leave on hold task, on attack.
                m_Switch[BotSwitch[i]].ptHold->SetState(bot_task::TASK_STATE_ATTACK);
                m_Switch[BotSwitch[i]].ptHold->SetTargetObject(m_OtherTeamPlayer[ClosestEnemy]);
            }
        }
    }
    if (g_BotDebug.ShowTaskDistribution && this->m_nTeamBots > 1)
    {
        x_printfxy(4, 17, "Storm Bots");
        x_printfxy(4, 26, "Inferno Bots");
        
        s32 i;
        for (i = 0 ; i < m_nTeamBots; i++)
        {
            x_printfxy(5, 9*(m_TeamBits + 1)+i, "Bot #%d: %s:%s, Switch #%d", 
                 i, m_TeamBot[i]->m_CurTask->GetTaskName(), 
                m_TeamBot[i]->m_CurTask->GetTaskStateName(),
                BotSwitch[i]);
        }
    }
#if defined (acheng)
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying != -1)
        {
            ASSERT(m_Switch[i].ptDeploy->GetAssignedBot());
        }
        else
        {
            ASSERT(!m_Switch[i].ptDeploy->GetAssignedBot());
        }
    }
#endif
#ifdef TARGET_PC
    // Debugging test
    for (i = 0; i < m_nTeamBots; i++)
    {
        if (m_TeamBot[i]->m_CurTask->GetID() == bot_task::TASK_TYPE_DEPLOY_INVEN)
        {
            // Confirm that his aux direction is properly set.
            ASSERT(m_TeamBot[i]->m_CurrentGoal.GetAuxDirection().X == -1.0f
                && m_TeamBot[i]->m_CurrentGoal.GetAuxDirection().Z == -1.0f);
        }
    }
#endif
}

//==============================================================================

void cnh_master_ai::ResetAssignments ( void )
{
    s32 i;
    for (i = 0; i < m_nTeamBots; i++)
    {
        // Clear all assignments - except if he's already deploying.
        if (m_TeamBot[i]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR]
            || (m_TeamBot[i]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR]) 
            || (m_TeamBot[i]->GetLoadout().Inventory
                .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION]) )
                continue;
        if (m_TeamBot[i]->HasTask())
        {
            m_TeamBot[i]->GetTask()->UnassignBot(i);
            m_TeamBot[i]->SetTask(NULL);
        }
    }

    if (m_PlayersChanged)
    {
        // Task Bit values are all screwed up.  Reset them.
        for (i = 0; i < m_nTasks; i++)
            m_GoalList[i].ResetAssignments();
    }

    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying != -1)
        {
            // See if he gets to keep his task.
            if (m_TeamBot[m_Switch[i].iDeploying]->HasTask()) // only deployers with packs have tasks now.
            {
                //keep it.
                m_Switch[i].ptDeploy->SetAssignedBot(m_Switch[i].iDeploying);
            }
            else
            {
                if(m_Switch[i].ptDeploy->GetAssignedBot())
                {
                    ASSERT(m_Switch[i].iDeploying != -1);
                    m_TeamBot[m_Switch[i].iDeploying]->SetTask(NULL);
                    m_Switch[i].ptDeploy->ResetAssignments();
                }
                m_Switch[i].iDeploying = -1;
            }
        }
        if (m_Switch[i].iDeploying != -1)
        {
            ASSERT(m_Switch[i].ptDeploy->GetAssignedBot());
        }
        else
        {
            ASSERT(!m_Switch[i].ptDeploy->GetAssignedBot());
        }

    }
}

//==============================================================================

s32 cnh_master_ai::FindClosestSwitch( s32 iBot, u32& Assigned, xbool UnheldOnly)
{
    s32 i;
    s32 ClosestSwitch = -1;
    vector3 vDistance;
    f32 CurDist;
    f32 ClosestDist = F32_MAX;

    s32 AbsClosestSwitch = -1;
    f32 AbsClosestDist = F32_MAX;

    flipflop* Switch;

    for (i = 0; i < m_nSwitches; i++)
    {
        if (UnheldOnly )
        {
            Switch = (flipflop*)ObjMgr.GetObjectFromID(m_Switch[i].SwitchID);
            // Continue if we already have this switch.
            if ((Switch->GetTeamBits() & m_TeamBits) && 
                        (~m_TeamBits & Switch->GetTeamBits()) == 0)
                continue;
        }

        vDistance = m_Switch[i].ptCapture->GetTargetPos() - m_TeamBot[iBot]->GetPosition();
        CurDist = vDistance.LengthSquared();
        if (CurDist < AbsClosestDist)
        {
            AbsClosestSwitch=i;
            AbsClosestDist = CurDist;
        }
        if (Assigned & 1<<i)
            continue;
        else
            if (CurDist < ClosestDist)
            {
                ClosestSwitch=i;
                ClosestDist = CurDist;
            }
    }
    if (ClosestSwitch != -1)
    {
        Assigned |= 1<<ClosestSwitch;
        return ClosestSwitch;
    }
    else
    {
        return AbsClosestSwitch;
    }

}

//==============================================================================
void cnh_master_ai::OffenseBotDied( s32 BotIdx )
{
    m_ForceReset = TRUE;
    s32 i;
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying == BotIdx)
        {
            m_Switch[i].ptDeploy->UnassignBot(BotIdx);
            m_Switch[i].iDeploying = -1;
        }
    }
}
//==============================================================================

void cnh_master_ai::AssetPlacementFailed ( s32 BotIdx )
{
    // Find the switch with this bot.
    s32 i;
    s32 CurrentSwitch = -1;
    for (i = 0; i < m_nSwitches; i++)
    {
        if (m_Switch[i].iDeploying == BotIdx)
        {
            CurrentSwitch = i;
            break;
        }
    }

    // This bot has already been removed from this task.
    if(CurrentSwitch == -1)
        return;

    flipflop* Switch = (flipflop*)ObjMgr.GetObjectFromID(m_Switch[i].SwitchID);
    // Continue if we still have control of this switch.
    if (!(Switch->GetTeamBits() & m_TeamBits))
    {
        m_CurrentDeployAsset[CurrentSwitch] = NULL;
        return;
    }

    xbool Removed = FALSE;
    xbool Replaced = FALSE;

    // Deploy task may have already been cancelled through ScanAssets.
    if (m_CurrentDeployAsset[CurrentSwitch])
    {

        // Remove the bad asset spot, and try to replace it.
        switch (m_DeployType[CurrentSwitch])
        {
        case (DEPLOY_TYPE_TURRET):
            for ( i = 0; i < m_DeploySpots.m_nTurrets; i++)
            {
                if (m_CurrentDeployAsset[CurrentSwitch]->GetAssetPos() 
                            == m_DeploySpots.m_pTurretList[i].GetAssetPos())
                {
                    RemoveDeployable(m_DeployType[CurrentSwitch], i);
                    Removed = TRUE;
                    if (ChooseTurretSpot(m_Switch[CurrentSwitch].SwitchID))
                        Replaced = TRUE;
                    break;
                }
            }
            break;

        case (DEPLOY_TYPE_INVEN):
            for ( i = 0; i < m_DeploySpots.m_nInvens; i++)
            {
                if (m_CurrentDeployAsset[CurrentSwitch]->GetAssetPos() 
                            == m_DeploySpots.m_pInvenList[i].GetAssetPos())
                {
                    RemoveDeployable(m_DeployType[CurrentSwitch], i);
                    Removed = TRUE;
                    if (ChooseInvenSpot(m_Switch[CurrentSwitch].SwitchID))
                        Replaced = TRUE;
                    break;
                }
            }
            break;

        case (DEPLOY_TYPE_SENSOR):
            for ( i = 0; i < m_DeploySpots.m_nSensors; i++)
            {
                if (m_CurrentDeployAsset[CurrentSwitch]->GetAssetPos() 
                            == m_DeploySpots.m_pSensorList[i].GetAssetPos())
                {
                    RemoveDeployable(m_DeployType[CurrentSwitch], i);
                    Removed = TRUE;
                    if (ChooseSensorSpot(m_Switch[CurrentSwitch].SwitchID))
                        Replaced = TRUE;
                    break;
                }
            }
            break;
        }
    
        ASSERT(Removed);
    }

    if (!Replaced)
    {
        m_CurrentDeployAsset[CurrentSwitch] = NULL;
        ChooseAssetSpotFn(m_Switch[CurrentSwitch].SwitchID);
    }

    if (m_CurrentDeployAsset[CurrentSwitch])
    {
        SetUpDeployTask(CurrentSwitch);
    }
}

//==============================================================================

void cnh_master_ai::SetUpDeployTask ( s32 i )
{
    if (!m_TeamBot[m_Switch[i].iDeploying]->HasTask())
    {
        AssignBotTask(m_Switch[i].iDeploying, m_Switch[i].ptDeploy);
    }
    bot_task::task_state Task = 
        (bot_task::task_state)m_TeamBot[m_Switch[i].iDeploying]->GetTask()->GetState();

    ASSERT(Task == bot_task::TASK_STATE_DEPLOY_TURRET 
        || Task == bot_task::TASK_STATE_DEPLOY_INVEN
        || Task == bot_task::TASK_STATE_DEPLOY_SENSOR );

    m_Switch[i].ptDeploy->SetTargetPos(m_CurrentDeployAsset[i]->GetStandPos());

    if (m_DeployType[i] == DEPLOY_TYPE_TURRET)
    {
        m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.SetAuxLocation(
                  ((turret_spot*)m_CurrentDeployAsset[i])->GetAssetPos());
        m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.SetAuxDirection(
                    ((turret_spot*)m_CurrentDeployAsset[i])->GetNormal());
        m_Switch[i].ptDeploy->SetID(bot_task::TASK_TYPE_DEPLOY_TURRET);
        m_Switch[i].ptDeploy->SetState(bot_task::TASK_STATE_DEPLOY_TURRET);
    }
    else if (m_DeployType[i] == DEPLOY_TYPE_INVEN)
    {
        m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.SetAuxLocation(
                  ((inven_spot*)m_CurrentDeployAsset[i])->GetAssetPos());
        m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.SetAuxDirection(
                    ((inven_spot*)m_CurrentDeployAsset[i])->GetYaw());
        m_Switch[i].ptDeploy->SetID(bot_task::TASK_TYPE_DEPLOY_INVEN);
        m_Switch[i].ptDeploy->SetState(bot_task::TASK_STATE_DEPLOY_INVEN);
    }
    else if (m_DeployType[i] == DEPLOY_TYPE_SENSOR)
    {
        m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.SetAuxLocation(
                  ((sensor_spot*)m_CurrentDeployAsset[i])->GetAssetPos());
        m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.SetAuxDirection(
                    ((sensor_spot*)m_CurrentDeployAsset[i])->GetNormal());
        m_Switch[i].ptDeploy->SetID(bot_task::TASK_TYPE_DEPLOY_SENSOR);
        m_Switch[i].ptDeploy->SetState(bot_task::TASK_STATE_DEPLOY_SENSOR);
    }
    ASSERT(m_TeamBot[m_Switch[i].iDeploying]->m_CurrentGoal.GetAuxDirection().LengthSquared() < 100.0f);
    BotSwitch[m_Switch[i].iDeploying] = i;
}

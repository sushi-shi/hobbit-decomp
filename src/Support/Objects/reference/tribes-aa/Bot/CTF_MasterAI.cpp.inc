//==============================================================================
//  
//  CTF_MasterAI.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "CTF_MasterAI.hpp"
#include "..\AADS\Support\GameMgr\GameMgr.hpp"

#define PATROL_RADIUS 50
#define ENGAGE_RADIUS 100
#define RETREAT_RADIUS 150

#define PATROL_RADIUS_SQ  (PATROL_RADIUS * PATROL RADIUS)
#define ENGAGE_RADIUS_SQ  (ENGAGE_RADIUS * ENGAGE_RADIUS)
#define RETREAT_RADIUS_SQ (RETREAT_RADIUS * RETREAT_RADIUS)

#define MORTAR_CAPPER_RADIUS 25
#define MORTAR_CAPPER_RADIUS_SQ (MORTAR_CAPPER_RADIUS * MORTAR_CAPPER_RADIUS)

//==============================================================================
//  Storage
//==============================================================================
extern s32 TeamBitsFound;
extern bot_object::bot_debug g_BotDebug;

s32         StormX = -1;
bot_object* ExamineBot = NULL;
#if defined (acheng)
    static const char TEAM[3][15] = {"", "Storm", "Inferno",};
#endif
//==============================================================================
//  CTF MASTER AI FUNCTIONS
//==============================================================================

ctf_master_ai::ctf_master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
    m_nDeployAssets = 1;
    LastAssetCheck = 0;
}

//==============================================================================

ctf_master_ai::~ctf_master_ai ( void )
{
    UnloadMission();
}

//==============================================================================


void ctf_master_ai::UnloadMission( void )
{
    m_TeamBits = 0;
    m_CapTask = NULL;
//    m_DestroyTask = NULL;

    m_LightDefendTask = NULL;
    m_HeavyDefendTask = NULL;
    m_RetrieveTask = NULL;
    m_RepairTask = NULL;
    m_DeployTask = NULL;
    m_SnipeTask = NULL;

    m_MissionInitialized = FALSE;
    TeamBitsFound = 0;
}

//==============================================================================

void ctf_master_ai::UpdatePlayerPointers( void )
{
    master_ai::UpdatePlayerPointers();

    if (m_PlayersChanged && m_MissionInitialized)
    {
        s32 i;
        // Task Bit values are all screwed up.  Reset them.
        for (i = 0; i < m_nTasks; i++)
            m_GoalList[i].ResetAssignments();

        for (i = 0; i < m_nTeamBots; i++ )
        {
            m_TeamBot[i]->SetTask(NULL);
        }
        m_DeployBot = -1;
        m_iSniperBot = -1;
    }
}

//==============================================================================

void ctf_master_ai::InitializeMission( void )
{
    ASSERT (!m_MissionInitialized);
    m_MissionInitialized = TRUE;

    // Set the team bit flag appropriately.
    UpdatePlayerPointers();
    m_nOffense = MAX((s32)(m_nTeamBots * 0.50), 1); // At least one

    // Grab all the necessary object pointers.
    flag* pFlagObj;

    ObjMgr.StartTypeLoop( object::TYPE_FLAG);
    while ( (pFlagObj = (flag *)(ObjMgr.GetNextInType())) != NULL )
    {
        // Set our team bits if not already set.
        if (!m_TeamBits)
        {
            if ( pFlagObj->GetTeamBits() & TeamBitsFound )
                // This flag is on another team.
            {
                m_OtherFlagPos = pFlagObj->GetPosition() + vector3(0.0f, 0.1f, 0.0f);
                m_EnemyFlagID = pFlagObj->GetObjectID();
            }
            else 
            {
                // This flag must be on our team!
                m_TeamBits = pFlagObj->GetTeamBits();
                TeamBitsFound |= m_TeamBits;
                m_TeamFlagPos = pFlagObj->GetPosition() + vector3(0.0f, 0.1f, 0.0f);
                m_FlagID = pFlagObj->GetObjectID();
            }
        }

        else if ( pFlagObj->GetTeamBits() & m_TeamBits )
        {
            m_TeamFlagPos = pFlagObj->GetPosition();
            m_FlagID = pFlagObj->GetObjectID();
        }
        else
        {
            m_OtherFlagPos = pFlagObj->GetPosition();
            m_EnemyFlagID = pFlagObj->GetObjectID();
        }
    }
    ObjMgr.EndTypeLoop();

    m_MortarFlag = FALSE;

    // Store the position of the team flag stand.
    m_TeamFlagStand = m_TeamFlagPos;
    m_OtherFlagStand = m_OtherFlagPos;

    // Allocate memory for each of the storage containers.

    m_nTasks = NUM_CTF_TASKS;

    m_GoalList[0].SetID(bot_task::TASK_TYPE_CAPTURE_FLAG);
    m_CapTask = &m_GoalList[0];
    m_CapTask->SetPriority(100);

    m_GoalList[1].SetID(bot_task::TASK_TYPE_DEFEND_FLAG);
    m_HeavyDefendTask = &m_GoalList[1];
    m_HeavyDefendTask->SetState(bot_task::TASK_STATE_ROAM_AREA);

    m_GoalList[2].SetID(bot_task::TASK_TYPE_RETRIEVE_FLAG);
    m_RetrieveTask = &m_GoalList[2];

    m_GoalList[3].SetID(bot_task::TASK_TYPE_REPAIR);
    m_RepairTask = &m_GoalList[3];

    m_GoalList[4].SetID(bot_task::TASK_TYPE_DESTROY);
    m_GoalList[5].SetID(bot_task::TASK_TYPE_DESTROY);
    m_GoalList[6].SetID(bot_task::TASK_TYPE_DESTROY);
    m_GoalList[7].SetID(bot_task::TASK_TYPE_DESTROY);
    m_GoalList[4].SetState(bot_task::TASK_STATE_DESTROY);
    m_GoalList[5].SetState(bot_task::TASK_STATE_DESTROY);
    m_GoalList[6].SetState(bot_task::TASK_STATE_DESTROY);
    m_GoalList[7].SetState(bot_task::TASK_STATE_DESTROY);
    m_DestroyTaskList[0] = &m_GoalList[4];
    m_DestroyTaskList[1] = &m_GoalList[5];
    m_DestroyTaskList[2] = &m_GoalList[6];
    m_DestroyTaskList[3] = &m_GoalList[7];
    m_nDestroyTasks = 4;
    m_DestroyTaskList[0]->SetPriority(100);
    m_DestroyTaskList[1]->SetPriority(100);
    m_DestroyTaskList[2]->SetPriority(100);
    m_DestroyTaskList[3]->SetPriority(100);

    m_DestroyRun = FALSE;
    m_LastDestroyRun = m_Timer.ReadSec();
    m_nOffenseDeaths = 0;
    m_nExpectedDestroyers = 1;

    m_GoalList[8].SetID(bot_task::TASK_TYPE_DEPLOY_TURRET);
    m_DeployTask = &m_GoalList[8];

    m_GoalList[9].SetID(bot_task::TASK_TYPE_SNIPE);
    m_SnipeTask = &m_GoalList[9];
    m_SnipeTask->SetState(bot_task::TASK_STATE_SNIPE);
    m_SnipeTask->SetPriority(100);

    m_GoalList[10].SetID(bot_task::TASK_TYPE_LIGHT_DEFEND_FLAG);
    m_LightDefendTask = &m_GoalList[10];
    m_LightDefendTask->SetState(bot_task::TASK_STATE_ROAM_AREA);

    m_FlagCarrier = -1;
}

//==============================================================================

void ctf_master_ai::Update()
{
#if defined (acheng)
    s32 i;
    for (i = 0; i < m_nTasks; i++)
    {
        ASSERT(BotsOnTask(&m_GoalList[i], m_GoalList[i].GetAssignedBot()));
    }
#endif
    UpdatePlayerPointers();

    // Set the new values for the collected data.
    PollAllObjects();

    if (m_nTeamBots)
    {
        // Determine priority values for each task.
        PrioritizeTasks     ( );

        // Sort list based on priority values.
//        SortList            ( );

        // (Re)assign tasks based on bot availability.
        AssignAllTasks      ( );

        // Make sure each task state is current.
        UpdateAllTaskStates ( );
    }
#if defined (acheng)
    for (i = 0; i < m_nTasks; i++)
    {
        ASSERT(BotsOnTask(&m_GoalList[i], m_GoalList[i].GetAssignedBot()));
    }
#endif
}


//==============================================================================

xbool ctf_master_ai::RefreshFlags( void )
{
    vector3 vTeamFlag;
    vector3 vOtherFlag;
    
    s32 OldTFS = m_TeamFlagStatus, OldOFS = m_OtherFlagStatus;

    pGameLogic->GetFlagStatus( m_TeamBits,   m_TeamFlagStatus,  vTeamFlag );
    pGameLogic->GetFlagStatus( m_TeamBits^3, m_OtherFlagStatus, vOtherFlag );
    
    if (m_TeamFlagStatus != -3)//FLAG_STATUS_UNKNOWN))
    {
        m_TeamFlagPos = vTeamFlag + vector3(0.0f, 0.1f, 0.0f);
    }
    if (m_OtherFlagStatus != -3)//FLAG_STATUS_UNKNOWN))
    {
        m_OtherFlagPos = vOtherFlag + vector3(0.0f, 0.1f, 0.0f);
    }

    if (OldTFS == m_TeamFlagStatus && OldOFS == m_OtherFlagStatus)
        return FALSE;
    else
    {
        flag* pFlagObj;
        ObjMgr.StartTypeLoop( object::TYPE_FLAG);
        while ( (pFlagObj = (flag *)(ObjMgr.GetNextInType())) != NULL )
        {
            if ( pFlagObj->GetTeamBits() & m_TeamBits )
            {
                m_FlagID = pFlagObj->GetObjectID();
                break;
            }
        }
        ObjMgr.EndTypeLoop();
        return TRUE;
    }
        
}

//==============================================================================
void ctf_master_ai::PollAllObjects ( void )
{
    ASSERT(m_MissionInitialized);

    if (m_nTeamBots == 0)
        return;

    // Offense-Defense Distribution of bots.
    m_nOffense = MAX((s32)(m_nTeamBots * 0.50), 1); // At least one
    m_nDefense = m_nTeamBots - m_nOffense;

    m_CapTask->SetMaxBots(MAX(m_nOffense - m_nExpectedDestroyers, 1));
    m_RetrieveTask->SetMaxBots(MAX((s32)(m_nTeamBots * 0.50), 2));
    m_HeavyDefendTask->SetMaxBots(MAX((s32)(m_nTeamBots * 0.25), 1));
    m_LightDefendTask->SetMaxBots(MAX((s32)(m_nTeamBots * 0.25), 1));
    m_RepairTask->SetMaxBots(1);
    xbool FlagStatusChanged = RefreshFlags();

    m_HeavyDefendTask->SetPriority(50);
    m_LightDefendTask->SetPriority(50);
    m_RetrieveTask->SetPriority(0);
    m_DeployTask->SetPriority(0);   

    f32 DistSq;

    xbool UnderAttack = (m_TeamFlagStatus > -2 ||       // our flag not on stand
                                                        // or enemies imminent.
        (     (m_TeamFlagStatus == -2) 
           && (SetTaskDefend(m_HeavyDefendTask, m_TeamFlagStand, 
                            ENGAGE_RADIUS_SQ, RETREAT_RADIUS_SQ, DistSq))));

    *m_LightDefendTask = *m_HeavyDefendTask;

    ASSERT(m_LightDefendTask->GetTargetObject() ||
            m_LightDefendTask->GetTargetPos().InRange(-1000.0f, 10000.0f));
    ASSERT(m_HeavyDefendTask->GetTargetObject() ||
            m_HeavyDefendTask->GetTargetPos().InRange(-1000.0f, 10000.0f));

    m_SnipeTask->SetTargetObject(NULL);
    // Collect all relevant stats on key objects
    switch (m_TeamFlagStatus)
    {
        case (-3):  // FLAG_STATUS_UNKNOWN ?
        break;

        // Is our flag on the flag stand?
        case (-2):  //FLAG_STATUS_ON_STAND  
        {
            // If so, are any enemies within a threatening range?
            // Update the distance of the closest enemy
            m_RetrieveTask->SetPriority(0);

            // Mark defend as important when being invaded.
            if (UnderAttack)
            {
                m_HeavyDefendTask->SetPriority(100);
                m_LightDefendTask->SetPriority(100);
                if (DistSq < MORTAR_CAPPER_RADIUS_SQ)          // 25 m
                {
                    m_MortarFlag = TRUE;
                }
                else if (DistSq > MORTAR_CAPPER_RADIUS_SQ * 2) // ~35 m
                {
                    m_MortarFlag = FALSE;
                }
            }

        }
        break;
    
        // Our flag needs to be retrieved.
        case (-1): //FLAG_STATUS_DROPPED
        {
            // Where's our flag?!?
            m_RetrieveTask->SetState(bot_task::TASK_STATE_PURSUE);
            m_RetrieveTask->SetTargetObject(NULL);
            m_RetrieveTask->SetTargetPos(m_TeamFlagPos);
            m_RetrieveTask->SetPriority(100);
        }
        break;
        
        default:    // Someone has our flag.
        {
            m_MortarFlag = FALSE;

            // WHO's got our flag?!?
            s32 i;
            for (i = 0 ; i < m_nOtherTeamPlayers; i++)
            {
                if (m_OtherTeamPlayer[i]->HasFlag())
                {
                    // GET HIS PUNK ASS!
                    m_RetrieveTask->SetTargetObject(m_OtherTeamPlayer[i]);
                    m_RetrieveTask->SetState(bot_task::TASK_STATE_ATTACK);
                    m_RetrieveTask->SetPriority(100);
                    m_SnipeTask->SetTargetObject(m_OtherTeamPlayer[i]);
                    m_HeavyDefendTask->SetTargetObject(m_OtherTeamPlayer[i]);
                    m_HeavyDefendTask->SetState(bot_task::TASK_STATE_ATTACK);
                    
                    break;
                }
            }
        }
        break;
    }

    if (m_MortarFlag)
    {
        m_HeavyDefendTask->SetState(bot_task::TASK_STATE_MORTAR);
        m_HeavyDefendTask->SetTargetObject(ObjMgr.GetObjectFromID(m_FlagID));
    }

    switch (m_OtherFlagStatus)
    {
        // Where is the enemy flag?
        case (-3):      //         FLAG_STATUS_UNKNOWN   = -3,
            // dunno where the flag is.. 
        break;

        case (-2): //FLAG_STATUS_ON_STAND  
            // intentional fallthrough
        case (-1):  // FLAG_STATUS_DROPPED   
        // No one has the flag.  GO GET IT!
        {
            if (m_FlagCarrier != -1)
            {
                if (m_TeamBot[m_FlagCarrier]->GetTask() == m_CapTask)
                {
                    // The guy who had the flag no longer has it..  
                    m_CapTask->UnassignBot(m_FlagCarrier);
                    m_TeamBot[m_FlagCarrier]->SetTask(NULL);
                }
            }
            m_FlagCarrier = -1;
            m_CapTask->SetState(bot_task::TASK_STATE_PURSUE);
            m_CapTask->SetTargetObject(NULL);
            m_CapTask->SetTargetPos(m_OtherFlagPos);
        }
        break;

        default:
        {
            // if anyone has the flag, update the capture task state.
            s32 i;
            for (i = 0; i < m_nTeamBots; i++)
            {
                if (m_TeamBot[i]->HasFlag())
                {
                    m_FlagCarrier = i;

                    // Is our flag on the stand?  if so, get over there.
                    if (m_TeamFlagStatus == -2)
                        m_CapTask->SetState(bot_task::TASK_STATE_GOTO_POINT);
                    else
                        m_CapTask->SetState(bot_task::TASK_STATE_ROAM_AREA);

                    m_CapTask->SetTargetObject(NULL);
                    m_CapTask->SetTargetPos(m_TeamFlagStand);
                    break;
                }
                else if (m_FlagCarrier != i && 
                                          m_TeamBot[i]->GetTask() == m_CapTask)
                {
                    // Unassign the cap task from this guy.
                    AssignBotTask(i, NULL);
                }
            }
            if (m_FlagCarrier == -1)
            {
            // Someone on our team has our flag, but not a bot.  Damned humans.
                m_CapTask->SetMaxBots(0);
            }
        }
        break;
    }

    s32 RetrieveBots = m_RetrieveTask->GetPriority() ? 
        m_RetrieveTask->MaxBotsToBeAssigned() : 0;

    xbool Destroy = !UnderAttack 
                    || m_nTeamBots < m_CapTask->MaxBotsToBeAssigned() +
                            RetrieveBots + 1;

    // Check for objects to destroy & any needed repairs.
    xbool AssetsChanged = ScanAssets( m_DestroyTaskList, m_RepairTask, !Destroy);

    // Reset all assignments if necessary.
    if (AssetsChanged || ForceReset || FlagStatusChanged)
        ResetAssignments();

    ForceReset = FALSE;

    if (m_FlagCarrier != -1 && m_TeamBot[m_FlagCarrier]->HasFlag())
    {
        // Only flag carrier has his task reserved.
        AssignBotTask(m_FlagCarrier, m_CapTask);
        m_nOffense = MIN(m_nTeamBots, 2); 
        m_nDefense = m_nTeamBots - m_nOffense;
    }
#if defined (acheng)
    x_printfxy(5, 10+m_TeamBits*2, "%s Turrets: %d", TEAM[m_TeamBits], 
                                                           m_nDeployedTurrets);
    x_printfxy(5, 11+m_TeamBits*2, "%s Sensors: %d", TEAM[m_TeamBits], 
                                                           m_nDeployedSensors);
#endif

    if (m_iSniperBot != -1)
    {
        // is it time to give him a new position?
//        if (m_TeamBot[m_iSniperBot]->BeenSnipingHereLongEnough())
//            m_iCurrentSnipePos = -1;
    }

    if ((m_nDeployedTurrets+1 >= MAX_TURRETS_ALLOWED
        && m_nDeployedInvens + 1 >= MAX_INVENS_ALLOWED)
        ||
        !(m_DeploySpots.m_nTurrets || m_DeploySpots.m_nInvens))
    {
        m_DeployTask->SetPriority(0);
        return;
    }
    // Check if we already have a guy deploying.
    if (m_DeployBot != -1)
    {
        // Do we already have a valid deploy location?
        if ( m_CurrentDeployAsset[0] )
        {
            SetUpDeployTask();
            return;
        }
        else
        {
            // Find a new deploy location
            // Is he already wearing a pack?
            if (m_TeamBot[m_DeployBot]->GetLoadout().Inventory
            .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR]
            && !m_SensorFull)
                ChooseSensorSpot(ObjMgr.NullID);
            else if (m_TeamBot[m_DeployBot]->GetLoadout().Inventory
            .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR]
            && !m_TurretFull)
                ChooseTurretSpot(ObjMgr.NullID);
            else if (m_TeamBot[m_DeployBot]->GetLoadout().Inventory
            .Counts[player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION]
            && !m_InvenFull)
                ChooseInvenSpot(ObjMgr.NullID);
            else
            {
                ChooseAssetSpotFn(ObjMgr.NullID);
                LastAssetCheck = 30;
            }

            if (m_CurrentDeployAsset[0])
            {
                SetUpDeployTask();
            }
        }
    }
        // No one is set on deploying.
    else
    {
       // Do we already have a valid deploy location?  (bot may have died.)
        if ( m_CurrentDeployAsset[0] )
        {
            SetUpDeployTask();
        }
        else
        {   // No deploy location, no bot previously set on this assignment.
            // Find a new valid deploy location if we can.
            if (ChooseAssetSpotFn(ObjMgr.NullID))
            {
                ASSERT(m_CurrentDeployAsset[0]);

                SetUpDeployTask();
            }
            else
                LastAssetCheck = 30;
        }
    }
}

//==============================================================================

void ctf_master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
    s32 i;
    s32 ClosestBot;
    s32 nBotsToAssign;
    s32 nOffenseUnassigned = m_nOffense - m_CapTask->GetNumAssignedBots()
                                        - m_DestroyTaskList[0]->GetNumAssignedBots()
                                        - m_DestroyTaskList[1]->GetNumAssignedBots()
                                        - m_DestroyTaskList[2]->GetNumAssignedBots()
                                        - m_DestroyTaskList[3]->GetNumAssignedBots();
    s32 nDefenseUnassigned = m_nDefense - m_RetrieveTask->GetNumAssignedBots()
                                        - m_DeployTask->GetNumAssignedBots()
                                        - m_SnipeTask->GetNumAssignedBots();

    s32 nRetrievers = 0;
    f32 DistanceSq = 0;

    s32 WhileCtr = 0;

    u32 Excluded = 0;
    u32 ExcludeDeployer = (m_DeployBot == -1 ? 0 : 1<<m_DeployBot);
    // Assign Special Forces: Cap when flag is in the field
    if (m_OtherFlagStatus == -1)
    {
        // still at enemy base?
        if ((m_OtherFlagPos - m_OtherFlagStand).LengthSquared() < 10000)
        {
            // Exclude heavys
            ClosestBot = FindClosestAvailableBot(m_CapTask, DistanceSq, Excluded | m_HeavyList);
        }
        else
            // Assign closest bot to grab
            ClosestBot = FindClosestAvailableBot(m_CapTask, DistanceSq, Excluded);
        if (ClosestBot != -1)
        {
            AssignBotTask(ClosestBot, m_CapTask);
            m_TeamBot[ClosestBot]->DisableLoadout();
            nOffenseUnassigned--;
            Excluded |= (1<<ClosestBot);
        }
        // Assign all bots within a radius of 20 m

        while (DistanceSq < 400)
        {
            if (++WhileCtr > 16)
            {
                ASSERT(!"Stuck in while!");
                break;
            }

            // still at enemy base?
            if ((m_OtherFlagPos - m_OtherFlagStand).LengthSquared() < 10000)
            {
                // Exclude heavys
                ClosestBot = FindClosestAvailableBot(m_CapTask, DistanceSq, Excluded | m_HeavyList | EXCLUDE_ASSIGNED);
            }
            else
                ClosestBot = FindClosestAvailableBot(m_CapTask, DistanceSq, Excluded);
            if (ClosestBot != -1)
            {
                AssignBotTask(ClosestBot, m_CapTask);
                m_TeamBot[ClosestBot]->DisableLoadout();
                Excluded |= (1<<ClosestBot);

                if (nOffenseUnassigned > 0)
                    nOffenseUnassigned--;
                else
                    nDefenseUnassigned--;
            }
            else
                break;
        }
    }
    
    WhileCtr = 0;
    // When our flag is in the field, anyone closeby should retrieve
    if (m_TeamFlagStatus >= -1)
    {
        if( m_FlagCarrier != -1 && 
            m_OtherFlagStatus >= 0 && 
            m_nTeamBots > 1 && 
            m_TeamFlagStatus >= 0 )
        {
            Excluded |= (1<<m_FlagCarrier);
        }
        // Assign all bots within a radius of 50 m.
        ClosestBot = FindClosestAvailableBot(m_RetrieveTask, DistanceSq, Excluded  );

        // ########################################################################
        // To do:
        // if an enemy is within X meters of the flag and no team bot is closer, 
        //  shoot the flag with splash weapon.

        while (DistanceSq < 625)
        {
            if (++WhileCtr > 16)
            {
                ASSERT(!"Stuck in while!");
                break;
            }

            // ########################################################################
            // To do:  
            // Check if we assigned the capper. if so, see if anyone else can retrieve,
            //      or if the coast is clear of enemies.
            if (ClosestBot != -1)
            {
                m_TeamBot[ClosestBot]->DisableLoadout();
                Excluded |= (1<<ClosestBot);
                if (m_BotArmorType[ClosestBot] != player_object::ARMOR_TYPE_HEAVY)
                {
                    AssignBotTask(ClosestBot, m_RetrieveTask);
                    nDefenseUnassigned--;
                    nRetrievers++;
                }
                ClosestBot = FindClosestAvailableBot(m_RetrieveTask, DistanceSq, 
                    Excluded | m_HeavyList | ExcludeDeployer);
            }
            else
                break;
        }

        if( m_FlagCarrier != -1 && m_OtherFlagStatus >= 0 && m_nTeamBots > 1 )
            Excluded |= (1<<m_FlagCarrier);

        while (nRetrievers < (m_nTeamBots / 2))
        {
            if (++WhileCtr > 16)
            {
                ASSERT(!"Stuck in while!");
                break;
            }

            // Assign light-armor bots on offense & defense.  
            ClosestBot = FindClosestAvailableBot(m_RetrieveTask, DistanceSq, 
                Excluded | m_HeavyList | ExcludeDeployer | EXCLUDE_ASSIGNED);

            if (ClosestBot != -1)
            {
                // Check armor type.  skip to next if not light.
                ASSERT (m_BotArmorType[ClosestBot] != player_object::ARMOR_TYPE_HEAVY);
/*                {
                    if (m_BotArmorType[ClosestBot] == player_object::ARMOR_TYPE_HEAVY &&
                        DistanceSq < MORTAR_CAPPER_RADIUS_SQ)
                    {
                        AssignBotTask(ClosestBot, m_HeavyDefendTask);
                        nDefenseUnassigned--;
                    }
                    // #####################################################################
                    // To do: set on mortar if heavy with mortars, target flag carrier.
                    Excluded |= (1<<ClosestBot);
                    continue;
                }
*/
                // Maintain snipe task, may be better suited to snipe.
                if (ClosestBot == m_iSniperBot && DistanceSq > 400)
                {
                    AssignBotTask(m_iSniperBot, m_SnipeTask);
                    Excluded |= (1<<ClosestBot);
                    m_TeamBot[m_iSniperBot]->EnableLoadout();
                    nDefenseUnassigned--;
                    continue;
                }

                // Perhaps we should be going for the enemy flag instead?
                if (  nOffenseUnassigned > 0 && 
                      m_OtherFlagStatus < 0 && 
                      m_OtherFlagStatus != -3)// on stand or in field
                {
                    f32 DistToEnemyFlag = (m_TeamBot[ClosestBot]->GetPosition() -
                            m_OtherFlagPos).LengthSquared();

                    ASSERT(DistToEnemyFlag > 0.0f && DistToEnemyFlag < 100000000);
                    if (DistToEnemyFlag < DistanceSq)
                    {
                        AssignBotTask(ClosestBot, m_CapTask);
                        nOffenseUnassigned--;
                        m_TeamBot[ClosestBot]->DisableLoadout();
                        Excluded |= (1<<ClosestBot);
                        continue;
                    }
                }

                AssignBotTask(ClosestBot, m_RetrieveTask);
                m_TeamBot[ClosestBot]->DisableLoadout();
                Excluded |= (1<<ClosestBot);
                nDefenseUnassigned--;
                nRetrievers++;

                if (m_RetrieveTask->GetNumAssignedBots() >=
                    m_RetrieveTask->MaxBotsToBeAssigned())
                    break;

            }
            else
                break;
        }
    }

    f32 Temp;

    // Assign Cappers:
    if (m_FlagCarrier == -1) // No one has the enemy flag
    {
        nBotsToAssign = m_CapTask->MaxBotsToBeAssigned() - 
                                    m_CapTask->GetNumAssignedBots();
    }
    else 
    {
        nBotsToAssign = 1 - m_CapTask->GetNumAssignedBots();
    }
    for (i = 0; (i < nBotsToAssign) && (nOffenseUnassigned > 0); i++)
    {
        ClosestBot = FindClosestAvailableBot(m_CapTask, Temp, 
            m_HeavyList | ExcludeDeployer | EXCLUDE_ASSIGNED);
        if (ClosestBot != -1)
        {
            AssignBotTask(ClosestBot, m_CapTask);
            m_TeamBot[ClosestBot]->DisableLoadout();
            nOffenseUnassigned--;
        }
    }

    // Assign Retrievers: 
    if (m_RetrieveTask->GetPriority())
    {
        nBotsToAssign = m_RetrieveTask->MaxBotsToBeAssigned() - 
                                    m_RetrieveTask->GetNumAssignedBots();
        for (i = 0; i < nBotsToAssign; i++)
        {
            if (m_RetrieveTask->GetNumAssignedBots())
            {
                // Exclude deployers if we have at least one retriever.
                ClosestBot = FindClosestAvailableBot(m_RetrieveTask, Temp, 
                    ExcludeDeployer | EXCLUDE_ASSIGNED);
            } 
            else
                ClosestBot = FindClosestAvailableBot(m_RetrieveTask, Temp);
            if (ClosestBot != -1)
            {
                AssignBotTask(ClosestBot, m_RetrieveTask);
                m_TeamBot[ClosestBot]->DisableLoadout();
                nDefenseUnassigned--;
            }
        }
    }

    // Assign Repairers:
    if (m_RepairTask->GetPriority() 
        && nDefenseUnassigned > 0
        && m_RepairTask->GetNumAssignedBots() < m_RepairTask->MaxBotsToBeAssigned())
    {
        // Cancel the task if the repair object is destroyed or is at full health
        if( (m_RepairTask->GetTargetObject() == NULL) ||
            (m_RepairTask->GetTargetObject()->GetHealth() == 1) )
            m_RepairTask->SetPriority(0);
        else
        {
            if (m_RepairBot == -1 || m_TeamBot[m_RepairBot]->IsDead())
            {
                ClosestBot = FindClosestAvailableBot(m_RepairTask, Temp);
                if (ClosestBot != -1)
                {
                    AssignBotTask(ClosestBot, m_RepairTask);
                    m_TeamBot[ClosestBot]->DisableLoadout();
                    nDefenseUnassigned--;

                    if (ClosestBot == m_DeployBot)
                    {
                        f32 AltDist;
                        s32 NextClosestBot = 
                            FindClosestAvailableBot(m_RepairTask, AltDist);
                        if ( NextClosestBot != -1
                            &&
                            abs((s32)(AltDist - Temp)) < ENGAGE_RADIUS_SQ)
                        {
                            // Reassign with this new bot.
                            AssignBotTask(NextClosestBot, m_RepairTask);
                            m_TeamBot[NextClosestBot]->DisableLoadout();

                            m_RepairTask->UnassignBot(m_DeployBot);
                            AssignBotTask(m_DeployBot, m_DeployTask);
                            m_TeamBot[m_DeployBot]->EnableLoadout();
                            nDefenseUnassigned--;
                        }
                    }
                }
            }
            else
            {
                AssignBotTask(m_RepairBot, m_RepairTask);
                m_TeamBot[m_RepairBot]->DisableLoadout();
                nDefenseUnassigned--;
            }
            
        }
    }

    // Assign Deployer
    if (m_DeployTask->GetPriority() 
        && nDefenseUnassigned > 0 
        && (m_DeployTask->GetNumAssignedBots() < m_DeployTask->MaxBotsToBeAssigned()))
    {
        if(  (m_CurrentDeployAsset[0] == NULL) 
            || m_DeployType[0] == DEPLOY_TYPE_TURRET && m_nDeployedTurrets >= MAX_TURRETS_ALLOWED - 1
            || m_DeployType[0] == DEPLOY_TYPE_INVEN && m_nDeployedInvens >= MAX_INVENS_ALLOWED - 1
            || m_DeployType[0] == DEPLOY_TYPE_SENSOR && m_nDeployedSensors >= MAX_SENSORS_ALLOWED - 1 )
            m_DeployTask->SetPriority(0);
        else if (m_DeployBot != -1 && !m_TeamBot[m_DeployBot]->HasTask())
        {
            SetUpDeployAssignment(m_DeployBot);
            nDefenseUnassigned--;
        }
        else
        {   
            ClosestBot = FindClosestAvailableBot(m_DeployTask, Temp);
            if (ClosestBot != -1)
            {
                SetUpDeployAssignment(ClosestBot);
                nDefenseUnassigned--;
            }
        }
    }
    else
    {
        xbool Done = FALSE;
        if(  (m_CurrentDeployAsset[0] == NULL) )
        {
            // Check if we have any more assets to deploy.
            if( m_nDeployedTurrets >= MAX_TURRETS_ALLOWED - 1
                && m_nDeployedInvens >= MAX_INVENS_ALLOWED - 1
                && m_nDeployedSensors >= MAX_SENSORS_ALLOWED - 1 )
                // No more assets to deploy.
            {
                Done = TRUE;
            }
            else
            {
                // Get a new asset to deploy.
                if (--LastAssetCheck <= 0)     
                    ChooseAssetSpotFn(ObjMgr.NullID);
                else
                {
                    LastAssetCheck = 30;
                }
                if (m_CurrentDeployAsset[0] == NULL)
                    Done = TRUE;
            }
        }
        
        if (m_DeployTask->GetNumAssignedBots() == 1 && !Done)
        {
            ASSERT(m_CurrentDeployAsset[0]);
            m_DeployBot = m_DeployTask->GetAssignedBotID();
            ASSERT(m_DeployBot != -1);
            ASSERT(m_TeamBot[m_DeployBot]->m_CurTask->GetID() >= bot_task::TASK_TYPE_DEPLOY_TURRET 
                && m_TeamBot[m_DeployBot]->m_CurTask->GetID() <= bot_task::TASK_TYPE_DEPLOY_SENSOR);

            SetUpDeployAssignment(m_DeployBot);
            nDefenseUnassigned--;
        }
    }

    s32 k;
    u32 Assigned;
    // Assign Destroyers:
    // Go through each destroy task

    // Two runs through destroyers- first assign all destroy tasks with
    // their m_DestroyBot set already.
    // Then, if any bots left to assign, assign more.
    for (k = 0; k < MAX_DESTROY_BOTS; k++)
    if ( m_DestroyTaskList[k]->GetPriority() )
    {
        Assigned = m_DestroyTaskList[k]->GetAssignedBot();
        if (m_DestroyBot[k] != -1)
        {
            // if someone's already assigned, make sure he's our destroy bot.
            if (Assigned)
            {
#if defined (acheng)
//                // This bot is already assigned to this task.
//                ASSERT(m_DestroyTaskList[k]->GetAssignedBot() == 
//                         (u32)(1<<m_DestroyBot[k]) );
//                ASSERT(m_DestroyTaskList[k]->GetNumAssignedBots() <= 1);
#endif
                continue;
            }
            else
            {
                m_TeamBot[m_DestroyBot[k]]->EnableLoadout();
                AssignBotTask(m_DestroyBot[k], m_DestroyTaskList[k]);
                nOffenseUnassigned--;
                ASSERT(m_DestroyTaskList[k]->GetNumAssignedBots() <= 1);
            }
        }
    } 
    
    // Assign the remainder of the offense to destroy.
    for (k = 0; k < MAX_DESTROY_BOTS; k++)
    if (m_DestroyTaskList[k]->GetPriority() && nOffenseUnassigned > 0)
    {
        Assigned = m_DestroyTaskList[k]->GetAssignedBot();
        if (m_DestroyBot[k] == -1)
        {
            // someone's already assigned.
            if (Assigned)
            {
                ASSERT(m_DestroyTaskList[k]->GetNumAssignedBots() <= 1);
                m_DestroyBot[k] = m_DestroyTaskList[k]->GetAssignedBotID();
                continue;
            }
            // Find a bot to be put on this task
            for (i = 0; i < m_nTeamBots && nOffenseUnassigned > 0; i++)
            {
                if (m_TeamBot[i]->HasTask())    continue;

                AssignBotTask(i, m_DestroyTaskList[k]);
                nOffenseUnassigned--;
                m_TeamBot[i]->EnableLoadout();
                m_DestroyTaskList[k]->SetState(bot_task::TASK_STATE_DESTROY);
                m_DestroyBot[k] = i;
                break;
            }
            ASSERT(m_DestroyTaskList[k]->GetNumAssignedBots() <= 1);
        }
        else
        {
            // make sure he's already assigned. 
#if defined (acheng)
//            // This bot is already assigned to this task.
//            ASSERT(m_DestroyTaskList[k]->GetAssignedBot() == 
//                     (u32)(1<<m_DestroyBot[k]) );
//            ASSERT(m_DestroyTaskList[k]->GetNumAssignedBots() <= 1);
#endif
            continue;
        }
    }

    // Assign someone to snipe.
    if (m_nSnipePoints > 0 
        && nDefenseUnassigned > 1
        && m_SnipeTask->GetNumAssignedBots() < m_SnipeTask->MaxBotsToBeAssigned())
    {   
        // Do we need to find a new sniping position?
        if (m_iCurrentSnipePos == -1)
        {
            // Find a new sniping position.
            PickSnipePoint();
            m_LastSnipeUpdate = (s32)m_Timer.ReadSec();
        }

        // Do we need to find a new sniper?
        if (m_iSniperBot == -1 || m_TeamBot[m_iSniperBot]->HasTask())
        {
            // Find a new sniper.
            m_iSniperBot = FindClosestAvailableBot(m_pSnipePoint[m_iCurrentSnipePos], Temp);
        }

        if (m_iSniperBot != -1)
        {
            AssignBotTask(m_iSniperBot, m_SnipeTask);
            m_SnipeTask->SetTargetPos(m_pSnipePoint[m_iCurrentSnipePos]);
            m_TeamBot[m_iSniperBot]->EnableLoadout();
            nDefenseUnassigned--;
        }
    }

    s32 LightCtr = m_SnipeTask->GetNumAssignedBots();
    s32 HeavyCtr = m_DeployTask->GetNumAssignedBots();
    // Assign Defense to the rest:
    for (i = 0; i < m_nTeamBots; i++)
    {
        if (!m_TeamBot[i]->HasTask())
        {
            if(( (m_TeamBot[i]->GetArmorType() == player_object::ARMOR_TYPE_HEAVY) // Already a heavy?
                && (HeavyCtr < (m_nDefense / 2) ) )
                || (LightCtr >= m_nDefense/2) )
            {
                HeavyCtr++;
                AssignBotTask(i, m_HeavyDefendTask);
                if (m_HeavyDefendTask->GetState() == bot_task::TASK_STATE_ROAM_AREA
                   || m_HeavyDefendTask->GetState() == bot_task::TASK_STATE_MORTAR)
                    m_TeamBot[i]->EnableLoadout();
                else
                    m_TeamBot[i]->DisableLoadout();
            }
            else 
            {
                LightCtr++;
                AssignBotTask(i, m_LightDefendTask);
                if (m_LightDefendTask->GetState() == bot_task::TASK_STATE_ROAM_AREA)
                    m_TeamBot[i]->EnableLoadout();
                else
                    m_TeamBot[i]->DisableLoadout();
            }
        }
    }

    if (g_BotDebug.ShowOffense)
    {
#if defined (acheng)
        x_printfxy(4, 17 + 5*(m_TeamBits - 1), "%s Offense: ", TEAM[m_TeamBits]);
#endif
        s32 CurLine = 18 + 5*(m_TeamBits - 1);
        for (i = 0; i < m_nTeamBots; i++)
        {
            if (m_TeamBot[i]->m_CurTask &&
                m_TeamBot[i]->m_CurTask->GetTaskName()[0] == 'X')
            {
                if (m_TeamBot[i]->m_CurTask->GetID() == bot_task::TASK_TYPE_DESTROY)
                {
                    object::type Type = object::TYPE_NULL;
                    if (m_TeamBot[i]->m_CurTask->GetTargetObject())
                        Type = m_TeamBot[i]->m_CurTask->GetTargetObject()->GetType();
                    x_printfxy(4, CurLine++, "Bot #%d: %s \t- Target: %s", i, 
                        (const char*)m_TeamBot[i]->m_CurrentGoal.GetGoalString(),
                        Type == object::TYPE_GENERATOR ? "generator" : 
                        (Type == object::TYPE_TURRET_FIXED ?"fixed turret":
                        (Type == object::TYPE_TURRET_CLAMP ?"deployed turret ":
                        (Type == object::TYPE_TURRET_SENTRY?"sentry turret":
                        (Type == object::TYPE_SENSOR_REMOTE?"deployed sensor ": 
                        (Type == object::TYPE_NULL?"none": "other")))))
                        );        
                }
                else
                {
                    // Capper
                    x_printfxy(4, CurLine++, "Bot #%d: %s ", i, 
                        (const char*)m_TeamBot[i]->m_CurTask->GetTaskStateName() );        
                }
            }
            
        }
    }
    else if (g_BotDebug.ShowTaskDistribution)
    {
        static s32 ctr = 0;
        if( ctr == 0 && input_WasPressed( INPUT_KBD_NUMPAD0, 0 ) )
        {
            StormX++;
            if (StormX >= m_nTeamBots)
                StormX = 0;
            if (m_TeamBotObjectID[StormX] != obj_mgr::NullID)
                ExamineBot = m_TeamBot[StormX];
            else
                ExamineBot = NULL;
            ctr++;
        }
        if (ctr > 0)
        {
            ctr++;
            if (ctr > 4)
                ctr = 0;
        }
        x_printfxy(4, 17, "Storm Bots");
        x_printfxy(4, 26, "Inferno Bots");
        xbool Marked = FALSE;
        for (i = 0 ; i < m_nTeamBots; i++)
        {
            if (m_TeamBits == 1 && i == StormX) 
            {
                Marked = TRUE;
            }
            if (m_TeamBot[i]->m_CurTask->GetID() == bot_task::TASK_TYPE_DESTROY
                && m_TeamBot[i]->m_CurTask->GetTargetObject())
            {
                object::type Type = m_TeamBot[i]->m_CurTask->GetTargetObject()->GetType();
                x_printfxy(4, 9*(m_TeamBits + 1)+i, "%cBot #%d: %s:\t%s:\t%s\t- Target: %s", 
                    (Marked?'*':' '),
                    i, 
                    m_TeamBot[i]->m_CurTask->GetTaskName(), 
                    m_TeamBot[i]->m_CurTask->GetTaskStateName(),
                    (const char*)m_TeamBot[i]->m_CurrentGoal.GetGoalString(),
                    Type == object::TYPE_GENERATOR ? "generator" : 
                    (Type == object::TYPE_TURRET_FIXED ?"fixed turret":
                    (Type == object::TYPE_TURRET_CLAMP ?"deployed turret ":
                    (Type == object::TYPE_TURRET_SENTRY?"sentry turret":
                    (Type == object::TYPE_SENSOR_REMOTE?"deployed sensor ": "other"))))
                    );
            }
            else
            {
                x_printfxy(4, 9*(m_TeamBits + 1)+i, "%cBot #%d: %s:\t%s:\t%s", 
                    (Marked?'*':' '),
                    i, 
                    m_TeamBot[i]->m_CurTask->GetTaskName(), 
                    m_TeamBot[i]->m_CurTask->GetTaskStateName(),
                    (const char*)m_TeamBot[i]->m_CurrentGoal.GetGoalString());
            }

            Marked = FALSE;
        }
    }

    CleanUpTasks();
#ifndef T2_DESIGNER_BUILD
#if defined (acheng) 
static char TS[4] = " SI";
    x_printfxy(40, 4+m_TeamBits, "%c Destroy Run: %s, Deaths %d", TS[m_TeamBits], m_DestroyRun?"ON":"OFF", m_nOffenseDeaths);
#endif
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

void ctf_master_ai::UpdateAllTaskStates ( void )
{



    return;




#if 0
    
    s32 i;
    bot_task* pCurTask;
    bot_object* pCurBot;

    for ( i = 0; i < m_nTeamBots; ++i )
    {
        pCurBot = m_TeamBot[i];

        if ( pCurBot == NULL )
        {
            ASSERT( pCurBot ); // m_TeamBot/m_nTeamBots are messed up
            continue;
        }

        pCurTask = pCurBot->GetTask();
        if ( pCurTask != NULL )
        {
            if ( !HasIdealLoadout( i ) )
            {
                // Get the loadout
                pCurTask->SetState( bot_task::TASK_STATE_LOADOUT );
    
                player_object::loadout IdealLoadout;
                default_loadouts::GetLoadout(
                    pCurBot->GetCharacterType()
                    , pCurTask->GetIdealLoadout()
                    , IdealLoadout );
                
                pCurBot->SetInventoryLoadout( IdealLoadout );
                pCurTask->SetTargetPos(
                    GetNearestInvenPos( pCurBot->GetPos() ) );
            }
        }
    }
#endif
}
    
//==============================================================================

void ctf_master_ai::RegisterOffendingTurret ( object::id ObjID )
{
    turret* Turret;
    Turret = (turret*)ObjMgr.GetObjectFromID(ObjID);

    if (Turret->GetTeamBits() & m_TeamBits) 
        // This turret is on our side!
        return;

    s32 i;
    s32 Slot = -1;

    for (i = 0; i < MAX_DESTROY_BOTS; i++)
    {
        if (m_DestroyBot[i] != -1
            && (Turret->GetPosition() - m_TeamBot[m_DestroyBot[i]]->GetPosition())
            .LengthSquared() < 6400)
        {
            m_DestroyTaskList[i]->SetTargetObject(Turret);
            m_DestroyTaskList[i]->SetTargetPos(Turret->GetPosition());
        }
    }

    // Check if already there.
    for (i = 0; i < 5; i++)
    {
        if (m_FiveMostWanted[i] == ObjID)
            return;
        else if (m_FiveMostWanted[i] == ObjMgr.NullID)
            Slot = i;
    }

    if (Slot == -1)
        m_FiveMostWanted[x_rand()%5] = ObjID;
    else
        m_FiveMostWanted[Slot] = ObjID;
}

//==============================================================================

void ctf_master_ai::ResetDestroyTask(s32 TaskNum)
{
    m_DestroyTaskList[TaskNum]->SetTargetObject(NULL);
}

//==============================================================================

void ctf_master_ai::OffenseBotDied ( s32 BotIdx )
{
    ForceReset = TRUE;
    s32 i;

    if (m_DeployBot == BotIdx)
        m_DeployBot = -1;

    for (i = 0; i < 4; i++)
    {
        if (m_DestroyBot[i] == BotIdx)
        {
            m_DestroyBot[i] = -1;
#if defined (acheng)
            x_printf("%s Destroyer (%d) died", TEAM[m_TeamBits], BotIdx);
#endif
            break;
        }
    }

    if (m_nOffense < 2)
        return;
    // Are we already on a destroy run?
    if (m_DestroyRun)
    {
        // Hit ceiling yet?
        if (m_Timer.ReadSec() - m_LastDestroyRun  > 60.0f 
            && m_nExpectedDestroyers >= MAX(m_nOffense - 1, 2))
        {
            m_DestroyRun = FALSE;
            m_nOffenseDeaths = 0;
            m_LastDestroyRun = m_Timer.ReadSec();
            if (m_nExpectedDestroyers > 1)
            {
                m_nExpectedDestroyers--;
            }
        }
        // Haven't hit ceiling, so set him to destroy.
        else
        {
            if (m_nExpectedDestroyers < m_nOffense)
            {
                xbool AlreadyDestroyer = FALSE;
                // Check if he's already a destroyer.
                for (i = 0; i < MAX_DESTROY_BOTS; i++)
                {
                    if (m_DestroyBot[i] == BotIdx)
                        AlreadyDestroyer = TRUE;
                        break;
                }
                if (!AlreadyDestroyer)
                {
                    m_nExpectedDestroyers++;
                }
            }
        }
    }

    // Not on destroy run.  Is it time?
    else
    {
        if ( (++m_nOffenseDeaths > MAX(m_nTeamBots, 4))
        && (m_Timer.ReadSec() - m_LastDestroyRun > 90.0f))
        {
            m_DestroyRun = TRUE;
            m_nOffenseDeaths = 0;
            m_LastDestroyRun = m_Timer.ReadSec();
            if (m_nExpectedDestroyers < m_nOffense)
            {
                xbool AlreadyDestroyer = FALSE;
                // Check if he's already a destroyer.
                s32 i;
                for (i = 0; i < MAX_DESTROY_BOTS; i++)
                {
                    if (m_DestroyBot[i] == BotIdx)
                        AlreadyDestroyer = TRUE;
                        break;
                }
                if (!AlreadyDestroyer)
                {
                    m_nExpectedDestroyers++;
                }
            }
        }
        else 
        {
            m_nExpectedDestroyers = 1;
        }
    }
}

//==============================================================================

void ctf_master_ai::CleanUpTasks ( void )
{
    s32 i;
    // u32 Assignment[15];
    x_memset(m_DestroyBot, -1, sizeof(m_DestroyBot));

    // Reset every task.
    for (i = 0; i < m_nTasks; i++)
    {
        //  Assignment[i] = m_GoalList[i].GetAssignedBot();
        m_GoalList[i].ResetAssignments();
    }

    // Check each bot & determine their task & add the bot to the task.
    for (i = 0; i < m_nTeamBots; i++)
    {
        if (m_TeamBot[i]->HasTask())
        {
            m_TeamBot[i]->GetTask()->SetAssignedBot(i);
            if (m_TeamBot[i]->GetTask()->GetID() == bot_task::TASK_TYPE_DESTROY)
            {
                SetDestroyBot(i);
            }
        }
    }
    return;
    /*
    for (i = 0; i < m_nTasks; i++)
    {
        if (Assignment[i] != m_GoalList[i].GetAssignedBot())
        {
            x_printf("Task assignments changed upon cleanup!\n");
            x_printf("%s task thought %d was assigned, actual: %d assigned\n",
                m_GoalList[i].GetTaskName(), Assignment[i], m_GoalList[i].GetAssignedBot());
        }
    }
    */
}

//==============================================================================

void ctf_master_ai::SetDestroyBot ( s32 BotID)
{
    s32 i;
    for (i = 0; i < 4; i++)
    {
        if (m_DestroyTaskList[i] == m_TeamBot[BotID]->m_CurTask)
        {
            m_DestroyBot[i] = BotID;
            return;
        }
    }
    ASSERT(0);
}

//==============================================================================

void ctf_master_ai::SetUpDeployTask ( void )
{
    m_DeployTask->SetPriority(100);
    if (m_DeployType[0] == DEPLOY_TYPE_TURRET)
    {
        m_DeployTask->SetID(bot_task::TASK_TYPE_DEPLOY_TURRET);
        m_DeployTask->SetState(bot_task::TASK_STATE_DEPLOY_TURRET);
    }
    else if (m_DeployType[0] == DEPLOY_TYPE_INVEN)
    {
        m_DeployTask->SetID(bot_task::TASK_TYPE_DEPLOY_INVEN);
        m_DeployTask->SetState(bot_task::TASK_STATE_DEPLOY_INVEN);
    }

    else if (m_DeployType[0] == DEPLOY_TYPE_SENSOR)
    {
        m_DeployTask->SetID(bot_task::TASK_TYPE_DEPLOY_SENSOR);
        m_DeployTask->SetState(bot_task::TASK_STATE_DEPLOY_SENSOR);
    }

    ASSERT(m_CurrentDeployAsset[0]);
    m_DeployTask->SetTargetPos(m_CurrentDeployAsset[0]->GetStandPos());
}

//==============================================================================

void ctf_master_ai::SetUpDeployAssignment (  s32 BotID )
{
    AssignBotTask(BotID , m_DeployTask);
    m_TeamBot[BotID ]->EnableLoadout();
    if (m_DeployType[0] == DEPLOY_TYPE_TURRET)
    {
        m_TeamBot[BotID ]->m_CurrentGoal.SetAuxLocation(
                  ((turret_spot*)m_CurrentDeployAsset[0])->GetAssetPos());
        m_TeamBot[BotID ]->m_CurrentGoal.SetAuxDirection(
                    ((turret_spot*)m_CurrentDeployAsset[0])->GetNormal());
    }
    else if (m_DeployType[0] == DEPLOY_TYPE_INVEN)
    {
        m_TeamBot[BotID ]->m_CurrentGoal.SetAuxLocation(
                 ((inven_spot*)m_CurrentDeployAsset[0])->GetAssetPos());
        m_TeamBot[BotID ]->m_CurrentGoal.SetAuxDirection(
                 ((inven_spot*)m_CurrentDeployAsset[0])->GetYaw());
    }
    else if (m_DeployType[0] == DEPLOY_TYPE_SENSOR)
    {
        m_TeamBot[BotID ]->m_CurrentGoal.SetAuxLocation(
                 ((sensor_spot*)m_CurrentDeployAsset[0])->GetAssetPos());
        m_TeamBot[BotID ]->m_CurrentGoal.SetAuxDirection(
            ((sensor_spot*)m_CurrentDeployAsset[0])->GetNormal());
    }
    ASSERT(m_TeamBot[BotID ]->m_CurrentGoal.GetAuxDirection().LengthSquared() < 100.0f);
}

//==============================================================================

void ctf_master_ai::AssetPlacementFailed ( s32 BotIdx )
{
    (void)BotIdx;
    s32 i;
    xbool Replaced = FALSE;
    xbool Removed = FALSE;

    // Deploy task may have already been cancelled through ScanAssets.
    if (m_CurrentDeployAsset[0])
    {
        // Remove the bad asset spot, and try to replace it.
        switch (m_DeployType[0])
        {
        case (DEPLOY_TYPE_TURRET):
            for ( i = 0; i < m_DeploySpots.m_nTurrets; i++)
            {
                if (m_CurrentDeployAsset[0]->GetAssetPos() == m_DeploySpots.m_pTurretList[i].GetAssetPos())
                {
                    RemoveDeployable(m_DeployType[0], i);
                    Removed = TRUE;
                    if (ChooseTurretSpot(ObjMgr.NullID))
                        Replaced = TRUE;
                    break;
                }
            }
            break;

        case (DEPLOY_TYPE_INVEN):
            for ( i = 0; i < m_DeploySpots.m_nInvens; i++)
            {
                if (m_CurrentDeployAsset[0]->GetAssetPos() == m_DeploySpots.m_pInvenList[i].GetAssetPos())
                {
                    RemoveDeployable(m_DeployType[0], i);
                    Removed = TRUE;
                    if (ChooseInvenSpot(ObjMgr.NullID))
                        Replaced = TRUE;
                    break;
                }
            }
            break;

        case (DEPLOY_TYPE_SENSOR):
            for ( i = 0; i < m_DeploySpots.m_nSensors; i++)
            {
                if (m_CurrentDeployAsset[0]->GetAssetPos() == m_DeploySpots.m_pSensorList[i].GetAssetPos())
                {
                    RemoveDeployable(m_DeployType[0], i);
                    Removed = TRUE;
                    if (ChooseSensorSpot(ObjMgr.NullID))
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
        m_CurrentDeployAsset[0] = NULL;
        ChooseAssetSpotFn(ObjMgr.NullID);
    }

    if (m_CurrentDeployAsset[0])
    {
        SetUpDeployTask();
    }
}
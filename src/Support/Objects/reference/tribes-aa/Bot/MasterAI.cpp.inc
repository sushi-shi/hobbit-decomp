//==============================================================================
//  
//  MasterAI.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "MasterAI.hpp"
#include "GameMgr/GameMgr.hpp"
#include "Deployable.hpp"
#include "Objects/Projectiles/Sensor.hpp"
#include "Objects/Projectiles/Generator.hpp"
#include "Objects/Vehicles/Vehicle.hpp"
#include "AADS\fe_Globals.hpp"
//#include "Objects/Projectiles/Sensor.hpp"
#define MIN_DIST_BTWN_TURRETS_SQ 400
#define MIN_DIST_BTWN_SENSORS    60
#define MIN_DIST_BTWN_SENSORS_SQ MIN_DIST_BTWN_SENSORS*MIN_DIST_BTWN_SENSORS
#define DESTROY_CULL_DIST_SQ    10000               // 100 m
#define DESTROY_SHORT_CULL_DIST_SQ 225              // 15 m
#define DESTROY_FIXED_TURRET_CULL_DIST_SQ    22500  // 150 m

#ifdef acheng
#define SCAN_ASSET_STATS 1
#endif
//==============================================================================
//  Storage
//==============================================================================


s32 TeamBitsFound = 0;

extern xbool g_RunningPathEditor;

u32 g_SCtr = 0;

//==============================================================================
//  MASTER AI FUNCTIONS
//==============================================================================

master_ai::master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
}

//==============================================================================

master_ai::~master_ai ( void )
{
}

//==============================================================================

void master_ai::Initialize ( void )
{
    ASSERT(m_TeamBits == 0);
    m_MissionInitialized = FALSE;

    x_memset(m_SortedGoals, NULL, sizeof(m_SortedGoals)); 
    x_memset(m_CurrentDeployAsset, NULL, sizeof(m_CurrentDeployAsset)); 

    m_nTeamBots = 0;
    m_nTasks = 0;
    m_PlayersChanged = FALSE;

    m_DeploySpots.m_nInvens  = 0;
    m_DeploySpots.m_nTurrets = 0;
    m_DeploySpots.m_nSensors = 0;
    m_DeploySpots.m_nMines   = 0;

    m_DeployBot = -1;

    m_nSnipePoints = 0;

    m_nDeployedSensors = 0;
    m_nDeployedTurrets = 0;
    m_nDeployAssets = 0;
    m_nDeployedInvens = 0;

    m_TurretFull = FALSE;
    m_SensorFull = FALSE;
    m_InvenFull = FALSE;

    m_iCurrentSnipePos = -1;
    m_iCurrDestroy = -1;
    m_iLastUpdated = -1;

    m_nDestroyTasks = 0;
    m_EnemyVehicle = ObjMgr.NullID;

    s32 i;
    for (i = 0; i < MAX_DESTROY_BOTS; i++)
    {
        m_DestroyBot[i] = -1;
    }

    for (i = 0; i < 5; i++)
    {
        m_FiveMostWanted[i] = ObjMgr.NullID;
    }

    m_Difficulty = fegl.ServerSettings.BotsDifficulty;
    m_Timer.Reset();
    m_Timer.Start();
}


//==============================================================================

void master_ai::Update()
{
    if ( g_RunningPathEditor ) 
    {
        return;
    }
    UpdatePlayerPointers();
    ResetAssignments();
    
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
}
//==============================================================================

void master_ai::UpdatePlayerPointers( void )
{
    // Set the team bit flag appropriately.

    m_PlayersChanged = FALSE;

    // Reset values
    m_HeavyList = 0;
    m_nDestroyBots = 0;
//    m_DeployBot = -1;
    m_iSniperBot = -1;
    m_RepairBot = -1;
//    x_memset(m_DestroyBot, -1, sizeof(m_DestroyBot));
    m_EnemyVehicle = ObjMgr.NullID;

    player_object* CurrPlayer;
    bot_object*    CurrBot;
    s32 i = 0;
    s32 j = 0;

    RemoveDeadPlayers();

    // Grab all the player object pointers.
    ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
    while ( (CurrPlayer = (player_object*)(ObjMgr.GetNextInType())) != NULL )
    {
        // If no team bits have been set yet, set this one to the first it finds.
        if (!TeamBitsFound)
        {
            m_TeamBits = 0x01;
            TeamBitsFound |= m_TeamBits;
//            m_TeamBits = CurrPlayer->GetTeamBits();
//            TeamBitsFound |= m_TeamBits;
        }
        
        if (CurrPlayer->GetTeamBits() & m_TeamBits )
        {
            // Don't add player objects to our masterAI stuff- only bots.
        }
        else 
        {
            // This guy plays for another team!
            ASSERT (j < 32);
            if (m_OtherTeamPlayerObjectID[j] != CurrPlayer->GetObjectID())
            {
                m_PlayersChanged = TRUE;
                m_OtherTeamPlayerObjectID[j] = CurrPlayer->GetObjectID();
            }
            m_OtherTeamPlayer[j] = CurrPlayer;
            j++;

            object* pVehicle = CurrPlayer->IsAttachedToVehicle();
            if (pVehicle)
            {
//                m_EnemyVehicle = pVehicle->GetObjectID();
            }

        }
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
                // This guy must be on our team!
                m_TeamBits = CurrBot->GetTeamBits();
                TeamBitsFound |= m_TeamBits;
            }
        }
        
        if ( CurrBot->IsAttachedToVehicle() ) 
        {
            // This bot will not be controlled by any Master AI.
            continue;
        }
        
        if ( CurrBot->GetTeamBits() & m_TeamBits )
        {
            ASSERT (i < 16);
#ifdef E3
            if (CurrBot->m_CurTask && 
                (CurrBot->m_CurTask->GetID() == bot_task::TASK_TYPE_OFF_DUTY) )
            {
                // This bot will not be controlled by any Master AI.
                continue;
            }
#endif
            xbool ThisPlayerChanged = FALSE;
            if (m_TeamBotObjectID[i] != CurrBot->GetObjectID())
            {
                ThisPlayerChanged = TRUE;
                m_PlayersChanged = TRUE;
                m_TeamBotObjectID[i] = CurrBot->GetObjectID();
            }
            m_TeamBot[i] = CurrBot;
            if (ThisPlayerChanged)
            {
                // do nothing to attempt to maintain his task.
            }
            else if( CurrBot->m_CurTask && 
                (CurrBot->m_CurTask->GetID() == bot_task::TASK_TYPE_DESTROY) )
            {
                ASSERT(m_nDestroyBots <= MAX_DESTROY_BOTS);
                SetDestroyBot(i);
                m_nDestroyBots++;
            }
            else if (m_DeployBot == -1 &&
                (CurrBot->m_PrevTask && 
                (  CurrBot->m_PrevTask->GetState() == bot_task::TASK_STATE_DEPLOY_TURRET 
                || CurrBot->m_PrevTask->GetState() == bot_task::TASK_STATE_DEPLOY_SENSOR 
                || CurrBot->m_PrevTask->GetState() == bot_task::TASK_STATE_DEPLOY_INVEN 
                || CurrBot->m_Loadout.Inventory.Counts[player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR]
                || CurrBot->m_Loadout.Inventory.Counts[player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION]
                || CurrBot->m_Loadout.Inventory.Counts[player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR])))
            {
                // Maintain any deploy task
                m_DeployBot = i;
            }

            // any sniper bots?
            else if (
                (CurrBot->m_PrevTask && 
                (CurrBot->m_PrevTask->GetState() == bot_task::TASK_STATE_SNIPE))
                || CurrBot->m_Loadout.Inventory.Counts[player_object::INVENT_TYPE_WEAPON_SNIPER_RIFLE])
            {
                m_iSniperBot= i;
            }

            else if ((CurrBot->m_PrevTask) &&
                    CurrBot->m_PrevTask->GetState() == bot_task::TASK_STATE_REPAIR)
            {
                m_RepairBot = i;
            }
            
            // Record the bot's armor
            CurrBot->m_MAIIndex = i;
            m_BotArmorType[i] = CurrBot->GetArmorType();
            if (m_BotArmorType[i] == player_object::ARMOR_TYPE_HEAVY)
            {
                m_HeavyList |= 1<<i;
            }

            i++;
        }
        else if ( CurrBot->GetTeamBits() & ~m_TeamBits )
        {

            ASSERT (j < 32);
            if (m_OtherTeamPlayerObjectID[j] != CurrBot->GetObjectID())
            {
                m_PlayersChanged = TRUE;
                m_OtherTeamPlayerObjectID[j] = CurrBot->GetObjectID();
            }
            m_OtherTeamPlayer[j] = CurrBot;
            j++;
        }
    }
    ObjMgr.EndTypeLoop();

    if (m_PlayersChanged)
    {
        // All our indices are hosed.  Clear them all.
        if (m_RepairBot != -1)
            m_RepairBot = -1;
        s32 count;
        m_iCurrDestroy = -1;
        for (count = 0; count <  MAX_DESTROY_BOTS; count++)
        {
            if (m_DestroyBot[count] != -1)
                m_DestroyBot[count] = -1;
        }
        if (m_DeployBot != -1)
                m_DeployBot = -1;
        if (m_iSniperBot != -1)
            m_iSniperBot = -1;

        // Reset Stored Task assignments.
        for (s32 Count = 0; Count < MAX_TASKS; Count++)
        {
            if (m_GoalList[Count].GetState() == bot_task::TASK_STATE_DESTROY
                || m_GoalList[Count].GetState() == bot_task::TASK_STATE_REPAIR)
                m_GoalList[Count].ResetAssignments();
            if (m_LastTask[Count].GetState() == bot_task::TASK_STATE_DESTROY
                || m_LastTask[Count].GetState() == bot_task::TASK_STATE_REPAIR)
            m_LastTask[Count].ResetAssignments();
        }
    }
   
    m_nTeamBots = i;
    m_nOtherTeamPlayers = j; 
}

//==============================================================================

void master_ai::PollAllObjects ( void )
{
    ASSERT(m_MissionInitialized);

    if (m_nTeamBots == 0)
        return;
}

//==============================================================================

void master_ai::PrioritizeTasks ( void )
{
    ASSERT(m_MissionInitialized);
    // Run through the task list and set all the priority values.
    s32 i;
    for ( i = 0; i < m_nTasks; i++ )
    {
        m_GoalList[i].Update();
    }
}

//==============================================================================

void master_ai::SortList ( void )
{
    ASSERT(m_MissionInitialized);
    // Sort the goal list by priority value.

    // simple bubble sort on task list
    s32 i, j;
    bot_task* temp;
    xbool done;
    for (i = 0; i < m_nTasks; i++)
    {
        m_SortedGoals[i] = &m_GoalList[i];
    }
    for (i = 0; i < m_nTasks; i++)
    {
        done = true;
        for (j = 0; j < m_nTasks-1; j++)
        {
            if (m_SortedGoals[j]->GetPriority() < m_SortedGoals[j+1]->GetPriority())
            {
                done = false;
                temp = m_SortedGoals[j];
                m_SortedGoals[j] = m_SortedGoals[j+1];
                m_SortedGoals[j+1] = temp;
            }
        }
        if (done)
            break;
    }
}

//==============================================================================

void master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
}

//==============================================================================

void master_ai::ResetAssignments ( void )
{
    s32 i;
    for (i = 0; i < m_nTeamBots; i++)
    {
        // Clear all assignments.
        if (m_TeamBot[i]->HasTask())
        {
            m_TeamBot[i]->GetTask()->UnassignBot(i);
            m_TeamBot[i]->SetTask(NULL);
        }
    }
    
    if (m_PlayersChanged || GameMgr.GetGameType() == GAME_CTF)
    {
        // Task Bit values are all screwed up.  Reset them.
        for (i = 0; i < m_nTasks; i++)
            m_GoalList[i].ResetAssignments();
    }
}

//==============================================================================

void master_ai::UpdateAllTaskStates ( void )
{
    // do nothing
}
    
//==============================================================================

// Returns index of closest bot who doesn't already have an assignment.
s32 master_ai::FindClosestAvailableBot( bot_task *Task, f32& DistSq, u32 Excluded /* =EXCLUDE_ASSIGNED */)
{
    return FindClosestAvailableBot(Task->GetTargetPos(), DistSq, Excluded);
}

//==============================================================================

// Returns index of closest bot who doesn't already have an assignment.
s32 master_ai::FindClosestAvailableBot( const vector3& TaskPos, f32& DistSq, u32 Excluded /* =EXCLUDE_ASSIGNED */)
{
    s32 i, ClosestBot = -1;
    f32 MinDist = F32_MAX;

    vector3 BotPos;

    vector3 vDistance;
    f32 Distance;

    for (i = 0; i < m_nTeamBots; i++)
    {
        if (Excluded & EXCLUDE_ASSIGNED )
        {
            if ( m_TeamBot[i]->HasTask() )
                continue;
        }

        {
            // Use exclusion list passed.
            if ( (1<<i) & Excluded )
                continue;
        }

        BotPos = m_TeamBot[i]->GetPosition();
        vDistance = BotPos - TaskPos;
        Distance = vDistance.LengthSquared();
        if (Distance < MinDist)
        {
            MinDist = Distance;
            ClosestBot = i;
        }
    }
    if (ClosestBot == -1)
    {
        DistSq = -1;
        return -1;
    }

    if (m_TeamBot[ClosestBot]->HasTask())
    {
        // We're about to reassign this boy.
        m_TeamBot[ClosestBot]->GetTask()->UnassignBot(ClosestBot);
    }

    DistSq = MinDist;
    return ClosestBot;
}

//==============================================================================

s32 master_ai::FindClosestEnemy( const vector3& Pos, f32& DistSq, s32 RadiusSquared ) const
{
    // Find the closest enemy within a particular radius, returns -1 if none.
    //      if radius == 0 or negative, then radius is infinity.
    s32 i;
    s32 ClosestBot = -1;
    vector3 vDistance;
    f32 Dist = 0;
    f32 MinDist = F32_MAX;
    s32 MaxDist = ((RadiusSquared>0)?RadiusSquared:S32_MAX);

    for (i = 0 ; i < m_nOtherTeamPlayers; i++)
    {
        if (m_OtherTeamPlayer[i]->IsDead())
            continue;
        vDistance = m_OtherTeamPlayer[i]->GetPosition() - Pos;
        Dist = vDistance.LengthSquared();

        if(Dist < MinDist && Dist < MaxDist)
        {
            MinDist = Dist;
            ClosestBot = i;
        }
    }
    DistSq = MinDist;
    return ClosestBot;
}

//==============================================================================

void master_ai::AssignBotTask ( s32 BotID, bot_task* Task )
{
    bot_task* Prev = NULL;
    ASSERT(m_MissionInitialized);
    if (m_TeamBot[BotID]->HasTask())
    {
        Prev = m_TeamBot[BotID]->GetTask();
        Prev->UnassignBot(BotID);
    }
    m_TeamBot[BotID]->SetTask(Task);
    if (Task)
        Task->SetAssignedBot(BotID);
}
 
//==============================================================================

xbool master_ai::HasIdealLoadout ( s32 BotID )
{
    player_object::loadout CurLoad, IdealLoad;
    default_loadouts::GetLoadout(m_TeamBot[BotID]->GetCharacterType(), 
        m_TeamBot[BotID]->GetTask()->GetIdealLoadout(), IdealLoad);

    CurLoad = m_TeamBot[BotID]->GetLoadout();
    
    return (IdealLoad.Armor == CurLoad.Armor);
    // Find a better way to compare the two loadouts!

    /*
        armor_type          Armor;                              // Type of armor
        
        invent_type         Weapons[PLAYER_WEAPON_SLOTS];       // Type of weapons
        s32                 NumWeapons;                         // Number of weapons

        object::type        VehicleType ;                       // Type of vehicle to get at stations

        inventory           Inventory ;                         // Items

*/
}

//==============================================================================
// Return TRUE if under attack.
xbool master_ai::SetTaskDefend( bot_task *Task,  // Task to set to attack/roam
                       const vector3& DefendPos,
                        s32 EngageRadiusSq,     // Radius within which to attack
                        s32 RetreatRadiusSq,    // Radius at which to retreat
                        f32& DistSq)          // Task position moves? (Escort?)
{
    s32 MaxDist = 0;
    
    // Currently defending against intruder?
    if (Task->GetState() == bot_task::TASK_STATE_ATTACK
        || Task->GetState() == bot_task::TASK_STATE_MORTAR)
    {
        MaxDist = RetreatRadiusSq;
    }
    // or just patrolling the base?
    else if (Task->GetState() == bot_task::TASK_STATE_ROAM_AREA)
    {
        MaxDist = EngageRadiusSq;
    }

    ASSERT(MaxDist != 0);

    // Determine who (if any) is the closest to our flag within a radius.
    s32 ClosestEnemyToHome = FindClosestEnemy(DefendPos, DistSq, MaxDist);

    // If there is a bot within our radius
    if (ClosestEnemyToHome != -1)
    {
        Task->SetTargetObject(m_OtherTeamPlayer[ClosestEnemyToHome ]);
        Task->SetState(bot_task::TASK_STATE_ATTACK);
        vector3 TargetPos = m_OtherTeamPlayer[ClosestEnemyToHome]->GetPosition() + DefendPos;
        TargetPos /= 2;
        Task->SetTargetPos( TargetPos );

        return TRUE;
    }
    // Otherwise just patrol around the base.
    else
    {
        Task->SetTargetObject(NULL);
        Task->SetState(bot_task::TASK_STATE_ROAM_AREA);
        Task->SetTargetPos(DefendPos);
        ASSERT(Task->GetTargetPos().InRange(-1000.0f, 10000.0f));
        return FALSE;
    }

}

//==============================================================================

struct bottarget
{
	object* pObj;
	f32		DistSq;
};

//==============================================================================

void SortInTargetList ( object* pObj, 
					    vector3& BotPos, 
						xarray<bottarget>&Targets, 
						xarray<s32>& TargetOrder,
						f32 MaxDist )
{
	f32 DistSq = (BotPos - pObj->GetPosition()).LengthSquared();
	if (DistSq < MaxDist)
	{
		// Set up a list of targets, and sort them.
		bottarget& BotTarget = Targets.Append();
		BotTarget.pObj = pObj;
		BotTarget.DistSq = DistSq;

		// Figure out where this one goes.
		s32 i;
		for (i = 0; i < TargetOrder.GetCount(); i++)
		{
			if (DistSq < Targets[i].DistSq)
				break;
		}
		s32 Idx = Targets.GetCount() - 1;// index should be last appended.
		TargetOrder.Insert(i, Idx);// Insert here.
	}
}

//==============================================================================

// Scan all assets,set up Destroy & Repair tasks, and record deployed locations.
xbool master_ai::ScanAssets( bot_task **DestroyTaskList, bot_task *RepairTask, xbool UnderAttack )
{

    //#############
    // Need to sync correct destroy task with this bot with multiple destroy tasks.
    // Next Destroy
    if (m_nDestroyTasks)
    {
        s32 NextUpdateIndex = (m_iLastUpdated + 1) % m_nDestroyTasks;
        if (NextUpdateIndex != m_iCurrDestroy)
        {
            m_iCurrDestroy = NextUpdateIndex;
        }
    }

    bot_task *DestroyTask = NULL;
    bot_object *DestroyBot = NULL;
    vector3   BotPos;
    BotPos.Zero();

    // Set the destroy task and destroy bot we are updating.
    if (DestroyTaskList && m_iCurrDestroy >= 0)
    {
        DestroyTask = DestroyTaskList[m_iCurrDestroy];
        if (m_DestroyBot[m_iCurrDestroy] != -1)
        {
            DestroyBot = m_TeamBot[m_DestroyBot[m_iCurrDestroy]];
            BotPos = DestroyBot->GetPosition();
        }
    }

#ifndef T2_DESIGNER_BUILD
#if defined (acheng) 
    x_printfxy(5, 7+m_TeamBits, "UnderAttack = %s", (UnderAttack?"TRUE":"FALSE"));
#endif
#endif
    
    s32 i;
    if (g_SCtr & 0x01)  // odd frames  
    {
		if (m_PlayersChanged)
		{
//			x_DebugMsg("Players Changed, skipping cached tasks\n");
		}
		else
        if( (m_TeamBits & 0x01) // Storm Skips
            && DestroyTask && RepairTask )
//            && m_LastTask[m_iCurrDestroy].GetID() != bot_task::TASK_TYPE_NONE
//            && m_LastTask[m_nDestroyTasks + 1].GetID() != bot_task::TASK_TYPE_NONE)
        {
            for (i = 0; i < m_nDestroyTasks; i++)
            {
                *DestroyTaskList[i] = m_LastTask[i];
                // Check the destroy targets if still valid.
                if (DestroyTaskList[i]->GetTargetObject() == NULL
                    || ( (DestroyTaskList[i]->GetTargetObject()->GetAttrBits() & object::ATTR_ASSET)
                        &&(((asset*)(DestroyTaskList[i]->GetTargetObject()))->GetHealth() == 0.0f)))
                {
                    DestroyTaskList[i]->SetTargetObject(NULL);
                    m_LastTask[i].SetTargetObject(NULL);
                }
                // Check if the assignments are still valid.
                if (DestroyTaskList[i]->GetAssignedBotID() >= m_nTeamBots)
                    DestroyTaskList[i]->ResetAssignments();

                if (DestroyTaskList[i]->GetAssignedBot() != (u32)(1 << m_DestroyBot[i]))
                {
                    m_DestroyBot[i] = DestroyTaskList[i]->GetAssignedBotID();
                    ASSERT(m_DestroyBot[i] < m_nTeamBots);
                    if (i == m_iCurrDestroy)
                    {
                        if (m_DestroyBot[m_iCurrDestroy] != -1)
                        {
                            DestroyBot = m_TeamBot[m_DestroyBot[m_iCurrDestroy]];
                            BotPos = DestroyBot->GetPosition();
                        }
                        else
                            DestroyBot = NULL;
                    }
                }
            }
            *RepairTask = m_LastTask[m_nDestroyTasks + 1];  
            ASSERT(RepairTask->GetAssignedBotID() < m_nTeamBots);
            return FALSE;
        }
    }
    else        // even frames
    {
		if (m_PlayersChanged)
		{
//			x_DebugMsg("Players Changed, skipping cached tasks\n");
		}
		else
        if( (m_TeamBits & 0x02) // Inferno Skips
            && DestroyTask && RepairTask)
//            && m_LastTask[m_iCurrDestroy].GetID() != bot_task::TASK_TYPE_NONE
//            && m_LastTask[m_nDestroyTasks + 1].GetID() != bot_task::TASK_TYPE_NONE)
        {
            for (i = 0; i < m_nDestroyTasks; i++)
            {
                *DestroyTaskList[i] = m_LastTask[i];
                // Check the destroy targets if still valid.
                if (DestroyTaskList[i]->GetTargetObject() == NULL
                    || ( (DestroyTaskList[i]->GetTargetObject()->GetAttrBits() & object::ATTR_ASSET)
                        &&(((asset*)(DestroyTaskList[i]->GetTargetObject()))->GetHealth() == 0.0f)))
                {
                    DestroyTaskList[i]->SetTargetObject(NULL);
                    m_LastTask[i].SetTargetObject(NULL);
                }
                // Check if the assignments are still valid.
                if (DestroyTaskList[i]->GetAssignedBotID() >= m_nTeamBots)
                    DestroyTaskList[i]->ResetAssignments();

                if (DestroyTaskList[i]->GetAssignedBot() != (u32)(1 << m_DestroyBot[i]))
                {
                    m_DestroyBot[i] = DestroyTaskList[i]->GetAssignedBotID();
                    ASSERT(m_DestroyBot[i] < m_nTeamBots);
                    if (i == m_iCurrDestroy)
                    {
                        if (m_DestroyBot[m_iCurrDestroy] != -1)
                        {
                            DestroyBot = m_TeamBot[m_DestroyBot[m_iCurrDestroy]];
                            BotPos = DestroyBot->GetPosition();
                        }
                        else
                            DestroyBot = NULL;
                    }
                }
            }
            *RepairTask = m_LastTask[m_nDestroyTasks + 1];  
            ASSERT(RepairTask->GetAssignedBotID() < m_nTeamBots);
            return FALSE;
        }
    }

#if SCAN_ASSET_STATS
	static xtimer Timer;
	Timer.Reset();
	Timer.Start();
	xtimer X[12];
	X[0].Start();
#endif
    m_iLastUpdated = m_iCurrDestroy;

    xbool DestroyTaskChanged = FALSE;
    xbool RepairTaskChanged = FALSE;
    xbool DestroySet = (DestroyTask // Have a destroy task?
                        && DestroyTask->GetTargetObject() != NULL  // Already have a target?
                        && ((DestroyTask->GetTargetObject()->GetAttrBits() & object::ATTR_ASSET)
                            &&((asset*)(DestroyTask->GetTargetObject()))->GetEnabled() ) // Still there?
                        );
    f32 DestroySetDistSq = -1;
    f32 DistToAsset = -1;
    if (DestroyBot && DestroySet)
    {
        DestroySetDistSq = (BotPos - 
            DestroyTask->GetTargetObject()->GetPosition()).LengthSquared();
        if (DestroyTask->GetTargetObject()->GetType() != object::TYPE_GENERATOR)
        {
            if (DestroySetDistSq > 100000)
            {
                DestroyTask->SetTargetObject(NULL);
                DestroySet = FALSE;
                DistToAsset = -1;
            }
        }
    }
#if SCAN_ASSET_STATS
	X[0].Stop();
	X[1].Start();
#endif
    xbool FoundRepair = FALSE;

/*    if (UnderAttack)
    {
        RepairTask->SetPriority(0);
        FoundRepair = TRUE;
    }
*/
    asset* TopPriority = NULL, *BestTarget = NULL;
    s32 LastTurretCount = m_nDeployedTurrets;
    s32 LastInvenCount = m_nDeployedInvens;
    s32 LastSensorCount = m_nDeployedSensors;
    m_nDeployedTurrets = 0;
    m_nDeployedSensors = 0;
    m_nDeployedInvens = 0;

    xbool GensOn = FALSE;

	xarray<bottarget> Targets;
	xarray<s32> TargetOrder;

    // Examine Generators for destroy and repair
    generator* Gen;
    ObjMgr.StartTypeLoop(object::TYPE_GENERATOR);
    while ((Gen = (generator*)ObjMgr.GetNextInType()) != NULL)
    {
        if (Gen->GetTeamBits() == 0)
        {
            break;
        }
        if (!(Gen->GetTeamBits() & m_TeamBits) )
        {
			if (Gen->GetEnabled())
				GensOn = TRUE;
            // Enemy Generator
            if( DestroyTask
                && !DestroySet 
                && GensOn )
            {
                // Primary Objective- take out generators
                BestTarget = (asset*)Gen;

                // Set this as target if close enough.
                if (DestroyBot)
				{
					SortInTargetList(Gen, BotPos, Targets, TargetOrder, DESTROY_CULL_DIST_SQ);
				}
/*                 
                 && DestroyBot->CanHitTarget(*Gen))
                {
                    DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                    DestroyTask->SetTargetObject(Gen);
                    DestroySet = TRUE;
                }
				*/
            }
        }
        else if (!FoundRepair)
        {
            // Check for necessary repairs
            if (Gen->GetEnabled() == FALSE)
            {
                m_bRepairing = TRUE;
                FoundRepair = TRUE;
                TopPriority = (asset*)Gen;
            }
            else if( m_bRepairing && Gen->GetHealth() < 1.0f )
            {
                TopPriority = (asset*)Gen;
            }
        }
    }
    ObjMgr.EndTypeLoop();
	
	// Run through list of targets and choose one.
//	ASSERT(Targets.GetCount() == TargetOrder.GetCount());
//	for (i = 0; i < Targets.GetCount(); i++)
//	{
//		object* CurrTarget = Targets[TargetOrder[i]].pObj;
//		if (DestroyBot->CanHitTarget(*CurrTarget))
//		{
//            DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
//            DestroyTask->SetTargetObject(CurrTarget);
//            DestroySet = TRUE;
//			break; // Found closest, so quit.
//		}
//	}
//	Targets.Clear();
//	TargetOrder.Clear();

#if SCAN_ASSET_STATS
	X[1].Stop();
	X[2].Start();
#endif
    // Examine Turrets for destroy and repair
    turret* Turret;
    ObjMgr.StartTypeLoop(object::TYPE_TURRET_FIXED);
    while ((Turret = (turret*)ObjMgr.GetNextInType()) != NULL)
    {
        if (Turret->GetTeamBits() == 0)
        {
            break;
        }
        if (!(Turret->GetTeamBits() & m_TeamBits) && !Turret->IsMPB())
        {
            if (!DestroyTask)       // Have a destroy task?
                continue;
            
            DistToAsset = (BotPos - Turret->GetPosition()).LengthSquared();

            // Enemy turret
            if (  DistToAsset < DESTROY_CULL_DIST_SQ
                  && (Turret->GetEnabled()  
                      || (!GensOn && Turret->GetHealth() ) ) )// Turret works?
            {
                if (DestroySet)            // Already set a destroy target?
                {
                    // Check if this is closer
                    if(DestroyBot)
					{
						SortInTargetList(Turret, BotPos, Targets, TargetOrder, DESTROY_CULL_DIST_SQ);
					}
					/*
                        && (DistToAsset < DestroySetDistSq)
                        && DestroyBot->CanHitTarget(*Turret))
                    {
                        DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                        DestroyTask->SetTargetObject(Turret);
                        DestroySet = TRUE;
                        DestroySetDistSq = DistToAsset;                    
                    }*/
                }
                else
                {
                    // Secondary Objective- take out turrets
					if (DestroyBot)
						SortInTargetList(Turret, BotPos, Targets, TargetOrder, DESTROY_FIXED_TURRET_CULL_DIST_SQ);

                    if (!BestTarget)
                        BestTarget = (asset*)Turret;

					/*
						if( DestroyBot          // Have a destroy bot?
												// Within radius of target?
							&& DistToAsset < DESTROY_FIXED_TURRET_CULL_DIST_SQ)
						{
							DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
							DestroyTask->SetTargetObject(Turret);
							DestroySet = TRUE;
						}
					*/
                }
            }
        }
        else 
        {
            if ( !FoundRepair  )
            {
                if ( Turret->GetEnabled() == FALSE )
                {
                    FoundRepair = TRUE;
                    m_bRepairing = TRUE;
                    TopPriority = (asset*)Turret;
                }
                else if (m_bRepairing && Turret->GetHealth() < 1.0f)
                {
                    TopPriority = (asset*)Turret;
                }
            }
        }
    }
    ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[2].Stop();
	X[3].Start();
#endif
    ObjMgr.StartTypeLoop(object::TYPE_TURRET_SENTRY);
    while ((Turret = (turret*)ObjMgr.GetNextInType()) != NULL)
    {
        if (Turret->GetTeamBits() == 0)
        {
            break;
        }
        if (!(Turret->GetTeamBits() & m_TeamBits) )
        {
            // Enemy turret
            if (DestroyTask             // Have destroy task
                && !DestroySet          // Haven't set destroy task yet
                && DestroyBot           // Have a destroy bot
                  && (Turret->GetEnabled()  
                      || (!GensOn && Turret->GetHealth() ) ) // Turret works?
                                        // Close enough to destroy?
                && (BotPos - Turret->GetPosition()).LengthSquared() < DESTROY_CULL_DIST_SQ)
            {
                // Check if this is closer
                if(DestroyBot)
				{
					SortInTargetList(Turret, BotPos, Targets, TargetOrder, DESTROY_CULL_DIST_SQ);
				}
				/*
                if(DestroyBot->CanHitTarget(*Turret))
                {
                    BestTarget = (asset*) Turret;
                    DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                    DestroyTask->SetTargetObject(Turret);
                    DestroySet = TRUE;
                }*/
            }
        }
        else 
        {
            if ( !FoundRepair && !TopPriority  )
            {
                if ( Turret->GetEnabled() == FALSE )
                {
                    m_bRepairing = TRUE;
                    TopPriority = (asset*)Turret;
                }
                else if (m_bRepairing && Turret->GetHealth() < 1.0f)
                {
                    TopPriority = (asset*)Turret;
                    FoundRepair = TRUE;
                }
            }
        }
    }
    ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[3].Stop();
	X[4].Start();
#endif
    ObjMgr.StartTypeLoop(object::TYPE_TURRET_CLAMP);
    while ((Turret = (turret*)ObjMgr.GetNextInType()) != NULL)
    {
        if ((Turret->GetTeamBits() & m_TeamBits) )
        {
            // Store this turret in our deployed list.
            if (m_nDeployedTurrets >= MAX_TURRETS_ALLOWED)
            {
#if defined (acheng)
                x_printfxy(30, 20, "Exceeded maximum allowed turrets.");
#endif
                m_nDeployedTurrets = MAX_TURRETS_ALLOWED - 1;
            }
            m_DeployedTurret[m_nDeployedTurrets] = Turret->GetPosition();
            m_nDeployedTurrets++;

            if ( !FoundRepair && !TopPriority )
            {
#if 0 // Disable repair of deployed turrets, unless already repairing.
                if ( Turret->GetEnabled() == FALSE )
                {
                    FoundRepair = TRUE;
                    m_bRepairing = TRUE;
                    TopPriority = (asset*)Turret;
                }
                else 
#endif
                if (m_bRepairing && Turret->GetHealth() < 1.0f)
                {
                    TopPriority = (asset*)Turret;
                }
            }
        }
        // Enemy turret
        else 
        {
            if (DestroyBot)
                DistToAsset = (BotPos - Turret->GetPosition()).LengthSquared();
            if (   DestroyTask             // Have a destroy task?
                  && Turret->GetEnabled()   // Turret works?
                  && DestroyBot             // Have a destroy bot?
                                            // Within radius of target?
                  && DistToAsset < DESTROY_CULL_DIST_SQ)
            {
				SortInTargetList(Turret, BotPos, Targets, TargetOrder, DESTROY_CULL_DIST_SQ);
            }
        }
    }
    ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[4].Stop();
	X[5].Start();
#endif
	// Run through list of targets and choose one.
	ASSERT(Targets.GetCount() == TargetOrder.GetCount());
	for (i = 0; i < Targets.GetCount(); i++)
	{
		object* CurrTarget = Targets[TargetOrder[i]].pObj;
		if (DestroyBot->CanHitTarget(*CurrTarget))
		{
            DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
            DestroyTask->SetTargetObject(CurrTarget);
            DestroySetDistSq = Targets[TargetOrder[i]].DistSq;
            DestroySet = TRUE;
			break; // Found closest, so quit.
		}
	}

    station* Station;
    // Don't repair inven stations if under attack
    if (!UnderAttack)
    {
        ObjMgr.StartTypeLoop(object::TYPE_STATION_FIXED);
        while ((Station = (station*)ObjMgr.GetNextInType()) != NULL)
        {
            if (Station->GetTeamBits() == 0)
            {
                break;
            }
            if ((Station->GetTeamBits() & m_TeamBits) )
            {
                if ( !FoundRepair && !TopPriority  )
                {
                    if ( Station->GetEnabled() == FALSE )
                    {
                        m_bRepairing = TRUE;
                        TopPriority = (asset*)Station;
                    }
                    else if (m_bRepairing && Station->GetHealth() < 1.0f)
                    {
                        TopPriority = (asset*)Station;
                        FoundRepair = TRUE;
                    }
                }
            }
            else
            {
                // Enemy station
                if (DestroyTask && !DestroySet && Station->GetEnabled() && !BestTarget)
                {
                    BestTarget = (asset*)Station;
                }
            }
        }
        ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[5].Stop();
	X[6].Start();
#endif
        ObjMgr.StartTypeLoop(object::TYPE_STATION_VEHICLE);
        while ((Station = (station*)ObjMgr.GetNextInType()) != NULL)
        {
            if (Station->GetTeamBits() == 0)
            {
                break;
            }
            if ((Station->GetTeamBits() & m_TeamBits) )
            {
                if ( !FoundRepair && !TopPriority  )
                {
                    if ( Station->GetEnabled() == FALSE )
                    {
                        m_bRepairing = TRUE;
                        TopPriority = (asset*)Station;
                    }
                    else if (m_bRepairing && Station->GetHealth() < 1.0f)
                    {
                        TopPriority = (asset*)Station;
                        FoundRepair = TRUE;
                    }
                }
            }
            else
            {
                // Enemy station
                if (DestroyTask && !DestroySet && Station->GetEnabled() && !BestTarget)
                {
                    BestTarget = (asset*)Station;
                }
            }
        }
        ObjMgr.EndTypeLoop();
    }
#if SCAN_ASSET_STATS
	X[6].Stop();
	X[7].Start();
#endif
    ObjMgr.StartTypeLoop(object::TYPE_STATION_DEPLOYED);
    while ((Station = (station*)ObjMgr.GetNextInType()) != NULL)
    {
        if ((Station->GetTeamBits() & m_TeamBits) )
        {
            if (m_nDeployedInvens >= MAX_INVENS_ALLOWED)
            {
#if defined (acheng)
                x_printfxy(5,21,"Exceeded maximum allowed inven stations.");
#endif
                m_nDeployedInvens = MAX_INVENS_ALLOWED - 1;
            }
            m_DeployedInven[m_nDeployedInvens] = Station->GetPosition();
            m_nDeployedInvens++;
        }
        else
        {
            // Enemy station
            if (DestroyTask            // Have destroy task
                && !DestroySet          // Haven't set it already
                && DestroyBot           // Have a destroy bot
                && Station->GetEnabled() // The sensor needs destroying
                                        // Close enough to destroy?
                && (BotPos - 
                Station->GetPosition()).LengthSquared() 
                        < DESTROY_SHORT_CULL_DIST_SQ)
            {
                DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                DestroyTask->SetTargetObject(Station);
                DestroySet = TRUE;
                DestroySetDistSq = (BotPos - 
                DestroyTask->GetTargetObject()->GetPosition()).LengthSquared();
            }
        }
    }
    ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[7].Stop();
	X[8].Start();
#endif
    sensor* Sensor;
    ObjMgr.StartTypeLoop(object::TYPE_SENSOR_REMOTE);
    while ((Sensor = (sensor*)ObjMgr.GetNextInType()) != NULL)
    {
        if (Sensor->GetTeamBits() == 0)
        {
            break;
        }
        if ((Sensor->GetTeamBits() & m_TeamBits) )
        {
            // Store this SENSOR in our deployed list.
            if (m_nDeployedSensors >= MAX_SENSORS_ALLOWED)
            {
#if defined (acheng)
                x_printfxy(30, 22, "Exceeded maximum allowed sensors.");
#endif
                m_nDeployedSensors = MAX_SENSORS_ALLOWED - 1;
            }
            m_DeployedSensor[m_nDeployedSensors] = Sensor->GetPosition();
            m_nDeployedSensors++;

            if ( !FoundRepair && !TopPriority )
            {
#if 0 // Disable repair of deployed sensors, unless already repairing.
                if ( Sensor->GetEnabled() == FALSE )
                {
                    FoundRepair = TRUE;
                    m_bRepairing = TRUE;
                    TopPriority = (asset*)Sensor;
                }
                else 
#endif
                if (m_bRepairing && Sensor->GetHealth() < 1.0f)
                {
                    TopPriority = (asset*)Sensor;
                }
            }
        }
        else
        {
            // Enemy sensor
            if (DestroyTask            // Have destroy task
                && !DestroySet          // Haven't set it already
                && DestroyBot           // Have a destroy bot
                && Sensor->GetEnabled() // The sensor needs destroying
                                        // Close enough to destroy?
                && (BotPos - 
                Sensor->GetPosition()).LengthSquared() 
                                < DESTROY_SHORT_CULL_DIST_SQ)
            {
                DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                DestroyTask->SetTargetObject(Sensor);
                DestroySet = TRUE;
                DestroySetDistSq = (BotPos - 
                DestroyTask->GetTargetObject()->GetPosition()).LengthSquared();
            }

        }
    }
    ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[8].Stop();
	X[9].Start();
#endif

    // Check for MPB's
    vehicle* Vehicle;
    ObjMgr.StartTypeLoop(object::TYPE_VEHICLE_JERICHO2);
    while ((Vehicle = (vehicle*)ObjMgr.GetNextInType()) != NULL)
    {
        if (!(Vehicle->GetTeamBits() & m_TeamBits) )
        {
            DistToAsset = (BotPos - Vehicle->GetPosition()).LengthSquared();
            // Enemy vehicle
            if (DestroyTask                 // Have destroy task
                && DestroyBot               // Have a destroy bot
                && Vehicle->GetHealth()     // The vehicle needs destroying
                                            // Close enough to destroy?
                && ( DistToAsset < DESTROY_CULL_DIST_SQ))
            {
                if (!DestroySet || DistToAsset < DestroySetDistSq)
                {
                    DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                    DestroyTask->SetTargetObject(Vehicle);
                    DestroySet = TRUE;
                }                
            }
        }
    }
    ObjMgr.EndTypeLoop();
#if SCAN_ASSET_STATS
	X[9].Stop();
	X[10].Start();
#endif
    Vehicle = (vehicle*)ObjMgr.GetObjectFromID(m_EnemyVehicle);
    if (Vehicle)
    {
        DistToAsset = (BotPos - Vehicle->GetPosition()).LengthSquared();
        if (!DestroySet || DistToAsset < DestroySetDistSq)
        {
            DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
            DestroyTask->SetTargetObject(Vehicle);
            DestroySet = TRUE;
        }
    }

    if (TopPriority != NULL)
    {
        m_bRepairing = TRUE;
        RepairTask->SetPriority(100);
        RepairTask->SetMaxBots(1);
        RepairTask->SetState(bot_task::TASK_STATE_REPAIR);
        RepairTask->SetTargetObject(TopPriority);
    }
    else
    {
        ASSERT(FoundRepair == FALSE);
        RepairTask->SetPriority(0);
        RepairTask->SetTargetObject(NULL);
        m_bRepairing = FALSE;
        m_RepairBot = -1;
    }
#if SCAN_ASSET_STATS
	X[10].Stop();
	X[11].Start();
#endif
    if (DestroyTask)
    {
        if (!DestroySet)
        {
            if (BestTarget)
            {
                DestroyTask->SetState(bot_task::TASK_STATE_DESTROY);
                DestroyTask->SetTargetObject(BestTarget);
                DestroyTask->SetPriority(100);
                if((BestTarget->GetTeamBits() & m_TeamBits))
                {
                    x_printfxy(10, 18, "Warning: Destroy Task Target is friendly asset/n");
                }
            }
            else
            {
                DestroyTask->SetPriority(0);
            }
        }
        else
        {
            DestroyTask->SetPriority(100);
            if((DestroyTask->GetTargetObject()->GetTeamBits() & m_TeamBits))
            {
                x_printfxy(10, 18, "Warning: Destroy Task Target is friendly asset/n");
            }
        }
    }
    if (DestroyTask && RepairTask)
    {
        if (m_LastTask[m_iCurrDestroy].GetPriority() !=
                                    DestroyTask->GetPriority() 
            || m_LastTask[m_iCurrDestroy].GetTargetObject() !=
                                DestroyTask->GetTargetObject() )
            DestroyTaskChanged = TRUE;
        if (m_LastTask[m_nDestroyTasks + 1].GetPriority() !=
                                    RepairTask->GetPriority() 
            || m_LastTask[m_nDestroyTasks + 1].GetTargetObject() !=
                                RepairTask->GetTargetObject() )
            RepairTaskChanged = TRUE;
        m_LastTask[m_iCurrDestroy] = *DestroyTask;
        m_LastTask[m_nDestroyTasks + 1] = *RepairTask;
    }
    xbool DeployTaskChanged = FALSE;
    // Check current deploy status(es)
    for (s32 k = 0; k < m_nDeployAssets; k++)
    {
        if ( m_CurrentDeployAsset[k])
        {
            s32 DeployBot = m_DeployBot;
            if (m_DeployBot == -1)
                DeployBot = 0;

            switch (m_DeployType[k])
            {
                // Can't deploy there?  then the deploy task is done/invalid.
                case DEPLOY_TYPE_TURRET:
                {
                    if (GameMgr.AttemptDeployTurret(m_TeamBotObjectID[DeployBot], 
                        ((turret_spot*)m_CurrentDeployAsset[k])->GetAssetPos(),
                        ((turret_spot*)m_CurrentDeployAsset[k])->GetNormal(),
                        object::TYPE_TERRAIN, TRUE) == FALSE)
                    {
                        m_CurrentDeployAsset[k] = NULL;
                        DeployTaskChanged = TRUE;
                    }
                    // Check if he's been at the deploy point for n seconds.
                }
                break;
                case DEPLOY_TYPE_INVEN:
                {
                    if (GameMgr.AttemptDeployStation(m_TeamBotObjectID[DeployBot], 
                        ((inven_spot*)m_CurrentDeployAsset[k])->GetAssetPos(),
                        vector3(0,1,0), object::TYPE_TERRAIN, TRUE) == FALSE)
                    {
                        m_CurrentDeployAsset[k] = NULL;
                        DeployTaskChanged = TRUE;
                    }
                }
                break;
                case DEPLOY_TYPE_SENSOR:
                {
                    if (GameMgr.AttemptDeploySensor(m_TeamBotObjectID[DeployBot], 
                        ((sensor_spot*)m_CurrentDeployAsset[k])->GetAssetPos(),
                        ((sensor_spot*)m_CurrentDeployAsset[k])->GetNormal(),
                        object::TYPE_TERRAIN, TRUE) == FALSE)
                    {
                        m_CurrentDeployAsset[k] = NULL;
                        DeployTaskChanged = TRUE;
                    }
                }
                break;
                default:
                    ASSERT(0);
            }
        }
    }
#if SCAN_ASSET_STATS
	X[11].Stop();


	Timer.Stop();

	static f32 AvgTime = 0.0f;
	static f32 TotalTimes = 0.0f;
	static s32 TimeCtr = 0;
	static f32 MaxTime = 0.0f;
	xbool DisplayStats = FALSE;
	f32 CurrTime = Timer.ReadMs();
	if (CurrTime > MaxTime)
	{
		DisplayStats = TRUE;
		MaxTime = CurrTime;
	}
	TimeCtr++;
	TotalTimes += CurrTime;
	AvgTime = CurrTime/TimeCtr;

	if (DisplayStats)
	{
		x_DebugMsg("Curr = %f, Avg = %f, Max = %f\n", CurrTime, AvgTime, MaxTime);
		x_DebugMsg("\
 0:%.3f\n\
 1:%.3f\n\
 2:%.3f\n\
 3:%.3f\n\
 4:%.3f\n\
 5:%.3f\n\
 6:%.3f\n\
 7:%.3f\n\
 8:%.3f\n\
 9:%.3f\n\
10:%.3f\n\
11:%.3f\n",
X[0].ReadMs(),
X[1].ReadMs(),
X[2].ReadMs(),
X[3].ReadMs(),
X[4].ReadMs(),
X[5].ReadMs(),
X[6].ReadMs(),
X[7].ReadMs(),
X[8].ReadMs(),
X[9].ReadMs(),
X[10].ReadMs(),
X[11].ReadMs()
);
	}
#endif

    if (m_nDeployedTurrets < LastTurretCount)
        m_TurretFull = FALSE;
    if (m_nDeployedSensors < LastSensorCount)
        m_SensorFull = FALSE;
    if (m_nDeployedInvens < LastInvenCount)
        m_InvenFull = FALSE;

    if(     !DestroyTaskChanged
        &&  !RepairTaskChanged
        &&  !DeployTaskChanged)
        return FALSE;
    else
        return TRUE;
}     

//==============================================================================

void master_ai::RemoveDeadPlayers( void )
{
return;
#if 0
    // Examine all object id's to see who's dead.
    s32 i;//,j;
    bot_object* CurrBot;
    for (i = 0; i < m_nTeamBots; i++)
    {
        CurrBot = (bot_object*)ObjMgr.GetObjectFromID(m_TeamBotObjectID[i]);
        if (CurrBot->m_PlayerLastState == player_object::PLAYER_STATE_SPAWN)
        {
            CurrBot->m_CurTask = NULL;
            CurrBot->m_PrevTask = NULL;
            this->m_PlayersChanged = TRUE;
        }
    }
#endif
}

//==============================================================================

void master_ai::LoadDeployables ( X_FILE *Fp )
{
    if (Fp)
    {
        asset_spot::team Team;
        x_fread(&Team, sizeof(asset_spot::team), 1, Fp);
#if 0
        if (Team == asset_spot::TEAM_STORM)
        {
            ASSERT(m_TeamBits &= 0x01);
        }
        else
        {
            ASSERT(m_TeamBits &= 0x02);
        }
#endif
        vector3 StandPos;
        vector3 AssetPos;
        vector3 Normal;
        asset_spot::priority_value Priority;
        radian Yaw;
        u32 Flags;
        radian3 Rot;

        s32 i;

        flipflop* Switch[10];
        s32 nSwitches = 0;
        // Store all switches.
        ObjMgr.StartTypeLoop(object::TYPE_FLIPFLOP);
        while ((Switch[nSwitches] = (flipflop*)ObjMgr.GetNextInType()) != NULL)
        {
            nSwitches++;
            ASSERT(nSwitches < 10);
        }
        ObjMgr.EndTypeLoop();

        // This should look exactly the same as asset_editor::Load()
        x_fread(&m_DeploySpots.m_nInvens, sizeof(s32),                  1, Fp);
        x_fread(&m_DeploySpots.m_nTurrets,sizeof(s32),                  1, Fp);
        x_fread(&m_DeploySpots.m_nSensors,sizeof(s32),                  1, Fp);
        x_fread(&m_DeploySpots.m_nMines,  sizeof(s32),                  1, Fp);
        for (i = 0; i < m_DeploySpots.m_nInvens; i++)
        {
            x_fread( &StandPos,   sizeof(vector3),                      1, Fp);
            x_fread( &AssetPos,   sizeof(vector3),                      1, Fp);
            x_fread( &Yaw,        sizeof(radian),                       1, Fp);
            x_fread( &Priority,   sizeof(asset_spot::priority_value),   1, Fp);
            x_fread( &Flags,      sizeof(u32),                          1, Fp);
            ASSERT(Priority == 1 || Priority == 3 || Priority == 9);
            if (Flags & asset_spot::TEAM_NEUTRAL)
            {
               m_DeploySpots.m_pInvenList[i].Set(  StandPos,
                                                AssetPos,
                                                Yaw,
                                                Priority,
                                                Flags,
                                                GetSwitch(Switch, nSwitches, AssetPos));
            }
            else
               m_DeploySpots.m_pInvenList[i].Set(  StandPos,
                                                AssetPos,
                                                Yaw,
                                                Priority,
                                                Flags);

        }
        for (i = 0; i < m_DeploySpots.m_nTurrets; i++)
        {
            x_fread( &StandPos,   sizeof(vector3),                      1, Fp);
            x_fread( &AssetPos,   sizeof(vector3),                      1, Fp);
            x_fread( &Normal,     sizeof(vector3),                      1, Fp);
            x_fread( &Priority,   sizeof(asset_spot::priority_value),   1, Fp);
            x_fread( &Flags,      sizeof(u32),                          1, Fp);
            ASSERT(Priority == 1 || Priority == 3 || Priority == 9);
            if (Flags & asset_spot::TEAM_NEUTRAL)
            {
               m_DeploySpots.m_pTurretList[i].Set(  StandPos,
                                                AssetPos,
                                                Normal,
                                                Priority,
                                                Flags,
                                                GetSwitch(Switch, nSwitches, AssetPos));
            }
            else
                m_DeploySpots.m_pTurretList[i].Set(StandPos, AssetPos, Normal, Priority, Flags);
        }
        for (i = 0; i < m_DeploySpots.m_nSensors; i++)
        {
            x_fread( &StandPos,   sizeof(vector3),                      1, Fp);
            x_fread( &AssetPos,   sizeof(vector3),                      1, Fp);
            x_fread( &Normal,     sizeof(vector3),                      1, Fp);
            x_fread( &Priority,   sizeof(asset_spot::priority_value),   1, Fp);
            x_fread( &Flags,      sizeof(u32),                          1, Fp);
            ASSERT(Priority == 1 || Priority == 3 || Priority == 9);
            if (Flags & asset_spot::TEAM_NEUTRAL)
            {
               m_DeploySpots.m_pSensorList[i].Set(  StandPos,
                                                AssetPos,
                                                Normal,
                                                Priority,
                                                Flags,
                                                GetSwitch(Switch, nSwitches, AssetPos));
            }
            else
               m_DeploySpots.m_pSensorList[i].Set(  StandPos,
                                                AssetPos,
                                                Normal,
                                                Priority,
                                                Flags);
        }
        for (i = 0; i < m_DeploySpots.m_nMines; i++)
        {
            x_fread( &StandPos,   sizeof(vector3),                      1, Fp);
            x_fread( &Rot,        sizeof(radian3),                      1, Fp);
            x_fread( &Priority,   sizeof(asset_spot::priority_value),   1, Fp);
            x_fread( &Flags,      sizeof(u32),                          1, Fp);
            ASSERT(Priority == 1 || Priority == 3 || Priority == 9);
            if (Flags & asset_spot::TEAM_NEUTRAL)
            {
               m_DeploySpots.m_pMineList[i].Set(  StandPos,
                                                Rot,
                                                Priority,
                                                Flags,
                                                GetSwitch(Switch, nSwitches, StandPos));
            }
            else
               m_DeploySpots.m_pMineList[i].Set(  StandPos,
                                                Rot,
                                                Priority,
                                                Flags);
        }
        x_fread(&m_nSnipePoints, sizeof(s32), 1, Fp);
        for (i = 0; i < m_nSnipePoints; i++)
        {
            x_fread(&m_pSnipePoint[i], sizeof(vector3), 1, Fp);
        }

    }
    else
    {
        m_DeploySpots.m_nInvens = 0;
        m_DeploySpots.m_nMines = 0;
        m_DeploySpots.m_nTurrets = 0;
        m_DeploySpots.m_nSensors = 0;
        m_nSnipePoints = 0;
    }
    ASSERT(m_DeploySpots.m_nInvens <= 5);
    ASSERT(m_DeploySpots.m_nMines  <= 50);
    ASSERT(m_DeploySpots.m_nTurrets <= 50);
    ASSERT(m_DeploySpots.m_nSensors <= 50);

}

//==============================================================================

void master_ai::PickSnipePoint ( void )
{
    ASSERT( m_nSnipePoints > 0);
    s32 Rand;
    x_srand((s32)x_GetTime());
    if (m_iCurrentSnipePos == -1)
        Rand = x_rand()%m_nSnipePoints;
    else
        Rand = x_rand()%(m_nSnipePoints - 1);

    if (Rand == m_iCurrentSnipePos)
        Rand++;

    m_iCurrentSnipePos = Rand;
}

//==============================================================================

object::id master_ai::GetSwitch ( flipflop** Switch, s32 nSwitches,
                                          const vector3& Pos)
{
    ASSERT(nSwitches > 0);

    if (nSwitches == 1)
        return Switch[0]->GetObjectID();

    f32 MinDist = F32_MAX;
    s32 Index   = -1;
    vector3 V;
    f32 d;

    for( s32 i=0; i<nSwitches; i++ )
    {
        V = Switch[i]->GetPosition() - Pos;
        d = V.Dot( V );

        if( d < MinDist )
        {
            MinDist = d;
            Index = i;
        }
    }

    ASSERT(Index != -1);
    return Switch[Index]->GetObjectID();
}

//==============================================================================
 
xbool master_ai::CheckTurretPlacement( const vector3 &Location)
{
    s32 i = 0;
    xbool FoundTurret = FALSE;
    for (i = 0; i < m_DeploySpots.m_nTurrets; i++)
    {
        if (m_DeploySpots.m_pTurretList[i].GetAssetPos() == Location)
        {
            FoundTurret = TRUE;
            break;
        }
    }
    return FoundTurret;
}

//==============================================================================

const asset_spot* master_ai::ChooseTurretSpot (object::id SwitchID, s32 Spot )
{
    // switch-based turret?
    if (SwitchID != ObjMgr.NullID )
    {
        if ((ObjMgr.GetObjectFromID(SwitchID)->GetTeamBits() & m_TeamBits) == 0)
        {
            return NULL;
        }
    }
    s32 Current, Maximum;

    // First check max deploy values
    GameMgr.GetDeployStats( object::TYPE_TURRET_CLAMP, m_TeamBits, Current, Maximum );
    m_nDeployedTurrets = Current;

    if (Current >= Maximum)
    {
        m_TurretFull = TRUE;
        return NULL;
    }

    asset_spot* AssetChoice[asset_spot::MAX_TURRETS];
    s32 nChoices = 0;
    vector3 Candidate;
    s32 nHigh = 0, nMid = 0, nLow = 0;
    s32 i;

    // Go through turret list and add eligible turrets to list.
    for (i = 0; i < m_DeploySpots.m_nTurrets; i++)
    {
        Candidate = m_DeploySpots.m_pTurretList[i].GetAssetPos();

        // switch-based turret?
        if (SwitchID != ObjMgr.NullID )
        {
            // This is eligible if the closest switch matches.
            if( m_DeploySpots.m_pTurretList[i].GetFlags() 
                & asset_spot::TEAM_NEUTRAL ) 
            {
                if (m_DeploySpots.m_pTurretList[i].GetSwitch() != SwitchID)
                {
                    continue;
                }
#if defined (acheng)
                // Check if opponent has control of this switch.
                if (
                    ((flipflop*)ObjMgr.GetObjectFromID(
                    m_DeploySpots.m_pTurretList[i].GetSwitch()))->GetTeamBits() 
                        & ~m_TeamBits)
                {
                    // Error- function called on enemy switch.
                    ASSERT(0);
                    continue;
                }
#endif
            }
            else
            {
                // Not a neutral asset- skip it.
                continue;
            }
        }

        // Is it legal to place this turret?
        if (GameMgr.AttemptDeployTurret(this->m_TeamBotObjectID[0], Candidate,
            m_DeploySpots.m_pTurretList[i].GetNormal(), object::TYPE_TERRAIN, 
            TRUE) == FALSE )
            continue;
        else
        {
            // Candidate is a valid choice.
            AssetChoice[nChoices] = &m_DeploySpots.m_pTurretList[i];
            switch (AssetChoice[nChoices]->GetPriority())
            {
            case (asset_spot::PRIORITY_HIGH):
                ASSERT(!nMid && !nLow);
                nHigh++;
                break;
            case (asset_spot::PRIORITY_MEDIUM):
                ASSERT(!nLow);
                nMid++;
                break;
            case (asset_spot::PRIORITY_LOW):
                nLow++;
                break;
            default:
                ASSERT(FALSE);
                break;
            }
            nChoices++;
        }
    }

    if (!nChoices)
    {
        m_TurretFull = TRUE;
        return NULL;
    }


    // Now that we have a list of deployables to choose from, choose one.

    s32 Range = asset_spot::PRIORITY_HIGH * nHigh
                    + asset_spot::PRIORITY_MEDIUM * nMid
                    + asset_spot::PRIORITY_LOW * nLow;

    s32 Rand ;
    s32 Choice;

    if (Range > 0)
    {
        x_srand((s32)x_GetTime());
        Rand = x_rand()%Range;
    }
    else Rand = 0;

    m_DeployType[Spot] = DEPLOY_TYPE_TURRET;
    // Is it in the high range?
    if (Rand < asset_spot::PRIORITY_HIGH * nHigh)
    {
        Choice = Rand/asset_spot::PRIORITY_HIGH;
    }
    else
    {
        // Let's cut out the high choices.
        Choice = nHigh;  // Base choice.  The choice can only be incremented to.
        Rand -= asset_spot::PRIORITY_HIGH * nHigh;

        // Now is it in the mid range?
        if (Rand < asset_spot::PRIORITY_MEDIUM * nMid)
        {
            Choice += Rand/asset_spot::PRIORITY_MEDIUM;
        }

        else
        {
            // Cut out the mid choices.
            Choice += nMid;
            Rand -= asset_spot::PRIORITY_MEDIUM * nMid;
            
            // it's in the low range.
            ASSERT(Rand < asset_spot::PRIORITY_LOW * nLow);
            Choice += Rand/asset_spot::PRIORITY_LOW;
        }
    }
    ASSERT(Choice < nChoices);
    m_CurrentDeployAsset[Spot] = AssetChoice[Choice];
    return AssetChoice[Choice];
}

//==============================================================================

const asset_spot* master_ai::ChooseInvenSpot (object::id SwitchID, s32 Spot )
{
    // switch-based turret?
    if (SwitchID != ObjMgr.NullID )
    {
        if ((ObjMgr.GetObjectFromID(SwitchID)->GetTeamBits() & m_TeamBits) == 0)
        {
            return NULL;
        }
    }
    s32 Current, Maximum;

    // First check max deploy values
    GameMgr.GetDeployStats( object::TYPE_STATION_DEPLOYED, m_TeamBits, Current, Maximum );
    m_nDeployedInvens = Current;

    if (Current >= Maximum)
    {
        m_InvenFull = TRUE;
        return NULL;
    }

    asset_spot* AssetChoice[asset_spot::MAX_INVENS];
    s32 nChoices = 0;
    vector3 Candidate;
    s32 i;

    // Go through inven list and add eligible invens to list.
    for (i = 0; i < m_DeploySpots.m_nInvens; i++)
    {
        Candidate = m_DeploySpots.m_pInvenList[i].GetAssetPos();

        // switch-based INVEN?
        if (SwitchID != ObjMgr.NullID)
        {
            if (m_DeploySpots.m_pInvenList[i].GetFlags() 
                & asset_spot::TEAM_NEUTRAL) 
            {
                if (m_DeploySpots.m_pInvenList[i].GetSwitch() != SwitchID)
                {
                    continue;
                }
#if defined (acheng)
                // Check if opponent has control of this switch.
                if (((flipflop*)ObjMgr.GetObjectFromID(
                    m_DeploySpots.m_pInvenList[i].GetSwitch()))->GetTeamBits() 
                        & ~m_TeamBits)
                {
                    // Error- function called on enemy switch.
                    ASSERT(0);
                    continue;
                }
#endif
            }
            else
                // Not a neutral asset.
                continue;
        }

        // Acceptable deploy spot?
        if (GameMgr.AttemptDeployStation(m_TeamBotObjectID[0], Candidate,
                               vector3(0,1,0), object::TYPE_TERRAIN, TRUE) 
                               == FALSE)
            continue;
        else
        {
            //Valid Choice
            AssetChoice[nChoices] = &m_DeploySpots.m_pInvenList[i];
            nChoices++;
        }
    }

    if (!nChoices)
    {
        m_InvenFull = TRUE;
        return NULL;
    }

    // Now that we have a list of deployables to choose from, choose one.

    s32 Rand ;

    x_srand((s32)x_GetTime());
    Rand = x_rand()%nChoices;

    m_DeployType[Spot] = DEPLOY_TYPE_INVEN;

    ASSERT(Rand < nChoices);
    m_CurrentDeployAsset[Spot] = AssetChoice[Rand];
    return AssetChoice[Rand];
}


//==============================================================================

const asset_spot* master_ai::ChooseSensorSpot (object::id SwitchID, s32 Spot )
{
    // switch-based turret?
    if (SwitchID != ObjMgr.NullID )
    {
        if ((ObjMgr.GetObjectFromID(SwitchID)->GetTeamBits() & m_TeamBits) == 0)
        {
            return NULL;
        }
    }
    s32 Current, Maximum;

    // First check max deploy values
    GameMgr.GetDeployStats( object::TYPE_SENSOR_REMOTE, m_TeamBits, Current, Maximum );
    m_nDeployedSensors = Current;

    if (Current >= Maximum)
    {
        m_SensorFull = TRUE;
        return NULL;
    }

    asset_spot* AssetChoice[asset_spot::MAX_SENSORS];
    s32 nChoices = 0;
    vector3 Candidate;
    s32 i;

    // Go through inven list and add eligible invens to list.
    for (i = 0; i < m_DeploySpots.m_nSensors; i++)
    {
        Candidate = m_DeploySpots.m_pSensorList[i].GetAssetPos();

        // switch-based sensor?
        if (SwitchID != ObjMgr.NullID)
        {
            if ( m_DeploySpots.m_pSensorList[i].GetFlags() 
                & asset_spot::TEAM_NEUTRAL) 
            {
                if (m_DeploySpots.m_pSensorList[i].GetSwitch() != SwitchID)
                {
                    continue;
                }
#if defined (acheng)
                // Check if opponent has control of this switch.
                if (((flipflop*)ObjMgr.GetObjectFromID(
                    m_DeploySpots.m_pSensorList[i].GetSwitch()))->GetTeamBits() 
                        & ~m_TeamBits)
                {
                    // Error- function called on enemy switch.
                    ASSERT(0);
                    continue;
                }
#endif
            }
            else 
                continue;
        }

        // Acceptable deploy spot?
        if (GameMgr.AttemptDeploySensor(this->m_TeamBotObjectID[0], Candidate,
            m_DeploySpots.m_pSensorList[i].GetNormal(), object::TYPE_TERRAIN, 
                    TRUE) == FALSE)
            continue;
        else
        {
            if (SensorPlacementCheck(Candidate))
            {
                //Valid Choice
                AssetChoice[nChoices] = &m_DeploySpots.m_pSensorList[i];
                nChoices++;
            }
        }
    }

    if (!nChoices)
    {
        m_SensorFull = TRUE;
        return NULL;
    }

    // Now that we have a list of deployables to choose from, choose one.

    s32 Rand ;
    x_srand((s32)x_GetTime());
    Rand = x_rand()%nChoices;

    m_DeployType[Spot] = DEPLOY_TYPE_SENSOR;

    ASSERT(Rand < nChoices);
    m_CurrentDeployAsset[Spot] = AssetChoice[Rand];
    return AssetChoice[Rand];
}

//==============================================================================

const asset_spot* master_ai::ChooseAssetSpotFn( object::id SwitchID, s32 Spot )
{
    if ((m_TurretFull && m_InvenFull && m_SensorFull)
        || !(m_DeploySpots.m_nInvens || m_DeploySpots.m_nTurrets ) )
        return NULL;

    s32 TurretBias, InvenBias, SensorBias;
    s32 TurretCtr = 0, InvenCtr = 0, SensorCtr = 0;
/*
    if (SwitchID != ObjMgr.NullID)
    {
        // Get a count of all assets around the switch.
        asset* Asset;
        ObjMgr.Select(object::ATTR_ASSET, 
            ObjMgr.GetObjectFromID(SwitchID)->GetPosition(), 75);
        while ((Asset = (asset*)ObjMgr.GetNextSelected()) != NULL)
        {
            switch (Asset->GetType())
            {
                case (object::TYPE_TURRET_CLAMP):
                case (object::TYPE_TURRET_FIXED):
                case(object::TYPE_TURRET_SENTRY):
                    TurretCtr++;
                break;

                case (object::TYPE_STATION_FIXED):
                case (object::TYPE_STATION_DEPLOYED):
                    InvenCtr++;
                break;
                
                case (object::TYPE_SENSOR_REMOTE):
                case (object::TYPE_SENSOR_MEDIUM):
                case (object::TYPE_SENSOR_LARGE):
                    SensorCtr++;
                break;
            }
        }
        ObjMgr.ClearSelection();
    }
    else
*/
    {
        TurretCtr = m_nDeployedTurrets;
        InvenCtr  = m_nDeployedInvens;
        SensorCtr = m_nDeployedSensors;
    }
    TurretBias = m_TurretFull?0:MAX(30 - TurretCtr, 5);

    // If there is no inven station nearby, bias invens heavily.
    if (InvenCtr == 0)
        InvenBias = 50;
    else
        InvenBias = m_InvenFull?0:MAX(5 - InvenCtr, 1);

    if (SensorCtr)
        SensorBias = m_SensorFull?0:1;
    else 
        SensorBias = 3;


    s32 Range = TurretBias + InvenBias + SensorBias;

    if (Range == 0)
        return NULL;

    x_srand((s32)x_GetTime());
    s32 Rand = x_rand() % Range;

    if (Rand < TurretBias)
        return ChooseTurretSpot(SwitchID, Spot);
    else if (Rand < TurretBias + InvenBias )
        return ChooseInvenSpot(SwitchID, Spot);
    else
        return ChooseSensorSpot(SwitchID, Spot);
}

//==============================================================================

xbool master_ai::SensorPlacementCheck ( const vector3& Pos ) const
{
    s32 i;
    for (i = 0; i < m_nDeployedSensors; i++)
    {
        if ((m_DeployedSensor[i]-Pos).LengthSquared() < MIN_DIST_BTWN_SENSORS_SQ)
        {
            return FALSE;
        }
    }
    return TRUE;
}

//==============================================================================

void master_ai::RegisterOffendingTurret ( object::id ObjID )
{
    s32 i;
    s32 Slot = -1;

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

xbool master_ai::BotsOnTask ( bot_task *Task, u32 Assigned )
{
    s32 i;
    
    for (i = 0; i < 32; i++)
    {
        if (Assigned == 0)
        {
            return TRUE;
        }

        if (Assigned & 0x01)
        {
            // Confirm this bot is assigned.
            if (m_TeamBot[i]->m_CurTask != Task)
                return FALSE;
        }

        Assigned >>= 1;
    }
    ASSERT(0)
        ;return 0;
}

//==============================================================================

void master_ai::SetDestroyBot ( s32 BotID )
{
    (void)BotID;
}

//==============================================================================

void master_ai::AssetPlacementFailed( s32 BotID )
{
    (void)BotID;
}

//==============================================================================

void master_ai::RemoveDeployable ( master_ai::deploy_type Type, s32 Index ) 
{
    s32 i;
    switch (Type)
    {
    case (DEPLOY_TYPE_TURRET):
        ASSERT(Index < m_DeploySpots.m_nTurrets);
        for (i = Index; i < m_DeploySpots.m_nTurrets - 1; i++)
        {
            m_DeploySpots.m_pTurretList[Index] = m_DeploySpots.m_pTurretList[Index + 1];
        }
        m_DeploySpots.m_nTurrets--;
        break;

    case (DEPLOY_TYPE_INVEN):
        ASSERT(Index < m_DeploySpots.m_nInvens);
        for (i = Index; i < m_DeploySpots.m_nInvens - 1; i++)
        {
            m_DeploySpots.m_pInvenList[Index] = m_DeploySpots.m_pInvenList[Index + 1];
        }
        m_DeploySpots.m_nInvens--;
        break;

    case (DEPLOY_TYPE_SENSOR):
        ASSERT(Index < m_DeploySpots.m_nSensors);
        for (i = Index; i < m_DeploySpots.m_nSensors - 1; i++)
        {
            m_DeploySpots.m_pSensorList[Index] = m_DeploySpots.m_pSensorList[Index + 1];
        }
        m_DeploySpots.m_nSensors--;
        break;

    default:
        ASSERT(0);
        break;
    }
}
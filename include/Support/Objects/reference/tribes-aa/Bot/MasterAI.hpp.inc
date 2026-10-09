//==============================================================================
//  
//  MasterAI.hpp
//
//==============================================================================
#ifndef MASTER_AI_HPP
#define MASTER_AI_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include "BotObject.hpp"
#include "Deployable.hpp"
#include "Objects/Projectiles/FlipFlop.hpp"
//==============================================================================
//  DEFINES
//==============================================================================
#define PRIORITY_RADIUS_SQ 2500

//==============================================================================
//  TYPES
//==============================================================================
class master_ai
{
//------------------------------------------------------------------------------
//  Public Types
//------------------------------------------------------------------------------

    public:
        enum 
        {
            MAX_REPAIR_BOTS = 1,
            MAX_DESTROY_BOTS = 4,
            MAX_DEFENSE = 8,
            MAX_BOTS_PER_TEAM = 16,
            MAX_TASKS = 31,

            MAX_INVENS_ALLOWED = 5,
//******************************************************************************
//  TO DO: make these variable based on the formula
//******************************************************************************
            MAX_TURRETS_ALLOWED = 10,
            MAX_SENSORS_ALLOWED = 10,

            MAX_SNIPE_POINTS    = 50,

            MAX_DEPLOYERS = 8,

            MAX_CNH_SWITCHES = 4,
        };

        enum deploy_type
        {
            DEPLOY_TYPE_TURRET = 1,
            DEPLOY_TYPE_INVEN,
            DEPLOY_TYPE_SENSOR,
        };

        enum
        {
            EXCLUDE_ASSIGNED = 1<<31,
        };

        struct deployables
        {
            inven_spot      m_pInvenList[asset_spot::MAX_INVENS];
            s32             m_nInvens;

            turret_spot     m_pTurretList[asset_spot::MAX_TURRETS];
            s32             m_nTurrets;

            sensor_spot     m_pSensorList[asset_spot::MAX_SENSORS];
            s32             m_nSensors;

            mine_spot       m_pMineList[asset_spot::MAX_MINES];
            s32             m_nMines;
        };

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
    public:
                    master_ai           ( void );
virtual            ~master_ai           ( void );
virtual void        Initialize          ( void );

       // Set initialized & GoalList tasks
virtual void        InitializeMission   ( void )    = 0;
virtual void        UnloadMission       ( void )    = 0;

        void        LoadDeployables     ( X_FILE* Fp);

virtual void        Update              ( void );

        void        RefreshFlags        ( void );

        xbool       CheckTurretPlacement(const vector3 &Location);

virtual void        RegisterOffendingTurret ( object::id ObjID );

virtual void        AssetPlacementFailed    ( s32 BotID );
//------------------------------------------------------------------------------
//  Protected Storage
//------------------------------------------------------------------------------
    protected:
        // Which team are we working for?
        s32             m_TeamBits;

        // List of all tasks to do
        bot_task        m_GoalList[MAX_TASKS];
        s32             m_nTasks;

        bot_task*       m_SortedGoals[MAX_TASKS];

        bot_task        m_LastTask[MAX_TASKS];

        // Object ID's of all the bot objects.
        object::id      m_TeamBotObjectID[MAX_BOTS_PER_TEAM];
        bot_object*     m_TeamBot[MAX_BOTS_PER_TEAM];
        s32             m_nTeamBots;

        // Sustained tasks
        // Keep idx's of bots with repair packs handy.
        s32             m_RepairBot;
        xbool           m_bRepairing;

        // Keep idx's of destroyers handy.
        s32             m_DestroyBot[MAX_DESTROY_BOTS];
        s32             m_nDestroyBots;
        s32             m_nDestroyTasks;
        s32             m_iCurrDestroy;
        s32             m_iLastUpdated;
        object::id      m_EnemyVehicle;

        object::id      m_FiveMostWanted[5];  // Last five offending turrets.

        // idx of deploy bot
        s32             m_DeployBot;
        asset_spot*     m_CurrentDeployAsset[MAX_DEPLOYERS];
        deploy_type     m_DeployType[MAX_DEPLOYERS];
        s32             m_nDeployAssets;

        // idx of sniper bot
        s32             m_iSniperBot;

        // Armor Class of each bot
        player_object::armor_type m_BotArmorType[MAX_BOTS];
        // Heavy list
        u32             m_HeavyList;

        // Object ID's of all the other team players 
        object::id      m_OtherTeamPlayerObjectID[32];
        player_object*  m_OtherTeamPlayer[32];
        s32             m_nOtherTeamPlayers;

        xbool           m_MissionInitialized;
        xbool           m_PlayersChanged;

        // Acceptable locations for deploying assets, gathered from asset file
        deployables     m_DeploySpots;

        // List of existing deployed assets
        vector3         m_DeployedTurret[MAX_TURRETS_ALLOWED];
        s32             m_nDeployedTurrets;

        vector3         m_DeployedInven[MAX_INVENS_ALLOWED];
        s32             m_nDeployedInvens;

        vector3         m_DeployedSensor[MAX_SENSORS_ALLOWED];
        s32             m_nDeployedSensors;

        xbool           m_TurretFull;
        xbool           m_SensorFull;
        xbool           m_InvenFull;

        // Current sniper pos
        s32             m_iCurrentSnipePos;

        s32             m_nSnipePoints;
        vector3         m_pSnipePoint[MAX_SNIPE_POINTS];

        xtimer          m_Timer;

        s32             m_LastSnipeUpdate;

        f32             m_Difficulty;

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
    protected:
        // PollAllObjects
        // Set the task state(s) and either:
        //      1) TargetPosObj 
        //          or 
        //      2) set TargetPosObj = NULL
        //         set TargetPos
virtual void        PollAllObjects      ( void );

        // Determine priority values for each task.
        void        PrioritizeTasks     ( void );
        // Sort list based on priority values.
        void        SortList            ( void );

        // (Re)assign tasks based on bot availability.
virtual void        AssignAllTasks      ( void );

        void        AssignBotTask       ( s32 BotID, bot_task* Task );

        void        ResetAssignments    ( void );

virtual void        UpdateAllTaskStates ( void );
    
        // Find closest team bot who hasn't been assigned a task yet.
        s32         FindClosestAvailableBot(bot_task* Task, 
                                                 f32& DistSq, 
                                                 u32 Excluded = EXCLUDE_ASSIGNED);
        s32         FindClosestAvailableBot(const vector3& TaskPos, 
                                                      f32& DistSq,
                                                      u32  Excluded = EXCLUDE_ASSIGNED);

        // Find the closest enemy within a particular radius. Returns -1 if none.
        //      if radius == 0 or negative, then radius is infinity.
        s32         FindClosestEnemy(const vector3& Pos, f32& DistSq, s32 Radius = 0) const;

virtual void        UpdatePlayerPointers ( void );

        xbool       HasIdealLoadout     ( s32 BotID );
        vector3     GetNearestInvenPos  ( const vector3& SourcePos ) const;

        xbool       SetTaskDefend( bot_task *Task,      // Task to set to attack/roam
                              const  vector3& DefendPos,// Position to defend.
                                s32 EngageRadiusSq,     // Radius within which to attack
                                s32 RetreatRadiusSq,    // Radius at which to retreat
                                f32& DistSq); // Enemy distance

        // Scan all assets and set up Destroy & Repair tasks.
        xbool       ScanAssets     ( 
                                bot_task **DestroyTask,
                                bot_task *RepairTask,         // Task to set
                                xbool UnderAttack = FALSE );

        void        RemoveDeadPlayers   ( void );

virtual void        SetDestroyBot       ( s32 BotID );

//const   asset_spot* ChooseAssetSpot     ( s32 Spot = 0 );
//const   asset_spot* ChooseAssetSpotBySwitch     ( object::id SwitchID, s32 Spot );

const   asset_spot* ChooseTurretSpot     ( object::id SwitchID, s32 Spot = 0 );
const   asset_spot* ChooseInvenSpot      ( object::id SwitchID, s32 Spot = 0 );
const   asset_spot* ChooseSensorSpot     ( object::id SwitchID, s32 Spot = 0 );

const   asset_spot* ChooseAssetSpotFn    ( object::id SwitchID, s32 Spot = 0 );

        void        PickSnipePoint      ( void );

        xbool       SensorPlacementCheck ( const vector3& Pos ) const;

virtual void        ResetDestroyTask    ( s32 TaskNum ) {(void)TaskNum;}

// DEBUG TEST
        xbool       BotsOnTask          (bot_task* Task, u32 Assigned);   
        
// Remove the deployable from our list to choose from.
        void        RemoveDeployable    (deploy_type Type, s32 Index);
    private:
object::id  GetSwitch           ( flipflop** Switch, s32 nSwitches, 
                                          const vector3& Pos);

};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

//==============================================================================
#endif  // MASTER_AI_HPP
//==============================================================================



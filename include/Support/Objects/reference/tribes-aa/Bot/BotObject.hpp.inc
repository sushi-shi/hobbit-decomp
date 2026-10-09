//==============================================================================
//
//  BotObject.hpp
//
//==============================================================================

#ifndef BOTOBJECT_HPP
#define BOTOBJECT_HPP

#define TESTING_VANTAGE_POINTS 0

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Objects/Player/PlayerObject.hpp"
#include "BotGoal.hpp"
#include "PathFinder.hpp"
#include "PathKeeper.hpp"
#include "Task.hpp"
#include "Objects/projectiles/Mine.hpp"
#include "Objects/projectiles/Turret.hpp"
#include "Objects/Projectiles/Station.hpp"

//==============================================================================
class sensor;
//==============================================================================

radian radian_DeltaAngle(radian A, radian B);

#if BOT_UPDATE_TIMING
    struct bot_timing_data
    {
        s32 nLastCalls;
        s32 nCalls;
        xtimer Timer;
        xtimer UpdateTimer;
        s32 LastSec;
        f32 LastTime;
        void* pBot;
    };
#endif


//==============================================================================
//  TYPES
//==============================================================================
class bot_object : public player_object
{
    friend class master_ai;
    friend class ctf_master_ai;
    friend class cnh_master_ai;
    friend class player_object;
//Types
public:
    typedef enum
    {
        MOVE_STATE_DEFAULT = 0,
        MOVE_STATE_WALK,
        MOVE_STATE_HOVER,
        MOVE_STATE_FLY
    }  move_state;

    typedef enum
    {
        WEAPON_SCORE_USELESS = 0,
        WEAPON_SCORE_INAPPROPRIATE,
        WEAPON_SCORE_APPROPRIATE,
        WEAPON_SCORE_DESIRABLE
    } weapon_score;

    struct bot_debug
    {
        s32     DrawGraphEdgeIndex;
        xbool   DrawNavigationMarkers;
        xbool   DrawSteeringMarkers;
        xbool   DrawMasterAITaskLabels;
        xbool   DrawCurrentGoalLabels;
        xbool   DrawObjectIdLables;
        xbool   DrawMasterAITaskStateLabels;
        xbool   DrawGraph;
        xbool   DrawGraphUseZBuffer;
        xbool   DrawAimingMarkers;
        xbool   TestMortar;
        xbool   TestRepair;
        xbool   TestDestroy;
        xbool   TestEngage;
        xbool   TestAntagonize;
        xbool   TestDeploy;
        xbool   TestSnipe;
        xbool   TestRoam;
        xbool   TestLoadout;
        xbool   AntagonizeEnemiesIfCreatedByEditor;
        xbool   ShowInputTiming;
        xbool   ShowOffense;
        xbool   ShowTaskDistribution;
        xbool   DisableTimer;
        xbool   Invincibility;
        xbool   DrawClosestEdge;
        xbool   ShowDeployLines;
#if BOT_UPDATE_TIMING
        xbool   ViewFPS;
        xbool   ViewFPSPB;
#endif
    };

// Data
private:
    bot_goal    m_CurrentGoal;
    move_state  m_MoveState;
    xbool       m_RequireGoalUpdateNextFrame;

    // For Pursuit
    const object* m_pPursuitObject;
    vector3     m_QuarryLastPos;

    // Timing/Update stuff
    f32         m_ElapsedTimeSinceLastGoalUpdate;

    path_keeper m_PathKeeper;       // our navigator
    object::id  m_TargetObject ;    // Target object to attack
    f32         m_TargetTime ;      // Time before choosing another target
    f32         m_FireTime ;        // Time before next fire
    vector3     m_PrevTargetLocation;
    vector3     m_CheatDeltaVelocity;
    f32         m_ClosestDistToNextPointSQ;
    xtimer      m_DistToNextPointTimer;
    xbool       m_RunningEditorPath;
    xbool       m_CreatedByEditor;
    xbool       m_Testing;
    xbool       m_CurrentlySkiing;
    terr*       m_pTerr; // only points to the existing terrain...
    vector3     m_FaceLocation;
    vector3     m_FaceDirection;
//    xbool       m_DangerAlert;
    xbool       m_DontClearMove;
    object::id  m_pRetrievedPickups[10];
    s32         m_nRetrievedPickups;
    xbool       m_LoadoutDisabled;
    xbool       m_AmHealthy;
    xbool       m_NeedRepairPack;
    xtimer      m_AntagonizeTimer;
    xbool       m_AntagonizingTurrets;
    xtimer      m_RoamTimer;
    xtimer      m_TurretDodgeTimer;
    f32         m_RoamStandAroundTime;
    s32         m_AntagonizeFidgetTime;
    f32         m_FidgetMoveX;
    f32         m_FidgetMoveY;
    f32         m_Difficulty;
    f32         m_MaxMSThisTime;
    f32         m_MaxMSLastTime;
    f32         m_TotalMSThisTime;
    f32         m_TotalMSLastTime;
    s32         m_StartFrame;
    s32         m_Num;
    xtimer      m_NextWeaponKeyTimer;
    f32         m_DeltaTime;
    object::id  m_AimFudgeTargetID;
    xtimer      m_AimFudgeTimer;
    vector3     m_AimFudgeInitialAimDirection;
    xtimer      m_PainTimer;
    object::id  m_pPainOriginObjID;
    xbool       m_GoingToInven;
    xbool       m_GoingSomewhere;
    xbool       m_UsingFixedInven;
    xbool       m_SteppingOffInven;
    object::id  m_InvenId;
    xtimer      m_MineDropTimer;
    f32         m_NextMineDropTime;
    s32         m_RandLoadoutID;
    s32         m_DeployFailure;
    xtimer      m_FlareTimer;
    xtimer      m_SelectedMissileLauncherTimer;
    vector3     m_AntagonizePos;
    object::id  m_AntagonizeTarget;
    f32         m_StuckMoveX;
    xtimer      m_InventTimer;
    xbool       m_JettedWhileStuck;
    xbool       m_BackedUpWhileStuck;
    xtimer      m_LastJetStuckTimer;
    // Sniping
    vector3     m_SnipeLocation;
    xbool       m_SnipeFirstGoto;
    object::id  m_SniperTarget;
    xtimer      m_SniperSenseTimer;
    f32         m_SniperSenseTime;
    xtimer      m_SniperWanderTimer;
    f32         m_SniperWanderTime;

    // GetDestroyVantagePoint stuff
    xbool       m_WaitingForDestroyVantagePoint;
    vector3     m_VantagePointCandidates[200];
    s32         m_NumCandidates;
    s32         m_CurCandidateIndex;

    // Combat members
    s32         m_RequestedWeaponSlot; // Need to "next weapon" to this weapon
    vector3     m_LastLocation;
    vector3     m_LastTargetLocation;
    xbool       m_Aiming;
    object::id  m_AimingTarget;
    vector3     m_AimDirection;
    vector3     m_Bullseye;
    vector3     m_LastAimDirection;
    xbool       m_EngageDodging;
    f32         m_EngageSuperiorityHeight;
    f32         m_DodgeXKey;
    f32         m_DodgeYKey;
    xtimer      m_DodgingTimer;
    xtimer      m_ChainGunFireTimer;
    xtimer      m_MortarSelectTimer;
    object::id  m_MortarSelectObjectID;
    xbool       m_MortarSelectCanHitTarget;
    xbool       m_WillThrowGrenades;
    xtimer      m_RepairPackTimer;
    object::id  m_NearestRepairPack;

    // Last turret that hit me
    object::id  m_LastAttackTurret;
    xtimer      m_TurretPainTimer;

    // Task stuff
    bot_task*   m_CurTask;
    bot_task*   m_PrevTask;
    bot_task::task_type   m_LastTaskID;
    xtimer      m_TaskTimer;
    s32         m_LastTaskState;
    u32         m_WaveBit;          // SPM- Spawned with this attack wave.

    // MasterAI Spawn stuff
    vector3     m_SpawnLocation;
    s32         m_BotLoadout;
    s32         m_MAIIndex;
    
    // Steve's update stuff
    vector3     m_StartPos ;        // Start position of bot
    f32         m_DestTime ;        // Time before choosing a new destination
    vector3     m_DestPos ;         // Current position to aim for
    
    xbool       m_bAllowedInVehicle;

#if BOT_UPDATE_TIMING
    bot_timing_data m_BotTiming;
#endif
// Functions
public:

    // Constructor/destructor
    bot_object() ;
    ~bot_object() ;

    void        Render              ( void ) ;
    
    // Initialize vars  
    void        Reset           ( void ) ;

    // Call from server 
    void    Initialize          ( const vector3&    WorldPosition,
                                  f32               TVRefreshRate,
                                  character_type    CharType,
                                  skin_type         SkinType,
                                  voice_type        VoiceType,
                                  s32               DefaultLoadoutType,
                                  const xwchar*     Name = NULL,
                                  xbool             CreatedByEditor = FALSE ) ;

    // Logic functions
    update_code  AdvanceLogic   ( f32 DeltaTime );

    void    WriteDirtyBitsData  ( bitstream& BitStream, u32 DirtyBits, xbool MoveUpdate ) ;
    xbool   ReadDirtyBitsData   ( const bitstream& BitStream, f32 SecInPast ) ;

    void    OnAcceptInit        ( const bitstream& BitStream );
    void    OnProvideInit       (       bitstream& BitStream );
    void    OnPain              ( const pain& Pain ) ;
    
    void    Respawn             ( void ) ;
    void    Player_Setup_DEAD   ( void ) ;

    void    OnAdd               ( void );    
    void    OnRemove            ( void );    

    void    RunEditorPath       ( const vector3& Src
        , const vector3& Dst );
    void    RunEditorPath       ( const xarray<u16>& ArrayPoints );

    void    LoadGraph           ( const char* pFilename );
    s32     GetBotID            ( void );
    xbool   InsideBuilding      ( void ) const;
    xbool   InsideBuilding      ( const vector3& Point ) const;
    void    SetNum              ( s32 Num ) { m_Num = Num; }

    void    SetBotTeamBits      ( void );

    // Master AI functions
    xbool   HasTask             ( void ) const { return !(m_CurTask == NULL); }
    void    SetTask             ( bot_task* Task );
    bot_task* GetTask           ( void )    { return m_CurTask; }
    xbool   DebugGraphIsConnected( void ) { return m_PathKeeper.DebugGraphIsConnected(); }
    xbool   RunStressTest       ( void )    { return m_PathKeeper.RunStressTest(); }
    u32     GetWaveBit          ( void )    const {return m_WaveBit;}
    void    SetWaveBit          ( u32 WaveBit )  {m_WaveBit = WaveBit;}
    void    UnassignWave        ( u32 WaveBit )  {m_WaveBit &= ~WaveBit;}
    xbool   HasRepairPack       ( void )  const{ return m_Loadout.Inventory.Counts[INVENT_TYPE_ARMOR_PACK_REPAIR];}


    const vector3& GetSpawnLocation ( void ) { return m_SpawnLocation; }
    void SetSpawnLocation (const vector3& Location) { m_SpawnLocation = Location; }

    s32     GetBotLoadout       ( void )     { return m_BotLoadout; }
    void    SetBotLoadout       ( s32 Load ) { m_BotLoadout = Load; }
    void    SetMove             ( const player_object::move& Move );
    static vector3 GetTargetCenter( const object& Target );
    vector3 PickPointInRadius   ( const vector3& Center
                                    , f32 Radius
                                    , xbool Outdoors = FALSE ) const;

    void    DisableLoadout      ( void ) { m_LoadoutDisabled = TRUE; }
    void    EnableLoadout       ( void ) { m_LoadoutDisabled = FALSE; }
    xbool   LoadoutDisabled     ( void ) const { return m_LoadoutDisabled; }
    
    f32     GetBotDifficulty    ( void ) const { return m_Difficulty; }
    void    SetBotDifficulty    ( f32 Difficulty );
    void    ToggleArmor         ( void );
    
    void    SetAllowedInVehicle ( xbool Flag ) { m_bAllowedInVehicle = Flag;    }
    xbool   IsAllowedInVehicle  ( void )       { return( m_bAllowedInVehicle ); }
    
    s32     GetPermanentVantagePoints( graph::vantage_point* pVantagePoints, s32 MaxNumPoints );
    
protected:

    void    FreeSlot            ( void );
    void    MovePointAlongSegment(
        vector3&                Point
        , const vector3&        SegmentStart
        , const vector3&        SegmentEnd
        , f32                   Dist ) const;
    f32     GetMaxSpeed( void );
    f32     GetArrivalSlowingDist( void );
    f32     GetPathFollowingPredictionTime( void );
    f32     GetPathFollowingPursueTime( void ) const;
    f32     GetPathFollowingDistanceSQ( void ) const;
    f32     GetElevationToPos( const vector3& Pos ) const;
    xbool   CanSeePoint( const vector3& Point ) const;
    xbool   PointsCanSeeEachOther( const vector3& Point1
                , const vector3& Point2
                , u32 AttrMask ) const;
    xbool   PointsCanSeeEachOther( const vector3& Point1
                , const vector3& Point2
                , u32 AttrMask
                , collider::collision& Collision) const;
    object* GetDangerousTurret( void );

    void ApplyCheatPhysics( void );

    f32     GetMaxAir               ( void );

    void    Speak ( s32 SfxID );

    //----------------------------------------------------------------------
    // Methods found in BotInput.cpp
    //----------------------------------------------------------------------
    void    UpdateInput             ( void );
    void    UpdatePursueObject      ( xbool FaceGoToLocation = TRUE );
    void    UpdatePursueLocation    ( void );
    void    UpdatePursue            ( const vector3& Position
                                        , xbool FaceGoToLocation = TRUE );
    void    UpdateGoToLocation      ( xbool FaceThisLocation = TRUE );
    xbool   NearEnemyFlag           ( void ) const;
    void    UpdatePatrol            ( void );
    void    UpdateEngage            ( void );
    void    UpdateMortar            ( void );
    void    UpdateRepair            ( void );
    void    UpdateDestroy           ( void );
    void    UpdateAntagonize        ( void );
    void    UpdateRoam              ( void );
    vector3 GetAverageEnemyPos      ( void );
    void    UpdateLoadout           ( void );
    void    UpdatePickup            ( void );
    void    UpdateDeploy            ( void );
    void    UpdateSnipe             ( void );
    void    PursueLocation          ( const vector3& TargetLocation
                                        , const vector3& NextTargetLocation
                                        , xbool Arrive = FALSE
                                        , xbool ForceAerial = FALSE );
    void    FaceLocation            ( const vector3& TargetLocation );
    void    FaceDirection           ( const vector3& Direction );
    void    UpdateFaceLocation      ( void );
    xbool   AntagonizeEnemies       ( void );
    mine*   GetMostDangerousMine    ( void );
    turret* GetMostDangerousTurret  ( xbool CheckClampsOnly = FALSE );
    vehicle* GetNearestEnemyVehicle ( void );
    station* GetNearestEnemyStation ( void );
    sensor* GetNearestEnemySensor   ( void );
    
    xbool   AmNear                  ( const vector3& Point ) const;
    xbool   IsNear                  ( const vector3& Point1
                                        , const vector3& Point2 ) const;
    xbool   HaveArrivedAt           ( const vector3& Point );
    xbool   HaveArrivedAbove        ( const vector3& Point ) const;

    xbool   HavePassedPoint(
        const vector3&          Point
        , const vector3&        PrevPoint
        , const vector3&        NextPoint ) const;

    // steering behaviors
    void    Steer( const vector3& Steering );
    void    SeekPosition( const vector3& SeekPos );
    void    ArriveAtPosition( const vector3& ArrivalPos );
    //----------------------------------------------------------------------


    //----------------------------------------------------------------------
    // Methods in BotCombat.cpp
    //----------------------------------------------------------------------
    void            SelectWeapon    ( const object& TargetPlayer );
    void            AimAtTarget     ( const object& Target );
    xbool           CanSplashDamage ( const object& Target ) const;
    s32             WeaponInSlot    ( invent_type WeaponTYPE ) const;
    player_object*  GetNearestVisibleEnemy      ( void );
    xbool           IsPlayerCloserVisibleEnemy  (
                                     f32& NearestDistSQ
                                     , const player_object& TestPlayer );
    xbool           FacingTarget        ( const object& Target ) const;
    xbool           FacingAimDirection  ( f32 Distance = 0.0f ) const;
    player_object*  GetNearestEnemy     ( void ) const;
    void            TryToFireOnTarget   ( const object& Target
                                            , xbool ForceCanHit = FALSE );
    xbool           MortarTarget        ( const object& Target );
    void            RepairTarget        ( const object& Target );
    vector3         GetMuzzlePosition   ( void ) const;
    xbool           CanHitTarget        ( const object& Target );
    xbool           CanHitTargetArcing  ( player_object::invent_type WeaponType
                                            , const object& Target );
    void            GetTargetBullseye   ( vector3& Bullseye
                                        , const object& Target
                                        , const invent_type& WeaponType );
    xbool           WouldSplashSelf     ( invent_type WeaponType
                                           , const vector3& AimDirection );
    xbool           SplashTooClose      ( invent_type WeaponType
                                           , const vector3& CollisionPoint );

    weapon_score    ScoreWeapon         ( invent_type WeaponType
                                           , const object& Target );
    //----------------------------------------------------------------------

    
    //----------------------------------------------------------------------
    // Methods found in BotStrategy.cpp
    //----------------------------------------------------------------------
    void        UpdateGoal                  ( void );
    void        InitializePursuit           ( const object& Object );
    void        InitializePursuit           ( void );
    void        InitializeGoTo              ( const vector3&  Location
                                                , xbool Reinit = FALSE );
    void        InitializePatrol            ( void );
    void        InitializeEngage            ( player_object& Enemy );
    void        InitializeMortar            ( const object& Enemy );
    void        InitializeRepair            ( const object& Target );
    void        InitializeDestroy           ( const object& Target );
    void        InitializeAntagonize        ( void );
    vector3     PickNextPatrolPoint         ( void ) const;
    void        InitializeRoam              ( void );
    void        InitializeRoam              ( const vector3& RoamCenter );
    void        InitializeLoadout           ( const station& Inven );
    void        InitializePickup            ( const object& Pickup );
    void        InitializeDeploy            ( const vector3& StandPos
                                                , const vector3& DeployPos
                                                , object::type ObjectType );
    void        InitializeSnipe             ( const vector3& StandPos );
    xbool       HaveIdealLoadout            ( void ) const;
    xbool       NeedArmor                   ( void ) const;
    xbool       NeedInvenPack               ( void ) const;
    void        InitializeCurrentTask       ( void );
    f32         GetNearestInven             ( station& Inven
                                                , xbool DeployedOK = TRUE ) const;
    f32         GetNearestRepairPackPos     ( vector3& PackPos ); 
    bot_object::loadout GetIdealLoadout     ( void ) const;
    xbool       WeaponsMatch                ( const player_object::loadout& IdealLoadout ) const;
    xbool       AProjectileWeaponCanFire ( void ) const;
    xbool       PacksMatch                  ( const player_object::loadout& IdealLoadout ) const;
    xbool       BeltGearMatches             ( const player_object::loadout& IdealLoadout ) const;
    object*     GetDesiredPickup            ( void );
    void        StoreRetrievedPickup        ( const object& Object );
    xbool       FindRetrievedPickup         ( const object& Object ) const;
    xbool       WantPickup                  ( const pickup& Pickup ) const;
    xbool       WantCorpse                  ( void ) const;
    void        TestUpdate                  ( void );
    static player_object* GetFirstPlayer    ( void );
    void        GetDestroyVantagePoint      ( vector3& Point
                                                , const object& Target );
    void        GetDestroyVantageNodes      ( xbool& FoundLinearNode
                                                 , s32&          LinearNode
                                                 , xbool&        FoundMortarNode
                                                 , s32&          MortarNode
                                                 , const object& Target );
    
    xbool       Defending                   ( void ) const;
   //----------------------------------------------------------------------
    

    //----------------------------------------------------------------------
    // internal draw utils
    //----------------------------------------------------------------------
    struct line
    {
        vector3 From;
        vector3 To;
        xcolor Color;
    };

    struct sphere
    {
        vector3 Pos;
        f32 Radius;
        xcolor Color;
    };

    struct marker
    {
        vector3 Pos;
        xcolor Color;
    };

    xarray<line> m_Lines;
    xarray<sphere> m_Spheres;
    xarray<marker> m_Markers;
    
    void DrawLine( const vector3& From, const vector3& To, xcolor Color=XCOLOR_WHITE );
    void DrawSphere( const vector3& Pos, f32 Radius, xcolor Color=XCOLOR_WHITE );
    void DrawMarker( const vector3& Pos, xcolor Color=XCOLOR_WHITE );
};

//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

object*     BotObjectCreator( void );

//==============================================================================
#endif // BOTOBJECT_HPP
//==============================================================================

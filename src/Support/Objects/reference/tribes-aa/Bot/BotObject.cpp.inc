//==============================================================================
//
//  BotObject.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "BotLog.hpp"
#include "BotObject.hpp"
#include "PathFinder.hpp"
#include "NetLib/Bitstream.hpp"
#include "AADS/Globals.hpp"
#include "AADS\fe_Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "Objects/Terrain/Terrain.hpp"
#include "PathKeeper.hpp"
#include "PathGenerator.hpp"
#include "Graph.hpp"
#include "MAI_Manager.hpp"
#include "Support\Building\BuildingOBJ.hpp"
#include "GameMgr/GameMgr.hpp"

vector3* g_BotWorldPos;
//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BotObjectCreator;

obj_type_info   BotObjectTypeInfo( object::TYPE_BOT, 
                                   "BotObject", 
                                   BotObjectCreator, 
                                   object::ATTR_PLAYER | 
                                   object::ATTR_LABELED | 
                                   object::ATTR_SOLID_DYNAMIC |
                                   object::ATTR_DAMAGEABLE |
                                   object::ATTR_ELFABLE |
                                   object::ATTR_REPAIRABLE |
                                   object::ATTR_SENSED );

#if defined (acheng)
    extern bot_object* ExamineBot;
#endif
extern graph    g_Graph;
extern s32      g_BotFrameCount;
xbool g_RunningPathEditor = FALSE;

bot_object::bot_debug g_BotDebug;

static s32 s_NumBots = 0;
static s32 s_CurBotNum = 0;
static s32 s_CurFrame = 0;
static f32 s_ElapsedTime = 0.0f;

s32 s_BotsExecuted = 0;
static s32 MAX_INPUT_EXECUTIONS = 5;            // Stops when num bots exceeds this
static f32 MAX_INPUT_TIME_PER_FRAME = 2.0f;     // Stops when time has exceeded this
static f32 MAX_CAMPAIGN_INPUT_TIME_PER_FRAME = 4.0f; // Stops when time has exceeded this

#if BOT_UPDATE_TIMING

    extern bot_timing_data g_BotTiming;
    extern bot_timing_data* g_BotTimingArray[16];
#endif
//==============================================================================
//  VALUES
//==============================================================================
static const f32 HORIZ_AIM_INPUT = 0.4f;    // magnitude of horiz aiming input
static const f32 HORIZ_AIM_THRESHOLD = R_5; // don't aim below this threshold
static const f32 VERT_AIM_INPUT = 0.6f;     // magnitude of horiz aiming input
static const f32 VERT_AIM_THRESHOLD = R_2;  // don't aim below this threshold
static const f32 MOVE_THRESHOLD = 0.2f;     // don't move below this threshold
static const f32 MOVE_THRESHOLD_SQ = x_sqr( MOVE_THRESHOLD );
static const f32 HORIZ_JET_THRESHOLD = 5.0f;// don't jet inside this threshold
static const f32 VERT_JET_THRESHOLD = 2.0f; // don't jet inside this threshold
static const f32 FAST_ENOUGH_TO_USE_JETS = 15.0f; // use jets to stop below this

// don't jet if you're more than this far above your target
static const f32 MAX_VERT_DIFF_FOR_JETS = 2.0f;

static const f32 HOVER_TARGET_ALTITUDE_RANGE = 10.0f;
static const f32 HOVER_MAX_HEIGHT_OVER_TARGET = 0.2f;
static const f32 HOVER_TARGET_RANGE = 30.0f;
static const f32 HOVER_TARGET_RANGE_SQ = x_sqr( HOVER_TARGET_RANGE );
static const f32 MIN_FLY_DIFFERENCE = HOVER_TARGET_ALTITUDE_RANGE;
static const f32 MIN_FLY_RANGE = 100.0f;
static const f32 MIN_FLY_RANGE_SQ = x_sqr( MIN_FLY_RANGE );
static const f32 MAX_HOVER_VERT_SPEED = 3.0f;
static const f32 MIN_FLAT_VELOCITY_TO_START_HOVERING = 1.0f;

static const f32 COS_20 = x_cos( R_20 );
static const f32 COS_45 = x_cos( R_45 );
static const f32 COS_90 = x_cos( R_90 );

static const f32 CLOSE_ENOUGH = 0.75f;
static const f32 CLOSE_ENOUGH_SQ = x_sqr( CLOSE_ENOUGH );
static const f32 REALLY_CLOSE = 0.01f;
static const f32 REALLY_CLOSE_SQ = x_sqr( REALLY_CLOSE );

static const f32 MAX_VIABLE_DIST_TO_NEXT_POINT = 50.0f;
static const f32 MAX_VIABLE_DIST_TO_NEXT_POINT_SQ
    = x_sqr( MAX_VIABLE_DIST_TO_NEXT_POINT );
static const f32 PATH_FOLLOWING_RADIUS = 0.5f; // meters
static const f32 PATH_FOLLOWING_RADIUS_SQ = x_sqr( PATH_FOLLOWING_RADIUS );
static const f32 PATH_FOLLOWING_WEIGHT = 50.0f;
static const f32 MAX_DANGEROUS_TURRET_RANGE = 100.0f;
static const f32 MAX_DANGEROUS_TURRET_RANGE_SQ
                     = x_sqr( MAX_DANGEROUS_TURRET_RANGE );

// Chain gun takes 1 second to wind up, so let's fire for at least 2
extern f32 CHAINGUN_RAMP_SPEED_UP_TIME;
f32 CHAIN_GUN_FIRE_HOLD_TIME = CHAINGUN_RAMP_SPEED_UP_TIME + 1.0f;

xbool g_DrawSteeringMarkers = FALSE;
xbool g_DrawBotTasks = TRUE;



#define PRINT_PURSUE_INFO 0
#define PRINT_UPDATE_GOAL_INFORMATION 0
#define TIMING_UPDATE_INPUT 0

#if TIMING_UPDATE_INPUT
static xtimer worldTimeForInputUpdate;
static f32 maxInputUpdateTime = 0.0f;
static f32 totalInputUpdateTime = 0.0f;
static s32 numInputUpdateTimeSamples = 0;
#endif


//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

object* BotObjectCreator( void )
{
    return( (object*)(new bot_object) );
}

//==============================================================================
//  CLASS FUNCTIONS
//==============================================================================

// Constructor
bot_object::bot_object( void )
 :  m_RunningEditorPath( FALSE ),
    m_CreatedByEditor( FALSE ),
    m_Testing( FALSE ),
    m_DontClearMove( FALSE ),
    m_LoadoutDisabled( FALSE ),
    m_CurTask( NULL ),
    m_WaveBit( 1 ),
    m_BotLoadout( 0 ),
    m_MAIIndex(-1)
{
//    bot_log::Clear();

    // Initialize g_BotDebug
    g_BotDebug.DrawGraphEdgeIndex = -1; // do nothing
    g_BotDebug.DrawNavigationMarkers = FALSE;
    g_BotDebug.DrawSteeringMarkers = FALSE;
    g_BotDebug.DrawMasterAITaskLabels = FALSE;
    g_BotDebug.DrawMasterAITaskStateLabels = FALSE;
    g_BotDebug.DrawCurrentGoalLabels = FALSE;
    g_BotDebug.DrawObjectIdLables = FALSE;
    g_BotDebug.DrawGraph = FALSE;
    g_BotDebug.TestMortar = FALSE;
    g_BotDebug.TestRepair = FALSE;
    g_BotDebug.TestDestroy = FALSE;
    g_BotDebug.TestEngage = FALSE;
    g_BotDebug.TestAntagonize = FALSE;
    g_BotDebug.TestDeploy = FALSE;
    g_BotDebug.TestSnipe = FALSE;
    g_BotDebug.TestRoam = FALSE;
    g_BotDebug.TestLoadout = FALSE;
    g_BotDebug.DrawGraphUseZBuffer = TRUE; // zbuffer used when DrawGraph TRUE
    g_BotDebug.DrawAimingMarkers = FALSE;
    g_BotDebug.AntagonizeEnemiesIfCreatedByEditor = FALSE;
    g_BotDebug.ShowInputTiming = FALSE;
    g_BotDebug.ShowOffense= FALSE;
    g_BotDebug.ShowTaskDistribution = FALSE;
    g_BotDebug.DisableTimer = FALSE;
    fegl.BotInvulnerable = FALSE;
    g_BotDebug.DrawClosestEdge = FALSE;
    g_BotDebug.ShowDeployLines = FALSE;
    
#if BOT_UPDATE_TIMING
    g_BotDebug.ViewFPS = FALSE;
    g_BotDebug.ViewFPSPB = FALSE;
#endif
    // Modify g_BotDebug per user
#ifdef mreed
    g_BotDebug.DrawMasterAITaskLabels = TRUE;
    g_BotDebug.DrawMasterAITaskStateLabels = TRUE;
    g_BotDebug.DrawCurrentGoalLabels = TRUE;
    g_BotDebug.DrawObjectIdLables = TRUE;
    g_BotDebug.DrawNavigationMarkers = TRUE;
    g_BotDebug.DrawSteeringMarkers = FALSE;
    g_BotDebug.DrawGraph = FALSE;
    g_BotDebug.DrawGraphUseZBuffer = TRUE;
    g_BotDebug.DrawAimingMarkers = FALSE;
    g_BotDebug.ShowInputTiming = FALSE;
    g_BotDebug.TestAntagonize = FALSE;
    fegl.BotInvulnerable = TRUE;
#endif
 
#ifdef acheng
//    g_BotDebug.DrawCurrentGoalLabels = TRUE;
//    g_BotDebug.DrawObjectIdLables = TRUE;
//    g_BotDebug.DrawNavigationMarkers = TRUE;
//    g_BotDebug.DisableTimer = TRUE;
    fegl.BotInvulnerable = TRUE;
    g_BotDebug.ShowDeployLines = TRUE;
#if SHOW_CLOSEST_EDGE
    g_BotDebug.DrawGraph = TRUE;
    g_BotDebug.DrawClosestEdge = TRUE;
#endif
//    g_BotDebug.ShowOffense= TRUE;
    g_BotDebug.DrawMasterAITaskLabels = TRUE;
//    g_BotDebug.DrawGraph = TRUE;
//    g_BotDebug.DrawMasterAITaskStateLabels = TRUE;
//    g_BotDebug.ShowTaskDistribution = TRUE;
#endif

#ifdef athyssen
    g_BotDebug.DrawMasterAITaskLabels = FALSE;
#endif


#if BOT_UPDATE_TIMING
//    g_BotDebug.ViewFPS = TRUE;
//    g_BotDebug.ViewFPSPB = TRUE;
    if (!g_BotTiming.Timer.IsRunning())
    {
        g_BotTiming.Timer.Start();
        g_BotTiming.UpdateTimer.Reset();
        g_BotTiming.LastSec =
        g_BotTiming.nCalls =
        g_BotTiming.nLastCalls = 0;
        g_BotTiming.LastTime = 0.0f;

        for (s32 i = 0; i < 16; i++)
        {
            g_BotTimingArray[i] = NULL;
        }
    
    }
        m_BotTiming.Timer.Start();
        m_BotTiming.UpdateTimer.Reset();
        m_BotTiming.LastSec =
        m_BotTiming.nCalls =
        m_BotTiming.nLastCalls = 0;
        m_BotTiming.LastTime = 0.0f;
        m_BotTiming.pBot = this;

        g_BotTimingArray[GetBotID()] = &m_BotTiming;
#endif
    //==============================================================================
    //==============================================================================
    // HACK
    //==============================================================================
    //==============================================================================
    this->m_TeamBits = 2;
}

//==============================================================================
// InitBotAdvance()
//==============================================================================
void InitBotAdvance( void )
{
    s_NumBots = 0;
    s_CurBotNum = 0;
    s_CurFrame = 0;
}

//==============================================================================
// AdvanceAllBotLogic()
//==============================================================================
void AdvanceAllBotLogic( f32 DeltaTime )
{
    //--------------------------------------------------
    // Count and store the current bots
    //--------------------------------------------------
    s32 CurNumBots = 0;
    bot_object* pCurBot;
    bot_object* pBots[32];

    ObjMgr.StartTypeLoop( object::TYPE_BOT );

    while ( (pCurBot = (bot_object*)ObjMgr.GetNextInType()) )
    {
        pBots[CurNumBots] = pCurBot;
        ++CurNumBots;
    }

    ObjMgr.EndTypeLoop();

    s32 i;

    //--------------------------------------------------
    // If the number of bots has changed, or a bot
    // m_Num value is unset (-1), renumber all
    // of them and reset our input scheduling.
    //--------------------------------------------------
    if ( CurNumBots != s_NumBots )
    {
        s_NumBots = CurNumBots;

        for ( i = 0; i < s_NumBots; ++i )
        {
            (pBots[i])->SetNum( i );
        }

        s_CurBotNum = 0;
    }
    
    //--------------------------------------------------
    // Advance all the bots
    //--------------------------------------------------
    s_BotsExecuted=0;
    object::update_code UpdateCode;
    for ( i = 0; i < s_NumBots; ++i )
    {
        UpdateCode = (pBots[i])->OnAdvanceLogic( DeltaTime );

        switch( UpdateCode )
        {
        case object::UPDATE:
            ObjMgr.UpdateObject( pBots[i] );
            break;
        case object::DESTROY:
            if( (tgl.ServerPresent) || (((pBots[i])->GetObjectID()).Slot >= obj_mgr::MAX_SERVER_OBJECTS) )
            {
                ObjMgr.RemoveObject ( pBots[i] );
                ObjMgr.DestroyObject( pBots[i] );
            }
            break;
        default:
            break;
        }
    }
    //x_DebugMsg("Bots executed %1d\n",s_BotsExecuted);
}

//==============================================================================

// Destructor
bot_object::~bot_object()
{
    FreeSlot();
}

//==============================================================================

void bot_object::Reset()
{
    // Call base class
    player_object::Reset() ;

    // Reset bot vars
    m_CurrentGoal.SetGoalID( bot_goal::ID_NO_GOAL );
    m_MoveState = MOVE_STATE_DEFAULT;
    m_pPursuitObject = NULL;

    // Steve's update stuff
    m_StartPos = vector3(0,0,0) ;            // Start position of bot
    m_DestTime = 0.0f ;                      // Time before choosing a new
                                             // destination
    m_DestPos  = vector3(0,0,0) ;            // Current position to aim for

    m_TargetObject = obj_mgr::NullID ;       // Target object to attack
    m_TargetTime   = 0.0f ;                  // Time before choosing another
                                             // target
    m_FireTime     = 0.0f ;                  // Time before next fire

    m_ElapsedTimeSinceLastGoalUpdate  = 0.0f;
    m_RequireGoalUpdateNextFrame      = FALSE;
    m_CurrentlySkiing                 = FALSE;
    m_AimDirection.Set( 0.0f, 1.0f, 0.0f );
    m_LastAimDirection.Set( 0.0f, 1.0f, 0.0f );
    m_EngageDodging = FALSE;
    m_EngageSuperiorityHeight = x_frand( 10.0f, 30.0f );
//    m_DangerAlert = FALSE;
    m_DontClearMove = FALSE;

#if TIMING_UPDATE_INPUT
    worldTimeForInputUpdate.Start();
#endif

    m_CurTask = NULL;
    m_PrevTask = NULL;
    m_LastTaskID = bot_task::TASK_TYPE_NONE;
    m_LastTaskState = bot_task::TASK_STATE_INACTIVE;
    m_WaveBit = 1;
    m_nRetrievedPickups = 0;
    m_ChainGunFireTimer.Stop();
    m_AmHealthy = TRUE;
    m_NeedRepairPack = FALSE;

    m_FaceDirection = vector3(1.0f,0.0f,0.0f);
//    FaceDirection( vector3( 1.0f, 0.0f, 0.0f ) );

    m_Difficulty = 1.0f;
    m_NextWeaponKeyTimer.Reset();
    m_NextWeaponKeyTimer.Start();

    m_AimFudgeTargetID = obj_mgr::NullID;

    m_PainTimer.Reset();
    m_pPainOriginObjID = obj_mgr::NullID;

    m_LastAttackTurret = obj_mgr::NullID;
    m_TurretPainTimer.Stop();

    m_MortarSelectTimer.Reset();
    m_MortarSelectObjectID = obj_mgr::NullID;
    m_MortarSelectCanHitTarget = FALSE;

    m_MineDropTimer.Reset();
    m_MineDropTimer.Start();
    m_NextMineDropTime = x_frand( 2.0f, 10.0f );
    m_DeployFailure = 0;

    m_GoingSomewhere = FALSE;
    m_Aiming = FALSE;
    
    m_MAIIndex = -1;

    m_Num = -1;

    // Invalidate the number of bots to force m_Num to be reset.
    s_NumBots = -1;

    m_WillThrowGrenades = FALSE;
    
    m_FlareTimer.Reset();
    m_FlareTimer.Start();
    
    m_AntagonizingTurrets = FALSE;

    m_RepairPackTimer.Reset();
    m_RepairPackTimer.Start();
    m_NearestRepairPack = obj_mgr::NullID;
    m_SelectedMissileLauncherTimer.Reset();
    m_SelectedMissileLauncherTimer.Start();
    m_Bullseye.Zero();

    m_AntagonizePos.Zero();
    m_AntagonizeTarget = obj_mgr::NullID;

    m_StuckMoveX = (x_irand( 0, 100 ) > 50) ? 1.0f : -1.0f;
}


//==============================================================================

// Call from server
void bot_object::Initialize (
    const vector3&          WorldPosition
    , f32                   TVRefreshRate
    , character_type        CharType
    , skin_type             SkinType
    , voice_type            VoiceType
    , s32                   DefaultLoadoutType
    , const xwchar*         Name /* = NULL */
    , xbool                 CreatedByEditor /* = FALSE */ )
{
    Reset();

    // Call player initialize
    player_object::Initialize(TVRefreshRate,        // TVRefreshRate
                              CharType,             // Character type
                              SkinType,             // Skin type
                              VoiceType,            // Voice type
                              DefaultLoadoutType,   // Default loadout type
                              Name) ;               // Name

    m_CreatedByEditor = CreatedByEditor;
    m_Testing = FALSE;
    
    if ( m_CreatedByEditor )
    {
        // Find the ground in case the bot starts in the air
        vector3 Pos = WorldPosition ;
        Pos.Y += 10 ;
        collider Ray ;
        Ray.RaySetup( this, Pos, Pos - vector3(0,100,0) ) ;
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray ) ;
    
        // Get results
        collider::collision Collision;
        Ray.GetCollision( Collision );
        if (Collision.Collided)
        {
            Pos = Collision.Point;
        }
        
        // Start at given position
        SetPos( Pos );
    }
    if (MAI_Mgr.GetGameID() == MAI_SPM)
        SetPos( WorldPosition );

    g_BotWorldPos= &m_WorldPos;
    // Store the current terrain
    ObjMgr.StartTypeLoop( object::TYPE_TERRAIN );
    terrain* Terrain = (terrain*)(ObjMgr.GetNextInType());
    ObjMgr.EndTypeLoop();
    ASSERT( Terrain );
    m_pTerr = &(Terrain->GetTerr());

    // Set up the path keeper with terrain
    m_PathKeeper.Initialize( *m_pTerr );

    m_RequestedWeaponSlot = m_WeaponCurrentSlot;

#if 0 // for debugging when I need a heavy in path editor
    
            loadout Loadout;
            default_loadouts::GetLoadout(
                GetCharacterType()
                , static_cast<default_loadouts::type>(
                    default_loadouts::TASK_TYPE_MORTAR)
                , Loadout );
            SetInventoryLoadout( Loadout );
            InventoryReload();
#endif

    SetAllowedInVehicle( FALSE );
}

//==============================================================================

void bot_object::SetBotTeamBits( void )
{
    m_PathKeeper.SetTeamBits ( m_TeamBits );
}

//==============================================================================

void    bot_object::FreeSlot            ( void )
{
    m_PathKeeper.FreeSlot();
}


//==============================================================================

// Returns the smallest delta so that A+Delta = B        
radian radian_DeltaAngle(radian A, radian B)
{
    ASSERT( A > (-R_360*2) );
    ASSERT( A < ( R_360*2) );
    ASSERT( B > (-R_360*2) );
    ASSERT( B < ( R_360*2) );

    // Normalize A
    while(A > R_360)
        A -= R_360 ;
    while(A < -R_360)
        A += R_360 ;

    // Normalize B
    while(B > R_360)
        B -= R_360 ;
    while(B < -R_360)
        B += R_360 ;

    // Start with regular delta
    radian Delta = B - A ;

    // Try next rotation
    radian TryDelta = R_360 + Delta ;
    if (ABS(TryDelta) < ABS(Delta))
        Delta = TryDelta ;

    // Try prev rotation
    TryDelta = Delta - R_360 ;
    if (ABS(TryDelta) < ABS(Delta))
        Delta = TryDelta ;

    return Delta ;
}

//==============================================================================
xtimer BotLogicTimer[3];

xtimer BTimer[10];
xbool DO_PURSUE_LOC_EVERY_FRAME = TRUE;
xbool DO_AIMING_EVERY_FRAME = TRUE;

#ifdef acheng
s32 g_LastAttemptedInitGoal = -1;
s32 g_LastAttemptedUpdateGoal = -1;
s32 g_LastActualDeployInited = -1;
#endif
object::update_code bot_object::AdvanceLogic( f32 DeltaTime )
{
    ASSERT(m_Num != -1);

    BTimer[0].Start(); ///////////////////

    xtimer InputTimer;

    m_DeltaTime = DeltaTime;

    // On server. Perform logic
    if (tgl.ServerPresent)
    {
        // Update Input timing data
        if ( s_CurFrame != g_BotFrameCount )
        {
            s_CurFrame = g_BotFrameCount;
            s_ElapsedTime = 0.0f;
        }
        
BTimer[1].Start(); ///////////////////

        BotLogicTimer[0].Start();
        m_CheatDeltaVelocity.Zero();

        //-----------------------------------------------------------
        // SkipUpdate will freeze bots while running the path editor,
        // but only bots that weren't created by the editor
        //-----------------------------------------------------------
        xbool SkipUpdate = (g_RunningPathEditor && !m_CreatedByEditor);

        if ( !SkipUpdate )
        {
            m_PathKeeper.Update();
        }
        
        m_ElapsedTimeSinceLastGoalUpdate += DeltaTime;
        const xbool IsCampaignMission = (GameMgr.GetGameType()==GAME_CAMPAIGN);
        const xbool InputUpdateHasTime
            = (s_ElapsedTime
                <= (IsCampaignMission
                    ? MAX_CAMPAIGN_INPUT_TIME_PER_FRAME
                    : MAX_INPUT_TIME_PER_FRAME));
BTimer[1].Stop(); ///////////////////

        if ( !IsDead()
            && !SkipUpdate )
        {
BTimer[2].Start(); ///////////////////
            // Make sure we're currently working on the right thing,
            // and do it.
#ifdef acheng
            g_LastAttemptedInitGoal = m_MAIIndex;
#endif
            //
            // Deploy task is a special case- it is the only one in which the
            // Master AI directly adjusts values within the bot_goal.  This means
            // the deploy tasks must always be up to date, otherwise there may
            // be some sync issues between goals that have been initialized and 
            // goals that are being updated..  Hence to avoid bad things from
            // happening, we'll just update deploy goals every time.
            //
            // Always force update goal if deploying.
            if (m_CurTask &&
                (    m_CurrentGoal.GetGoalID() == bot_goal::ID_DEPLOY ||
                    (m_CurTask->GetState() >= bot_task::TASK_STATE_DEPLOY_TURRET &&
                     m_CurTask->GetState() <= bot_task::TASK_STATE_DEPLOY_SENSOR)))
                        m_RequireGoalUpdateNextFrame = TRUE;

            UpdateGoal();
BTimer[2].Stop(); ///////////////////
            const xbool OurTurn
                = (m_Num == s_CurBotNum)
                && InputUpdateHasTime
                && (IsCampaignMission
                    || (s_BotsExecuted<MAX_INPUT_EXECUTIONS));

            static const s32 NUM_FRAMES = 30;
#ifdef T2_DESIGNER_BUILD
            if ( TRUE )
#else
            if ( OurTurn )
#endif
            {
                if ( m_CurTask && (m_TaskTimer.ReadSec() > 120.0f ) )
                {
                    // this is taking a while, say something about it
                    x_DebugMsg( "BotTask has taken more than 2 minutes, Slot/Seq: %i/%i, Task: %s, State: %s, Goal: %s\n",
                        m_ObjectID.Slot, 
                        m_ObjectID.Seq, 
                        m_CurTask->GetTaskName(), 
                        m_CurTask->GetTaskStateName(), 
                        (const char*)m_CurrentGoal.GetGoalString() );

                    m_TaskTimer.Reset();
                    m_TaskTimer.Start();
                }

                s_BotsExecuted++;
                // maintain the current bot num
                ++s_CurBotNum;
                if ( s_CurBotNum >= s_NumBots )
                {
                    s_CurBotNum = 0;
                }

                if (m_CurTask || m_Testing || this->m_CreatedByEditor) 
                {
    BTimer[3].Start(); ///////////////////
                    InputTimer.Reset();
                    InputTimer.Start();
                    // Take the next step towards our goal
#ifdef acheng
            g_LastAttemptedUpdateGoal = m_MAIIndex;
#endif

                    UpdateInput();
                    if( !DO_AIMING_EVERY_FRAME && (m_DontClearMove == FALSE) )
                        UpdateFaceLocation();
                    InputTimer.Stop();
    BTimer[3].Stop(); ///////////////////

                    s_ElapsedTime += InputTimer.ReadMs();
                
                    m_TotalMSThisTime += InputTimer.ReadMs();
                
                    if ( InputTimer.ReadMs() > m_MaxMSThisTime )
                    {
                        m_MaxMSThisTime = InputTimer.ReadMs();
                    }

                    if ( g_BotFrameCount - m_StartFrame >= NUM_FRAMES )
                    {
                        m_StartFrame = g_BotFrameCount;
                        m_MaxMSLastTime = m_MaxMSThisTime;
                        m_MaxMSThisTime = 0.0f;
                        m_TotalMSLastTime = m_TotalMSThisTime;
                        m_TotalMSThisTime = 0.0f;
                    }

                }
                else
                {
                    // No task?
#if defined (acheng)
                    x_printf("Bot has no current task?\n");
#endif
                }
            }
            else
            {
                // not our turn to update
                if ( DO_PURSUE_LOC_EVERY_FRAME && m_GoingSomewhere )
                {
                    const xbool TargetNearCurrentPoint
                        = IsNear( m_CurrentGoal.GetTargetLocation()
                            , m_PathKeeper.GetCurrentPoint().Pos );

                    PursueLocation( m_PathKeeper.GetCurrentPoint().Pos
                        , m_PathKeeper.GetNextPoint().Pos
                        , TargetNearCurrentPoint );
                }

                // Don't hold this down -- it makes it tough to use the repair pack
                m_Move.PackKey = FALSE;
#if 0
                    
                // we're not doing a full update, but we should do some maintenance
                if ( OnGround()
                    && !(m_CurrentGoal.GetGoalID() == bot_goal::ID_ENGAGE)
                    && (m_CurrentGoal.GetGoalState()
                        == bot_goal::STATE_ENGAGE_FIGHTING)
                    && !m_DontClearMove
                    && (m_Vel.LengthSquared() > (0.25f * 0.25f)) )
                {
                    //--------------------------------------------------
                    // Don't continue to travel sideways -- lest we 
                    // overshoot and end up wobbling along our path
                    //--------------------------------------------------
                    vector3 HorizRot( m_Rot );
                    HorizRot.Y = 0.0f;
                    
                    vector3 HorizToTarget
                        = m_PathKeeper.GetNextPoint().Pos - m_WorldPos;
                    HorizToTarget.Y = 0.0f;
                    HorizToTarget.Normalize();

                    vector3 StickPos;
                    StickPos.X = m_Move.MoveX;
                    StickPos.Y = 0.0f;
                    StickPos.Z = m_Move.MoveY;

                    // rotate the StickPos into the world's orientation
                    StickPos.RotateY( HorizRot.GetYaw() );

                    // project in the direction of our goal
                    f32 Proj = StickPos.Dot( HorizToTarget );
                    StickPos.Normalize();
                    StickPos *= Proj;

                    // rotate the StickPos back to player's orientation
                    StickPos.RotateY( -HorizRot.GetYaw() );

                    // Set the real stick up
                    m_Move.MoveX = StickPos.X;
                    m_Move.MoveY = StickPos.Z;
                }
#endif
            }

            if ( g_BotDebug.ShowInputTiming )
            {
                x_printfxy( 0, (GetBotID() % 30) + 1
                    , "Bot: %i, Max %4.2f, Avg %4.2f, Goal: %s\n"
                    , GetBotID()
                    , m_MaxMSLastTime
                    , m_TotalMSLastTime / NUM_FRAMES
                    , (const char*)m_CurrentGoal.GetGoalString() );
            }
                
BTimer[4].Start(); ///////////////////
            // Face the right way
            //UpdateFaceLocation();
            if( DO_AIMING_EVERY_FRAME && (m_DontClearMove == FALSE ) )
                UpdateFaceLocation();
BTimer[4].Stop(); ///////////////////
        }
        else
        {
            if ( InputUpdateHasTime )
            {
                ++s_CurBotNum;

                if ( s_CurBotNum >= s_NumBots )
                {
                    s_CurBotNum = 0;
                }
            }
            
            if ( !m_DontClearMove || SkipUpdate )
            {
                m_Move.Clear();
            }
        }
        

        if ( m_CurrentlySkiing && !SkipUpdate )
        {
            m_Move.JumpKey = TRUE;
        }

        // Handle chain gun wind-up time -- don't fire sporadically
        if ( (m_Loadout.Weapons[m_WeaponCurrentSlot]
                == player_object::INVENT_TYPE_WEAPON_CHAIN_GUN)
            && m_ChainGunFireTimer.ReadSec() < 1.5f 
            && m_ChainGunFireTimer.IsRunning() )
        {
            m_Move.FireKey = TRUE;
        }
        
        // Do dumb ai for me!
        //#ifdef sbroumley
            //m_Move.FireKey = FALSE ;

            //static f32 DumbTime = 0 ;
            //static f32 DumbDir = -1 ;
            //DumbTime += DeltaTime ;
            //if (DumbTime > 4)
            //{
                //DumbTime -= 4 ;
                //DumbDir *= -1 ;
            //}

            // Create dumb move
            //m_Move.Clear() ;
            //m_Move.MoveX = DumbDir ;

        //#endif

        // TEMP!!
        {
            static xbool STOP_BOTS = FALSE ;
            if (STOP_BOTS)
            {
                m_Move.Clear() ;
            }
        }

BTimer[5].Start(); ///////////////////
        // Apply move
        m_Move.DeltaTime = DeltaTime ;
        ApplyMove();
        BotLogicTimer[0].Stop();
        BotLogicTimer[1].Start();
        ApplyPhysics();
        BotLogicTimer[1].Stop();
        BotLogicTimer[2].Start();
        ApplyCheatPhysics();
BTimer[5].Stop(); ///////////////////

BTimer[6].Start(); ///////////////////
        // Bot house keeping and state updates
        Advance(DeltaTime) ;
BTimer[6].Stop(); ///////////////////

        // Update
        BotLogicTimer[2].Stop();
    }
    else
    // On client, just simulate movement - dirty bits will update the pos etc.
    if (tgl.ClientPresent)
    {
        // Ghost players are always blendeding to the
        // latest predicted server position

        // Any prediction time left?
        if ((m_PredictionTime >= DeltaTime) || (m_PlayerState == PLAYER_STATE_TAUNT))
        {
            // Update prediction time
            m_PredictionTime -= DeltaTime ;

            // Keep predicting with the same move...
            m_Move.DeltaTime = DeltaTime ;
            ApplyMove() ;
            ApplyPhysics() ;
        }
        else
        {
            // Update weapon logic
	        Weapon_AdvanceState(DeltaTime) ;

            // Update player logic
	        Player_AdvanceState(DeltaTime) ;
        }

        // Player house keeping
        Advance(DeltaTime) ;
    }

BTimer[0].Stop(); ///////////////////

    return( UPDATE );
}

//==============================================================================

void bot_object::SetTask( bot_task *Task )
{
    if ( m_CurTask )
    {
        // MAR 7/23/02 I think this should be uncommented, but I'm scared to do it
        // without extensive testing
        //m_LastTaskID = m_CurTask->GetID();
    }

    m_TaskTimer.Reset(); 
    m_TaskTimer.Start(); 
    m_CurTask = Task; 
}

//==============================================================================

void bot_object::WriteDirtyBitsData  ( bitstream& BitStream, u32 DirtyBits, xbool MoveUpdate )
{
    // Call base class
    player_object::WriteDirtyBitsData(BitStream, DirtyBits, 1.0f, MoveUpdate) ;

    // Put bot stuff here...
}

//==============================================================================

xbool bot_object::ReadDirtyBitsData   ( const bitstream& BitStream, f32 SecInPast  )
{
    // Call base class
    xbool MoveUpdate = player_object::ReadDirtyBitsData(BitStream, SecInPast) ;

    // Put bot stuff here...

    // Return wether or not this was a move update
    return MoveUpdate ;
}

//==============================================================================

void bot_object::OnAcceptInit( const bitstream& Bitstream )
{
    // Call base class
    player_object::OnAcceptInit(Bitstream) ;

    // Add bot stuff here...
}

//==============================================================================

void bot_object::OnProvideInit( bitstream& Bitstream )
{
    // Call base class first
    player_object::OnProvideInit(Bitstream) ;

    // Add bot stuff here...
}

//==============================================================================

void bot_object::OnPain( const pain& Pain )
{
    player_object::OnPain( Pain );

    if ( Pain.MetalDamage < 0 )
    {
        // this is the good stuff :)
        return;
    }

    m_PainTimer.Reset();
    m_PainTimer.Start();
    m_pPainOriginObjID = Pain.OriginID;
    object* pPainOriginObj = ObjMgr.GetObjectFromID(m_pPainOriginObjID);
    if (pPainOriginObj && 
        ((pPainOriginObj->GetTeamBits() & m_TeamBits) == 0))
    {
        if ( pPainOriginObj->GetType() == object::TYPE_TURRET_CLAMP )
        {
		    MAI_Mgr.Ouch(m_TeamBits, Pain.OriginID);
            m_LastAttackTurret = Pain.OriginID;
            m_TurretPainTimer.Reset();
            m_TurretPainTimer.Start();
        }
		 else if (pPainOriginObj->GetType() == object::TYPE_TURRET_FIXED
			||    pPainOriginObj->GetType() == object::TYPE_TURRET_SENTRY )
		 {
			 MAI_Mgr.Ouch(m_TeamBits, Pain.OriginID);
		 }
	
        if ( (pPainOriginObj->GetType() == object::TYPE_PLAYER)
            && (pPainOriginObj->GetTeamBits() & GetTeamBits()) )
        {
            Speak( 133 );
        }
    }
    
}

//==============================================================================

void bot_object::Respawn( void )
{
    ASSERT(m_MAIIndex >= -1 && m_MAIIndex < 33);

    // Call base class
    player_object::Respawn( );

    // Bot on server?
    if (tgl.ServerPresent)
    {

        if (m_CurTask  && m_MAIIndex != -1)//&& GameMgr.GetGameType() == GAME_CTF )
        {
            m_CurTask->UnassignBot(m_MAIIndex);

            if (GameMgr.GetGameType() != GAME_CAMPAIGN) 
                                    // Campaign will not reevaluate target.
            {
                m_CurTask->SetTargetObject(NULL);
            }
        }
        m_RandLoadoutID = x_rand()%bot_task::TASK_TYPE_TOTAL;
        if (m_RandLoadoutID == bot_task::TASK_TYPE_DEPLOY_TURRET 
            ||m_RandLoadoutID == bot_task::TASK_TYPE_DEPLOY_INVEN
            ||m_RandLoadoutID == bot_task::TASK_TYPE_DEPLOY_SENSOR )
        {
            m_RandLoadoutID -= ((s32)(bot_task::TASK_TYPE_DEPLOY_TURRET) - 3);
        }
        m_CurrentGoal.SetGoalID( bot_goal::ID_NO_GOAL );
        m_CurTask = NULL;
        m_PrevTask = NULL;
        m_LastTaskID = bot_task::TASK_TYPE_NONE;
        m_LastTaskState = bot_task::TASK_STATE_INACTIVE;

        // Reset our desire to throw grenades
        m_WillThrowGrenades = x_frand( 0.0f, 1.0f ) > (1.0f - m_Difficulty);

        // Reset our desire to antagonize deployed turrets
        m_AntagonizingTurrets = x_frand( 0.0f, 1.0f) > (1.0f - m_Difficulty);

        // Reset our stuck-move horizontal direction
        m_StuckMoveX = (x_irand( 0, 100 ) > 50) ? 1.0f : -1.0f;
    }
}

//==============================================================================

void bot_object::Player_Setup_DEAD( void )
{
    ASSERT(m_MAIIndex >= -1 && m_MAIIndex < 33);

    // Bot on server?
    if (tgl.ServerPresent)
    {
        if (m_CurTask  && m_MAIIndex != -1)
        {
            m_CurTask->UnassignBot(m_MAIIndex);

            if (GameMgr.GetGameType() != GAME_CAMPAIGN) 
            {
            // Campaign will not reevaluate target, task may be shared w/others.
                m_CurTask->SetTargetObject(NULL);

                if (GameMgr.GetGameType() == GAME_CTF ||
                    GameMgr.GetGameType() == GAME_CNH )
                {
                    MAI_Mgr.OffenseDied(m_TeamBits, m_MAIIndex);
                }
            }
        }
        m_CurrentGoal.SetGoalID( bot_goal::ID_NO_GOAL );
        m_CurTask = NULL;
        m_PrevTask = NULL;

        m_LastTaskID = bot_task::TASK_TYPE_NONE;
        m_LastTaskState = bot_task::TASK_STATE_INACTIVE;
    }

    player_object::Player_Setup_DEAD ( );
}

//==============================================================================

void bot_object::OnAdd( void )
{
    player_object::OnAdd();
    SetBotDifficulty( fegl.ServerSettings.BotsDifficulty );
}

//==============================================================================

void bot_object::OnRemove( void )
{
    player_object::OnRemove();
}

//==============================================================================

void  bot_object::RunEditorPath( const vector3& Src
    , const vector3& Dst )
{
    m_RunningEditorPath = TRUE;
    m_WorldPos = Src + vector3( 0.0f, 0.2f, 0.0f );
    m_Vel = vector3( 0.0f, 0.0f, 0.0f );
    InitializeGoTo( Dst );
}

void  bot_object::RunEditorPath( const xarray<u16>& PathPointIdxs )
{
    m_PathKeeper.ForceEditorPath( PathPointIdxs );
    m_RunningEditorPath = TRUE;
    m_WorldPos = g_Graph.m_pNodeList[PathPointIdxs[0]].Pos;
    m_Vel = vector3( 0.0f, 0.0f, 0.0f );
    m_CurrentGoal.SetGoalID( bot_goal::ID_GO_TO_LOCATION );
    m_CurrentGoal.SetTargetLocation(
        g_Graph.m_pNodeList[PathPointIdxs[PathPointIdxs.GetCount() - 1]].Pos );
    m_PrevTargetLocation = m_WorldPos;
}



void bot_object::DrawLine( const vector3& From, const vector3& To, xcolor Color )
{
    line Line;
    Line.From = From;
    Line.To = To;
    Line.Color = Color;
    m_Lines.Append( Line );
}

void bot_object::DrawSphere( const vector3& Pos, f32 Radius, xcolor Color )
{
    sphere Sphere;
    Sphere.Pos = Pos;
    Sphere.Radius = Radius;
    Sphere.Color = Color;
    m_Spheres.Append( Sphere );
}

void bot_object::DrawMarker( const vector3& Pos, xcolor Color )
{
    marker Marker;
    Marker.Pos = Pos;
    Marker.Color = Color;
    m_Markers.Append( Marker );
}
s32 ClosestEdge = 0;
void bot_object::Render( void )
{
    player_object::Render();
    s32 i;

    const s32 EdgeIdx = g_BotDebug.DrawGraphEdgeIndex;
    if (  EdgeIdx > -1 && EdgeIdx < g_Graph.m_nEdges )
    {
        const graph::edge Edge = g_Graph.m_pEdgeList[EdgeIdx];
        const graph::node NodeA = g_Graph.m_pNodeList[Edge.NodeA];
        const graph::node NodeB = g_Graph.m_pNodeList[Edge.NodeB];
        s32 Color = XCOLOR_BLACK;

        switch ( Edge.Type )
        {
        case graph::EDGE_TYPE_GROWN:
            Color = XCOLOR_GREEN;
            break;
        case graph::EDGE_TYPE_AEREAL:
            Color = XCOLOR_BLUE;
            break;
        case graph::EDGE_TYPE_SKI:
            Color = XCOLOR_YELLOW;
            break;
        default:
            break;
        }
        
        draw_Line( NodeA.Pos, NodeB.Pos, Color );
    }

    for ( i = 0; i < m_Lines.GetCount(); ++i )
    {
        const line& Line = m_Lines[i];
        draw_Line( Line.From, Line.To, Line.Color );
    }
    
    for ( i = 0; i < m_Spheres.GetCount(); ++i )
    {
        const sphere& Sphere = m_Spheres[i];
        draw_Sphere( Sphere.Pos, Sphere.Radius, Sphere.Color );
    }
    
    for ( i = 0; i < m_Markers.GetCount(); ++i )
    {
        const marker& Marker = m_Markers[i];
        draw_Marker( Marker.Pos,Marker.Color );
    }

    // MAI Debug Stuff
    char TaskLabelStr[3] = "";
    char TaskStateLabelStr[30] = "";
    char GoalLabelStr[30] = "";
    char ObjIDLabelStr[30] = "";
    if ( g_BotDebug.DrawMasterAITaskLabels )
    {
        if (m_CurTask && g_DrawBotTasks)
        {
            x_strcpy( TaskLabelStr
                , xfs( "%c\n", m_CurTask->GetTaskName()[2] ) );
        }
    }
//    #if defined (acheng)
//    if (m_PathKeeper.IsUsingPartialPath())
//    {
//        draw_Marker(m_PathKeeper.GetPartialPathTarget(), XCOLOR_RED);
//    }
//    if (this == ExamineBot && !IsDead())
//    {
//        draw_BBox( this->GetBBox(), XCOLOR_BLUE );
//    }
//    #endif
    
    if (g_BotDebug.ShowDeployLines)
    {
        if (m_CurTask &&
            (m_CurTask->GetID() == bot_task::TASK_TYPE_DEPLOY_TURRET
            || m_CurTask->GetID() == bot_task::TASK_TYPE_DEPLOY_SENSOR
            || m_CurTask->GetID() == bot_task::TASK_TYPE_DEPLOY_INVEN ))
        {
            draw_Line(m_WorldPos, m_CurrentGoal.GetAuxLocation(), XCOLOR_WHITE);
        }
}

    if ( m_CurTask && g_BotDebug.DrawMasterAITaskStateLabels && m_CurTask )
    {
        x_strcpy( TaskStateLabelStr
            , xfs( "%s\n", m_CurTask->GetTaskStateName() ) );
    }

    if ( g_BotDebug.DrawCurrentGoalLabels )
    {
        x_strcpy( GoalLabelStr, xfs( "%s/%5.2f\n", ((const char*)m_CurrentGoal.GetGoalString()), m_Difficulty ) );
    }

    if ( g_BotDebug.DrawObjectIdLables )
    {
        x_strcpy( ObjIDLabelStr, xfs( "%d/%d\n", m_ObjectID.Slot, m_ObjectID.Seq ) );
    }

    draw_Label(m_WorldPos + vector3( 0, 2, 0 ), XCOLOR_WHITE, "%s%s%s%s"
        , TaskLabelStr
        , TaskStateLabelStr
        , GoalLabelStr
        , ObjIDLabelStr );

    if ( g_BotDebug.DrawGraph )
    {
#if SHOW_CLOSEST_EDGE
    const view&     View        = *eng_GetView(0);
    const vector3   ViewPos         = View.GetPosition();
        if (g_BotDebug.DrawClosestEdge)
        {
            ClosestEdge = m_PathKeeper.GetClosestEdge(ViewPos);
        }
#endif
        g_Graph.Render( g_BotDebug.DrawGraphUseZBuffer);
    }
    
#if BOT_UPDATE_TIMING
    s32 Second = (s32)g_BotTiming.Timer.ReadSec();

    if (Second != g_BotTiming.LastSec)
    {
        g_BotTiming.LastSec = Second;
        g_BotTiming.nLastCalls = g_BotTiming.nCalls;
        g_BotTiming.nCalls = 0;
        g_BotTiming.LastTime = g_BotTiming.UpdateTimer.ReadMs();
        g_BotTiming.UpdateTimer.Reset();

        s32 i;
        for (i = 0; i < 16; i++)
        {
            if (!g_BotTimingArray[i]) continue;
        g_BotTimingArray[i]->LastSec = Second;
        g_BotTimingArray[i]->nLastCalls = g_BotTimingArray[i]->nCalls;
        g_BotTimingArray[i]->nCalls = 0;
        g_BotTimingArray[i]->LastTime = g_BotTimingArray[i]->UpdateTimer.ReadMs();
        g_BotTimingArray[i]->UpdateTimer.Reset();
        }
    }

    if ( g_BotDebug.ViewFPS && g_BotTiming.nLastCalls)
    {
        x_printfxy(0, 15, "Updates per sec: %d", g_BotTiming.nLastCalls);
        x_printfxy(0, 16, "     Total time: %4.2f", g_BotTiming.LastTime);
        x_printfxy(0, 17, "Avg Time / call: %4.2f", g_BotTiming.LastTime/g_BotTiming.nLastCalls);

        if (g_BotDebug.ViewFPSPB)
        {
            for (i = 0; i < 16; i++)
            {
                if (!g_BotTimingArray[i]) continue;
                x_printfxy(0, 19+i, "Bot%d U/S: %d", i, g_BotTimingArray[i]->nLastCalls);
                x_printfxy(20, 19+i,"TT: %4.2f", g_BotTimingArray[i]->LastTime);
                x_printfxy(40, 19+i,"Avg: %4.2f", g_BotTimingArray[i]->LastTime/g_BotTimingArray[i]->nLastCalls);
                x_printfxy(55, 19+i, "Goal: %s", (const char*)(xfs( ((bot_object*)(g_BotTimingArray[i]->pBot))->m_CurrentGoal.GetGoalString() ) ));
//                x_printfxy(85, 19+i, "Trbld: %d", ( ((bot_object*)(g_BotTimingArray[i]->pBot))->m_PathKeeper.m_nTroubleEdges ) );

            }
        }
    }
#endif
}


//==============================================================================

void    bot_object::LoadGraph           (const char* pFilename)
{
    m_PathKeeper.LoadGraph(pFilename);
    
}

//==============================================================================

s32    bot_object::GetBotID            ( void )
{
    return m_PathKeeper.GetBotID();
}

//==============================================================================
void bot_object::ApplyCheatPhysics( void )
{
    // Apply 
    if (m_CheatDeltaVelocity != vector3(0,0,0))
    {
        m_Vel += m_CheatDeltaVelocity;
        m_DirtyBits |= PLAYER_DIRTY_BIT_VEL;
    
        // Truncate ready for sending...
        Truncate() ;
    }
}

//==============================================================================
// InsideBuilding
//
// Returns TRUE if the bot is inside a building
//==============================================================================

xbool bot_object::InsideBuilding( void ) const
{
    return InsideBuilding( m_WorldPos );
}

//==============================================================================
// InsideBuilding
//
// Returns TRUE if the point is inside a building
//==============================================================================

xbool bot_object::InsideBuilding( const vector3& Point ) const
{
    xbool bInsideBuilding = FALSE;
    building_obj* pBuilding = NULL;

    ObjMgr.StartTypeLoop( object::TYPE_BUILDING );
    while( (pBuilding = (building_obj*)ObjMgr.GetNextInType()) )
    {
        if( pBuilding->GetBBox().Intersect( Point ) )
        {
            bInsideBuilding = TRUE;
            break;
        };
    }
    ObjMgr.EndTypeLoop();

#ifdef nmreed
        static s32 LastFrameCalled = 0;
        static s32 nCallsThisFrame = 0;
        static s32 TotalCalls = 0;
        static s32 TotalFrames = 0;
        static s32 MaxCallsPerFrame = 0;
    

        if ( LastFrameCalled != g_BotFrameCount )
        {
            TotalFrames += (g_BotFrameCount - LastFrameCalled);
            LastFrameCalled = g_BotFrameCount;
            TotalCalls += nCallsThisFrame;

            if ( nCallsThisFrame > MaxCallsPerFrame )
            {
                MaxCallsPerFrame = nCallsThisFrame;
            }
        
            nCallsThisFrame = 0;

            if ( TotalFrames % 100 == 0 )
            {
                const f32 Avg = (f32)TotalCalls / (f32)TotalFrames;
                x_printf( "InsideBuilding Max:%i Avg:%5.2f\n", MaxCallsPerFrame, Avg );
            }
        }
        else
        {
            ++nCallsThisFrame;
        }
#endif
    
    if ( bInsideBuilding
        && PointsCanSeeEachOther(
            Point, Point + vector3( 0.0f, 100.0f, 0.0f ), ATTR_BUILDING ) )
    {
        // not really inside, just in the bbox
        bInsideBuilding = FALSE;
    }

    return bInsideBuilding;
}

//==============================================================================
// GetElevationToPos()
//
// Returns the radian angle representing the vertical elevation to
// the given position
//==============================================================================
f32 bot_object::GetElevationToPos( const vector3& Pos ) const
{
    vector3 Dir = Pos - m_WorldPos;
    Dir.Normalize();

    return vector3( 0.0f, 1.0f, 0.0f ).Dot( Dir );
}

//==============================================================================
// CanSeePoint()
//
// Returns TRUE if bot can see the given point, FALSE if not.
//==============================================================================
xbool bot_object::CanSeePoint( const vector3& Point ) const
{
#ifdef nmreed
        static s32 LastFrameCalled = 0;
        static s32 nCallsThisFrame = 0;
        static s32 TotalCalls = 0;
        static s32 TotalFrames = 0;
        static s32 MaxCallsPerFrame = 0;
    

        if ( LastFrameCalled != g_BotFrameCount )
        {
            TotalFrames += (g_BotFrameCount - LastFrameCalled);
            LastFrameCalled = g_BotFrameCount;
            TotalCalls += nCallsThisFrame;

            if ( nCallsThisFrame > MaxCallsPerFrame )
            {
                MaxCallsPerFrame = nCallsThisFrame;
            }
        
            nCallsThisFrame = 0;

            if ( TotalFrames % 100 == 0 )
            {
                const f32 Avg = (f32)TotalCalls / (f32)TotalFrames;
                x_printf( "CanSeePoint Max:%i Avg:%5.2f\n", MaxCallsPerFrame, Avg );
            }
        }
        else
        {
            ++nCallsThisFrame;
        }
#endif
    
    // Adjust slightly to account for the bot's height -- get to eye level
    const vector3 SourcePos = m_WorldPos + vector3( 0.0f, 1.0f, 0.0f );

    return PointsCanSeeEachOther( SourcePos, Point, ATTR_SOLID_STATIC );
}

//==============================================================================
// PointsCanSeeEachOther()
//==============================================================================
xbool bot_object::PointsCanSeeEachOther( const vector3& Point1
    , const vector3& Point2
    , u32 AttrMask ) const
{
    // Points too close?
    if ( (Point1 - Point2).LengthSquared() < collider::MIN_RAY_DIST_SQR)
        return TRUE ;

    // SteveB : Speedup - used line of sight ray
    collider            Ray;
    Ray.RaySetup    ( NULL, Point1, Point2, m_ObjectID.GetAsS32(), TRUE );
    ObjMgr.Collide  ( AttrMask, Ray );
    return (Ray.HasCollided() == FALSE) ;

    //collider::collision DummyCollision;
    //return PointsCanSeeEachOther( Point1, Point2, AttrMask, DummyCollision );
}

//==============================================================================
// PointsCanSeeEachOther()
//==============================================================================
xbool bot_object::PointsCanSeeEachOther( const vector3& Point1
    , const vector3& Point2
    , u32 AttrMask
    , collider::collision& Collision ) const
{
    xbool RetVal = FALSE;
    
    if ( (Point1 - Point2).LengthSquared() < collider::MIN_RAY_DIST_SQR )
    {
        RetVal = TRUE;
    }
    else
    {

#ifdef nmreed
        static s32 LastFrameCalled = 0;
        static s32 nCallsThisFrame = 0;
        static s32 TotalCalls = 0;
        static s32 TotalFrames = 0;
        static s32 MaxCallsPerFrame = 0;
    

        if ( LastFrameCalled != g_BotFrameCount )
        {
            TotalFrames += (g_BotFrameCount - LastFrameCalled);
            LastFrameCalled = g_BotFrameCount;
            TotalCalls += nCallsThisFrame;

            if ( nCallsThisFrame > MaxCallsPerFrame )
            {
                MaxCallsPerFrame = nCallsThisFrame;
            }
        
            nCallsThisFrame = 0;

            if ( TotalFrames % 100 == 0 )
            {
                const f32 Avg = (f32)TotalCalls / (f32)TotalFrames;
                x_printf( "PointsCanSeeEachOther Max:%i Avg:%f\n", MaxCallsPerFrame, Avg );
            }
        }
        else
        {
            ++nCallsThisFrame;
        }
#endif
    
        collider            Ray;
        Ray.RaySetup    ( NULL, Point1, Point2, m_ObjectID.GetAsS32() );
        ObjMgr.Collide  ( AttrMask, Ray );

        if ( Ray.HasCollided() == TRUE )
        {
            Ray.GetCollision( Collision );

            // Mike - Since the collision point is always on the
            //        line between the two points, this chunk of code
            //        will always set RetVal to FALSE 

            // see if the collision point is beyond the point we want to see
            //const f32 DistSQ = (Point2 - Point1).LengthSquared();
            //const f32 CollisionDistSQ
                //= (Point1 - Collision.Point).LengthSquared();
            //RetVal =  CollisionDistSQ > DistSQ;
            RetVal = FALSE ;
        }
        else
        {
            RetVal = TRUE;
        }
    }

    return RetVal;
}

//==============================================================================
void bot_object::SetMove( const player_object::move& Move )
{
    m_Move = Move;
    m_DontClearMove = TRUE;
}

//==============================================================================
// GetTargetCenter()
//==============================================================================
vector3 bot_object::GetTargetCenter( const object& Target )
{
    const bbox& BBox = Target.GetBBox();
    const vector3 Center( (BBox.Min + BBox.Max) * 0.5f );

    return Center;
}

//======================================================================
// PickPointInRadius()
//
// Given a center and a radius, randomly produce a valid location
// within that radius for a bot to travel to. If Outdoors, the point will
// not be inside a building, otherwise it will 
//======================================================================
vector3 bot_object::PickPointInRadius(
    const vector3& Center
    , f32 Radius
    , xbool Outdoors /* = FALSE */ ) const
{
    f32 RadiusSQ = x_sqr( Radius );
    s32 i,j;
    static const s32 MAX_VALID_NODES = 256;
    s32 NodeIds[MAX_VALID_NODES];
    s32 NumValidNodes = 0;

    for ( i = 0; i < g_Graph.m_nEdges; ++i )
    {
        const graph::edge& CurEdge = g_Graph.m_pEdgeList[i];

        if ( CurEdge.Type == graph::EDGE_TYPE_GROWN )
        {
            const vector3& A = g_Graph.m_pNodeList[CurEdge.NodeA].Pos;
            const vector3& B = g_Graph.m_pNodeList[CurEdge.NodeB].Pos;

            if ( (( A - Center).LengthSquared() <= RadiusSQ)
                && (!Outdoors == g_Graph.m_pNodeInside[CurEdge.NodeA]) )
            {
                for ( j = 0; j < NumValidNodes; ++j )
                {
                    if ( NodeIds[j] == CurEdge.NodeA )
                    {
                        break;
                    }
                }
                    
                if ( j == NumValidNodes )
                {
                    NodeIds[NumValidNodes] = CurEdge.NodeB;
                    ++NumValidNodes;
                }
                    
            }

            if ( (( B - Center).LengthSquared() <= RadiusSQ)
                && (!Outdoors == g_Graph.m_pNodeInside[CurEdge.NodeB]) )
            {
                for ( j = 0; j < NumValidNodes; ++j )
                {
                    if ( NodeIds[j] == CurEdge.NodeB )
                    {
                        break;
                    }
                }
                    
                if ( j == NumValidNodes )
                {
                    NodeIds[NumValidNodes] = CurEdge.NodeB;
                    ++NumValidNodes;
                }
                    
            }
                
        }
        ASSERT( NumValidNodes <= MAX_VALID_NODES );
    }
        
    //--------------------------------------------------
    // Pick a node
    //--------------------------------------------------
    const s32 NodeId = (NumValidNodes > 0) ? NodeIds[x_irand( 0, NumValidNodes - 1 )] : 0;
    const graph::node& Node = g_Graph.m_pNodeList[NodeId];
    return Node.Pos;
}

object* bot_object::GetDangerousTurret( void )
{
    f32 ClosestDistSQ = MAX_DANGEROUS_TURRET_RANGE_SQ;
    turret* pTurret;
    turret* pClosestTurret = NULL;
    collider::collision Collision;
    
    ObjMgr.Select( object::ATTR_DAMAGEABLE
        , m_WorldPos
        , MAX_DANGEROUS_TURRET_RANGE );
    while( (pTurret = (turret*)(ObjMgr.GetNextSelected())) )
    {
        const object::type Type = pTurret->GetType();
        if( (Type != object::TYPE_TURRET_FIXED)  &&  
            (Type != object::TYPE_TURRET_SENTRY) &&  
            (Type != object::TYPE_TURRET_CLAMP) )
        {
            continue;
        }
        
        const f32 DistSQ
            = (pTurret->GetPosition() - m_WorldPos).LengthSquared();
        
#ifdef nmreed
        if ( pTurret->GetKind() != turret::DEPLOYED
            && DistSQ < ClosestDistSQ )
        {
            static s32 LastFrameCalled = 0;
            static s32 nCallsThisFrame = 0;
            static s32 TotalCalls = 0;
            static s32 TotalFrames = 0;
            static s32 MaxCallsPerFrame = 0;

            if ( LastFrameCalled != g_BotFrameCount )
            {
                TotalFrames += (g_BotFrameCount - LastFrameCalled);
                LastFrameCalled = g_BotFrameCount;
                TotalCalls += nCallsThisFrame;

                if ( nCallsThisFrame > MaxCallsPerFrame )
                {
                    MaxCallsPerFrame = nCallsThisFrame;
                }
        
                nCallsThisFrame = 0;

                if ( TotalFrames % 100 == 0 )
                {
                    const f32 Avg = (f32)TotalCalls / (f32)TotalFrames;
                    x_printf( "GetDangerousTurret Max:%i Avg:%5.2f\n", MaxCallsPerFrame, Avg );
                }
            }
            else
            {
                ++nCallsThisFrame;
            }
        }
#endif
    
        if ( pTurret->GetKind() != turret::DEPLOYED
            && DistSQ < ClosestDistSQ
            && (PointsCanSeeEachOther(
                GetWeaponPos()
                , pTurret->GetPosition(), ATTR_SOLID_STATIC, Collision )
                || (((object*)Collision.pObjectHit)->GetObjectID()
                    == pTurret->GetObjectID())
                || (((object*)Collision.pObjectHit)->GetObjectID()
                    == GetObjectID())) )
        {
            ClosestDistSQ = DistSQ;
            pClosestTurret = pTurret;
        }
    }
    ObjMgr.ClearSelection();

    return pClosestTurret;
}

void bot_object::SetBotDifficulty ( f32 Difficulty )
{
    ASSERT( Difficulty >= 0.0f && Difficulty <= 1.0f );
    m_Difficulty = Difficulty;
}

void bot_object::ToggleArmor( void )
{
    if ( m_Loadout.Armor == ARMOR_TYPE_LIGHT )
    {
        loadout Loadout;
        default_loadouts::GetLoadout(
            GetCharacterType()
            , default_loadouts::STANDARD_BOT_HEAVY
            , Loadout );
        SetInventoryLoadout( Loadout );
        InventoryReload();

    }
    else
    {
        loadout Loadout;
        default_loadouts::GetLoadout(
            GetCharacterType()
            , default_loadouts::STANDARD_BOT_LIGHT
            , Loadout );
        SetInventoryLoadout( Loadout );
        InventoryReload();
    }
}


f32 bot_object::GetMaxAir()
{
    static const f32 MaxValue[3][3] = 
    {   // Standard,    Energy Pack,    Inven
//        {73.0f,         166.0f,         -1.0f,}, // Light
//        {55.0f,         99.0f,          30.0f,}, // Med
//        {34.0f,         52.0f,          16.0f,}, // Heavy
        {125.0f,         200.0f,         -1.0f,}, // Light
        {75.0f,          125.0f,         30.0f,}, // Med
        {35.0f,           70.0f,         16.0f,}, // Heavy
    };

    if (m_Loadout.Inventory.Counts[player_object::INVENT_TYPE_ARMOR_PACK_ENERGY])
        return MaxValue[(s32)m_ArmorType][1];
    else
    if (m_Loadout.Inventory.Counts[player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION])
        return MaxValue[(s32)m_ArmorType][2];
    else
        return MaxValue[(s32)m_ArmorType][0];
}

//==============================================================================

void bot_object::Speak( s32 SfxID )
{
    return;

    SfxID = GetVoiceSfxBase() + (SfxID & 0x7fffffff);
//    PlayVoiceMenuSound( SfxID, m_TeamBits, 0 );
}

//==============================================================================


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// old code...

#if 0


void bot_object::PursueLocation(
    const vector3& TargetLocation
    , const vector3& NextTargetLocation
    , xbool Arrive /* = false */ )


    
    (void)NextTargetLocation;

#if PRINT_PURSUE_INFO
    x_printf( "Move State: %s\n"
        , m_MoveState == MOVE_STATE_DEFAULT
        ? "MOVE_STATE_DEFAULT"
        : m_MoveState == MOVE_STATE_WALK
        ? "MOVE_STATE_WALK"
        : m_MoveState == MOVE_STATE_HOVER
        ? "MOVE_STATE_HOVER"
        : m_MoveState == MOVE_STATE_FLY
        ? "MOVE_STATE_FLY"
        : "?" );
#endif

    //----------------------------------------------------------------------
    // Make sure we stay on the path we're following,
    // if we are following a path
    //----------------------------------------------------------------------
    vector3 PathSteering( 0.0f, 0.0f, 0.0f );

    if ( !Arrive
        && (TargetLocation - NextTargetLocation).LengthSquared() > 1.0f )
    {
        GetSteeringForPathFollowing(
            PathSteering
            , TargetLocation
            , NextTargetLocation );
    }
        
    //----------------------------------------------------------------------
    // We're trying to arrive at the location, not fly past it. We're using
    // the "Arrival Behavior" described here:
    // http://www.red3d.com/cwr/steer/gdc99/index.html
    //
    // In short:
    // A desired velocity is determined pointing from the character towards
    // the target. Inside the stopping radius, desired velocity is ramped
    // down (e.g. linearly) to zero.
    //----------------------------------------------------------------------

    vector3 DeltaTarget = TargetLocation - m_WorldPos;

    if ( DeltaTarget.LengthSquared() <= MOVE_THRESHOLD_SQ ) 
    {
        return;
    }
    
    const f32 Distance = DeltaTarget.Length();
    const f32 RampedSpeed = Arrive
        ? (ARRIVAL_MAX_SPEED
            * (Distance / ARRIVAL_SLOWING_DISTANCE))
        : ( F32_MAX );
    const f32 CurrentSpeed = m_Vel.Length();
    const f32 ClippedSpeed = MIN(
        RampedSpeed, MAX( CurrentSpeed, ARRIVAL_MAX_SPEED ) );
    vector3 DesiredVelocity = ( ClippedSpeed / Distance ) * DeltaTarget;
    vector3 Steering = PathSteering + DesiredVelocity - m_Vel;

    if ( PathSteering.Y < 0.0f )
    {
        // We need to get down to stay on track, so let's cheat to
        // do it.
        m_UseDownJets = TRUE;
    }
    else 
    {
        m_UseDownJets = FALSE;

        if ( PathSteering.Y > 0.0f )
        {
            m_Move.JetKey = TRUE;
        }
    }

#define DRAW_DEBUG_LINES 0
#if DRAW_DEBUG_LINES
    //Draw some debug lines
    const vector3& start = m_WorldPos + vector3( 0, 2.0f, 0 );

    DrawLine( start, start + Steering, XCOLOR_BLUE );
    DrawLine( start, start + m_Vel, XCOLOR_RED );
    DrawLine( start, start + DesiredVelocity, XCOLOR_WHITE );
#endif

    DeltaTarget.RotateY( -m_Rot.Yaw );
    DesiredVelocity.RotateY( -m_Rot.Yaw );
    Steering.RotateY( -m_Rot.Yaw );

    //----------------------------------------------------------------------
    // Decide move direction
    //----------------------------------------------------------------------

    // left-right
    if ( ABS( DeltaTarget.X ) > MOVE_THRESHOLD )
    {
        m_Move.MoveX = Steering.X;
        if ( m_Move.MoveX < -1 ) 
        {
            m_Move.MoveX = -1;
        }
        else if ( m_Move.MoveX > 1 ) 
        {
            m_Move.MoveX = 1;
        }
    }

    // back-forth
    if ( ABS( DeltaTarget.Z ) > MOVE_THRESHOLD )
    {
        m_Move.MoveY = Steering.Z;
        if ( m_Move.MoveY < -1 ) 
        {
            m_Move.MoveY = -1;
        }
        else if ( m_Move.MoveY > 1 ) 
        {
            m_Move.MoveY = 1;
        }
    }

    //----------------------------------------------------------------------
    // Decide if we should be using jets
    //----------------------------------------------------------------------

    vector3 FlatDeltaTarget = DeltaTarget;
    FlatDeltaTarget.Y = 0.0f;

    vector3 FlatDesired = DesiredVelocity;
    FlatDesired.Y = 0.0f;
    const f32 FlatDesiredLength = FlatDesired.Length();

    vector3 FlatVelocity = m_Vel;
    FlatVelocity.Y = 0.0f;

    const f32 FlatVelocityLength = FlatVelocity.Length();

    // This is to stay arial if it makes sense to do so
    if ( !Arrive
        && DeltaTarget.Y < 0 )
    {
        // Just stay up here. Set our target either to the minimum of
        // our current height and horizontal distance from the target,
        // which will allow us to approach the final target
        const f32 TargetHeight = MIN(
            FlatDeltaTarget.Length() + TargetLocation.Y
            , m_WorldPos.Y );

        DeltaTarget.Y = (TargetHeight - 1.0f) - m_WorldPos.Y;
    }

    switch ( m_MoveState )
    {
    case bot_object::MOVE_STATE_DEFAULT:
        m_MoveState = bot_object::MOVE_STATE_WALK;
        break;
    case bot_object::MOVE_STATE_WALK:
        if ( m_Energy >= 100.0f )
        {
            // if we have the juice, are walking in the right direction,
            // are around the same altitude, are far
            // away from our target, then start hovering
            if (
                // have the juice
                m_Energy >= 100.0f  

                // are walking in the right direction or going up a steep
                && (GoingUpSteepHill()
                    || (FlatVelocity.Dot( FlatDesired )
                        / (FlatVelocityLength * FlatDesiredLength)
                        >= COS_45))

                // are moving so we don't have to accelerate in the air
                && FlatVelocityLength >= MIN_FLAT_VELOCITY_TO_START_HOVERING

                // are around the same altitude
                && (ABS( DeltaTarget.Y ) <= HOVER_TARGET_ALTITUDE_RANGE )

                // are far enough away
                && (!Arrive || FlatDeltaTarget.LengthSquared()
                    >= HOVER_TARGET_RANGE_SQ) )
            {
                m_MoveState = bot_object::MOVE_STATE_HOVER;
                m_Move.JumpKey = TRUE;
                m_Move.JetKey = TRUE;
            }
            // if we're way below our target, and close enough to it,
            //start flying
            else if ( DeltaTarget.Y >= MIN_FLY_DIFFERENCE
                && FlatDeltaTarget.LengthSquared() <= MIN_FLY_RANGE_SQ )
            {
                m_MoveState = bot_object::MOVE_STATE_FLY;
                m_Move.JumpKey = TRUE;
                m_Move.JetKey = TRUE;
            }
            else {
                vector3 NormalVel = m_Vel;
                NormalVel.Normalize();
                vector3 NormalSteering = Steering;
                Steering.Normalize();

                if ( !m_Flags.OnGround
                    && NormalVel.Dot( NormalSteering ) < COS_45 )
                {
                    m_Move.JetKey = TRUE;
                }
            }
        }
        
        break;
    case bot_object::MOVE_STATE_HOVER:
    {
        const xbool goingUpSteepHill = GoingUpSteepHill();
        
        // if we're out of juice, or getting close, or too far
        // below, start walking
        if ( m_Energy <= 0.0f
            || (!Arrive || FlatDeltaTarget.LengthSquared()
                <= HOVER_TARGET_RANGE_SQ)
            || (DeltaTarget.Y < -HOVER_TARGET_ALTITUDE_RANGE
                && !goingUpSteepHill) )
        {
            m_MoveState = bot_object::MOVE_STATE_WALK;
        }
        // if we're below our hover height, or we're going up a steep hill,
        // and we're not going too fast, then jet
        else if ( goingUpSteepHill
            || (-DeltaTarget.Y <= HOVER_MAX_HEIGHT_OVER_TARGET
                && ABS( m_Vel.Y ) < MAX_HOVER_VERT_SPEED) )
        {
            m_Move.JetKey = TRUE;

            // Try not to move straight into a hil
            if ( goingUpSteepHill && m_Move.MoveY > 0.0f ) 
            {
                m_Move.MoveY *= 0.1f;
            }
            
        }
        // otherwise, just coast...
        else 
        {
        }
        
        break;
    }
    
    case bot_object::MOVE_STATE_FLY:
    {
        // Try to vertically "arrive" at the target (as described above)
        const f32 VertDistance = ABS( DeltaTarget.Y );
        const f32 VertRampedSpeed = (Arrive)
            ? (ARRIVAL_MAX_SPEED
                * (VertDistance / ARRIVAL_SLOWING_DISTANCE))
            : (F32_MAX);
        const f32 VertCurrentSpeed = ABS( m_Vel.Y );
        const f32 VertClippedSpeed = MIN(
            VertRampedSpeed, MAX( VertCurrentSpeed, ARRIVAL_MAX_SPEED ) );
        const f32 VertDesiredVelocity = ( VertClippedSpeed / VertDistance )
            * DeltaTarget.Y;
        const f32 VertSteering = VertDesiredVelocity - m_Vel.Y;

        if ( VertSteering > 0.0f
            && m_Energy > 1.0f ) 
        {
            m_Move.JetKey = TRUE;

            if ( GoingUpSteepHill() ) 
            {
                m_Move.MoveY *= 0.2f;
            }
        }
        else
        {
            // don go up no mo
            m_MoveState = bot_object::MOVE_STATE_WALK;
        }

        break;
    }
    
    default:
        break;
    }

    // just in case we're falling too fast...
    if ( GetLandDamage() > 0.0f )
    {
        m_Move.JetKey = TRUE;
    }
}

void bot_object::GetSteeringForPathFollowing(
    vector3& PathSteering
    , const vector3& P1
    , const vector3& P2 ) const
{
    ASSERT( (P1-P2).Length() > 0.01f );
    //----------------------------------------------------------------------
    // predict where we'll be in a certain amount of time
    //----------------------------------------------------------------------
    const vector3& FuturePos = m_WorldPos
        + m_Vel * PATH_FOLLOWING_PREDICTION_TIME;

    //----------------------------------------------------------------------
    // compute how far FuturePos is from the line
    //----------------------------------------------------------------------
    const vector3& PathSeg = P2 - P1;
    const vector3& DeltaFuturePos = FuturePos - m_WorldPos;
    const f32 PathDist = PathSeg.Dot( DeltaFuturePos );
    const f32 Displacement = ABS( DeltaFuturePos.Length() - PathDist );

    //----------------------------------------------------------------------
    // if we're too far away, then steer towards the point on the
    // path closest to our predicted location
    //----------------------------------------------------------------------
    if ( Displacement > PATH_FOLLOWING_RADIUS )
    {
        vector3 PathDir = PathSeg;
        PathDir.Normalize();
        const vector3& ClosestPoint = P1 + PathDir * PathDist;
        PathSteering = ClosestPoint - FuturePos;
        PathSteering *= PATH_FOLLOWING_WEIGHT;
    }
    else 
    {
        PathSteering.Zero();
    }
}

void bot_object::UpdateStevesBot( f32 DeltaTime )
{
    // Update the start y pos incase the bot gets stuck below something
    {
        vector3 Pos = m_WorldPos ;
        collider Ray ;
        Ray.RaySetup( this, Pos, Pos - vector3(0,100,0) ) ;
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray ) ;
    
        // Get results
        collider::collision Collision;
        Ray.GetCollision( Collision );
        if (Collision.Collided)
        m_StartPos.Y = Collision.Point.Y ;
    }

    // Choose another destination?
    m_DestTime -= DeltaTime ;
    if (m_DestTime < 0)
    {
        // Choose new random spot
        m_DestPos = m_StartPos + vector3(
            x_frand(-10.0f, 10.0f)
            , 0
            , x_frand(-10.0f, 10.0f)) ;
            
        // Move in the air?
        if (x_frand(0,1) > 0.8)
        m_DestPos.Y = m_StartPos.Y + x_frand(0.0f, 5.0f) ;

        // Time until next dest pos
        m_DestTime = 5.0f ;
    }

    // Choose a new target?
    m_TargetTime -= DeltaTime ;
    if (m_TargetTime < 0)
    {
        m_TargetTime = 10.0f ;

        // Players
        player_object *pPlayer ;
        ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
        while( (pPlayer = (player_object*)ObjMgr.GetNextInType()) )
        {
            // Just choose first player for now...
            m_TargetObject = pPlayer->GetObjectID() ;
            break ;
        }
        ObjMgr.EndTypeLoop();
    }

    // Get target to look at
    vector3 LookTarget ;
    if (m_TargetObject != obj_mgr::NullID)
    LookTarget = ObjMgr.GetObjectFromID(m_TargetObject)->GetBBox().GetCenter() ;
    else
    LookTarget = m_DestPos ;

    // Get look info
    vector3 DeltaTarget = LookTarget - m_WorldBBox.GetCenter() ;
    radian  Pitch = DeltaTarget.GetPitch() ;
    radian  Yaw   = DeltaTarget.GetYaw() ;
        
    // See if bot can see the target
    xbool CanSeeTarget = FALSE ;
    if (DeltaTarget.Length() > 0.1f)
    {
        collider Ray ;
        Ray.RaySetup( this, m_WorldBBox.GetCenter(), LookTarget ) ;
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray ) ;

        // Get results
        collider::collision Collision;
        Ray.GetCollision( Collision );
        CanSeeTarget = !Collision.Collided ;
    }

    // Set look left/right
    xbool CanFire = TRUE ;
    radian DeltaAngle = radian_DeltaAngle(m_Rot.Yaw, Yaw) ;
    if (ABS(DeltaAngle) > R_2)
    {
        CanFire = FALSE ;
        if (DeltaAngle > 0)
        m_Move.LookX = 0.4f ;
        else
        m_Move.LookX = -0.4f ;
    }

    // Set look up/down
    DeltaAngle = radian_DeltaAngle(m_Rot.Pitch, Pitch) ;
    if (ABS(DeltaAngle) > R_2)
    {
        CanFire = FALSE ;
        if (DeltaAngle > 0)
        m_Move.LookY = 0.2f ;
        else
        m_Move.LookY = -0.2f ;
    }

    // Update fire time
    if (m_FireTime > 0)
    {
        m_FireTime -= DeltaTime ;
        if (m_FireTime < 0)
        m_FireTime = 0 ;
    }

    // Set fire
    if ((CanFire) && (CanSeeTarget) && (m_FireTime == 0))
    {
        m_Move.FireKey = TRUE ;
        m_FireTime = 3 ;
    }

    // Get delta target in local axis aligned space
    DeltaTarget = m_DestPos - m_WorldPos ;
    DeltaTarget.RotateY(-m_Rot.Yaw) ;

    // Set jet
    if (DeltaTarget.Y > 1)
    m_Move.JetKey = TRUE;

    // Set move left/right
    xbool AtDest = TRUE ;
    if (ABS(DeltaTarget.X) > 0.2f)
    {
        AtDest        = FALSE ;
        m_Move.MoveX = DeltaTarget.X ;

        // Keep within input range
        if (m_Move.MoveX < -1)
        m_Move.MoveX = -1 ;
        else
        if (m_Move.MoveX > 1)
        m_Move.MoveX = 1 ;
    }

    // Set move forward/backwards
    if (ABS(DeltaTarget.Z) > 0.2f)
    {
        AtDest        = FALSE ;
        m_Move.MoveY = DeltaTarget.Z ;

        // Keep within input range
        if (m_Move.MoveY < -1)
        m_Move.MoveY = -1 ;
        else
        if (m_Move.MoveY > 1)
        m_Move.MoveY = 1 ;
    }
}


xbool bot_object::GoingUpSteepHill( void ) const 
{
    // send a short ray straight down and take a look
    // at the normal of what we hit, if we hit anything
    collider Ray ;
    Ray.RaySetup( this, m_WorldPos, m_WorldPos - vector3( 0.0f, 10.0f, 0.0f ) ) ;
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray ) ;
    
    // Get results
    collider::collision Collision;
    Ray.GetCollision( Collision );
    if (Collision.Collided)
    {
        // make sure it's steep and we're moving into it
        const f32 CosNormalAndUp
            = Collision.Plane.Normal.Dot( vector3( 0.0f, 1.0f, 0.0f ) );
        if ( CosNormalAndUp < COS_20 ) 
        {
            // it is steep, check if we're moving into it
            vector3 FlatNormal = Collision.Plane.Normal;
            FlatNormal.Y = 0.0f;

            vector3 FlatVelocity = m_Vel;
            FlatVelocity.Y = 0.0f;

            if ( FlatNormal.Dot( FlatVelocity )
                / (FlatNormal.Length() * FlatVelocity.Length())
                < 0.0f ) 
            {
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

#endif



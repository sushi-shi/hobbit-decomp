//==============================================================================
//  
//  HUNTER_MasterAI.cpp // Master AI for Hunter.
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "HunterMasterAI.hpp"
#include "Objects\Projectiles\Flag.hpp"
#include "Objects\Projectiles\Nexus.hpp"

#define LOCAL_RADIUS 200

#define LOCAL_RADIUS_SQ  (LOCAL_RADIUS * LOCAL_RADIUS)
#define DIST_TO_FORCE_GOTO_NEXUS 100
#define DIST_TO_FORCE_GOTO_NEXUS_SQ (DIST_TO_FORCE_GOTO_NEXUS*DIST_TO_FORCE_GOTO_NEXUS)


//==============================================================================
//  Storage
//==============================================================================
extern s32 TeamBitsFound;

//==============================================================================
//  HUNTER MASTER AI FUNCTIONS
//==============================================================================

hunter_master_ai::hunter_master_ai ( void )
{
    m_TeamBits = 0;
    Initialize();
}

//==============================================================================

hunter_master_ai::~hunter_master_ai ( void )
{
    UnloadMission();
}

//==============================================================================


void hunter_master_ai::UnloadMission( void )
{
    m_TeamBits = 0;
    m_HuntTask = NULL;

    m_MissionInitialized = FALSE;
}

//==============================================================================

void hunter_master_ai::UpdatePlayerPointers( void )
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

void hunter_master_ai::InitializeMission( void )
{
    ASSERT (!m_MissionInitialized);
    m_MissionInitialized = TRUE;

    // Set the team bit flag appropriately.
    UpdatePlayerPointers();

    m_nTasks = 1;

    m_GoalList[0].SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
    m_HuntTask = &m_GoalList[0];
    m_HuntTask->SetPriority(1);
    m_HuntTask->SetMaxBots(1);

    // To do:
    // Acquire the location of the nexus
    nexus* pNexus;
    s32 i = 0;
    ObjMgr.StartTypeLoop(object::TYPE_NEXUS);
    while ( (pNexus = (nexus*)(ObjMgr.GetNextInType())) != NULL )
    {
        // Find the base nexus position.
        m_NexusPos = pNexus->GetPosition();
        collider Ray;
        Ray.RaySetup(NULL, m_NexusPos + vector3(0.0f, 4.0f, 0.0f),
                           m_NexusPos);
        ObjMgr.Collide(object::ATTR_PERMANENT, Ray);
    
        collider::collision Collision;
        Ray.GetCollision(Collision);

        m_NexusPos = Collision.Point + vector3(0.0f, 0.1f,0.0f);
        i++;
    }
    ObjMgr.EndTypeLoop();

    ASSERT(i==1);   // There should be one nexus on this board.

    // Pick a nexus return value for each bot.
    PickNexusValue();
}

//==============================================================================

void hunter_master_ai::PollAllObjects ( void )
{
    // Need to set the state and the target.
    ASSERT(m_MissionInitialized);

    // Is there anyone playing?
    if (m_nTeamBots == 0 || m_nOtherTeamPlayers == 0)
        return;

    // Everyone for themselves, right?
    ASSERT(m_nTeamBots == 1);

    // Examine the bot to see if we need to give a new nexus value.
    // This guy has some flags showing.
    if (m_TeamBot[0]->GetFlagCount() > 1)
    {
        m_RecalculateGotoValue = TRUE;
    }
    else 
    {
        // No flags.  Either he just died, or he just hit the nexus. 
        // Either way, if armed, reset the value and mark set.
        if (m_RecalculateGotoValue)
        {
            PickNexusValue();
        }
            // else if set, then do nothing.
    }
#if defined (acheng)
    x_printfxy(0, 15+m_TeamBot[0]->GetBotID(),"Bot #%i N val: %i, Curr: %i.", 
        m_TeamBot[0]->GetBotID(), m_GotoNexusValue, m_TeamBot[0]->GetFlagCount());
    x_printfxy(0, 14, "Player flags: %i", m_OtherTeamPlayer[0]->GetFlagCount());
#endif

    // Target any free flags in the local area.
    flag* pFlag, *pClosestFlag = NULL;

    // Select would be useful for this.  for now, find the best local flag
    //  using the old way.    
//    ObjMgr.Select(ATTR_FLAG, m_TeamBot[0]->GetPosition(), 25.0f);
//    while( (pFlag = (flag*)ObjMgr.GetNextSelected()) )
//    ObjMgr.ClearSelection();
    f32 DistSq, ClosestDistSq = F32_MAX;

    s32 SearchRadiusSq;
    if (m_TeamBot[0]->GetFlagCount() >= m_GotoNexusValue)
    {
        SearchRadiusSq = 200;
    }
    else
    {
        SearchRadiusSq = LOCAL_RADIUS_SQ;
    }

    vector3 vDist;
    m_nTotalFlags = 0;
    ObjMgr.StartTypeLoop( object::TYPE_FLAG );
    while( (pFlag = (flag *)(ObjMgr.GetNextInType())) != NULL )
    {
        m_nTotalFlags++;
        vDist = pFlag->GetPosition() - m_TeamBot[0]->GetPosition();
        
        DistSq = vDist.LengthSquared();

        if( DistSq < SearchRadiusSq )
                // Okay the flag is in the vicinity.  
                // Is it better than our current flag?
        {
            if( !pClosestFlag || (pFlag->GetValue() > pClosestFlag->GetValue()) )
                // First one, or, yes.
            {
                pClosestFlag  = pFlag;
                ClosestDistSq = DistSq;
            }
            else if( (pFlag->GetValue() == pClosestFlag->GetValue()) && (DistSq < ClosestDistSq) )
            {
                pClosestFlag  = pFlag;
                ClosestDistSq = DistSq;
            }
        }
    }
    ObjMgr.EndTypeLoop();

    //
    // Choose something to do from the following options...
    //

    if (pClosestFlag)
    {
        // Set this flag as our target.
        m_HuntTask->SetID(bot_task::TASK_TYPE_CAPTURE_FLAG);
        m_HuntTask->SetState(bot_task::TASK_STATE_PURSUE);
        m_HuntTask->SetTargetObject(NULL);
        m_HuntTask->SetTargetPos(pClosestFlag->GetPosition());
        m_HuntTask->SetPriority(100);
        return;
    }

    // No flags in the vicinity..  
    // Next check: time to return to nexus?
    if (m_TeamBot[0]->GetFlagCount() >= m_GotoNexusValue)
    {
        m_HuntTask->SetID(bot_task::TASK_TYPE_GOTO);
        m_HuntTask->SetState(bot_task::TASK_STATE_GOTO_POINT);
        m_HuntTask->SetTargetObject(NULL);
        m_HuntTask->SetTargetPos(m_NexusPos);
        m_HuntTask->SetPriority(100);
        return;
    }

    // Close enough to nexus?
    if (m_NexusPos.Difference(m_TeamBot[0]->GetPosition()) < DIST_TO_FORCE_GOTO_NEXUS_SQ)
    {
        if (m_TeamBot[0]->GetFlagCount() > 2)
        {
            m_HuntTask->SetID(bot_task::TASK_TYPE_GOTO);
            m_HuntTask->SetState(bot_task::TASK_STATE_GOTO_POINT);
            m_HuntTask->SetTargetObject(NULL);
            m_HuntTask->SetTargetPos(m_NexusPos);
            m_HuntTask->SetPriority(50);
            return;
        }
    }

    // Next check:  who's hording?
    //  Check who has all the flags, and engage him if he's hording..
    s32 HordingBot = FindFlagHorder();
    if( HordingBot != -1 )
    {
        m_HuntTask->SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
        m_HuntTask->SetState(bot_task::TASK_STATE_ATTACK);
        m_HuntTask->SetTargetObject(m_OtherTeamPlayer[HordingBot]);
        m_HuntTask->SetPriority(50);
        return;
    }

    // Do this if you can't find anything better to do.
    s32 ClosestEnemy = FindClosestEnemy(m_TeamBot[0]->GetPosition(), DistSq);

    if (ClosestEnemy != -1)
    {
        m_HuntTask->SetID(bot_task::TASK_TYPE_ENGAGE_ENEMY);
        m_HuntTask->SetTargetObject(m_OtherTeamPlayer[ClosestEnemy]);
        m_HuntTask->SetState(bot_task::TASK_STATE_ATTACK);
        if (DistSq < PRIORITY_RADIUS_SQ)
            m_HuntTask->SetPriority(100);
        else
            m_HuntTask->SetPriority(50);
        return;
    }

    // Everyone else must have died!  YOU KICK ASS, BOT!
    m_HuntTask->SetTargetObject(NULL);
    m_HuntTask->SetState(bot_task::TASK_STATE_INACTIVE);
}

//==============================================================================

void hunter_master_ai::AssignAllTasks ( void )
{
    ASSERT(m_MissionInitialized);
    s32 i;

    for (i = 0; i < m_nTeamBots; i++)
    {
        if (!m_TeamBot[i]->HasTask())
            AssignBotTask(i, m_HuntTask);
        if (m_TeamBot[i]->GetTask()->GetPriority() == 100)
        {
            m_TeamBot[i]->DisableLoadout();
        }
        else
            m_TeamBot[i]->EnableLoadout();
    }
}

//==============================================================================

s32 hunter_master_ai::FindFlagHorder ( void )
{
    s32 FlagHorder = -1;
    s32 i, MostFlags = 0;
    for (i = 0; i < m_nOtherTeamPlayers; i++)
    {
        if (m_OtherTeamPlayer[i]->GetFlagCount() > MostFlags)
        {
              MostFlags = m_OtherTeamPlayer[i]->GetFlagCount();
              FlagHorder = i;
        }
    }
    if (MostFlags < 10)
    {
        FlagHorder = -1;
    }
    return FlagHorder;
}

//==============================================================================

void hunter_master_ai::PickNexusValue( void )
{
    s32 nTotalPlayers = 1+m_nOtherTeamPlayers;
    m_GotoNexusValue = x_irand( 4, MAX(5, nTotalPlayers + 1));
    m_RecalculateGotoValue = FALSE;
}
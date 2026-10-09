//==============================================================================
//  INCLUDES
//==============================================================================

#include "MAI_Manager.hpp"
#include "Objects/Player/DefaultLoadouts.hpp"
#include "GameMgr/GameMgr.hpp"
#include "fe_globals.hpp"
#include "globals.hpp"

//==============================================================================
//  STORAGE
//==============================================================================

mai_manager MAI_Mgr;
xbool       mai_manager::m_Initialized = FALSE;
extern      s32 TeamBitsFound;
s32         g_BotFrameCount = 0;
extern      u32 g_SCtr;

//==============================================================================
//  MASTER AI MEMBER FUNCTIONS
//==============================================================================

mai_manager::mai_manager( void )
{
    ASSERT(!m_Initialized);
    m_Initialized = TRUE;
    m_GameID = MAI_NONE;
    m_nTeams = 0;
    m_bDisableMAI = FALSE;
}

//==============================================================================

mai_manager::~mai_manager( void )
{
    ASSERT(m_Initialized);
    m_Initialized = FALSE;
}

//==============================================================================

void mai_manager::EndOfMissionLoadInit( void )
{
//    if ( tgl.ServerPresent && (fegl.ServerSettings.BotsEnabled 
//                                || GameMgr.IsCampaignMission()) )
    if ( tgl.ServerPresent || GameMgr.IsCampaignMission() )
    {
        m_bDisableMAI = FALSE;
    }
    else
    {
         m_bDisableMAI = TRUE;
         return;
    }
    g_SCtr = 0;
    TeamBitsFound = 0;
    s32 i = 0;

    // Ask the GameMgr for the game type, and set the master AI using it.
    switch (GameMgr.GetGameType())
    {
        // Capture The Flag.
        case (GAME_CTF):
            m_GameID = MAI_CTF;
            m_nTeams = 2;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i] = new ctf_master_ai;
            }
#ifndef T2_DESIGNER_BUILD
                LoadDeployables();
#endif
        break;

        // Capture and Hold
        case (GAME_CNH):
            m_GameID = MAI_CNH;
            m_nTeams = 2;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i] = new cnh_master_ai;
            }
#ifndef T2_DESIGNER_BUILD
                LoadDeployables();
#endif
        break;

        // Team Deathmatch
        case (GAME_TDM):
            m_GameID = MAI_TDM;
            m_nTeams = 2;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i] = new tdm_master_ai;
            }
        break;

        // Deathmatch.
        case (GAME_DM):
            m_GameID = MAI_DM;
            m_nTeams = MAX_TEAMS;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i] = new dm_master_ai;
            }
        break;

        // Hunter.
        case (GAME_HUNTER):
            m_GameID = MAI_HUNTER;
            m_nTeams = MAX_TEAMS;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i] = new hunter_master_ai;
            }
        break;

        // Single Player Missions
        case (GAME_CAMPAIGN):
            m_GameID = MAI_SPM;

            m_nTeams = 2;

            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i] = new spm_master_ai;
            }
        break;
    
        // None?
        default:
            ASSERT(FALSE && "Unknown game type!");
    }
}

//==============================================================================

void mai_manager::InitMission( void )
{
    if ( m_bDisableMAI )
        return;

    s32 i = 0;

    // Ask the GameMgr for the game type, and set the master AI using it.
    switch (GameMgr.GetGameType())
    {
        // Capture The Flag.
        case (GAME_CTF):
            m_GameID = MAI_CTF;
            m_nTeams = 2;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i]->InitializeMission();
            }
        break;

        // Capture and Hold
        case (GAME_CNH):
            m_GameID = MAI_CNH;
            m_nTeams = 2;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i]->InitializeMission();
            }
        break;

        // Team Deathmatch
        case (GAME_TDM):
            m_GameID = MAI_TDM;
            m_nTeams = 2;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i]->InitializeMission();
            }
        break;

        // Deathmatch.
        case (GAME_DM):
            m_GameID = MAI_DM;
            m_nTeams = MAX_TEAMS;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i]->InitializeMission();
            }
        break;

        // Hunter.
        case (GAME_HUNTER):
            m_GameID = MAI_HUNTER;
            m_nTeams = MAX_TEAMS;
            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i]->InitializeMission();
            }
        break;

        // Single Player Missions
        case (GAME_CAMPAIGN):
            m_GameID = MAI_SPM;

            m_nTeams = 2;

            for (i = 0; i < m_nTeams; i++)
            {
                MasterAI[i]->InitializeMission();
            }
        break;
    
        // None?
        default:
            ASSERT(FALSE && "Unknown game type!");
    }
}

//==============================================================================
#if defined (acheng)
#endif
void mai_manager::Update( void )
{
    if ( m_bDisableMAI )
        return;

    ++g_BotFrameCount;
    
    s32 i;
#if defined (acheng)
    static s32 FrameCtr = 0;
    static f32 TimeSum = 0;
    static f32 AvgTime = 0;
    static xtimer Timer;
    Timer.Reset();
    Timer.Start();
#endif
    if (m_GameID != MAI_NONE)
    {
        g_SCtr++;
        for (i = 0; i < m_nTeams; i++)
            MasterAI[i]->Update();
    }
#ifndef T2_DESIGNER_BUILD
#if (defined (acheng) )
    Timer.Stop();
    TimeSum += Timer.ReadMs();
    FrameCtr++;
    if (FrameCtr >= 10)
    {
        AvgTime = TimeSum/FrameCtr;
        TimeSum = 0;
        FrameCtr = 0;
    }
    x_printfxy(5, 10, "Average Time for MAI_Mgr Update(): %2.4f", AvgTime);
#endif
#endif
}

//==============================================================================

void mai_manager::UnloadMission( void )
{
    if ( m_bDisableMAI )
        return;
    s32 i;
    for (i = 0; i < m_nTeams; i++)
    {
        MasterAI[i]->UnloadMission();
        delete MasterAI[i];
    }
    m_nTeams = 0;
    
    // Initialize the proper MAI based on the mission directory string.
    Path_Manager_InitializePathMng();
}

//==============================================================================

void mai_manager::Goto      ( s32 Team, const vector3& Location )
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Goto(Location);
}

//==============================================================================

void mai_manager::Repair    ( s32 Team, object::id ObjID )
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Repair(ObjID);
}

//==============================================================================

void mai_manager::Roam      ( s32 Team, const vector3& Location, u32 WaveBit )
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Roam(Location, WaveBit);
}

//==============================================================================

void mai_manager::Attack    ( s32 Team, object::id ObjID, u32 WaveBit )
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Attack(ObjID, WaveBit);
}

//==============================================================================

void mai_manager::Defend    ( s32 Team, const vector3& Location, u32 WaveBit )
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Defend(Location, WaveBit);
}

//==============================================================================

void mai_manager::Mortar    ( s32 Team, object::id ObjID, u32 WaveBit)
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Mortar(ObjID, WaveBit);
}

//==============================================================================

void mai_manager::Destroy   ( s32 Team, object::id ObjID, u32 WaveBit)
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Destroy(ObjID, WaveBit);
}

//==============================================================================

void mai_manager::DeathMatch( s32 Team, u32 WaveBit )
{ 
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->DeathMatch(WaveBit);
}

//==============================================================================

void mai_manager::Antagonize( s32 Team, u32 WaveBit )
{
    ASSERT(m_GameID == MAI_SPM);
    ASSERT(Team == 1 || Team == 2);
    ((spm_master_ai*)MasterAI[Team-1])->Antagonize(WaveBit);
}

//==============================================================================
 
void mai_manager::LoadDeployables ( void )
{
    /*
    char pFileName[100];
    x_strcpy(pFileName, tgl.MissionDir);
    x_strcat(pFileName, "/");
    x_strcat(pFileName, tgl.MissionName);
    x_strcat(pFileName, ".ass");

    Fp = x_fopen( pFileName, "rb" );


    Fp = x_fopen( xfs( "%s/%s.ass", tgl.MissionDir, tgl.MissionName ), "rb" );
*/
    ASSERT( m_bDisableMAI == FALSE );

    X_FILE* Fp;
    xfs XFS( "%s/%s.ass", tgl.MissionDir, tgl.MissionName );
    Fp = x_fopen( (const char*)XFS, "rb" );

    if (!Fp)
    {
        x_DebugMsg("Unable to open assets file, %s \n", (const char*)XFS);
        return;
    }

    // Storm Assets
    MasterAI[0]->LoadDeployables(Fp);

    // Inferno Assets
    MasterAI[1]->LoadDeployables(Fp);

    x_DebugMsg("Loaded Asset File %s.\n", (const char*)XFS);
    x_fclose( Fp );

}

//==============================================================================

xbool mai_manager::CheckTurretPlacement( const vector3 &Location, u32 TeamBits)
{
    ASSERT(TeamBits == 1 || TeamBits == 2);
    return MasterAI[TeamBits-1]->CheckTurretPlacement(Location);
}

//==============================================================================

void mai_manager::Ouch(u32 TeamBits, object::id ObjID)
{
    ASSERT(TeamBits == 1 || TeamBits == 2);
    MasterAI[TeamBits - 1]->RegisterOffendingTurret(ObjID);
}

//==============================================================================

void mai_manager::OffenseDied(u32 TeamBits, s32 BotIdx)
{
    if (GetGameID() == MAI_CTF)
    {
        ASSERT(TeamBits == 1 || TeamBits == 2);
        ((ctf_master_ai*)MasterAI[TeamBits - 1])->OffenseBotDied(BotIdx);
    }
    else if (GetGameID() == MAI_CNH)
    {
        ASSERT(TeamBits == 1 || TeamBits == 2);
        ((cnh_master_ai*)MasterAI[TeamBits - 1])->OffenseBotDied(BotIdx);
    }
}

//==============================================================================

void mai_manager::TurretSpotFailed ( u32 TeamBits, s32 BotIdx )
{
    ASSERT(GetGameID() == MAI_CTF || GetGameID() == MAI_CNH);
    ASSERT(TeamBits == 1 || TeamBits == 2);
    MasterAI[TeamBits - 1]->AssetPlacementFailed(BotIdx);
}

//==============================================================================

void mai_manager::SetBotTeamBits ( void )
{
    bot_object* Bot;
    ObjMgr.StartTypeLoop(object::TYPE_BOT);
    while ((Bot = (bot_object*)ObjMgr.GetNextInType()) != NULL)
    {
        Bot->SetBotTeamBits();
    }
    ObjMgr.EndTypeLoop();
}
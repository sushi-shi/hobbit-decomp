#ifndef MAI_MANAGER_HPP
#define MAI_MANAGER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include "CTF_MasterAI.hpp"
#include "SPM_MasterAI.hpp"
#include  "DM_MasterAI.hpp"
#include "TDM_MasterAI.hpp"
#include "CNH_MasterAI.hpp"
#include "HunterMasterAI.hpp"

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================

class mai_manager
{
//------------------------------------------------------------------------------
//  Types
//------------------------------------------------------------------------------
    public:
        enum            
        {
            ALL_BOTS = 0xFFFFFFFF,
            // increment this later (for deathmatch, etc)
            MAX_TEAMS = MAX_BOTS,
        };

//------------------------------------------------------------------------------
//  Public functions
//------------------------------------------------------------------------------
//  One-bot orders
        void        Goto                ( s32 Team, const vector3& Location );

        void        Repair              ( s32 Team, object::id ObjID );

// Multi-bot orders
        void        Roam                ( s32 Team, const vector3& Location, u32 WaveBit = ALL_BOTS );

        void        Attack              ( s32 Team, object::id ObjID, u32 WaveBit = ALL_BOTS );

        void        Defend              ( s32 Team, const vector3& Location, u32 WaveBit = ALL_BOTS );

        void        Mortar              ( s32 Team, object::id ObjID, u32 WaveBit = ALL_BOTS ); 

        void        Destroy             ( s32 Team, object::id ObjID, u32 WaveBit = ALL_BOTS ); 

        void        Antagonize          ( s32 Team, u32 WaveBit = ALL_BOTS );

        void        DeathMatch          ( s32 Team, u32 WaveBit = ALL_BOTS );

//------------------------------------------------------------------------------
    public:
                    mai_manager         ( void );
                   ~mai_manager         ( void );

        void        EndOfMissionLoadInit( void );
        void        InitMission         ( void );
        void        LoadDeployables     ( void );

        void        Update              ( void );
        
        void        UnloadMission       ( void );
        mai_type    GetGameID           ( void ) { return m_GameID; }

        xbool       CheckTurretPlacement(const vector3& Location, u32 TeamBits);

        void        Ouch                ( u32 TeamBits, object::id ObjID );

        void        OffenseDied         ( u32 TeamBits, s32 BotIdx );

        void        SetBotTeamBits      ( void );

        void        TurretSpotFailed    ( u32 TeamBits, s32 BotIdx );
//------------------------------------------------------------------------------
//  Members
//------------------------------------------------------------------------------
    protected:
static  xbool       m_Initialized;
        mai_type    m_GameID;

        master_ai*  MasterAI[MAX_TEAMS];
        s32         m_nTeams;
        xbool       m_bDisableMAI;
};

extern mai_manager MAI_Mgr;
#endif

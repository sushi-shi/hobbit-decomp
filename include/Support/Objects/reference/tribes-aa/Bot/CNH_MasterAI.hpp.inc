//==============================================================================
//  
//  CNH_MasterAI.hpp
//
//==============================================================================
#ifndef CNH_MASTER_AI_HPP
#define CNH_MASTER_AI_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include "masterAI.hpp"
#include "..\Projectiles\Flag.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

class cnh_master_ai : public master_ai
{
//------------------------------------------------------------------------------
//  Public Types
//------------------------------------------------------------------------------
    public:
        
        struct switch_data
        {
            bot_task*   ptCapture;
            bot_task*   ptHold;     // light defense
            bot_task*   ptHeavyHold;// heavy defense
            bot_task*   ptDeploy;
            bot_task*   ptSnipe;
            
            s32         iDeploying;

            object::id  SwitchID;  // id of switch
            xbool       bHeld;
            xbool       bUnderAttack;
            vector3     Position;
        };

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
    public:
                    cnh_master_ai           ( void );
                   ~cnh_master_ai           ( void );
        void        InitializeMission   ( void );
        void        UnloadMission       ( void );
        void        Update              ( void );

        void        OffenseBotDied      ( s32 BotIdx );

virtual void        AssetPlacementFailed ( s32 BotIdx );
//------------------------------------------------------------------------------
//  Protected Storage
//------------------------------------------------------------------------------
    protected:

        // Specific task pointers
/*
        bot_task*   m_CaptureTask[MAX_CNH_SWITCHES];

        bot_task*   m_HoldTask[MAX_CNH_SWITCHES];
        bot_task*   m_HeavyDefenseTask[MAX_CNH_SWITCHES];
        bot_task*   m_DeployTask[MAX_CNH_SWITCHES];
        bot_task*   m_RepairTask[MAX_CNH_SWITCHES];
        bot_task*   m_SnipeTask[MAX_CNH_SWITCHES];
        s32         m_iDeploying[MAX_CNH_SWITCHES];
        s32         m_iRepairing[MAX_CNH_SWITCHES];
        object::id  m_FlipFlop[MAX_CNH_SWITCHES];
*/
        bot_task*   m_RepairTask;
        s32         m_iRepairing;
        
        switch_data m_Switch[MAX_CNH_SWITCHES];
        // For debugging
        s32         BotSwitch[MAX_BOTS];

        s32         m_nSwitches;
        s32         m_nHeldSwitches;
        u32         m_HeldSwitchBits;
        u32         m_LastSwitchBits;
        xbool       m_ForceReset;

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
    protected:
        // Set the new values for the collected data.
        void        PollAllObjects      ( void );

        // (Re)assign tasks based on bot availability.
        void        AssignAllTasks      ( void );

virtual void        UpdatePlayerPointers ( void );

        void        ResetAssignments    ( void );

        void        InitializeSwitchData( void );

        void        QuerySwitchStatus   ( void );

        s32         FindClosestSwitch   ( s32 iBot, u32& Assigned, xbool UnheldOnly = FALSE );

        void        SetUpDeployTask     ( s32 i );
};

#endif
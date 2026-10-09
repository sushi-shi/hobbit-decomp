//==============================================================================
//  
//  CTF_MasterAI.hpp
//
//==============================================================================
#ifndef CTF_MASTER_AI_HPP
#define CTF_MASTER_AI_HPP

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

class ctf_master_ai : public master_ai
{
//------------------------------------------------------------------------------
//  Public Types
//------------------------------------------------------------------------------
    public:
        
        enum 
        {  
            NUM_CTF_TASKS = 10,
        };


//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
    public:
                    ctf_master_ai           ( void );
                   ~ctf_master_ai           ( void );
        void        InitializeMission   ( void );
        void        UnloadMission       ( void );

        void        Update              ( void );

        xbool       RefreshFlags        ( void );

        void        RegisterOffendingTurret ( object::id ObjID );

        void        OffenseBotDied      ( s32 BotIdx );

virtual void        AssetPlacementFailed ( s32 BotIdx );

//        void        RemoveTarget        (void* DeadGuy);
//------------------------------------------------------------------------------
//  Protected Storage
//------------------------------------------------------------------------------
    protected:

        // Specific task pointers
        // Offense
        s32             m_nOffense;
        bot_task*       m_CapTask;
        bot_task*       m_DestroyTaskList[MAX_DESTROY_BOTS];

        s32             m_nOffenseDeaths;
        s32             m_nExpectedDestroyers;
        xbool           m_DestroyRun;
        f32             m_LastDestroyRun;
//        bot_task*       m_DestroyTask;

        bot_task*       m_RetrieveTask;

        // Defense
        s32             m_nDefense;
        bot_task*       m_HeavyDefendTask;
        bot_task*       m_LightDefendTask;
        bot_task*       m_RepairTask;
        bot_task*       m_DeployTask;
        bot_task*       m_SnipeTask;
        xbool           m_MortarFlag;

        s32             m_FlagCarrier;
        
        // Position of the flags
        vector3         m_TeamFlagPos;
        vector3         m_OtherFlagPos;

        s32             m_TeamFlagStatus;
        s32             m_OtherFlagStatus;

        // Position of flag stands
        vector3         m_TeamFlagStand;
        vector3         m_OtherFlagStand;

        object::id      m_FlagID;
        object::id      m_EnemyFlagID;

        xbool           ForceReset;  // from bot death
        s32             LastAssetCheck; // Prevent constant asset placement searches
//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
    protected:
        // Set the new values for the collected data.
        void        PollAllObjects      ( void );

        // (Re)assign tasks based on bot availability.
        void        AssignAllTasks      ( void );

        void        UpdateAllTaskStates ( void );

        void        ResetDestroyTask    ( s32 TaskNum );

        void        CleanUpTasks        ( void );

        void        SetDestroyBot       ( s32 BotID );

virtual void        UpdatePlayerPointers ( void );


//------------------------------------------------------------------------------
//  Helper Functions
//------------------------------------------------------------------------------

    protected:
        void        SetUpDeployTask     ( void );
        void        SetUpDeployAssignment( s32 BotID );
};

#endif

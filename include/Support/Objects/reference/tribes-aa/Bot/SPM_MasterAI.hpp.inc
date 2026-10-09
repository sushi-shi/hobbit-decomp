//==============================================================================
//  
//  SPM_MasterAI.hpp
//
//==============================================================================
#ifndef SPM_MASTER_AI_HPP
#define SPM_MASTER_AI_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include "masterAI.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

class spm_master_ai : public master_ai
{
//------------------------------------------------------------------------------
//  Public Types
//------------------------------------------------------------------------------
    public:
        
        enum 
        {  
            NUM_SPM_TASKS = 24,
        };

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
    public:
                    spm_master_ai           ( void );
                   ~spm_master_ai           ( void );

        void        InitializeMission   ( void );
        void        UnloadMission       ( void );

//------------------------------------------------------------------------------
//  SPM functions
//------------------------------------------------------------------------------
    public:
        void        Goto                ( const vector3& Location );
        void        Repair              ( object::id ObjID );
        void        Roam                ( const vector3& Location, u32 WaveBit );
        void        Attack              ( object::id ObjID, u32 WaveBit );
        void        Defend              ( const vector3& Location, u32 WaveBit );
        void        Mortar              ( object::id ObjID, u32 WaveBit );
        void        Destroy             ( object::id ObjID, u32 WaveBit );
        void        DeathMatch          ( u32 WaveBit );
        void        Antagonize          ( u32 WaveBit );

//------------------------------------------------------------------------------
//  Protected Storage
//------------------------------------------------------------------------------
    protected:

        // Specific task pointers
        bot_task*   m_EngageTask;
        bot_task*   m_DefendTask;
        bot_task*   m_RepairTask;
        bot_task*   m_GotoTask;
        bot_task*   m_RoamTask;
        bot_task*   m_MortarTask;
        bot_task*   m_DestroyTask;
        bot_task*   m_AntagonizeTask;

        xbool       m_bDeathMatchMode;
        s32         m_DeathMatchStartTask;
        vector3     m_DefendLocation;
//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
    protected:
        // Set the new values for the collected data.
        void        PollAllObjects      ( void );

        // (Re)assign tasks based on bot availability.
        void        AssignAllTasks      ( void );

        void        ResetPriorities     ( u32 WaveBit );
};

#endif
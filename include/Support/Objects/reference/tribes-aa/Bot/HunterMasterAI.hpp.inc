//==============================================================================
//  
//  HUNTER_MasterAI.hpp
//
//==============================================================================
#ifndef HUNTER_MASTER_AI_HPP
#define HUNTER_MASTER_AI_HPP

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

class hunter_master_ai : public master_ai
{
//------------------------------------------------------------------------------
//  Public Types
//------------------------------------------------------------------------------
    public:
        enum 
        {  
            NUM_HUNTER_TASKS = 1,
        };

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
    public:
                    hunter_master_ai           ( void );
                   ~hunter_master_ai           ( void );

        void        InitializeMission   ( void );
        void        UnloadMission       ( void );


//------------------------------------------------------------------------------
//  Protected Storage
//------------------------------------------------------------------------------
    protected:

        // Specific task pointers
        bot_task*   m_HuntTask; // Only need one task since there is only 1 bot
        // Swaps between states: get open flag, attack flag carrier, goto nexus

        // Position of nexus
        vector3     m_NexusPos;

        // Total number of flags in the game
        s32         m_nTotalFlags;

        // Number of flags before returning to nexus
        s32         m_GotoNexusValue;

        // Time to recalculate GotoNexusValue
        xbool       m_RecalculateGotoValue;
//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
    protected:
        // We need a slightly different func for this type of game.
        void        UpdatePlayerPointers ( void );

        // Set the new values for the collected data.
        void        PollAllObjects      ( void );

        // (Re)assign tasks based on bot availability.
        void        AssignAllTasks      ( void );

        // Choose number of flags to collect before returning to nexus.
        void        PickNexusValue( void );

        // Any bots have too many flags for their own good?
        s32         FindFlagHorder      ( void );
};

//==============================================================================
#endif  // HUNTER_MASTER_AI_HPP
//==============================================================================

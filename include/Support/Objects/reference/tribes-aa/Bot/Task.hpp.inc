#ifndef     _TASK_HPP_
#define     _TASK_HPP_

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "..\Support\Objects\Player\DefaultLoadouts.hpp"
#include <Support/ObjectMgr/Object.hpp>
//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

enum mai_type 
{
    MAI_NONE,
    MAI_CTF,
    MAI_CNH,
    MAI_TDM,
    MAI_DM,
    MAI_HUNTER,
    MAI_RABBIT,
    MAI_SPM,
};

//==============================================================================

class bot_task
{
    public:
        enum task_type
        {
            TASK_TYPE_NONE = 0,
#ifdef E3
            TASK_TYPE_OFF_DUTY,
#endif

                // CTF specific types
            TASK_TYPE_DEFEND_FLAG,
            TASK_TYPE_LIGHT_DEFEND_FLAG,
            TASK_TYPE_RETRIEVE_FLAG,
            TASK_TYPE_CAPTURE_FLAG,
            TASK_TYPE_ESCORT_FLAG_CARRIER,

            TASK_TYPE_GOTO,
            TASK_TYPE_DEPLOY_TURRET,
            TASK_TYPE_DEPLOY_INVEN,
            TASK_TYPE_DEPLOY_SENSOR,
            TASK_TYPE_REPAIR,
            TASK_TYPE_DESTROY,
            TASK_TYPE_MORTAR,
            TASK_TYPE_SNIPE,
            TASK_TYPE_ENGAGE_ENEMY,
            TASK_TYPE_ANTAGONIZE,

            TASK_TYPE_END =     TASK_TYPE_ANTAGONIZE,

            TASK_TYPE_TOTAL
        };

        enum task_state
        {
            TASK_STATE_START = 0,

            TASK_STATE_INACTIVE = TASK_STATE_START,
            TASK_STATE_LOADOUT,
            TASK_STATE_GOTO_POINT,  // Cap Goto   , Single bot goto
            TASK_STATE_PURSUE,      // Goto moving object (i.e. flag)
            TASK_STATE_ROAM_AREA,   // Patrol Base, Multiple bot Goto
            TASK_STATE_ATTACK,      // 
            TASK_STATE_MORTAR,
            TASK_STATE_SNIPE,
            TASK_STATE_REPAIR,
            TASK_STATE_DESTROY,
            TASK_STATE_DEPLOY_TURRET,
            TASK_STATE_DEPLOY_INVEN,
            TASK_STATE_DEPLOY_SENSOR,
            TASK_STATE_ANTAGONIZE,
                
            TASK_STATE_END = TASK_STATE_ANTAGONIZE,
            TASK_STATE_TOTAL,


            // Old states   
#if 0
            CAPTURE_STATE_GOTO_ENEMY_FLAG,
            CAPTURE_STATE_TRANSIT_HOME,
            DEFEND_STATE_PATROL_BASE,
            DEFEND_STATE_ATTACK_ENEMY_NEAR_FLAG,
            RETRIEVE_STATE_ATTACK_ENEMY_WITH_FLAG,
            RETRIEVE_STATE_SECURE_FLAG,
            ENGAGE_STATE_ATTACK_ENEMY,
#endif
        };
        enum 
        {
            NUM_BOTS = 16,
        };

    public:
                        bot_task            ( void );
                       ~bot_task            ( void );
        void            Initialize      ( void );

        task_type       GetID           ( void )    const; 
        void            SetID           ( task_type Type ) 
                                        { m_TaskID = Type; }
        
        s32             GetPriority     ( void )    const ;
        void            SetPriority     ( s32 Value );

        xbool           IsAssigned      ( void )    const ;
        u32             GetAssignedBot  ( void )    const ;
        s32             GetAssignedBotID( void )    const ;
        void            SetAssignedBot  ( s32 BotID );
        void            UnassignBot     ( s32 BotID );
        u32             GetAssignedWave  ( void )    const {return m_WaveAssigned;}
        void            SetAssignedWave  ( u32 WaveBit )  {m_WaveAssigned |= WaveBit;}
        void            UnassignWave     ( u32 WaveBit )  {m_WaveAssigned &= ~WaveBit;}

        s32             GetNumAssignedBots ( void ) const; 
        s32             MaxBotsToBeAssigned( void ) const 
                                        { return m_nMaxBotsToBeAssigned; }
        void            SetMaxBots      ( s32 nBots )     
                                        { m_nMaxBotsToBeAssigned = nBots;}
        
const   vector3&        GetTargetPos    ( void )    const 
                                        { return m_TargetPos; }
        void            SetTargetPos    ( const vector3& Pos ) 
                                                    { m_TargetPos = Pos; }

        // Update priority, current state, and the target position if necessary.
        void            Update          ( void );
        char*           GetTaskName     ( void )    const ;
        char*           GetTaskStateName     ( void )    const ;

        s32             GetState        ( void ) const 
                                        { return m_State; }

        void            SetState        ( task_state State ) 
                                        { m_State = State; }

        default_loadouts::type  GetIdealLoadout ( void ) const ;

        xbool           IsImportant     ( void ) const;

        // To do:  change return type to object::id
const   object*         GetTargetObject ( void );
        void            SetTargetObject ( object *Target );

        void            ResetAssignments ( void ) { m_BotAssigned = 0; }

        void            ResetTimer      ( void );
        // Make the MAI reset the task's target/assigned bots/priority
        void            ForceReset      ( void ) { m_TimeToReset = 0; }
        xbool           CheckStatus     ( void ) { return m_TimeToReset <= 0; }

        void            operator=   ( const bot_task& RHS );

//==============================================================================
    protected:
        // Task ID
        task_type       m_TaskID;

        // Priority of this task
        s32             m_Priority;

        // Task status
        task_state      m_State;

        // Bit flags of Bot IDs Assigned to this task, 0 if none.
        u32             m_BotAssigned;

        // (For SPM) Attack wave bit value assigned to this task.
        u32             m_WaveAssigned;

//        s32     m_nBotsAssigned;

        // Pointer to the target object.
        object::id      m_TargetID;

        // Last known position of this target.
        vector3         m_TargetPos;

//        s32     m_CurrentState;
        s32             m_TimeToReset;

        // one bot per task for now.
        s32             m_nMaxBotsToBeAssigned;

};


#endif

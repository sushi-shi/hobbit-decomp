//==============================================================================
//
//  CorpseObject.hpp
//
//==============================================================================

#ifndef CORPSEOBJECT_HPP
#define CORPSEOBJECT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "PlayerObject.hpp"



//==============================================================================
//  FORWARD DECLARATIONS
//==============================================================================


//==============================================================================
//  TYPES
//==============================================================================

class corpse_object : public object
{
//==========================================================================
// Defines
//==========================================================================
public:

    //======================================================================
    // Corpse states
    //======================================================================
    enum corpse_state
    {
        CORPSE_STATE_START=0,

        CORPSE_STATE_MOVE = CORPSE_STATE_START,     // Moving around
        CORPSE_STATE_IDLE,                          // Still on the floor
        CORPSE_STATE_FADE_OUT,                      // Fading out

        CORPSE_STATE_END = CORPSE_STATE_FADE_OUT,
        CORPSE_STATE_TOTAL
    } ;


//==========================================================================
// Structures
//==========================================================================
protected:

//==========================================================================
// Static data
//==========================================================================
protected:

    static s32                  s_ActiveCount ;     // Current # of active corpses


//==========================================================================
// Data
//==========================================================================
protected:

    // Render vars
    xbool                                   m_IsDeadBot ;       // TRUE if corpse came from a bot
    player_object::character_type           m_CharType ;        // Character type
    player_object::armor_type               m_ArmorType ;       // Armor type
    shape_instance                          m_PlayerInstance ;  // Instance for player
    xcolor                                  m_Color ;           // Display color
    radian3                                 m_Rot ;             // Rotation direction
    
    // State vars                                                               
    corpse_state                            m_State ;           // Current state
    f32                                     m_StateTime  ;      // Time in current state
    f32                                     m_Age ;             // Age of corpse
                                        
    // Movement vars                    
    vector3                                 m_Vel ;             // Current velocity
    const player_object::move_info*         m_MoveInfo ;        // Pointer to move info
    quaternion                              m_GroundRot ;       // Ground rotation to use when drawing player                
    vector3                                 m_AnimDeltaPos ;    // Motion from animation
    xbool                                   m_OnSurface ;       // On a surface to stop on


//==================================================================================
// Functions
//==================================================================================
public:

        // Constructor/destructor
                                 corpse_object      ( void );
                                ~corpse_object      ( void );

        // Initialize
        void                    Initialize          ( const player_object& Player ) ;
        void                    OnAdd               ( void ) ;
        
        // Render functions             
        void                    Render              ( void ) ;
        void                    DebugRender         ( void ) ;
        
        // State functions          
        object::update_code     OnAdvanceLogic      ( f32 DeltaTime ) ;
        
        void                    AdvanceState        ( f32 DeltaTime ) ;
        void                    SetupState          ( corpse_state State, xbool SkipIfAlreadyInState = TRUE ) ;
        void                    ApplyPhysics        ( f32 DeltaTime ) ;
        void                    DoMovement          ( f32 DeltaTime ) ;
                                
        void                    Setup_MOVE          ( void ) ;
        void                    Advance_MOVE        ( f32 DeltaTime ) ;
                                
        void                    Setup_IDLE          ( void ) ;
        void                    Advance_IDLE        ( f32 DeltaTime ) ;
                                                           
        void                    Setup_FADE_OUT      ( void ) ;
        void                    Advance_FADE_OUT    ( f32 DeltaTime ) ;

protected:
};

//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

object*     CorpseObjectCreator( void );


//==============================================================================
#endif // CORPSEOBJECT_HPP
//==============================================================================

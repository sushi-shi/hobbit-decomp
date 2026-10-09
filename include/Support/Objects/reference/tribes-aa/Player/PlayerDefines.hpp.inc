//==============================================================================
//
//  PlayerDefines.hpp
//
//==============================================================================

#ifndef PLAYERDEFINES_HPP
#define PLAYERDEFINES_HPP

//==============================================================================
// INCLUDES
//==============================================================================

#include <xCore/x_files/x_files.hpp>
#include <xCore/Entropy/e_Input.hpp>


//==============================================================================
// STRUCTURES
//==============================================================================

// Structure to keep track of game keys
struct game_key
{
    // Data
    input_gadget    MainGadget ;    // Regular key to press
    input_gadget    ShiftGadget ;   // Shift key (if used)
    xbool           ShiftState ;    // State of shift key required

    // Functions
    xbool IsPressed(s32 ControllerID = 0) const ;
} ;




//==============================================================================
// STRUCTURES FROM DEFINES
//==============================================================================
#define DEFINE_ATTRIBUTE_F32(__name__, __value__)                           f32          __name__ ;
#define DEFINE_ATTRIBUTE_S32(__name__, __value__)                           s32          __name__ ;
#define DEFINE_ATTRIBUTE_R32(__name__, __value__)                           f32          __name__ ;
#define DEFINE_ATTRIBUTE_GAME_KEY(__name__, __main_gadget__, __shift_gadget__, __shift_state__) game_key __name__ ;
#define DEFINE_ATTRIBUTE_BBOX(__name__, __width__, __height__, __length__)  bbox         __name__ ;
#define DEFINE_ATTRIBUTE_V3(__name__, __x__, __y__, __z__)                  vector3      __name__ ;
#define DEFINE_ATTRIBUTE_ARRAY_S32(__name__, __total__)                     s32         __name__[__total__] ;
#define DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( __name__, __value__)
#define BEGIN_ENUMS( __name__ )                                             enum __name__ {
#define DEFINE_ENUM( __name__ )                                             __name__,
#define END_ENUMS( __name__ )                                               } ;


//==============================================================================

// Structure to contain movement accels etc used to move the player
// (one per character type)
struct move_info
{
// Data
    #include "PlayerMoveInfo.hpp"

// Functions
    void    SetDefaults     ( void ) ;
    xbool   LoadFromFile        ( const char *Filename ) ;
} ;

//==============================================================================

// Structure to contain common attributes that all players use
struct common_info
{
// Data
    #include "PlayerCommonInfo.hpp"

// Functions
    void    SetDefaults     ( void ) ;
    xbool   LoadFromFile    ( const char *Filename ) ;
} ;

//==============================================================================

// Structure to contain gadget input defines used to move the player
struct control_info
{
// Data
    #include "PlayerControlInfo.hpp"

// Functions
    void    SetDefaults     ( void ) ;
    xbool   LoadFromFile    ( const char *Filename ) ;

} ;

//==============================================================================

#undef DEFINE_ATTRIBUTE_F32
#undef DEFINE_ATTRIBUTE_S32
#undef DEFINE_ATTRIBUTE_R32
#undef DEFINE_ATTRIBUTE_GAME_KEY
#undef DEFINE_ATTRIBUTE_BBOX
#undef DEFINE_ATTRIBUTE_V3
#undef DEFINE_ATTRIBUTE_ARRAY_S32
#undef DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY
#undef BEGIN_ENUMS 
#undef DEFINE_ENUM
#undef END_ENUMS

//=========================================================================


#endif	// #ifdef PLAYERDEFINES_HPP


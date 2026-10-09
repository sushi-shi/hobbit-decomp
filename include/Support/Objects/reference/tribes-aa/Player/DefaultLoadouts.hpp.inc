//==============================================================================
//
//  DefaultLoadouts.hpp
//
//==============================================================================

#ifndef DEFAULT_LOADOUTS_HPP
#define DEFAULT_LOADOUTS_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "PlayerObject.hpp"



//==============================================================================
// NAMESPACES
//==============================================================================

// Namespace used to get at loadouts
namespace default_loadouts
{
// Defines

    enum type
    {
        // Misc type loadouts
        STANDARD_PLAYER,
        STANDARD_BOT_LIGHT,
        STANDARD_BOT_MEDIUM,
        STANDARD_BOT_HEAVY,

        // Standard game type loadouts
        STANDARD_CTF,
        STANDARD_CNH,
        STANDARD_TDM,
        STANDARD_DM,
        STANDARD_HUNTER,
        STANDARD_CAMPAIGN,
        STANDARD_CAMPAIGN2,
        STANDARD_EMPTY,
        STANDARD_HEAVY,
        STANDARD_HEAVY2,

        // These are loadouts for Master AI tasks -- these need to be
        // in the same order as in Task.hpp
        // (NOTE: Update the TASK_TYPE_START and TASK_TYPE_END if you modify this list!!)
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

        TOTAL,

        // Misc defines (NOTE: Update these if you modify the list!!!)
        TASK_TYPE_START = TASK_TYPE_DEFEND_FLAG,
        TASK_TYPE_END   = TASK_TYPE_ANTAGONIZE,

    } ;

// Functions

    // Returns loudout, given the player/bot character type and the loadout type
    void GetLoadout ( player_object::character_type CharacterType,
                      default_loadouts::type        Type,
                      player_object::loadout&       Loadout ) ;

} ;



//==============================================================================
#endif // DEFAULT_LOADOUTS_HPP
//==============================================================================

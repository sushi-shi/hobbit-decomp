//==============================================================================
//
//  DefaultLoadouts.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "DefaultLoadouts.hpp"

//=========================================================================
// STRUCTURE
//=========================================================================

// Structure used to describe loadout definitions
struct loadout_def
{
    player_object::armor_type       ArmorType ;
    object::type                    VehicleType ;
    s32                             NumWeapons ;
    player_object::invent_type      Weapons[5] ;
    s32                             GrenadeCount ;
    s32                             MineCount ;
    s32                             BeaconCount ;
    s32                             HealthKitCount ;
    player_object::invent_type      PackType ;
} ;


//=========================================================================
// DATA
//=========================================================================

//---------------------------------------------------------------------
// STANDARD_PLAYER
//---------------------------------------------------------------------
loadout_def StandardPlayer =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_BOT_LIGHT
//---------------------------------------------------------------------
loadout_def StandardBotLight =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_BOT_MEDIUM
//---------------------------------------------------------------------
loadout_def StandardBotMedium =
{
    player_object::ARMOR_TYPE_MEDIUM,       // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    4,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_BOT_HEAVY
//---------------------------------------------------------------------
loadout_def StandardBotHeavy =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    5,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_CTF
//---------------------------------------------------------------------
loadout_def StandardCTF =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_CNH
//---------------------------------------------------------------------
loadout_def StandardCNH =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_TDM
//---------------------------------------------------------------------
loadout_def StandardTDM =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    3,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_DM
//---------------------------------------------------------------------
loadout_def StandardDM =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    3,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE,        // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_HUNTER
//---------------------------------------------------------------------
loadout_def StandardHUNTER =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,   // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_CAMPAIGN
//---------------------------------------------------------------------
loadout_def StandardCAMPAIGN =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,   // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_CAMPAIGN2
//---------------------------------------------------------------------
loadout_def StandardCAMPAIGN2 =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_REPAIR,   // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_EMPTY
//---------------------------------------------------------------------
loadout_def StandardEMPTY =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_NONE,
        player_object::INVENT_TYPE_NONE,
        player_object::INVENT_TYPE_NONE,
    },
    0,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE         // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_HEAVY
//---------------------------------------------------------------------
loadout_def StandardHEAVY =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_THUNDERSWORD,      // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_NONE,
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN,
        player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,
    },
    0,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_NONE         // Pack type
} ;

//---------------------------------------------------------------------
// STANDARD_HEAVY2
//---------------------------------------------------------------------
loadout_def StandardHEAVY2 =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_THUNDERSWORD,      // Vehicle type
    5,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN,
        player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_AMMO  // Pack type
} ;

//---------------------------------------------------------------------
// Bot tasks
//---------------------------------------------------------------------

loadout_def TaskTypeDefendFlag =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    5,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeLightDefendFlag =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeRetrieveFlag =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    5,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeCaptureFlag =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeEscortFlagCarrier =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeGoto =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeDeployTurret =
{
    player_object::ARMOR_TYPE_MEDIUM,       // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    4,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR, // Pack type
};

loadout_def TaskTypeRepair =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_REPAIR, // Pack type
};

loadout_def TaskTypeDestroy =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    5,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN,
        player_object::INVENT_TYPE_WEAPON_PLASMA_GUN,
        player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_AMMO,  // Pack type
};

loadout_def TaskTypeDeployInven =
{
    player_object::ARMOR_TYPE_MEDIUM,       // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    4,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION, // Pack type
};

loadout_def TaskTypeDeploySensor =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR, // Pack type
};

loadout_def TaskTypeMortar =
{
    player_object::ARMOR_TYPE_HEAVY,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    5,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_PLASMA_GUN,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_MORTAR_GUN
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_AMMO,        // Pack type
};

loadout_def TaskTypeSnipe =
{
    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_SNIPER_RIFLE,
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY, // Pack type
};

loadout_def TaskTypeEngageEnemy =
{

    player_object::ARMOR_TYPE_LIGHT,        // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    3,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,        // Pack type
};

loadout_def TaskTypeAntagonize =
{

    player_object::ARMOR_TYPE_MEDIUM,       // Armor type
    object::TYPE_VEHICLE_WILDCAT,           // Vehicle type
    4,                                      // Number of weapons
    {                                       // Weapons list
        player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,
        player_object::INVENT_TYPE_WEAPON_BLASTER,
        player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
    },
    5,                                      // Grenade count
    0,                                      // Mine count
    0,                                      // Beacon count
    0,                                      // Health kit count
    player_object::INVENT_TYPE_ARMOR_PACK_ENERGY, // Pack type
};

loadout_def* LoadoutList[] =
{
    // Standard loadouts
    &StandardPlayer,            // STANDARD_PLAYER
    &StandardBotLight,          // STANDARD_BOT_LIGHT
    &StandardBotMedium,         // STANDARD_BOT_MEDIUM
    &StandardBotHeavy,          // STANDARD_BOT_HEAVY

    // Game type loadouts
    &StandardCTF,               // STANDARD_CTF
    &StandardCNH,               // STANDARD_CNH
    &StandardTDM,               // STANDARD_TDM
    &StandardDM,                // STANDARD_DM
    &StandardHUNTER,            // STANDARD_HUNTER
    &StandardCAMPAIGN,          // STANDARD_CAMPAIGN
    &StandardCAMPAIGN2,         // STANDARD_CAMPAIGN2
    &StandardEMPTY,             // STANDARD_EMPTY
    &StandardHEAVY,             // STANDARD_HEAVY
    &StandardHEAVY2,            // STANDARD_HEAVY2

    // Bot task loadouts
    &TaskTypeDefendFlag,        // TASK_TYPE_DEFEND_FLAG
    &TaskTypeLightDefendFlag,   // TASK_TYPE_LIGHT_DEFEND_FLAG
    &TaskTypeRetrieveFlag,      // TASK_TYPE_RETRIEVE_FLAG
    &TaskTypeCaptureFlag,       // TASK_TYPE_CAPTURE_FLAG
    &TaskTypeEscortFlagCarrier, // TASK_TYPE_ESCORT_FLAG_CARRIER
    &TaskTypeGoto,              // TASK_TYPE_GOTO
    &TaskTypeDeployTurret,      // TASK_TYPE_DEPLOY_TURRET
    &TaskTypeDeployInven,       // TASK_TYPE_DEPLOY_INVEN
    &TaskTypeDeploySensor,      // TASK_TYPE_DEPLOY_SENSOR
    &TaskTypeRepair,            // TASK_TYPE_REPAIR
    &TaskTypeDestroy,           // TASK_TYPE_DESTROY
    &TaskTypeMortar,            // TASK_TYPE_MORTAR
    &TaskTypeSnipe,             // TASK_TYPE_SNIPE
    &TaskTypeEngageEnemy,       // TASK_TYPE_ENGAGE_ENEMY
    &TaskTypeAntagonize,        // TASK_TYPE_ANTAGONIZE
} ;

//=========================================================================
// FUNCTIONS
//=========================================================================

// Sets up loudout, given the player/bot character type and the loadout type
void default_loadouts::GetLoadout ( player_object::character_type CharacterType,
                                    default_loadouts::type        Type,
                                    player_object::loadout&       Loadout )
{
    s32                         i ;
    player_object::invent_type  InventType ;

    // Make sure all entries are in defined in the table
    ASSERT(sizeof(LoadoutList) == (sizeof(loadout_def*) * TOTAL)) ;

    // Lookup definition data
    ASSERT(Type >= 0) ;
    ASSERT(Type < TOTAL) ;
    loadout_def& Def = *LoadoutList[Type] ;

    // Clear loadout
    Loadout.Clear() ;

    // Setup loadout from definition
    Loadout.Armor       = Def.ArmorType ;
    Loadout.VehicleType = Def.VehicleType ;

    // Get move info for this character
    const player_object::move_info& MoveInfo = player_object::GetMoveInfo(CharacterType, Loadout.Armor) ;

    // Setup weapons and the ammo used by the weapons
    Loadout.NumWeapons  = Def.NumWeapons ;
    for (i = 0 ; i < Loadout.NumWeapons ; i++)
    {
        // Must be a valid weapon
        ASSERT(IN_RANGE(player_object::INVENT_TYPE_WEAPON_START, Def.Weapons[i], player_object::INVENT_TYPE_WEAPON_END)) ;

        // Lookup weapon info
        const player_object::weapon_info& Info = player_object::GetWeaponInfo(Def.Weapons[i]) ;

        // Add weapon
        Loadout.Weapons[i]                       = Def.Weapons[i] ;
        Loadout.Inventory.Counts[Def.Weapons[i]] = 1 ;

        // Validate ammo type used by weapon
        ASSERT( (Info.AmmoType == player_object::INVENT_TYPE_NONE) ||
                (IN_RANGE( player_object::INVENT_TYPE_WEAPON_AMMO_START, Info.AmmoType, player_object::INVENT_TYPE_WEAPON_AMMO_END )) );

        // Add weapon ammo
        Loadout.Inventory.Counts[Info.AmmoType] = MoveInfo.INVENT_MAX[(s32)Info.AmmoType] ;
    }

    // Add other items
    Loadout.Inventory.Counts[player_object::INVENT_TYPE_GRENADE_BASIC]          = Def.GrenadeCount ;
    Loadout.Inventory.Counts[player_object::INVENT_TYPE_MINE]                   = Def.MineCount ;
    Loadout.Inventory.Counts[player_object::INVENT_TYPE_BELT_GEAR_BEACON]       = Def.BeaconCount ;
    Loadout.Inventory.Counts[player_object::INVENT_TYPE_BELT_GEAR_HEALTH_KIT]   = Def.HealthKitCount ;
    if (Def.PackType != player_object::INVENT_TYPE_NONE)
        Loadout.Inventory.Counts[Def.PackType] = MoveInfo.INVENT_MAX[Def.PackType] ;

    // Validate definition
    for (i = player_object::INVENT_TYPE_START ; i <= player_object::INVENT_TYPE_END ; i++)
    {
        // Convert to type for easier debugging
        InventType = (player_object::invent_type)i ;

        // Make sure item can be carried by this type
        ASSERTS(Loadout.Inventory.Counts[i] <= MoveInfo.INVENT_MAX[i], "this armor cannot carry this item!") ;
    }
}

//=========================================================================

//==============================================================================
//
//  PlayerObject.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "PlayerObject.hpp"
#include "CorpseObject.hpp"
#include "DefaultLoadouts.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "AADS/Globals.hpp"
#include "AADS/MoveManager.hpp"
#include "AADS/fe_globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AudioMgr/Audio.hpp"
#include <Support/PointLight/PointLight.hpp>
#include <Support/UI/ui_manager.hpp>
#include "hud/hud_manager.hpp"
#include "GameMgr/GameMgr.hpp"
#include "Objects/Vehicles/Vehicle.hpp"
#include "Objects/Projectiles/SatchelCharge.hpp"

/*
s32 VEHICLE_TEST=1;
extern s32 BYPASS;
extern s32 MISSION;

extern s32 g_uiLastSelectController;
*/

//==============================================================================
// DEFINES
//==============================================================================

static xbool TWEAK_PLAYER_VIEW = FALSE ;


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   PlayerObjectCreator;


obj_type_info   PlayerObjectTypeInfo( object::TYPE_PLAYER, 
                                      "PlayerObject", 
                                      PlayerObjectCreator, 
                                      object::ATTR_REPAIRABLE       | 
                                      object::ATTR_ELFABLE          |
                                      object::ATTR_PLAYER           | 
                                      object::ATTR_LABELED          | 
                                      object::ATTR_SOLID_DYNAMIC    |
                                      object::ATTR_DAMAGEABLE       |
                                      object::ATTR_SENSED );
                  
random SpawnRandom;

              
//==============================================================================
//  STATIC DATA
//==============================================================================

player_object::common_info  player_object::s_CommonInfo ;           // Common info for all players
player_object::move_info    player_object::s_MoveInfo[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL] ; // Move info

player_object::character_info player_object::s_CharacterInfo[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL] =
{
    // CHARACTER_TYPE_FEMALE
    {
        {   SHAPE_TYPE_CHARACTER_LIGHT_FEMALE,  "Data/Characters/LightFemaleDefines.txt"   },
        {   SHAPE_TYPE_CHARACTER_MEDIUM_FEMALE, "Data/Characters/MediumFemaleDefines.txt"  },
        {   SHAPE_TYPE_CHARACTER_HEAVY_MALE,    "Data/Characters/HeavyFemaleDefines.txt"   },
    },
    
    // CHARACTER_TYPE_MALE
    {
        {   SHAPE_TYPE_CHARACTER_LIGHT_MALE,    "Data/Characters/LightMaleDefines.txt"     },
        {   SHAPE_TYPE_CHARACTER_MEDIUM_MALE,   "Data/Characters/MediumMaleDefines.txt"    },
        {   SHAPE_TYPE_CHARACTER_HEAVY_MALE,    "Data/Characters/HeavyMaleDefines.txt"     },
    },
    
    // CHARACTER_TYPE_BIO
    {
        {   SHAPE_TYPE_CHARACTER_LIGHT_BIO,     "Data/Characters/LightBioDefines.txt"      },
        {   SHAPE_TYPE_CHARACTER_MEDIUM_BIO,    "Data/Characters/MediumBioDefines.txt"     },
        {   SHAPE_TYPE_CHARACTER_HEAVY_BIO,     "Data/Characters/HeavyBioDefines.txt"      },
    },
} ;


player_object::weapon_info player_object::s_WeaponInfo[INVENT_TYPE_WEAPON_TOTAL] =
{
    // INVENT_TYPE_WEAPON_NONE (dummy weapon info for when not carrying a weapon)
    {   INVENT_TYPE_NONE,                       // Ammo type
        0,                                      // Ammo needed to allow fire
        0,                                      // Ammo used per fire
        0,                                      // Weapon Fire Delay
        0.0f,                                   // % of vel inherit from player
        100.0f,                                 // Muzzle Speed (m/s)
        5.0f,                                   // Max Age (s)
    },

    // INVENT_TYPE_WEAPON_DISK_LAUNCHER,
    {   INVENT_TYPE_WEAPON_AMMO_DISK,           // Ammo type
        1,                                      // Ammo needed to allow fire
        1,                                      // Ammo used per fire
        0.75f,                                  // Weapon Fire Delay
        0.0f,                                   // % of vel inherit from player
        200.0f,                                 // Muzzle Speed (m/s)
        2.5f,                                   // Max Age (s)
    },

    // INVENT_TYPE_WEAPON_PLASMA_GUN,
    {   INVENT_TYPE_WEAPON_AMMO_PLASMA,         // Ammo type
        1,                                      // Ammo needed to allow fire
        1,                                      // Ammo used per fire
        0.5f,                                   // Weapon Fire Delay
        0.0f,                                   // % of vel inherit from player
        150.0f,                                 // Muzzle Speed (m/s)
        3.0f,                                   // Max Age (s)
    },                                          
                                                
    //INVENT_TYPE_WEAPON_SNIPER_RIFLE,                 
    {   INVENT_TYPE_NONE,                       // Ammo type
        15.0f  / 100.0f,                        // Ammo needed to allow fire
        500.0f / 100.0f,                        // Ammo used per fire
        0.0f,                                   // Weapon Fire Delay
        0.0f,                                   // % of vel inherit from player
        F32_MAX,                                // Muzzle Speed (m/s)
        1.0f,                                   // Max Age (s)
    },                                          
                                                
    //INVENT_TYPE_WEAPON_MORTAR_GUN,                       
    {   INVENT_TYPE_WEAPON_AMMO_MORTAR,         // Ammo type
        1,                                      // Ammo needed to allow fire
        1,                                      // Ammo used per fire
        2.8f,                                   // Weapon Fire Delay
        0.0f,                                   // % of vel inherit from player
        90.0f,                                  // Muzzle Speed (m/s)
        180.0f,                                 // Max Age (s)
    },

    //INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
    {   INVENT_TYPE_WEAPON_AMMO_GRENADE,        // Ammo type
        1,                                      // Ammo needed to allow fire
        1,                                      // Ammo used per fire
        0.9f,                                   // Weapon Fire Delay
        0.0f,                                   // % of vel inherit from player
        75.0f,                                  // Muzzle Speed (m/s)
        60.0f,                                  // Max Age (s)
    },

    //INVENT_TYPE_WEAPON_CHAIN_GUN,
    {   INVENT_TYPE_WEAPON_AMMO_BULLET,         // Ammo type
        1,                                      // Ammo needed to allow fire
        1,                                      // Ammo used per fire
        0.0f,                                   // Weapon Fire Delay    
        0.0f,                                   // % of vel inherit from player
        425.0f,                                 // Muzzle Speed (m/s)
        1.0f,                                   // Max Age (s)
    },

    //INVENT_TYPE_WEAPON_BLASTER,
    {   INVENT_TYPE_NONE,                       // Ammo type
        0.075f,                                 // Ammo needed to allow fire
        0.075f,                                 // Ammo used per fire
        0.3f,                                   // Weapon Fire Delay    
        0.0f,                                   // % of vel inherit from player
        250.0f,                                 // Muzzle Speed (m/s)
        1.25f,                                  // Max Age (s)
    },                                          
                                                
    //INVENT_TYPE_WEAPON_ELF_GUN,                      
    {   INVENT_TYPE_NONE,                       // Ammo type
        10.0f / 100.0f,                         // Ammo needed to allow fire
        0.0f  / 100.0f,                         // Ammo used per fire
        0.0f,                                   // Weapon Fire Delay    
        0.0f,                                   // % of vel inherit from player
        0.0f,                                   // Muzzle Speed (m/s)
        0.0f,                                   // Max Age (s)
    },                                          
                                                
    //INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,             
    {   INVENT_TYPE_WEAPON_AMMO_MISSILE,        // Ammo type
        1,                                      // Ammo needed to allow fire
        1,                                      // Ammo used per fire
        2.0f,                                   // Weapon Fire Delay    
        0.0f,                                   // % of vel inherit from player
        0.0f,                                   // Muzzle Speed (m/s)
        0.0f,                                   // Max Age (s)
    },

    //INVENT_TYPE_WEAPON_REPAIR_GUN,
    {   INVENT_TYPE_NONE,                       // Ammo type (dummy)
       10.0f / 100.0f,                          // Ammo needed to allow fire
        0.0f / 100.0f,                          // Ammo used per fire
        1.0f,                                   // Weapon Fire Delay   
        0.0f,                                   // % of vel inherit from player
        0.0f,                                   // Muzzle Speed (m/s)
        0.0f,                                   // Max Age (s)
    },

    //INVENT_TYPE_WEAPON_SHOCKLANCE
    {   INVENT_TYPE_NONE,                       // Ammo type (dummy)
        10.0f / 100.0f,                         // Ammo needed to allow fire
        1.0f  / 100.0f,                         // Ammo used per fire
        0,                                      // Weapon Fire Delay    
        0.0f,                                   // % of vel inherit from player
        250.0f,                                 // Muzzle Speed (m/s)
        1.75f,                                  // Max Age (s)
    },

    //INVENT_TYPE_WEAPON_TARGETING_LASER
    {   INVENT_TYPE_NONE,                       // Ammo type (dummy)
        0.0f / 100.0f,                          // Ammo needed to allow fire
        0.0f / 100.0f,                          // Ammo used per fire
        0,                                      // Weapon Fire Delay    
        0.0f,                                   // % of vel inherit from player
        0.0f,                                   // Muzzle Speed (m/s)
        0.0f,                                   // Max Age (s)
    },

} ;


player_object::invent_info player_object::s_InventInfo[INVENT_TYPE_TOTAL] =
{
    // Weapons


    // INVENT_TYPE_NONE
    {   SHAPE_TYPE_NONE,                        // Weapon shape type
        pickup::KIND_NONE,                      // Pickup kind
        1,                                      // MaxAmount
        0,                                      // Mass
    },

    // INVENT_TYPE_WEAPON_DISK_LAUNCHER,
    {   SHAPE_TYPE_WEAPON_DISC_LAUNCHER,        // Weapon shape type
        pickup::KIND_WEAPON_DISK_LAUNCHER,      // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    // INVENT_TYPE_WEAPON_PLASMA_GUN,
    {   SHAPE_TYPE_WEAPON_PLASMA_GUN,           // Weapon shape type
        pickup::KIND_WEAPON_PLASMA_GUN,         // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },                                          
                                                
    //INVENT_TYPE_WEAPON_SNIPER_RIFLE,                 
    {   SHAPE_TYPE_WEAPON_SNIPER_RIFLE,         // Weapon shape type
        pickup::KIND_WEAPON_SNIPER_RIFLE,       // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },                                          
                                                
    //INVENT_TYPE_WEAPON_MORTAR_GUN,                       
    {   SHAPE_TYPE_WEAPON_MORTAR_LAUNCHER,      // Weapon shape type
        pickup::KIND_WEAPON_MORTAR_GUN,         // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
    {   SHAPE_TYPE_WEAPON_GRENADE_LAUNCHER,     // Weapon shape type
        pickup::KIND_WEAPON_GRENADE_LAUNCHER,   // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_CHAIN_GUN,
    {   SHAPE_TYPE_WEAPON_CHAIN_GUN,            // Weapon shape type
        pickup::KIND_WEAPON_CHAIN_GUN,          // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_BLASTER,
    {   SHAPE_TYPE_WEAPON_BLASTER,              // Weapon shape type
        pickup::KIND_WEAPON_BLASTER,            // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },                                          
                                                
    //INVENT_TYPE_WEAPON_ELF_GUN,                      
    {   SHAPE_TYPE_WEAPON_ELF,                  // Weapon shape type
        pickup::KIND_WEAPON_ELF_GUN,            // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },                                          
                                                
    //INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,             
    {   SHAPE_TYPE_WEAPON_MISSILE_LAUNCHER,     // Weapon shape type
        pickup::KIND_WEAPON_MISSILE_LAUNCHER,   // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_REPAIR_GUN,
    {   SHAPE_TYPE_WEAPON_REPAIR,               // Weapon shape type
        pickup::KIND_NONE,                      // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_SHOCKLANCE ** DELETED FROM GAME
    {   SHAPE_TYPE_WEAPON_SHOCKLANCE,           // Weapon shape type
        pickup::KIND_NONE,                      // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_TARGETING_LASER
    {   SHAPE_TYPE_WEAPON_TARGETING,            // Weapon shape type
        pickup::KIND_NONE,                      // Pickup kind
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    // Weapon ammo
    //INVENT_TYPE_WEAPON_AMMO_DISK
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_AMMO_DISC,                 // Pickup kind (if any)
        25+25,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_AMMO_BULLET
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_AMMO_CHAIN_GUN,            // Pickup kind (if any)
        200+150,                                // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_AMMO_GRENADE
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_AMMO_GRENADE_LAUNCHER,     // Pickup kind (if any)
        15+15,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_AMMO_PLASMA
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_AMMO_PLASMA,               // Pickup kind (if any)
        50+30,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_AMMO_MORTAR
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        10+10,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_WEAPON_AMMO_MISSILE
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        8+4,                                    // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },


    // Secondary weapons

    //INVENT_TYPE_GRENADE_BASIC
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_AMMO_GRENADE_BASIC,        // Pickup kind (if any)
        10+10,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_GRENADE_FLASH
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        10+10,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_GRENADE_CONCUSSION
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        10+10,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

    //INVENT_TYPE_GRENADE_FLARE
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        10+10,                                  // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },


    //INVENT_TYPE_MINE
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_AMMO_MINE,                 // Pickup kind (if any)
        3+5,                                    // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },


    // Armor packs

    //INVENT_TYPE_ARMOR_PACK_AMMO
    {   SHAPE_TYPE_ARMOR_PACK_AMMO,             // Shape type (if any)
        pickup::KIND_ARMOR_PACK_AMMO,           // Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_ARMOR_PACK_CLOAKING ** DELETED FROM GAME
    {   SHAPE_TYPE_ARMOR_PACK_CLOAKING,         // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_ARMOR_PACK_ENERGY
    {   SHAPE_TYPE_ARMOR_PACK_ENERGY,           // Shape type (if any)
        pickup::KIND_ARMOR_PACK_ENERGY,         // Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_ARMOR_PACK_REPAIR
    {   SHAPE_TYPE_ARMOR_PACK_REPAIR,           // Shape type (if any)
        pickup::KIND_ARMOR_PACK_REPAIR,         // Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_ARMOR_PACK_SENSOR_JAMMER
    {   SHAPE_TYPE_ARMOR_PACK_SENSOR_JAMMER,    // Shape type (if any)
        pickup::KIND_ARMOR_PACK_SENSOR_JAMMER,        // Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_ARMOR_PACK_SHIELD
    {   SHAPE_TYPE_ARMOR_PACK_SHIELD,           // Shape type (if any)
        pickup::KIND_ARMOR_PACK_SHIELD,         // Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },


    // Deployable packs

    //INVENT_TYPE_DEPLOY_PACK_BARREL_AA
    {   SHAPE_TYPE_DEPLOY_PACK_BARREL_AA,       // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_BARREL_AA,     // Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_BARREL_ELF
    {   SHAPE_TYPE_DEPLOY_PACK_BARREL_ELF,      // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_BARREL_ELF,    // Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_BARREL_MORTAR
    {   SHAPE_TYPE_DEPLOY_PACK_BARREL_MORTAR,   // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_BARREL_MORTAR, // Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_BARREL_MISSILE
    {   SHAPE_TYPE_DEPLOY_PACK_BARREL_MISSILE,  // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_BARREL_MISSILE,// Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_BARREL_PLASMA
    {   SHAPE_TYPE_DEPLOY_PACK_BARREL_PLASMA,   // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_BARREL_PLASMA, // Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },


    //INVENT_TYPE_DEPLOY_PACK_INVENT_STATION
    {   SHAPE_TYPE_DEPLOY_PACK_INVENT_STATION,  // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_INVENT_STATION,// Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_MOTION_SENSOR ** DELETED FROM GAME
    {   SHAPE_TYPE_DEPLOY_PACK_MOTION_SENSOR,   // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        1,                                      // MaxAmount
        2,                                      // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR
    {   SHAPE_TYPE_DEPLOY_PACK_PULSE_SENSOR,    // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_PULSE_SENSOR,  // Pickup kind (if any)
        2,                                      // MaxAmount
        2,                                      // Mass
    },

    
    //INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR
    {   SHAPE_TYPE_DEPLOY_PACK_TURRET_INDOOR,   // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_TURRET_INDOOR, // Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

    //INVENT_TYPE_DEPLOY_PACK_TURRET_OUTDOOR
    {   SHAPE_TYPE_DEPLOY_PACK_TURRET_OUTDOOR,  // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_TURRET_OUTDOOR,// Pickup kind (if any)
        1,                                      // MaxAmount
        15,                                     // Mass
    },

   
    //INVENT_TYPE_DEPLOY_PACK_SATCHEL_CHARGE
    {   SHAPE_TYPE_DEPLOY_PACK_SATCHEL_CHARGE,  // Shape type (if any)
        pickup::KIND_DEPLOY_PACK_SATCHEL_CHARGE,// Pickup kind (if any)
        1,                                      // MaxAmount
        1,                                      // Mass
    },


    // Belt gear

    //INVENT_TYPE_BELT_GEAR_BEACON
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_NONE,                      // Pickup kind (if any)
        3,                                      // MaxAmount
        1,                                      // Mass
    },

    //INVENT_TYPE_BELT_GEAR_HEALTH_KIT
    {   SHAPE_TYPE_NONE,                        // Shape type (if any)
        pickup::KIND_HEALTH_KIT,                // Pickup kind (if any)
        2+1,                                    // MaxAmount (NOTE: Affected by ammo pack!)
        1,                                      // Mass
    },

} ;


// Camera/weapon lookup data (defined in PlayerObject.cpp)
// (3 character types, 3 armor types, 3 view types, 32 positions to blend between)
vector3 player_object::s_ViewPosLookup[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL][3][32] ;
vector3 player_object::s_WeaponPosLookup[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL][3][32] ;


// General purpose collider (defined in PlayerObject.cpp)
collider player_object::s_Collider ;


xbool   ManiacMode[2] = {0,0};

//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

object* PlayerObjectCreator( void )
{
    return( (object*)(new player_object) );
}

//==============================================================================
//  STATIC FUNCTIONS
//==============================================================================

void player_object::LoadDefines( void )
{
    s32             i,j ;

    // Load common move info
    if (!TWEAK_PLAYER_VIEW)
    {
        s_CommonInfo.SetDefaults() ;
        s_CommonInfo.LoadFromFile("Data/Characters/CommonDefines.txt") ;
    }

    // Load character move infos
    for (i = 0 ; i < CHARACTER_TYPE_TOTAL ; i++)
    {
        for (j = 0 ; j < ARMOR_TYPE_TOTAL ; j++)
        {
            // Setup armor defines
            if (!TWEAK_PLAYER_VIEW)
            {
                // Begin with defaults
                s_MoveInfo[i][j].SetDefaults() ;
                switch(j)
                {
                    case ARMOR_TYPE_LIGHT:
                        s_MoveInfo[i][j].LoadFromFile("Data/Characters/DefaultLightDefines.txt") ;
                        break ;
                
                    case ARMOR_TYPE_MEDIUM:
                        s_MoveInfo[i][j].LoadFromFile("Data/Characters/DefaultMediumDefines.txt") ;
                        break ;

                    case ARMOR_TYPE_HEAVY:
                        s_MoveInfo[i][j].LoadFromFile("Data/Characters/DefaultHeavyDefines.txt") ;
                        break ;
                }

                // Now use defines in character file
                s_MoveInfo[i][j].LoadFromFile(s_CharacterInfo[i][j].MoveDefineFile) ;
            }
        }
	}
}

//==============================================================================

void player_object::SetupViewAndWeaponLookupTable( void )
{
    s32             i,j,v,a ;
    shape_instance  Inst ;

    // Remove root xz only
    Inst.SetRemoveRootNodePosX(TRUE) ;
    Inst.SetRemoveRootNodePosY(FALSE) ;
    Inst.SetRemoveRootNodePosZ(TRUE) ;

    // Load character move infos
    for (i = 0 ; i < CHARACTER_TYPE_TOTAL ; i++)
    {
        for (j = 0 ; j < ARMOR_TYPE_TOTAL ; j++)
        {
            // Lookup shape to use
            s32     ShapeType = GetCharacterInfo((character_type)i,(armor_type)j).ShapeType ;
            shape*  Shape     = tgl.GameShapeManager.GetShapeByType(ShapeType) ;
            ASSERT(ShapeType) ;
            ASSERT(Shape) ;
            
            // Setup an instance like a player
            Inst.SetShape(Shape) ;
            Inst.SetAnimByType(ANIM_TYPE_NONE) ;
            Inst.SetScale(GetMoveInfo((character_type)i,(armor_type)j).SCALE) ;

            // Loop through all views
            for (v = 0 ; v < 3 ; v++)
            {
                // Set additive body anim
                anim_state *BodyUD = Inst.AddAdditiveAnim(ADDITIVE_ANIM_ID_BODY_LOOK_UD) ;
                ASSERT(BodyUD) ;

                // Set masked idle anim
	            anim_state *IdleMask = Inst.AddMaskedAnim(MASKED_ANIM_ID_IDLE) ;
	            ASSERT(IdleMask) ;
	            IdleMask->SetWeight(0.95f) ;   // Make upper half of body be 95% idle

                // HARD CODED RELATIVE TO WEAPON - LOOKS GREAT AND THE SAME ON ALL PLAYER MODELS - AND EASY TO TWEAK!
                static vector3 NormalHeadOffset(0.0f, 0.1f, 0.05f) ;
                static vector3 NormalHandOffset(0.25f, 0.5f, -0.4f) ;

                static vector3 SniperHeadOffset(0.0f, 0.1f, 0.05f) ;
                static vector3 SniperHandOffset(0.3f, 0.3f, -0.35f) ;

                static vector3 RocketHeadOffset(0.0f, 0.1f, 0.05f) ;
                static vector3 RocketHandOffset(0.4f, 0.1f, -0.45f) ;

                // Lookup body anim and view offsets to use
                s32     BodyLookAnimType ;
                vector3 HeadOffset, HandOffset ;
                switch(v)
                {
                    default:
                    case 0: BodyLookAnimType = ANIM_TYPE_CHARACTER_LOOK_SOUTH_NORTH ;
                        HeadOffset = NormalHeadOffset ;
                        HandOffset = NormalHandOffset;
                        break ;

                    case 1: BodyLookAnimType = ANIM_TYPE_CHARACTER_SNIPER_LOOK_SOUTH_NORTH ;
                        HeadOffset = SniperHeadOffset ;
                        HandOffset = SniperHandOffset;
                        break ;

                    case 2: BodyLookAnimType = ANIM_TYPE_CHARACTER_ROCKET_LOOK_SOUTH_NORTH ;
                        HeadOffset = RocketHeadOffset ;
                        HandOffset = RocketHandOffset;
                        break ;
                }
                anim* BodyLookAnim = Shape->GetAnimByType(BodyLookAnimType) ;
                if (!BodyLookAnim)
                    BodyLookAnim = Shape->GetAnimByType(ANIM_TYPE_CHARACTER_LOOK_SOUTH_NORTH) ;

                // Set additive body anim
                ASSERT(BodyUD) ;
                ASSERT(BodyLookAnim) ;
                BodyUD->SetAnim(BodyLookAnim) ;

                // Lookup head up down look anim
                anim* HeadUDAnim = Shape->GetAnimByType(ANIM_TYPE_CHARACTER_HEAD_UP_DOWN) ;
                ASSERT(HeadUDAnim) ;

                // Set additive head up/down anim
                anim_state *HeadUD = Inst.AddAdditiveAnim(ADDITIVE_ANIM_ID_HEAD_LOOK_UD) ;
                ASSERT(HeadUD) ;
                HeadUD->SetAnim(HeadUDAnim) ;
                HeadUD->SetFrameParametric(0.5f) ;

                // Lookup idle anim to use
                anim* IdleAnim = Shape->GetAnimByType(ANIM_TYPE_CHARACTER_IDLE) ;

                // Set masked idle anim
		        ASSERT(IdleMask) ;
		        IdleMask->SetAnim(IdleAnim) ;

                // Loop through all angles
                for (a = 0 ; a < 32 ; a++)
                {
                    // Advance the instance to make it dirty
                    Inst.Advance(0) ;

                    // Calc pitch
                    f32 Pitch = ((f32)a - 15.5f) / 15.5f ;
                    if (Pitch < 0)
                        Pitch *= -s_CommonInfo.VIEW_MIN_PITCH ;
                    else
                        Pitch *= s_CommonInfo.VIEW_MAX_PITCH ;
                    
                    // Calc pitch as a value from 0 (-VIEW_MIN_PITCH) to 1 (VIEW_MAX_PITCH)
                    f32 ParametricPitch = (f32)a / 31.0f ;
                    BodyUD->SetFrameParametric(ParametricPitch) ;

                    // Lookup hand pos from geometry and setup hand rot
                    vector3     HandPos = Inst.GetHotPointWorldPos(HOT_POINT_TYPE_WEAPON_MOUNT) ;
                    quaternion  HandRot(radian3(Pitch,0,0)) ;

                    // Offset view from hand
                    vector3 ViewPos = HandPos + HeadOffset + (HandRot * HandOffset) ;

                    // Store weapon and view pos
                    s_ViewPosLookup  [i][j][v][a] = ViewPos ;
                    s_WeaponPosLookup[i][j][v][a] = HandPos ;
                }                
            }
        }
	}
}

//==============================================================================

const player_object::character_info& player_object::GetCharacterInfo(character_type CharType, armor_type ArmorType)
{
    ASSERT(CharType  >= CHARACTER_TYPE_START) ;
    ASSERT(CharType  <= CHARACTER_TYPE_END) ;

    ASSERT(ArmorType  >= ARMOR_TYPE_START) ;
    ASSERT(ArmorType  <= ARMOR_TYPE_END) ;

    return s_CharacterInfo[CharType][ArmorType] ;
}

//==============================================================================

const player_object::move_info& player_object::GetMoveInfo(character_type CharType, armor_type ArmorType)
{
    ASSERT(CharType  >= CHARACTER_TYPE_START) ;
    ASSERT(CharType  <= CHARACTER_TYPE_END) ;

    ASSERT(ArmorType  >= ARMOR_TYPE_START) ;
    ASSERT(ArmorType  <= ARMOR_TYPE_END) ;

    return s_MoveInfo[CharType][ArmorType] ;
}

//==============================================================================

const player_object::common_info& player_object::GetCommonInfo( void )
{
    return s_CommonInfo ;    
}

//==============================================================================

const player_object::control_info& player_object::GetControlInfo( void )
{
    ASSERT(GetType() == TYPE_PLAYER) ;
    return fegl.WarriorSetups[m_WarriorID].ControlInfo ;
}

//==============================================================================

const player_object::weapon_info& player_object::GetWeaponInfo( invent_type WeaponType )
{
    ASSERT(WeaponType >= INVENT_TYPE_WEAPON_START) ;
    ASSERT(WeaponType <= INVENT_TYPE_WEAPON_END) ;

    return s_WeaponInfo[WeaponType];
}

//==============================================================================

//==============================================================================
//  CLASS FUNCTIONS
//==============================================================================

// Constructor
player_object::player_object( void )
{
    m_Glow          = FALSE;
    m_GlowRadius    = 1.0f;
    m_TVRefreshRate = eng_GetTVRefreshRate() ;

	// Input vars
	m_ProcessInput      = FALSE ;	        // TRUE if player should pay attention to input
	m_View			    = NULL ;			// View to control if processing input
	m_HasInputFocus     = FALSE ;			// TRUE if player has the input, else FALSE
    m_pMM			    = NULL;
    m_LastMoveReceived  = -1;
    m_DirtyBits		    = PLAYER_DIRTY_BIT_ALL ;

    // Put in valid state
    Reset() ;

   
// TEST FOR VU OPTIMIZATION (SKIPPING CONVERTING FROM FLOAT TO INT)
#if 0
    f32 f ;
    s32 i ;

    // convert float to int (screws up top 10 bits of int)
    f = 100.0f ;
    f += (3<<22) ;
    i = *(s32 *)&f ;

    // convert int to float
    i = 100 ;
    f = *(f32 *)&i ;
    f -= (3<<22) ;
   
    i++ ;
#endif

}

//==============================================================================

// Destructor
player_object::~player_object()
{
    // Disable particles before they are destroyed
    for (s32 i = 0 ; i < MAX_JETS ; i++)
        m_JetPfx[i].SetEnabled(FALSE) ;

    // Delete users from UI and HUD

    if( m_HUDUserID != NULL )
        tgl.pHUDManager->DeleteUser( m_HUDUserID );

    if( m_UIUserID != NULL )
        tgl.pUIManager->DeleteUser( m_UIUserID );
}

//==============================================================================

// Sets everything to a valid state
void player_object::Reset()
{
    s32 i ;

    // Client/server vars
    m_DirtyBits             = PLAYER_DIRTY_BIT_ALL ;

    // State vars
    m_PlayerState           = PLAYER_STATE_IDLE ;           // Current state
    m_PlayerLastState       = PLAYER_STATE_IDLE ;           // Last state
    m_PlayerStateTime       = 0.0f ;                        // Time in current state

    m_ContactInfo.Clear() ;                                 // Contact info
    
    m_Health                = 100 ;                         // 0->100
    m_HealthIncrease        = 0 ;                           // 0->100
    m_Energy                = 100.0f ;                      // 0->100
    m_Heat                  = 0.0f ;                        // 0->1
   
//    SetManiacMode( FALSE );

    // Spawn vars
    m_SpawnTime  = 0 ;                                       // Total time of spawn (makes player invunerable)

    // Damage info
    m_DamagePain.Clear() ;
    m_DamageTotalWiggleTime = 0.0f;
    m_DamageWiggleTime      = 0.0f;
    m_DamageWiggleRadius    = 0.0f;

    // Weapon target info
    m_WeaponTargetID        = obj_mgr::NullID ;             // which player is being zapped

    // Weapon Lock Info
    m_MissileLock             = FALSE;                      // TRUE when locked
    m_MissileTargetTrackCount = 0 ;                         // >0 when being tracked by a missile launcer
    m_MissileTargetLockCount  = 0 ;                         // >0 when being locked by a missile

    // Vehicle vars
    m_VehicleID             = obj_mgr::NullID ;             // ID of vehicle (if in one)
    m_LastVehicleID         = obj_mgr::NullID ;
    m_VehicleSeat           = -1 ;                          // Vehicle seat number (if in one)
    m_VehicleDismountTime   = 0 ;                           // Timer for player getting of a vehicle

    // Movement vars
    m_PendingDeltaTime      = 0 ;                           // Delta time not processed in logic
    m_Rot.Zero() ;                                          // Rotation direction
    m_WorldPos.Zero() ;                                     // Current position
    m_GhostCompPos.Zero() ;                                 // Current compression position (for ghosts)
    m_Vel.Zero() ;                                          // Current velocity
    m_Friction              = 0.0f ;                        // Current friction 
    m_AnimMotionDeltaPos.Zero() ;                           // Delta position from animation playback
    m_AnimMotionDeltaRot.Zero() ;                           // Delta rotation from animation playback

    m_AverageVelocity.Zero();
    m_VelocityID            = 0;
    
    for( i=0; i<MAX_VELOCITIES; i++ )
    {
        m_Velocities[i].Zero();
    }
   
    // Lighting vars
    m_EnvAmbientColor       = XCOLOR_WHITE;
    m_EnvLightingDir        = vector3(0,1,0);
    m_EnvDirColor           = XCOLOR_WHITE;
    
    m_GroundRot.Identity() ;
    m_GroundOrientBlend     = 0.0f ;                        // 0=ident,      1=all ground

    m_LastRunFrame           = 0.0f ;
    m_LastContactSurface     = contact_info::SURFACE_TYPE_NONE ;
    m_LastOutOfBounds        = FALSE ;

    m_Jetting                = FALSE ;                      // TRUE if player is jetting
    m_Falling                = FALSE ;                      // TRUE if player is falling
    m_Jumping                = FALSE ;                      // TRUE if player is jumping
    m_Skiing                 = FALSE ;                      // TRUE if player is skiing
    m_JumpSurfaceLastContactTime    = 0 ;                   // Time since player has been on a jump surface        
    m_LastContactTime               = 0 ;                   // Time since player has been on a surface
    m_PickNewMoveStateTime          = 0 ;                   // Time before picking a new move state
    m_LandRecoverTime               = 0 ;                   // Time to recover from a hard land
    m_DeathAnimIndex                = -1 ;                  // Index of death anim to play
    m_Respawn                       = TRUE ;                // Allows player to magically resurrect!
    m_Armed                         = TRUE ;                // Give player weapon
    m_RequestSuicide                = FALSE ;               // If TRUE, player will commit suicide

    // Useful lookup vars
    m_ViewPos.Zero() ;                                      // World position of camera for 1st person
    m_WeaponFireL2W.Identity() ;                            // Local to world position of fire point
    m_WeaponFireRot.Zero() ;                                // World rot of weapon fire point

    // Results of "ComputeAutoAim" call
    m_AutoAimTargetID = obj_mgr::NullID ;                   // Object ID of current auto aim target
    m_AutoAimScore = 0 ;                                    // Score of auto aim
    m_AutoAimTargetCenter.Zero() ;                          // Center of target
    m_AutoAimDir.Zero() ;                                   // Dir to aim for
    m_AutoAimPos.Zero() ;                                   // Pos to aim for
    m_AutoAimArcPivot.Zero() ;                              // Arc pivot to aim for

    m_Move.Clear() ;
    m_MovesRemaining = 0 ;                                  // Used to help auto aim prediction
    m_PredictionTime        = 0.0f ;                        // Client ghost prediction time
    m_BlendMove.Clear() ;
    m_FrameRateDeltaTime = 0.0f ;

    m_ImpactVel.Zero() ;                                    // Velocity before last collision impact
    m_ImpactSpeed = 0 ;                                     // Impact speed into collision plane
    m_GroundImpactVel.Zero() ;                              // Impact vel with last ground plane
    m_GroundImpactSpeed = 0 ;                               // Impact speed with last ground plane
    m_NodeCollID       = 0 ;                                // Used by node collision

    // Input vars
    m_LastMoveReceived      = -1;
    m_ControllerID          = 0;                            // Multi-player controller ID
    
    m_GrenadeKeyTime        = 0 ;                           // Time that grenade key was held down
    m_GrenadeThrowKeyTime   = 0 ;                           // If > 0, player is trying to throw a grenade
    m_GrenadeDelayTime      = 0 ;                           // Must be equal to zero to be able to throw a grenade
    
    m_MineKeyTime           = 0 ;                           // Time that mine key was held down
    m_MineThrowKeyTime      = 0 ;                           // If > 0, player is trying to throw a mine
    m_MineDelayTime         = 0 ;                           // Must be equal to zero to be able to throw a mine
    
    m_PackKeyTime           = 0 ;                           // Used for throwing packs (satchels)

    m_SpamTimer             = 0 ;                           // Timer to stop play spamming
    m_Muted                 = FALSE;

    m_Input.Clear() ;                                       // Current control input
    m_LastInput.Clear() ;                                   // Control input of last tick
    m_DisabledInput.Clear();                                // Keys that are disabled until released
    m_InputMask.Clear();
    m_ClearDisable = TRUE;

    // Network vars
    for (i = 0 ; i < NET_SAMPLES ; i++)
        m_NetSample[i].Init() ;
    m_NetSampleFrame = -1 ;
	m_NetSampleIndex = 0 ;
    m_NetSampleAverage.Init() ;
    m_NetSampleCurrent.Init() ;
    m_NetMoveUpdate = FALSE ;

    // View vars
    m_ViewBlend             = 0.0f ;                        // Blend between views
    m_ViewFreeLookYaw       = 0.0f ;                        // Free look offset
    m_ViewZoomState         = VIEW_ZOOM_STATE_OFF ;         // Zoom state
    m_ViewShowZoomTime      = s_CommonInfo.VIEW_SHOW_ZOOM_CHANGE_TIME ;  // Time to show zoom (0=off)
    
    InitSpringCamera( NULL );
    m_CamDistanceT          = 1.0f;

    // Sound vars
    audio_InitLooping (m_JetSfxID ) ;
    audio_InitLooping (m_WeaponSfxID ) ;
    audio_InitLooping (m_WeaponSfxID2 ) ;
 
    audio_InitLooping (m_MissileFirerSearchSfxID ) ;
    audio_InitLooping (m_MissileFirerLockSfxID ) ;
    audio_InitLooping (m_MissileTargetTrackSfxID ) ;
    audio_InitLooping (m_MissileTargetLockSfxID ) ;

    audio_InitLooping (m_ShieldPackLoopSfxID ) ;
    audio_InitLooping (m_ShieldPackStartSfxID ) ;

    m_NetSfxID                = 0 ;
    m_NetSfxTeamBits          = 0 ;
    m_NetSfxTauntAnimType     = 0 ;

    // Character vars           
    m_CharacterType         = CHARACTER_TYPE_FEMALE ;               // Current character
    m_ArmorType             = ARMOR_TYPE_LIGHT ;                    // Current armor type
    m_SkinType              = SKIN_TYPE_BEAGLE ;                    // Current skin type
    m_VoiceType             = VOICE_TYPE_FEMALE_HEROINE ;           // Current voice type
    m_Name[0]               = '\0';                                 // Name of player
    m_CharacterInfo         = NULL ;                                // Quick lookup pointer to char info
    m_MoveInfo              = NULL ;                                // Quick lookup pointer to move info
    SetCharacter(m_CharacterType, m_ArmorType, m_SkinType, m_VoiceType) ;    // Setup shape        

    // Weapon vars
    m_WeaponCurrentSlot     = 0;                                    // Current weapon slot
    m_WeaponCurrentType     = INVENT_TYPE_WEAPON_DISK_LAUNCHER ;    // Current weapon in hand
    m_WeaponRequestedType   = INVENT_TYPE_WEAPON_DISK_LAUNCHER ;    // Requested weapon
    m_WeaponState           = WEAPON_STATE_IDLE ;                   // Current weapon state
    m_WeaponLastState       = WEAPON_STATE_IDLE ;                   // Last weapon state
    m_WeaponStateTime       = 0.0f ;                                // Total time in current state
    m_WeaponLastStateTime   = 0.0f ;                                // Last frames state time
    m_WeaponInfo            = &s_WeaponInfo[m_WeaponCurrentType] ;
    m_WeaponSpin            = 0.0f ;                                // Current weapon rotation angle
    m_WeaponSpinSpeed       = 0.0f ;                                // Current weapon speed
    m_WeaponOrientBlend     = 0.0f ;                                // 0=all player, 1=all anim
    m_WeaponFireDelay[0]    = 0.0f ;                                // Delta between fires
    m_WeaponFireDelay[1]    = 0.0f ;                                // Delta between fires
    m_WeaponFireDelay[2]    = 0.0f ;                                // Delta between fires
    m_WeaponFireDelay[3]    = 0.0f ;                                // Delta between fires
    m_WeaponFireDelay[4]    = 0.0f ;                                // Delta between fires
    m_WeaponFireDelay[5]    = 0.0f ;                                // Delta between fires
    m_WeaponHasFired        = FALSE ;                               // currently only used for chaingun
    m_WeaponPullBackOffset.Zero() ;                                 // Offset to pull back from walls etc

    // Clear inventories
    m_Loadout.Clear() ;
    m_ClientLoadout.Clear() ;
    m_DefaultLoadoutType = default_loadouts::STANDARD_PLAYER ;

    // Clear inventory loadout
    m_InventoryLoadout.Clear() ;
    m_InventoryLoadoutChanged = FALSE ;

    // Clear view settings changed flag
    m_ViewChanged = FALSE;

	// Clear change team vars
	m_ChangeTeam = FALSE ;						// Flags the player wants to change teams

    // Jet vars
    m_nJets = 0 ;
    for (i = 0 ; i <  MAX_JETS ; i++)
    {
        // Clear hot point
        m_JetHotPoint[i] = NULL ;

        // Create pfx
        if (!m_JetPfx[i].IsCreated())
            m_JetPfx[i].Create(&tgl.ParticlePool, &tgl.ParticleLibrary, PARTICLE_TYPE_JETPACK, m_WorldPos, vector3(0,1,0)) ;

        // Disable pfx for now
        m_JetPfx[i].SetEnabled(FALSE) ;
    }

    // Screen flash vars (done at end since adding ammo will flash!)
    m_ScreenFlashTime  = 0 ;                        // Total time for flash
    m_ScreenFlashColor = XCOLOR_BLACK ;             // Color of flash
    m_ScreenFlashCount = 0 ;                        // Current count (0 = off, 1 = full)
    m_ScreenFlashSpeed = 0 ;                        // Speed of flash 1/Time

    // Sound vars
    m_SoundType             = SOUNDTYPE_NONE ;      // Used for net sending

    // Draw vars
    m_DrawPos.Zero() ;                              // Position player is drawn at
    m_DrawRot.Zero() ;                              // Rotation player is drawn at
    m_DrawViewFreeLookYaw       = 0 ;               // Rotation of free look cam/player head

    m_DrawBBox.Clear() ;                            // Drawing bbox of player
    m_ScreenPixelSize           = 0 ;               // Player pixel size on screen
    m_MaxScreenPixelSize        = 0 ;               // Max screen player pixel size from last frame
    m_PlayerInView              = FALSE;
    m_ForwardBackwardAnimFrame  = 0 ;               // Forward/backward anim frame
    m_SideAnimFrame             = 0 ;               // Side anim frame

    // Instance draw vars
    m_PlayerRot.Zero() ;                            // Player instance world rotation
    m_PlayerDrawRot.Zero() ;                        // Player instance blended world rotation
    m_WeaponRot.Zero() ;                            // Weapon instance world rotation
    m_WeaponDrawRot.Zero() ;                        // Weapon instance blended world rotation
    m_WeaponPos.Zero() ;                            // Weapon instance world position
    m_WeaponDrawPos.Zero() ;                        // Weapon instance blended world position

    // Glow
    ClearGlow();

    // Flags
    DetachFlag();
    SetFlagCount( 0 );

    // Clear UserID for UI
    m_UIUserID = 0;
    m_HUDUserID = 0;
    m_WarriorID = -1;

    // Grab view options from warrior setup
    ResetViewTypes() ;

    // Pack vars
    m_PackCurrentType = INVENT_TYPE_NONE ;              // Current type of pack that player has (if any)
    m_PackActive      = FALSE ;                         // TRUE if pack has been activated
    m_SatchelChargeID = obj_mgr::NullID ;               // ID of satchel object (if any)

    // target lock
    m_TargetIsLocked = FALSE;
    m_LockedTarget = obj_mgr::NullID;

	// Exchange vars
	m_ExchangePickupKind = pickup::KIND_NONE ;			// Type of pickup that can be exchanged

    // Init voting vars
    m_VoteCanStart = TRUE ;                             // Flags that player can start a vote
    m_VoteCanCast  = FALSE ;                            // Flags that player can cast a vote
    m_VoteType     = VOTE_TYPE_NULL ;                   // Type of vote that player is doing
    m_VoteData     = 0 ;                                // Data associated with vote type
}

//==============================================================================

// Puts player at random spawn point
void player_object::ChooseSpawnPoint( void )
{
    // Only reposition on the server
    if (tgl.ServerPresent)
    {        
        // Find best spawn point
        // We want the point where the minimum distance to another player
        // is maximized
        s32 BestI=-1;
        s32 NTries=(tgl.NSpawnPoints/2);

        // Attempt to find a spot with nothing within 10 meters
        while( NTries )
        {
            s32 I = SpawnRandom.irand(0,tgl.NSpawnPoints-1);
            if( (!GameMgr.GetScore().IsTeamBased) || (tgl.SpawnPoint[I].TeamBits & this->GetTeamBits()) )
            {
                f32 MinD = F32_MAX;
                object* pObj ;

                ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
                while( (pObj = ObjMgr.GetNextInType()) )
                {
                    if( pObj != this )
                    {
                        f32 D = (pObj->GetPosition() - tgl.SpawnPoint[I].Pos).LengthSquared();
                        MinD = MIN(MinD,D);
                    }
                }
                ObjMgr.EndTypeLoop();

                ObjMgr.StartTypeLoop( object::TYPE_BOT );
                while( (pObj = ObjMgr.GetNextInType()) )
                {
                    if( pObj != this )
                    {
                        f32 D = (pObj->GetPosition() - tgl.SpawnPoint[I].Pos).LengthSquared();
                        MinD = MIN(MinD,D);
                    }
                }
                ObjMgr.EndTypeLoop();

                /*
                ObjMgr.StartTypeLoop( object::TYPE_CORPSE );
                while( (pObj = ObjMgr.GetNextInType()) )
                {
                    if( pObj != this )
                    {
                        f32 D = (pObj->GetPosition() - tgl.SpawnPoint[I].Pos).LengthSquared();
                        MinD = MIN(MinD,D);
                    }
                }
                ObjMgr.EndTypeLoop();
                */

                // Check for ANY solid objects inside the players bounding box
                bbox BBox;
                SetupBounds( tgl.SpawnPoint[I].Pos, BBox );
                
                ObjMgr.Select( object::ATTR_SOLID_DYNAMIC | object::ATTR_PERMEABLE, BBox );
                while( (pObj = ObjMgr.GetNextSelected()) )
                {
                    if( pObj != this )
                    {
                        MinD = 0.0f;
                        break;
                    }
                }
                ObjMgr.ClearSelection();

                /*
                //
                // Do final check to make sure there is ground less than 2m below the spawn point
                //
                collider  Ray;
                Ray.RaySetup( NULL, tgl.SpawnPoint[I].Pos, tgl.SpawnPoint[I].Pos+vector3(0,-2,0), -1, TRUE );
                ObjMgr.Collide( object::ATTR_SOLID, Ray );
                if( !Ray.HasCollided() )
                {
                    MinD = 0.0f;
                }

                if( MinD > 10.0f )
                {
                    BestI = I;
                    break;
                }
                */

            }
            NTries--;
        }

        // Could not find decent spot so just pick one
        if( BestI == -1 )
        {
            while( 1 )
            {
                s32 I = SpawnRandom.irand( 0, tgl.NSpawnPoints-1 );
                if( (!GameMgr.GetScore().IsTeamBased) || (tgl.SpawnPoint[I].TeamBits & this->GetTeamBits()) )
                {
                    BestI = I;
                    break;
                }
            }
        }

        ASSERT( BestI != -1 );

        // Lookup current spawn point
        ASSERT(BestI >= 0) ;
        ASSERT(BestI < tgl.NSpawnPoints) ;
        const spawn_point& SpawnPoint = tgl.SpawnPoint[BestI] ;

        // Set pos
        SetPos(SpawnPoint.Pos) ;

        /*
        if( VEHICLE_TEST )
        {
            if( (BYPASS == TRUE) && (MISSION == 16) )
                SetPos( vector3( 215.33f, 213.0f, 548.14f ) );
        }
        */

        // Set rot
        radian3 WorldRot ;
        WorldRot.Pitch = SpawnPoint.Pitch ;
        WorldRot.Yaw   = SpawnPoint.Yaw ;
        WorldRot.Roll  = 0.0f;
        SetRot(WorldRot) ;
    }

    // Send all data!
    m_DirtyBits |= PLAYER_DIRTY_BIT_ALL ;

    // Clear movement
	m_ViewFreeLookYaw = 0.0f ;
    m_Vel.Zero();
    m_Move.Clear() ;

    // Clear damage vars
    m_DamagePain.Clear() ;
    m_DamageTotalWiggleTime = 0.0f;
    m_DamageWiggleTime      = 0.0f;
    m_DamageWiggleRadius    = 0.0f;

    // Clear ground rotation
    m_GroundRot.Identity() ;
    m_GroundOrientBlend = 0.0f ; // 0=ident, 1=all ground

    // Update object manager
    ObjMgr.UpdateObject( this );
}

//==============================================================================

// Puts player at a new spawn position and starts off the fx
void player_object::Respawn()
{
    // Make sure things can hit this fine warrior!
    m_AttrBits |= (object::ATTR_SOLID_DYNAMIC | object::ATTR_REPAIRABLE | object::ATTR_ELFABLE | object::ATTR_SENSED) ;

    // Stop other players going through
    m_AttrBits &= ~object::ATTR_PERMEABLE ;

    // Disable all keys until they are released
    m_DisabledInput.Set() ;

    // Put player at new spawn point on server and reset vars
    if (tgl.ServerPresent)
    {
        
        // Put at new spawn point
        ChooseSpawnPoint() ;

        // Send all data!
        m_DirtyBits |= PLAYER_DIRTY_BIT_ALL ;

        // Reset energy
        ASSERT(m_MoveInfo) ;
        m_Energy = m_MoveInfo->MAX_ENERGY ;

        // Reset health
        m_Health = 100;
        m_HealthIncrease = 0 ;

        // Spawn in the eyes of the Game Logic.
        pGameLogic->PlayerSpawned( m_ObjectID );

        // Load default Inventory Loadout
        InventoryReset();

        // Clear reference to satchel charge
        DetachSatchelCharge(TRUE) ; // Fizzle out old

        // Put into idle mode
        Player_SetupState(PLAYER_STATE_IDLE) ;

        // Put weapon into activate state
        Weapon_SetupState(WEAPON_STATE_ACTIVATE) ;
    }

    // Give player their weapon back
    m_Armed = TRUE ;

    // Make weapon draw use all controlled angles
    m_WeaponOrientBlend = 0 ;
    
    // Make the weapons update so the requested index gets passed to the current weapon.
    Weapon_CheckForUpdatingShape();

    // Play the sfx
    if (m_ProcessInput)
        audio_Play(SFX_MISC_RESPAWN) ;
    else
        audio_Play(SFX_MISC_RESPAWN, &m_WorldPos, AUDFLAG_LOCALIZED) ;

    // Start the spawn timer
    m_SpawnTime = SPAWN_TIME ;
    
    InitSpringCamera( NULL );

    // Disarmed during spawn
    SetArmed( FALSE );

    // Grab view options from warrior setup
    ResetViewTypes() ;
}

//==============================================================================

void player_object::CommitSuicide( void )
{
    // Only on server
    if (tgl.ServerPresent)
        ObjMgr.InflictPain( pain::MISC_PLAYER_SUICIDE, this, m_WorldPos, m_ObjectID, m_ObjectID );
    else
    {
        // SB.
        // On client, set request suicide which will be used in "CollectInput"
        // and sent to the server via the m_Move.SuicideKey, and then the server
        // will come into this function, zap the player, then tell the client!
        m_RequestSuicide = TRUE ;
    }
}

//==============================================================================

void player_object::CreateCorpse( void )
{
/*
    // Make sure clients also create corpses
    if (tgl.ServerPresent)
        m_DirtyBits |= PLAYER_DIRTY_BIT_CREATE_CORPSE ;

    // Set the last vehicle bought to NULL
    SetLastVehiclePurchased( obj_mgr::NullID );

    // Create a corpse out of the current pose
	corpse_object* Corpse = (corpse_object*)ObjMgr.CreateObject( object::TYPE_CORPSE ) ;
    ASSERT(Corpse) ;

    // Init the corpse
    Corpse->Initialize(*this) ;

    // Create the corpse on the client side only
    ObjMgr.AddObject( Corpse, obj_mgr::AVAILABLE_CLIENT_SLOT );
*/
}

//==============================================================================

void player_object::SetProcessInput( xbool ProcessInput )
{
    m_ProcessInput = ProcessInput;
}

//==============================================================================

void player_object::Initialize( f32             TVRefreshRate,
                                character_type  CharType,
                                skin_type       SkinType,
                                voice_type      VoiceType,
                                s32             DefaultLoadoutType,
                                const xwchar*   Name /* = NULL */)
{
    // Setup character vars (armor type will get corrected during InventoryReset)
    SetCharacter(CharType, ARMOR_TYPE_LIGHT, SkinType, VoiceType) ;
    
    // Keep name 
    if (Name)
    {
        ASSERT(x_wstrlen(Name) < NAME_LENGTH) ;
        x_wstrcpy(m_Name, Name) ;
/*
        // Turn on monkey mode?
        {
            char tempname[NAME_LENGTH];

            x_wstrcpy(tempname,Name);
            x_wstrtoupper(tempname);

            if( x_strncmp(tempname,"MONKEY",6)==0 )
            {
                x_printf("Monkey Initialized\n");
                SetManiacMode( TRUE );
            }
        }
*/
    }

    // Reset the inventory to the default
    m_DefaultLoadoutType = DefaultLoadoutType ;
    InventoryReset() ;    

    m_ProcessInput  = FALSE;
    m_View          = NULL;
    m_HasInputFocus = FALSE;
    m_TVRefreshRate = TVRefreshRate ;
    m_ControllerID  = -1;
    
    m_pPauseDialog  = 0;

    // Set init state
    //Player_SetupState(PLAYER_STATE_CONNECTING) ;
    Player_SetupState(PLAYER_STATE_IDLE) ;

    m_DirtyBits |= PLAYER_DIRTY_BIT_ALL ;
    m_pMM = NULL;

    // Grab view options from warrior setup
    ResetViewTypes() ;
}

//==============================================================================

void player_object::ServerInitialize( s32 ControllerID )
{
    ASSERT( tgl.ServerPresent ) ;
    ASSERT( ControllerID < tgl.NRequestedPlayers );

    //======================================================================
    // Setup controller vars
    //======================================================================

    // Is this player controlled?
    if (ControllerID != -1)
    {
        m_ProcessInput  = TRUE ;    // TRUE if player should pay attention to input
        m_View          = &tgl.PC[ControllerID].View; // View to control if processing input
        m_HasInputFocus = TRUE;         // TRUE if player has the input, else FALSE
        m_ControllerID  = ControllerID;

        s32 height,width;
        eng_GetRes(width,height);
        
        irect Bounds;
        if( tgl.NRequestedPlayers == 1 )
        {
            Bounds.Set( 0, 0, width, height );
        }
        else
        {
            Bounds.Set( 0, 0, width, height/2 );
            // ANDY'S HACK FOR RETICLE
            //if( m_ControllerID > 0 ) Bounds.Translate( 0, (height/2)-8 );
            if( m_ControllerID > 0 ) Bounds.Translate( 0, (height/2) );
        }
        m_WarriorID = ControllerID;

        /*
        // CJG: Hack to fix controller number up if one player game initiated from
        //      the second controller port
        if( (fegl.GameMode == FE_MODE_ONLINE_ONE_PLAYER) ||
            (fegl.GameMode == FE_MODE_OFFLINE_ONE_PLAYER) ||
            (fegl.GameMode == FE_MODE_CAMPAIGN) )
        {
            m_ControllerID = g_uiLastSelectController;
        }
        */

        m_UIUserID  = tgl.pUIManager->CreateUser( m_ControllerID, Bounds, (s32)this );
        m_HUDUserID = tgl.pHUDManager->CreateUser( this, m_UIUserID );

        m_pPauseDialog  = 0;

        // Set the favorite into the player
        if( m_WarriorID != -1 )
        {
            SetInventoryLoadout( fegl.WarriorSetups[m_WarriorID].Favorites[fegl.WarriorSetups[m_WarriorID].ActiveFavorite[fegl.WarriorSetups[m_WarriorID].ActiveFavoriteArmor]] );
        }
    }

    //======================================================================
    // Setup init pos/state
    //======================================================================

    // Put at random spawn point
    //ChooseSpawnPoint() ;
    //Player_SetupState(PLAYER_STATE_IDLE) ;

    // Spawn for now until game manager states are complete...
    Respawn() ;

    // Grab view options from warrior setup
    ResetViewTypes() ;

    // SB - COMMENT THIS IN TO TEST THE "connect when player is already dead" senario.
    //m_Health = 0 ;
    //m_Energy = 0 ;
    //Player_SetupState(PLAYER_STATE_DEAD, FALSE) ;
    //m_DirtyBits |= PLAYER_DIRTY_BIT_HEALTH ;
    //m_DirtyBits |= PLAYER_DIRTY_BIT_ENERGY ;
}

//==============================================================================

// SB - IMPORTANT INFO-
// This function is called on a client when the game manager finally sets the client
// player into motion.
// NOTE:
// This function must not change player/weapon state/health/energy etc since all of this
// info comes from the server. If you do mess with it then you can break the case where a player
// is killed on the server before the client is setup. Having the player in a state other than dead,
// with zero health is bad!
void player_object::ClientInitialize( xbool             ProcessInput,
                                      s32               ControllerID,
                                      move_manager*     MoveManager)
{
    s32 height,width;

    // Remember character settings
    character_type  CharacterType;
    armor_type      ArmorType;
    skin_type       SkinType;
    voice_type      VoiceType;

    CharacterType   = m_CharacterType;
    ArmorType       = m_ArmorType;
    SkinType        = m_SkinType;
    VoiceType       = m_VoiceType;

    SetCharacter( CharacterType, ArmorType, SkinType, VoiceType );

    // Input vars
    m_ProcessInput  = ProcessInput ;    // TRUE if player should pay attention to input
    m_HasInputFocus = ProcessInput ;            // TRUE if player has the input, else FALSE
    m_ControllerID  = ControllerID;

    m_DirtyBits |= PLAYER_DIRTY_BIT_ALL ;
    m_pMM = MoveManager;

    // Create a UI User for this player
    if( ProcessInput )
    {
        // Controlling players should always have a move manager
        ASSERT(MoveManager) ;

        // Should have a view!!!
        ASSERT( ControllerID < tgl.NRequestedPlayers );
        m_View  = &tgl.PC[ControllerID].View;   // View to control if processing input

        eng_GetRes(width,height);
        irect Bounds;
        if( tgl.NRequestedPlayers == 1 )
        {
            Bounds.Set( 0, 0, width, height );
        }
        else
        {
            Bounds.Set( 0, 0, width, height/2 );
            if( m_ControllerID > 0 ) Bounds.Translate( 0, (height/2) );
        }

        m_WarriorID = ControllerID;

        /*
        // CJG: Hack to fix controller number up if one player game initiated from
        //      the second controller port
        if( (fegl.GameMode == FE_MODE_ONLINE_ONE_PLAYER) ||
            (fegl.GameMode == FE_MODE_OFFLINE_ONE_PLAYER) ||
            (fegl.GameMode == FE_MODE_CAMPAIGN) )
        {
            m_ControllerID = g_uiLastSelectController;
        }
        */

        m_UIUserID  = tgl.pUIManager->CreateUser( m_ControllerID, Bounds, (s32)this );
        m_HUDUserID = tgl.pHUDManager->CreateUser( this, m_UIUserID );

        m_pPauseDialog  = 0;
    }
    else
    {
        m_View = NULL;
    }

    // Set the favorite into the player
    if( m_WarriorID != -1 )
    {
        SetInventoryLoadout( fegl.WarriorSetups[m_WarriorID].Favorites[fegl.WarriorSetups[m_WarriorID].ActiveFavorite[fegl.WarriorSetups[m_WarriorID].ActiveFavoriteArmor]] );
    }

    // Grab view options from warrior setup
    ResetViewTypes() ;
}

//==============================================================================

#ifdef TARGET_PS2
extern "C"
{
    void *ps2_SetStack(void *Addr) ;
}
#endif


//==============================================================================

object::update_code player_object::OnAdvanceLogic( f32 DeltaTime )
{
    // Keep spawn random changing
    SpawnRandom.irand(0,100);

    // Resolve locked target ID.
    if( m_TargetIsLocked && (m_LockedTarget.Seq == -1) )
    {
        // Just fetching the target will update the ID.
        ObjMgr.GetObjectFromID( m_LockedTarget );   
    }

    // Recalculate player views?
    if (TWEAK_PLAYER_VIEW)
    {
        // Every few seconds, update
        static f32 UpdateTime=0;
        UpdateTime += DeltaTime ;
        if (UpdateTime >= 2)
        {
            UpdateTime = 0 ;
            LoadDefines() ;
        }
    }

    // Keep copy of frame rate delta time for the timers that user it...
    m_FrameRateDeltaTime = DeltaTime ;

    // Update network vars
    m_NetSampleCurrent.RecieveDeltaTime += DeltaTime ;

    //x_printfxy(10,10, "RDI:%f", m_NetSampleAverage.RecieveDeltaTime) ;

    //if( m_ProcessInput )
    //    x_printf("Player %1d has PI\n",m_ObjectID);

#ifdef TARGET_PS2
    static xbool UseFastStack=FALSE ;

    register void* SP = NULL ;
    if (UseFastStack)
        SP = ps2_SetStack((void*)0x70004000) ;
#endif

    // Call virtual advance function (may be bots etc)
    static object::update_code UpdateCode ;
    UpdateCode = AdvanceLogic(DeltaTime) ;

#ifdef TARGET_PS2
    if (UseFastStack)
        ps2_SetStack(SP) ;
#endif

    // Clear frame rate delta time so that net calls do not mess up timers etc
    m_FrameRateDeltaTime = 0.0f ;

    // Make sure ghost compression pos is up to date
    UpdateGhostCompPos() ;

    // Return logic update code
    return UpdateCode ;
}

//==============================================================================

void player_object::SetFlagCount( s32 Count )
{
    if( Count != m_FlagCount )
    {
        m_DirtyBits |= PLAYER_DIRTY_BIT_FLAG_STATUS;
        m_FlagCount = Count;
    }
}

//==============================================================================

s32 player_object::GetFlagCount( void )
{
    return( m_FlagCount );
}

//==============================================================================

// Flag carrying functions
void player_object::AttachFlag(s32 FlagTexture)
{
    // Always attach incase the flag texture has changed...

    // Lookup flag shape
    shape* FlagShape = tgl.GameShapeManager.GetShapeByType(SHAPE_TYPE_MISC_FLAG) ;
    ASSERT(FlagShape) ;

    // Turn on drawing of flag
    m_FlagInstance.SetShape(FlagShape) ;
    m_FlagInstance.SetTextureAnimFPS(0) ; // Stop texture anim
    m_FlagInstance.SetTextureFrame((f32)FlagTexture) ;

    // Attach flag to player instance flag mount pos
    ASSERT(m_PlayerInstance.GetShape()) ;
    hot_point *MountHotPoint = m_FlagInstance.GetHotPointByType(HOT_POINT_TYPE_FLAG_MOUNT) ;
    if (MountHotPoint)
    {
        m_FlagInstance.SetParentInstance(&m_PlayerInstance, HOT_POINT_TYPE_FLAG_MOUNT) ;
        m_FlagInstance.SetPos(-MountHotPoint->GetPos()) ;
    }

    // Send across net
    m_DirtyBits |= PLAYER_DIRTY_BIT_FLAG_STATUS ;

    // Turn on the glow
    SetGlow();
}

//==============================================================================

vector3 player_object::DetachFlag()
{
    // Only detach if it's currently attached
    if (m_FlagInstance.GetShape())
    {
        // Turn off drawing of flag
        m_FlagInstance.SetShape(NULL) ;

        // Send across net
        m_DirtyBits |= PLAYER_DIRTY_BIT_FLAG_STATUS ;

        // Turn off the glow
        ClearGlow();
    }

    // Return position of flag on players back
    return m_PlayerInstance.GetHotPointWorldPos(HOT_POINT_TYPE_FLAG_MOUNT) ;
}

//==============================================================================

xbool player_object::HasFlag( void ) const
{
    return( m_FlagCount != 0 );
}

//==============================================================================

void player_object::SetGlow( f32 Radius )
{
    ASSERT( Radius >   0.0f );
    ASSERT( Radius <= 10.0f );

    if( m_Glow )
    {
        ptlight_SetRadius( m_PointLight, Radius );
    }
    else
    {
        vector3 Position = m_WorldPos;
        Position.Y  += 1.0f;
        m_PointLight = ptlight_Create( Position, Radius, XCOLOR_WHITE );
        m_Glow       = TRUE;
    }

    m_DirtyBits |= PLAYER_DIRTY_BIT_GLOW_STATUS;
}

//==============================================================================

void player_object::ClearGlow( void )
{
    if( m_Glow )
    {
        m_Glow = FALSE;
        ptlight_Destroy( m_PointLight );
        m_DirtyBits |= PLAYER_DIRTY_BIT_GLOW_STATUS;
    }
}

//==============================================================================
// Satchel charge functions
//=============================================================================

// Connects the player to the satchel charge and fizzles out any current satchel charge
void player_object::AttachSatchelCharge ( object::id SatchelChargeID )
{
    // Only need to be processed on the server - no dirty bits needed
    if (tgl.ServerPresent)
    {
        // Fizzle out any currently attached satchel charge
        DetachSatchelCharge(TRUE) ;

        // Record connection
        m_SatchelChargeID = SatchelChargeID ;
    }
}

//==============================================================================

// Detaches from current satchel charge (if any) and fizzles it out upon request
void player_object::DetachSatchelCharge ( xbool Fizzle )
{
    // Only need to be processed on the server - no dirty bits needed
    if (tgl.ServerPresent)
    {
        // Fizzle out any current satchel charge?
        if (Fizzle)
        {
            // Attached to satchel charge?
            satchel_charge* SatchelCharge = GetSatchelCharge() ;
            if (SatchelCharge)
            {
                SatchelCharge->SetOriginID(obj_mgr::NullID) ;
                SatchelCharge->Detonate(satchel_charge::EXPLOSION_FIZZLE) ;
            }
        }

        // Clear connection
        m_SatchelChargeID = obj_mgr::NullID ;
    }
}

//==============================================================================

// Returns pointer to current attached satchel charge (if any)
satchel_charge* player_object::GetSatchelCharge( void )
{
    // Should only be called on server!
    ASSERT(tgl.ServerPresent) ;

    // Lookup satchel oibject if there
    if (m_SatchelChargeID != obj_mgr::NullID)
    {
        // Attempy to get satchel charge object
        satchel_charge* pSatchelCharge = (satchel_charge*)ObjMgr.GetObjectFromID(m_SatchelChargeID) ;
        if (pSatchelCharge)
        {
            if (pSatchelCharge->GetType() == object::TYPE_SATCHEL_CHARGE)
                return pSatchelCharge ;
        }
    }

    return NULL ;
}

//==============================================================================
// Weapon target functions
//=============================================================================

void player_object::SetWeaponTargetID  ( object::id ID )
{
    // Dirty?
    if (ID != m_WeaponTargetID)
    {
        // Record and set dirty bit
        m_WeaponTargetID = ID ;
        m_DirtyBits     |= PLAYER_DIRTY_BIT_TARGET_DATA ;
    }
}

//==============================================================================

object::id  player_object::GetWeaponTargetID ( void )
{
    return m_WeaponTargetID;
}


//==============================================================================
// Missile functions
//==============================================================================

void player_object::SetMissileLock ( xbool MissileLock )
{
    // Dirty?
    if (MissileLock != m_MissileLock)
    {
        // Record and set dirty bit
        m_MissileLock = MissileLock ;
        m_DirtyBits  |= PLAYER_DIRTY_BIT_TARGET_DATA ;
    }
}

//==============================================================================

void player_object::UpdateMissileTargetCounts()
{
    // Only do on server since the client gets his info from the dirty bits
    if (tgl.ServerPresent)
    {
        // Update track?
        if (m_MissileTargetTrackCount)
        {
            // Dead?
            if (m_Health == 0)
                m_MissileTargetTrackCount = 0 ;
            else
                m_MissileTargetTrackCount-- ;

            // Only need to tell client if track state changes
            if (m_MissileTargetTrackCount == 0)
                m_DirtyBits |= PLAYER_DIRTY_BIT_TARGET_DATA;
        }

        // Update lock?
        if (m_MissileTargetLockCount)
        {
            // Dead?
            if (m_Health == 0)
                m_MissileTargetLockCount = 0 ;
            else
                m_MissileTargetLockCount-- ;

            // Only need to tell client if lock state changes
            if (m_MissileTargetLockCount == 0)
                m_DirtyBits |= PLAYER_DIRTY_BIT_TARGET_DATA;
        }
    }
}

//==============================================================================

void player_object::MissileTrack( void )
{
    // Only do on server, clients recieve net data for missile track status
    if (!tgl.ServerPresent)
        return ;

    // Don't track the dead
    if (m_Health == 0)
        return ;

    // Only need to tell client if track state changes
    if (m_MissileTargetTrackCount == 0)
        m_DirtyBits |= PLAYER_DIRTY_BIT_TARGET_DATA;

    m_MissileTargetTrackCount = 5 ;
}

//==============================================================================

void player_object::MissileLock( void )
{
    // Only do on server, clients recieve net data for missile lock status
    if (!tgl.ServerPresent)
        return ;

    // Don't track the dead
    if (m_Health == 0)
        return ;

    // Only need to tell client if lock state changes
    if (m_MissileTargetLockCount == 0)
        m_DirtyBits |= PLAYER_DIRTY_BIT_TARGET_DATA;

    m_MissileTargetLockCount = 5 ;
}

//==============================================================================

void player_object::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    // Skip if in an invalid state
    if( (m_PlayerState == PLAYER_STATE_CONNECTING)     ||
        (m_PlayerState == PLAYER_STATE_WAIT_FOR_GAME)  ||
        (m_PlayerState == PLAYER_STATE_DEAD) )
        return;

    // Skip if this is the vehicle the player is attached to
    {
        object* pVehicle = IsAttachedToVehicle();
        if( pVehicle && (pVehicle == Collider.GetMovingObject()) )
            return;
    }    

    // Skip if this is a collision between two bots
    {
        object* pObject = (object*)Collider.GetMovingObject();
        if ( pObject &&
            (GetType() == object::TYPE_BOT) &&
            (pObject->GetType() == object::TYPE_BOT) )
        {
            return;
        }
    }

    // For node collision
    Collider.SetID(m_NodeCollID) ;

    // Collide with geometry?
    object* pMovingObject = (object*)Collider.GetMovingObject() ;
    if( pMovingObject )
    {
        switch(pMovingObject->GetType())
        {
            // Collide with geometry
            case object::TYPE_SATCHEL_CHARGE:
                m_PlayerInstance.ApplyCollision(Collider) ;
                return ;
        }
    }
    
    // Do world bbox level collision with ray colliders
    if ( Collider.GetType() == collider::RAY )
    {
        m_PlayerInstance.ApplyNodeBBoxCollision( Collider );
        return;
    }

    // Collide with bounding box
    Collider.ApplyBBox( m_WorldBBox );
}

//==============================================================================

matrix4* player_object::GetNodeL2W( s32 ID, s32 NodeIndex )
{
    // ID's must match?
    if (ID != m_NodeCollID)
        return NULL ;

    // Make sure instances are uptodate
    UpdateInstanceOrient() ;
    
    // Get player instance render nodes
    render_node* RenderNodes = m_PlayerInstance.CalculateRenderNodes() ;
    if (RenderNodes)
        return &RenderNodes[NodeIndex].L2W ;
    else
        return NULL ;
}

//==============================================================================

void player_object::HitByPlayer( player_object* pPlayer )
{
    s32 i ;

    // Should only be called on the server
    ASSERT(tgl.pServer) ;

    // Is player dead?
    if (m_Health == 0)
    {
        // Loop through all weapon ammo
        for (i = INVENT_TYPE_WEAPON_AMMO_START ; i <= INVENT_TYPE_WEAPON_AMMO_END ; i++)
        {
            // Any ammo left?
            s32  Ammo = GetInventCount((invent_type)i) ;
            if (Ammo)
            {
                // Add to player
                pPlayer->AddInvent((invent_type)i, Ammo) ;

                // Clear so no one else can get it
                SetInventCount((invent_type)i, 0) ;

                // Do fx
                pPlayer->FlashScreen(0.25f, xcolor(0,0,255,128)) ;
                pPlayer->PlaySound(SFX_MISC_PICKUP04);
            }
        }
    }
}

//==============================================================================

void player_object::OnAdd( void )
{
    // Be careful what you put in here as the server "OnProvideInit" packets
    // have already been recieved and process. ie. the ghost players state etc
    // are setup! So don't do things like setting the player state!
}

//==============================================================================
void player_object::OnRemove( void )
{
    // Get off vehicle if on one
    DetachFromVehicle() ;

    // Turn off glow
    ClearGlow();

    // Stop all sounds incase they are playing
    audio_StopLooping(m_WeaponSfxID) ;
    audio_StopLooping(m_WeaponSfxID2) ;
    audio_StopLooping(m_JetSfxID) ;
    audio_StopLooping(m_MissileFirerSearchSfxID) ;
    audio_StopLooping(m_MissileFirerLockSfxID) ;
    audio_StopLooping(m_MissileTargetTrackSfxID) ;
    audio_StopLooping(m_MissileTargetLockSfxID) ;
    audio_StopLooping(m_ShieldPackLoopSfxID);

    // Exit the current weapon state
    Weapon_CallExitState();

    // Kill feedback
    m_FeedbackKickDelay = 0.0f;

    // Disconnect from satchel charged
    DetachSatchelCharge(TRUE) ; // Fizzle out old

    // Leave a corpse behind if game is running.
    if( tgl.GameRunning )
    {
        m_Respawn = FALSE ;

        // Put player into dead mode
        Player_SetupState(PLAYER_STATE_DEAD) ;

        // Create corpse object
        CreateCorpse() ;
    }

}

//==============================================================================

void player_object::SetHealth( f32 Health )
{
    m_Health = Health * 100.0f;
}

//==============================================================================

f32 player_object::GetHealth( void ) const
{
    // Return 0->1
    return (m_Health / 100.0f) ;    
}

//==============================================================================

f32 player_object::GetEnergy( void ) const
{
    // TO DO: Don't use max energy, always use 100.0f for max.
    return( m_Energy / m_MoveInfo->MAX_ENERGY );
}

//==============================================================================

xbool player_object::IsShielded( void ) const
{
    return( (m_PackCurrentType == INVENT_TYPE_ARMOR_PACK_SHIELD) && 
            (m_PackActive) );
}

//==============================================================================

f32 player_object::GetPainRadius( void ) const
{
    return( 1.0f );
}

//==============================================================================

f32 Percent = 0.65f;

vector3 player_object::GetPainPoint( void ) const
{
    radian3 Rot    = m_PlayerInstance.GetRot();
    f32     Height = m_WorldBBox.Max.Y - m_WorldBBox.Min.Y;
    vector3 Result( 0.0f, Height * Percent, 0.0f );
    Result.Rotate( Rot );
    Result += m_WorldPos;
    return( Result );
}

//==============================================================================

const xwchar* player_object::GetLabel( void ) const
{
    return m_Name ;
}

//==============================================================================

f32 player_object::GetSenseRadius( void ) const
{
    if( IsDead() )        return(  0.0f );
    else                  return( 25.0f );
}

//==============================================================================


//==============================================================================
// Vehicle functions
//==============================================================================

void player_object::InitSpringCamera( vehicle* pVehicle )
{
    // Set initial values for spring camera
    m_CamRotAng    = radian3( R_0, R_0, R_0 );
    m_CamRotOld    = radian3( R_0, R_0, R_0 );
    m_CamTiltAng   = radian3( R_0, R_0, R_0 ); 
    m_CamTiltOld   = radian3( R_0, R_0, R_0 );  
    m_CamDistanceT = 1.0f;
    
    // Get initial yaw
    if( pVehicle )
    {
        m_CamRotAng.Yaw = pVehicle->GetRot().Yaw;
        m_CamRotOld.Yaw = pVehicle->GetRot().Yaw;
    }
    else
    {
        m_CamRotAng.Yaw = m_Rot.Yaw;
        m_CamRotOld.Yaw = m_Rot.Yaw;
    }
}

//==============================================================================

void player_object::AttachToVehicle(vehicle* Vehicle, s32 Seat)
{
    ASSERT(Vehicle) ;
    ASSERT(Seat >= 0) ;
    ASSERT(Seat < vehicle::MAX_SEATS) ;

    // Attach to vehicle and get riding it baby...
    m_VehicleSeat   = Seat ;
    m_VehicleID     = Vehicle->GetObjectID() ;
    m_LastVehicleID = m_VehicleID;
    Player_SetupState(PLAYER_STATE_VEHICLE) ;

    InitSpringCamera( Vehicle );

    // Let the game logic know.  (Campaign missions care about this.)
    pGameLogic->PlayerOnVehicle( m_ObjectID, m_VehicleID );

    // Put vehicle dirty bits into the player?
    if ((m_ProcessInput) || (tgl.pServer))
        m_DirtyBits |= Vehicle->GetDirtyBits() ;

    // No more target lock for you!
    if( m_TargetIsLocked )
    {
        m_TargetIsLocked = FALSE;
        m_LockedTarget   = obj_mgr::NullID;
        m_DirtyBits |= PLAYER_DIRTY_BIT_TARGET_LOCK;

        // Audio for server.
        if( m_ProcessInput )
            audio_Play( SFX_TARGET_LOCK_LOST );
    }

    // Flag dirty bits
    m_DirtyBits |= PLAYER_DIRTY_BIT_VEHICLE_STATUS ;
}

//==============================================================================

// Updates player and vehicle vars to break the player/vehicle connection (if there is one)
void player_object::DetachFromVehicle( void )
{
    xbool IsSatDown = (IsSatDownInVehicle() != NULL);

    // Clear rotation
    if( IsSatDown == TRUE )
        m_Rot.Zero() ;

    // Attached?
    vehicle* Vehicle = IsAttachedToVehicle() ;
    if (Vehicle)
    {
        // Free the vehicle seat
        Vehicle->FreeSeat( m_VehicleSeat ) ;

        // inherit the motion of the vehicle
        m_Vel += Vehicle->GetVel();
        m_DirtyBits |= PLAYER_DIRTY_BIT_VEL;

        // Clear vehicle vars
        m_VehicleID   = obj_mgr::NullID ;             // ID of vehicle (if in one)
        m_VehicleSeat = -1 ;                          // Vehicle seat number (if in one)

        // Flag dirty bits
        m_DirtyBits |= PLAYER_DIRTY_BIT_VEHICLE_STATUS ;

        // Clear all vehicle dirty bits
        m_DirtyBits &= ~PLAYER_DIRTY_BIT_VEHICLE_USER_BITS ;

        // Keep vehicle yaw
        if( IsSatDown == TRUE )
            m_Rot.Yaw = Vehicle->GetRot().Yaw ;
        else
            // Convert rotation of player (relative to vehicle) back to a world rotation
            m_Rot.Yaw += Vehicle->GetRot().Yaw; 
        
        // Snap the spring camera behind us
        InitSpringCamera( NULL );
    }
}

//==============================================================================

// Returns current status, given a pointer to the vehicle they are on (or NULL if none)
player_object::vehicle_status player_object::GetVehicleStatus( vehicle* Vehicle ) const
{
    // Any vehicle there?
    if (Vehicle)
    {
        // Piloting?
        if (m_VehicleSeat == 0)
            return VEHICLE_STATUS_PILOT ;
        else
            return VEHICLE_STATUS_MOVING_PASSENGER ;
/*
        // Get seat info
        const vehicle::seat& SeatInfo = Vehicle->GetSeat(m_VehicleSeat) ;

        // Can the player look and shoot?
        if (SeatInfo.CanLookAndShoot)
            return VEHICLE_STATUS_MOVING_PASSENGER ;

        // Controlling a weapon on the vehicle?
        switch(SeatInfo.Type)
        {
            case vehicle::SEAT_TYPE_BOMBER:
            case vehicle::SEAT_TYPE_GUNNER:
                return VEHICLE_STATUS_MOVING_PASSENGER ;
        }

        // Just there for the ride...
        return VEHICLE_STATUS_STATIC_PASSENGER ;
*/    
    }

    // Not in a vehicle
    return VEHICLE_STATUS_NULL ;
}

//==============================================================================

// Returns vehicle if player is on or in one
vehicle* player_object::IsAttachedToVehicle( void ) const
{
    // On a vehicle?
    if (m_VehicleID != obj_mgr::NullID)
    {
        // Make sure vehicle vars are setup
        ASSERT(m_VehicleID != obj_mgr::NullID) ;
        ASSERT(m_VehicleSeat != -1) ;

        // Get vehicle
        object* Object = ObjMgr.GetObjectFromSlot(m_VehicleID.Slot) ;
        if (Object)
        {
            // Is this a vehicle?
            if (Object->GetAttrBits() & object::ATTR_VEHICLE)
                return (vehicle*)Object ;
        }            
    }

    // Not attached to a vehicle
    return NULL ;
}

//==============================================================================

// Returns pointer to vehicle if player is piloting one...
vehicle* player_object::IsVehiclePilot( void ) const
{
    // On a vehicle?
    vehicle* Vehicle = IsAttachedToVehicle() ;
    if (Vehicle)
    {
        // If on seat 0 (it's always the pilot seat), return vehicle
        if (m_VehicleSeat == 0)
            return Vehicle ;
    }

    // Not piloting a vehicle
    return NULL ;
}

//==============================================================================

// Returns pointer to vehicle if player is a passenger on one
vehicle* player_object::IsVehiclePassenger( void ) const
{
    // On a vehicle?
    vehicle* Vehicle = IsAttachedToVehicle() ;
    if (Vehicle)
    {
        // Player is a passenger, unless they are the pilot
        if (m_VehicleSeat != 0)
            return Vehicle ;
    }

    // Not a passenger
    return NULL ;
}

//==============================================================================

// Returns pointer to vehicle if player is in the sitting pose in one
vehicle* player_object::IsSatDownInVehicle( void ) const
{
    // Get vehicle
    vehicle* Vehicle = IsAttachedToVehicle() ;
    if (Vehicle)
    {
        // Get seat info to setup whether the player is sat or not!
        const vehicle::seat& SeatInfo = Vehicle->GetSeat(m_VehicleSeat) ;
        
        // If player cannot look and shoot, then he is sat down
        if (SeatInfo.CanLookAndShoot == FALSE)
            return Vehicle ;
    }

    // Not sat in vehicle
    return NULL ;
}
    
//==============================================================================

// Returns the vehicle seat the player is in (or -1 if not in a vehicle)
s32 player_object::GetVehicleSeat( void ) const
{
    return m_VehicleSeat;
}

//==============================================================================

// Called by vehicles when they move so players can update themselves
void player_object::VehicleMoved( vehicle* Vehicle )
{
    // Only update the player only if they are in this vehicle
    if (IsAttachedToVehicle() == Vehicle)
    {
        // Updates draw vars and world pos when attached to a vehicle
        UpdateDrawVars() ;

        // Make m_ViewPos (for camera positioning) and m_WeaponFirePos are in sync
        UpdateViewAndWeaponFirePos() ;
    }
}

//==============================================================================

xbool player_object::GetManiacMode( void ) const
{
    return (m_WarriorID == -1) ? FALSE : ((ManiacMode[m_WarriorID]) ? 1 : 0);
}

//==============================================================================

void player_object::UnbindTargetID( object::id TargetID )
{
    if( m_TargetIsLocked && (TargetID == m_LockedTarget) )
    {
        // Unlock!
        m_TargetIsLocked = FALSE;
        m_LockedTarget   = obj_mgr::NullID;
        m_DirtyBits     |= PLAYER_DIRTY_BIT_TARGET_LOCK;

        // Audio for server.
        if( m_ProcessInput )
            audio_Play( SFX_TARGET_LOCK_LOST );
    }
}

//==============================================================================

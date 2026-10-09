//==============================================================================
//
//  PlayerObject.hpp
//
//==============================================================================

#ifndef PLAYEROBJECT_HPP
#define PLAYEROBJECT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "Particles/ParticleEffect.hpp"
#include "Objects/Pickups/Pickups.hpp"


//==============================================================================
// DIRTY BIT DEFINES
//==============================================================================

// Dirty bits that will get set upon initialization
#define PLAYER_DIRTY_BIT_PLAYER_STATE			(1<< 0)
#define PLAYER_DIRTY_BIT_POS					(1<< 1)
#define PLAYER_DIRTY_BIT_GHOST_COMP_POS_X		(1<< 2)
#define PLAYER_DIRTY_BIT_GHOST_COMP_POS_Y		(1<< 3)
#define PLAYER_DIRTY_BIT_GHOST_COMP_POS_Z		(1<< 4)
#define PLAYER_DIRTY_BIT_VEL					(1<< 5)
#define PLAYER_DIRTY_BIT_ROT					(1<< 6)
#define PLAYER_DIRTY_BIT_WEAPON_TYPE			(1<< 7)
#define PLAYER_DIRTY_BIT_WEAPON_STATE			(1<< 8)
#define PLAYER_DIRTY_BIT_HEALTH					(1<< 9)
#define PLAYER_DIRTY_BIT_ENERGY					(1<<10)
#define PLAYER_DIRTY_BIT_LOADOUT				(1<<11)
#define PLAYER_DIRTY_BIT_CHARACTER				(1<<12)
#define PLAYER_DIRTY_BIT_FLAG_STATUS			(1<<13)
#define PLAYER_DIRTY_BIT_GLOW_STATUS			(1<<14)
#define PLAYER_DIRTY_BIT_VEHICLE_STATUS			(1<<15)
#define PLAYER_DIRTY_BIT_PACK_TYPE				(1<<16)
#define PLAYER_DIRTY_BIT_TARGET_DATA			(1<<17)
#define PLAYER_DIRTY_BIT_SPAWN					(1<<18)
#define PLAYER_DIRTY_BIT_TARGET_LOCK			(1<<19)
#define PLAYER_DIRTY_BIT_EXCHANGE_PICKUP		(1<<20)

 // The next define contains all the dirty bits that are set when a player is initialized
#define PLAYER_DIRTY_BIT_ALL                    ((1<<(1+20))-1)

// Dirty bits that do not get set upon initialization
#define PLAYER_DIRTY_BIT_WIGGLE					(1<<21)
#define PLAYER_DIRTY_BIT_CREATE_CORPSE  		(1<<22)
#define PLAYER_DIRTY_BIT_SCREEN_FLASH			(1<<23)
#define PLAYER_DIRTY_BIT_PLAY_SOUND				(1<<24)

// Unused bits
//#define PLAYER_DIRTY_BIT_UNUSED_BIT0            (1<<22)

// NOTE - VEHICLE BITS ARE SPECIAL SINCE THEY ARE SHARED BY THE VEHICLE ITSELF WHEN THE PLAYER IS IN IT
//        THEY SHOULD NOT BE INCLUDED IN THE "PLAYER_DIRTY_BIT_ALL" DEFINE
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT0      (1<<25)
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT1      (1<<26)
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT2      (1<<27)
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT3      (1<<28)
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT4      (1<<29)
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT5      (1<<30)
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BIT6      (1<<31)

// All vehicle dirty bits
#define PLAYER_DIRTY_BIT_VEHICLE_USER_BITS       \
    (                                            \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT0  |    \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT1  |    \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT2  |    \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT3  |    \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT4  |    \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT5  |    \
        PLAYER_DIRTY_BIT_VEHICLE_USER_BIT6		 \
    )


// All compression position dirty bits
#define PLAYER_DIRTY_BIT_GHOST_COMP_POS         \
     (  PLAYER_DIRTY_BIT_GHOST_COMP_POS_X |     \
        PLAYER_DIRTY_BIT_GHOST_COMP_POS_Y |     \
        PLAYER_DIRTY_BIT_GHOST_COMP_POS_Z   )

// All unused bits
#define PLAYER_DIRTY_BIT_UNUSED             0
//  (   PLAYER_DIRTY_BIT_UNUSED_BIT0 )



#define PLAYER_MIN_FLAG_TEXTURE 0
#define PLAYER_MAX_FLAG_TEXTURE 2

//#if 1 //(defined (acheng) & defined(TARGET_PC))
//#define DESIGNER_BUILD  1
//#else
//#define DESIGNER_BUILD  0
//#endif

#define MAX_VELOCITIES  21

//==============================================================================
//  FORWARD DECLARATIONS
//==============================================================================
class pickup ;
class move_manager ;
class vehicle ;
class satchel_charge ;


//==============================================================================
//  TYPES
//==============================================================================

class player_object : public object
{

//==========================================================================
// Friend classes
//==========================================================================
friend class corpse_object ;


//==========================================================================
// Defines
//==========================================================================
public:

    //======================================================================
    // Player states
    //======================================================================
    enum player_state
    {
        // Initial states
        PLAYER_STATE_CONNECTING,    // Connecting to server (server is waiting for client to finish loading)

        // None moving states
        PLAYER_STATE_WAIT_FOR_GAME, // Waiting for the game to start
        PLAYER_STATE_TAUNT,         // Playing a taunting animation
        PLAYER_STATE_AT_INVENT,     // At inventory station
        PLAYER_STATE_DEAD,          // Dead

        // States that require move input..
        PLAYER_STATE_IDLE,
        PLAYER_STATE_RUN,
        PLAYER_STATE_JUMP,
        PLAYER_STATE_LAND,
        PLAYER_STATE_JET_THRUST,
        PLAYER_STATE_JET_FALL,
        PLAYER_STATE_FALL,

        // Special case moving states
        PLAYER_STATE_VEHICLE,       // In a vehicle


        // NOTE: Update these if you add/modify any states!!!
        PLAYER_STATE_TOTAL,

        PLAYER_STATE_START      = PLAYER_STATE_CONNECTING,
        PLAYER_STATE_END        = PLAYER_STATE_VEHICLE,

        PLAYER_STATE_MOVE_START = PLAYER_STATE_IDLE,
        PLAYER_STATE_MOVE_END   = PLAYER_STATE_FALL,

    } ;

    //======================================================================
    // Layered anims defines
    //======================================================================
    enum additive_anim_id
    {
        ADDITIVE_ANIM_ID_RECOIL,            // Used when firing weapon
        ADDITIVE_ANIM_ID_BODY_LOOK_UD,      // Used to aim up and down
        ADDITIVE_ANIM_ID_HEAD_LOOK_LR,      // Used to make head look left/right
        ADDITIVE_ANIM_ID_HEAD_LOOK_UD,      // Used to make head look up/down
        
        ADDITIVE_ANIM_ID_TOTAL
    } ;

    //======================================================================
    // Masked anims defines
    //======================================================================
    enum masked_anim_id
    {
        MASKED_ANIM_ID_IDLE,        // Used when running etc.
        
        MASKED_ANIM_ID_TOTAL
    } ;

    //======================================================================
    // Views
    //======================================================================
    enum view_type
    {
        VIEW_TYPE_1ST_PERSON,       // View from eye
        VIEW_TYPE_3RD_PERSON,       // View from behind character

        VIEW_TYPE_TOTAL
    } ;

    //======================================================================
    // Characters
    //======================================================================
    enum character_type
    {
        CHARACTER_TYPE_START = 0,

        CHARACTER_TYPE_FEMALE=CHARACTER_TYPE_START,
        CHARACTER_TYPE_MALE,
        CHARACTER_TYPE_BIO,

        CHARACTER_TYPE_END = CHARACTER_TYPE_BIO,
        CHARACTER_TYPE_TOTAL
    } ;

    //======================================================================
    // Armour
    //======================================================================
    enum armor_type
    {
        ARMOR_TYPE_START = 0,

        ARMOR_TYPE_LIGHT=ARMOR_TYPE_START,
        ARMOR_TYPE_MEDIUM,
        ARMOR_TYPE_HEAVY,

        ARMOR_TYPE_END = ARMOR_TYPE_HEAVY,
        ARMOR_TYPE_TOTAL
    } ;

    //======================================================================
    // Skin
    //======================================================================
    enum skin_type
    {
        SKIN_TYPE_START = 0,

        SKIN_TYPE_BEAGLE=SKIN_TYPE_START,
        SKIN_TYPE_COTP,
        SKIN_TYPE_DSWORD,
        SKIN_TYPE_SWOLF,

        SKIN_TYPE_END = SKIN_TYPE_SWOLF,
        SKIN_TYPE_TOTAL
    } ;

    //======================================================================
    // Voice
    //======================================================================
    enum voice_type
    {
        VOICE_TYPE_START = 0,

        VOICE_TYPE_FEMALE_HEROINE = VOICE_TYPE_START,
        VOICE_TYPE_FEMALE_PROFESSIONAL,
        VOICE_TYPE_FEMALE_CADET,
        VOICE_TYPE_FEMALE_VETERAN,
        VOICE_TYPE_FEMALE_AMAZON,

        VOICE_TYPE_MALE_HERO,
        VOICE_TYPE_MALE_ICEMAN,
        VOICE_TYPE_MALE_ROGUE,
        VOICE_TYPE_MALE_HARDCASE,
        VOICE_TYPE_MALE_PSYCHO,

        VOICE_TYPE_BIO_WARRIOR,
        VOICE_TYPE_BIO_MONSTER,
        VOICE_TYPE_BIO_PREDATOR,

        VOICE_TYPE_END = VOICE_TYPE_BIO_PREDATOR,
        VOICE_TYPE_TOTAL
    } ;


    
    //======================================================================
    // List all items that the player can carry
    //======================================================================

    // NOTE: IF YOU ADD OR RE-ARRAGE THESE ENUMS, MAKE SURE YOU UPDATE
    // player_object::invent_info player_object::s_InventInfo[INVENT_TYPE_TOTAL] IN PLAYEROBJECT.CPP

    enum invent_type
    {
        // Weapons (NOTE: If you modify these, update the misc section defines below!)
        INVENT_TYPE_NONE,       // SPECIAL - Used for no weapon, or energy ammo!

        INVENT_TYPE_WEAPON_DISK_LAUNCHER,
        INVENT_TYPE_WEAPON_PLASMA_GUN,
        INVENT_TYPE_WEAPON_SNIPER_RIFLE,
        INVENT_TYPE_WEAPON_MORTAR_GUN,
        INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,
        INVENT_TYPE_WEAPON_CHAIN_GUN,
        INVENT_TYPE_WEAPON_BLASTER,
        INVENT_TYPE_WEAPON_ELF_GUN,
        INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,
        INVENT_TYPE_WEAPON_REPAIR_GUN,
        INVENT_TYPE_WEAPON_SHOCKLANCE,
        INVENT_TYPE_WEAPON_TARGETING_LASER,

        // Weapon ammo (NOTE: If you modify these, update the misc section defines below!)
        INVENT_TYPE_WEAPON_AMMO_DISK,
        INVENT_TYPE_WEAPON_AMMO_BULLET,
        INVENT_TYPE_WEAPON_AMMO_GRENADE,
        INVENT_TYPE_WEAPON_AMMO_PLASMA,
        INVENT_TYPE_WEAPON_AMMO_MORTAR,
        INVENT_TYPE_WEAPON_AMMO_MISSILE,

        // Secondary weapons
        INVENT_TYPE_GRENADE_BASIC,
        INVENT_TYPE_GRENADE_FLASH,
        INVENT_TYPE_GRENADE_CONCUSSION,
        INVENT_TYPE_GRENADE_FLARE,
        INVENT_TYPE_MINE,

        // Armor packs (NOTE: If you modify these, update the misc section defines below!)
        INVENT_TYPE_ARMOR_PACK_AMMO,
        INVENT_TYPE_ARMOR_PACK_CLOAKING,
        INVENT_TYPE_ARMOR_PACK_ENERGY,
        INVENT_TYPE_ARMOR_PACK_REPAIR,
        INVENT_TYPE_ARMOR_PACK_SENSOR_JAMMER,
        INVENT_TYPE_ARMOR_PACK_SHIELD,

        // Deployable packs (NOTE: If you modify these, update the misc section defines below!)
        INVENT_TYPE_DEPLOY_PACK_BARREL_AA,
        INVENT_TYPE_DEPLOY_PACK_BARREL_ELF,
        INVENT_TYPE_DEPLOY_PACK_BARREL_MORTAR,
        INVENT_TYPE_DEPLOY_PACK_BARREL_MISSILE,
        INVENT_TYPE_DEPLOY_PACK_BARREL_PLASMA,

        INVENT_TYPE_DEPLOY_PACK_INVENT_STATION,
        INVENT_TYPE_DEPLOY_PACK_MOTION_SENSOR,
        INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR,
        
        INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR,
        INVENT_TYPE_DEPLOY_PACK_TURRET_OUTDOOR,
       
        INVENT_TYPE_DEPLOY_PACK_SATCHEL_CHARGE,
 
        // Belt gear (NOTE: If you modify these, update the misc section defines below!)
        INVENT_TYPE_BELT_GEAR_BEACON,
        INVENT_TYPE_BELT_GEAR_HEALTH_KIT,

        // Misc
        INVENT_TYPE_START               = INVENT_TYPE_NONE,
        INVENT_TYPE_END                 = INVENT_TYPE_BELT_GEAR_HEALTH_KIT,
        INVENT_TYPE_TOTAL               = (INVENT_TYPE_END - INVENT_TYPE_START + 1),

        INVENT_TYPE_WEAPON_START        = INVENT_TYPE_NONE,    // SPECIAL - Used for no weapon, or energy ammo
        INVENT_TYPE_WEAPON_END          = INVENT_TYPE_WEAPON_TARGETING_LASER,
        INVENT_TYPE_WEAPON_TOTAL        = (INVENT_TYPE_WEAPON_END - INVENT_TYPE_WEAPON_START + 1),

        INVENT_TYPE_WEAPON_AMMO_START   = INVENT_TYPE_WEAPON_AMMO_DISK,
        INVENT_TYPE_WEAPON_AMMO_END     = INVENT_TYPE_WEAPON_AMMO_MISSILE,
        INVENT_TYPE_WEAPON_AMMO_TOTAL   = (INVENT_TYPE_WEAPON_AMMO_END - INVENT_TYPE_WEAPON_AMMO_START + 1),

        INVENT_TYPE_PACK_START          = INVENT_TYPE_ARMOR_PACK_AMMO,
        INVENT_TYPE_PACK_END            = INVENT_TYPE_DEPLOY_PACK_SATCHEL_CHARGE,
        INVENT_TYPE_PACK_TOTAL          = (INVENT_TYPE_PACK_END - INVENT_TYPE_PACK_START + 1),

    } ;


    //======================================================================
    // List of weapon states
    //======================================================================
   
    enum weapon_state
    {
        WEAPON_STATE_START = 0,

        WEAPON_STATE_ACTIVATE = WEAPON_STATE_START,
        WEAPON_STATE_IDLE,
        WEAPON_STATE_FIRE,
        WEAPON_STATE_RELOAD,
        WEAPON_STATE_DEACTIVATE,
        
        WEAPON_STATE_END = WEAPON_STATE_DEACTIVATE,
        WEAPON_STATE_TOTAL
    } ;

    //======================================================================
    // View zoom 
    //======================================================================
    enum view_zoom_state
    {
        VIEW_ZOOM_STATE_OFF,
        VIEW_ZOOM_STATE_ON,

        VIEW_ZOOM_STATE_TOTAL,
    } ;

    //======================================================================
    // Misc
    //======================================================================
    enum 
    {
        NAME_LENGTH          = 32,  // # of chars in player name
        NET_SAMPLES			 = 16,	// Average of these samples is used in blending
        PLAYER_WEAPON_SLOTS  = 5,   // Number of weapon slots
        MAX_JETS             = 3,   // Max number of jet hot points

    };

    //======================================================================
    // Vehicle defines
    //======================================================================

    enum vehicle_status
    {
        // Status
        VEHICLE_STATUS_NULL,                // Not in a vehicle
        VEHICLE_STATUS_PILOT,               // Piloting vehicle
        VEHICLE_STATUS_STATIC_PASSENGER,    // Static passenger
        VEHICLE_STATUS_MOVING_PASSENGER,    // Looking and shooting passenger

        // Misc
        VEHICLE_STATUS_START = VEHICLE_STATUS_NULL,
        VEHICLE_STATUS_END   = VEHICLE_STATUS_MOVING_PASSENGER
    } ;


    //======================================================================
    // Vote defines
    //======================================================================
    enum vote_type
    {
        // NOTE: This first item MUST BE ZERO in order for the player net/logic to work!
        VOTE_TYPE_NULL,                         // No vote info to send
                                                
        VOTE_TYPE_CHANGE_MAP,                   // Flags player wants to change map
        VOTE_TYPE_KICK_PLAYER,                  // Flags player wants to kick out another player
        VOTE_TYPE_NO,                           // Flags player wants to vote NO
        VOTE_TYPE_YES,                          // Flags player wants to vote YES

        // Do not change these types...
        VOTE_TYPE_TOTAL,                        // Total vote types
        VOTE_TYPE_END   = VOTE_TYPE_TOTAL-1,    // End vote type that can be send
        VOTE_TYPE_START = VOTE_TYPE_NULL+1      // Start bote type that can be sent
    } ;

    //======================================================================
    // Player structures that are loaded from text files
    //======================================================================
    #include "PlayerDefines.hpp"



//==========================================================================
// Structures
//==========================================================================
protected:

    // Contact info
    struct contact_info
    {
        // Defines

        enum surface_type
        {
            SURFACE_TYPE_NONE,
            SURFACE_TYPE_SOFT,
            SURFACE_TYPE_HARD,
            SURFACE_TYPE_SNOW,
            SURFACE_TYPE_METAL,
    
            SURFACE_TYPE_TOTAL
        } ;

        // Surface within 2 meters of  player
        object::type    BelowObjectType ;   // Type of object the surface is (or object::TYPE_NULL)
        surface_type    BelowSurface ;      // Type of surface
        vector3         BelowNormal ;       // Normal of contact surface
        quaternion      BelowRotation ;     // Rotation of ground (can be used to re-direct vels etc)

        surface_type    ContactSurface ;    // Surface player is touching

        surface_type    JumpSurface ;       // If on a jump surface, the type, else SURFACE_TYPE_NONE
        vector3         JumpNormal ;        // Last valid jump normal

        surface_type    RunSurface ;        // If on a run surface, the type, else SURFACE_TYPE_NONE
        vector3         RunNormal ;         // Last valid run normal        

        f32             DistToSurface ;     // Distance (within a few meters) of the ground

        // Functions
        void Clear()
        {
            BelowObjectType = object::TYPE_NULL ;
            BelowSurface    = SURFACE_TYPE_NONE ;
            BelowNormal.Set(0,1,0) ;
            BelowRotation.Identity() ;

            ContactSurface = SURFACE_TYPE_NONE ;

            JumpSurface = SURFACE_TYPE_NONE ;
            JumpNormal.Set(0,1,0) ;

            RunSurface = SURFACE_TYPE_NONE ;
            RunNormal.Set(0,1,0) ;

            DistToSurface = F32_MAX ;
        }

        contact_info()  { Clear() ; } 
    } ;

public:

    // Input structure
    struct player_input
    {
        // Movement keys
        s8          FireKey ;           // Fire button
        s8          JetKey ;            // Jetpack button
        s8          JumpKey ;           // Jump button
        s8          ZoomKey ;           // Zoom button
        s8          FreeLookKey ;       // Free look key
                                        
        // Change keys                  
        s8          NextWeaponKey ;     // Next weapon key
        s8          PrevWeaponKey ;     // Previous weapon key
        s8          ZoomInKey ;         // Zoom in
        s8          ZoomOutKey ;        // Zoom out
        s8          ViewChangeKey ;     // View change button
                                        
        // Use keys                     
        s8          MineKey ;           // Deploy mine button
        s8          PackKey ;           // Use pack button
        s8          GrenadeKey ;        // Throw grenade button
        s8          HealthKitKey ;      // Use health kit button
                                        
        // Drop keys                    
        s8          DropWeaponKey ;     
        s8          DropPackKey ;       
        s8          DropBeaconKey ;     
        s8          DropFlagKey ;       
                                        
        // Special keys                 
        s8          FocusKey ;          // Focus key
        s8          SuicideKey ;        // Suicide key
        s8          VoiceMenuKey ;      // Voice command menu key
        s8          OptionsMenuKey ;    // Options menu key  
        s8          TargetingLaserKey ; // Brings up targeting laser weapon
        s8          TargetLockKey;      // Player locking onto a target
        s8          ComplimentKey;      // Player complimenting
        s8          TauntKey;           // Player taunting
        s8          ExchangeKey;        // Player exchanging weapon or pack

        s8          SpecialJumpKey;     // Non-maskable jump button (for vehicle training)

        // Movement values              
        f32         MoveX ;             // Move left-right value (-1 to 1)
        f32         MoveY ;             // Move forward-backwards value (-1 to 1)
        f32         LookX ;             // Look left-right value (-1 to 1)
        f32         LookY ;             // Look up-down value (-1 to 1)

        // Clears out structure
        void player_input::Clear()
        {
            x_memset((void*)this, 0, sizeof(player_input)) ;
        }

        // Sets all key presses
        void player_input::Set()
        {
            x_memset((void*)this, 0xFF, sizeof(player_input)) ;
            *(u32*)&MoveX = 0x3 ;
            *(u32*)&MoveY = 0x3 ;
            *(u32*)&LookX = 0x3 ;
            *(u32*)&LookY = 0x3 ;
            OptionsMenuKey = 0;
        }

        // Constructor - clears out structure
        player_input::player_input()
        {
            Clear() ;
        }
    } ;

public:

    // Inventory
    struct inventory
    {
        // Data
        s32                     Counts[INVENT_TYPE_TOTAL] ;   // List of all invent counts

        // Functions
        void    Clear           ( void ) ;

        // Net functions
        void    Write           (       bitstream& BitStream ) const ;
        void    Read            ( const bitstream& BitStream ) ;

        void    ProvideUpdate   (       bitstream& BitStream, inventory& ClientInventory ) const ;
        void    AcceptUpdate    ( const bitstream& BitStream ) ;
    } ;

    // Loadout
    struct loadout
    {
        // Data
        armor_type              Armor;                              // Type of armor
                                
        invent_type             Weapons[PLAYER_WEAPON_SLOTS];       // Type of weapons
        s32                     NumWeapons;                         // Number of weapons
                                
        object::type            VehicleType ;                       // Type of vehicle to get at stations
                                
        inventory               Inventory ;                         // Items

        // Functions
        void    Clear ( void ) ;

        // Net functions
        void    Write           (       bitstream& BitStream ) const ;
        void    Read            ( const bitstream& BitStream ) ;

        void    ProvideUpdate   (       bitstream& BitStream, loadout& ClientLoadout ) const ;
        void    AcceptUpdate    ( const bitstream& BitStream ) ;
    };

    // Move structure that is sent to server
    struct move
    {
        // Time vars
        u32                 DeltaVBlanks ;              // How many tv frames this move applies        
        f32                 DeltaTime ;                 // How long this move applies

        // Movement input vars
        f32                 MoveX, MoveY ;              // Stick movement
        f32                 LookX, LookY ;              // Stick look

        // View vars
        u32                 ViewZoomFactor ;            // View zoom factor multiplier
        xbool               ViewChanged;
        xbool               View3rdPlayer;
        xbool               View3rdVehicle;

        // Movement key vars
        s8                  FireKey ;                   // Fire button
        s8                  JetKey ;                    // Jetpack button
        s8                  JumpKey ;                   // Jump button
        s8                  FreeLookKey ;               // Free look key

        // Weapon change key vars
        s8                  NextWeaponKey ;             // Next weapon key
        s8                  PrevWeaponKey ;             // Previous weapon key
                                                
        // "Use" key vars
        s8                  MineKey ;                   // Deploy mine button
        s8                  PackKey ;                   // Use pack button
        s8                  GrenadeKey ;                // Throw grenade button
        s8                  HealthKitKey ;              // Use health kit button
        
        // "Drop" key vars
        s8                  DropWeaponKey ;             // Drops current weapon
        s8                  DropPackKey ;               // Drops pack
        s8                  DropBeaconKey ;             // Drops beacon
        s8                  DropFlagKey ;               // Drops flag

        // Special key vars
        s8                  SuicideKey ;                // Comitt suicide
        s8                  TargetingLaserKey ;         // Switch to targeting laser
        s8                  TargetLockKey;              // Target Lock
        s8                  ComplimentKey;              // Compliment
        s8                  TauntKey;                   // Taunt
        s8                  ExchangeKey;                // Exchange

        // Net sound vars
        s32                 NetSfxID ;                  // Net sound to play
        u32                 NetSfxTeamBits ;            // Net sound team bits
        s32                 NetSfxTauntAnimType ;       // Net sound taunt anim

        // Inventory vars
        xbool               InventoryLoadoutChanged ;   // Flags sending of new inventory loadout
        loadout             InventoryLoadout ;          // Current inventory load

		// Team bit vars
		s8					ChangeTeam ;				// Flags the player wants to change teams

        // Voting vars
        s8                  VoteType ;                  // Flags player wants to start or reply to a vote
        s8                  VoteData ;                  // Data associated with the vote

        // Misc debug vars
        s8                  MonkeyMode ;                // He's in monkey mode

        // Initializes all members
        void Clear          ( void ) ;

        // Truncate ready for sending over net
        void Truncate       ( f32 TVRefreshRate ) ;

        // Util functions to write a f32 that has the range -1 to 1
        void TruncateF32    ( f32& Value, s32 Bits ) ;
        void WriteF32       ( bitstream& BitStream, f32  Value, s32 Bits ) ;
        void ReadF32        ( const bitstream& BitStream, f32& Value, s32 Bits ) ;

        // Read/write move to net bitstream (used when writing/reading from client->server)
        void Write          ( bitstream& BitStream ) ;
        void Read           ( const bitstream& BitStream, f32 TVRefreshRate ) ;

        // Read/write move to net bitstream (used when writing/reading from server->client)
        void ProvideUpdate  (       bitstream& BitStream ) ;
        void AcceptUpdate   ( const bitstream& BitStream ) ;
    } ;

	// Struct used to keep average of net data stats
	struct net_sample
	{
		// Data
		f32	RecieveDeltaTime ;	// Time since last recieve
		f32 SecsInPast ;		// Time for data to travel here

		// Init stats
		void Init( void )
		{
			RecieveDeltaTime = 0 ;
			SecsInPast       = 0 ;
		}

        // Clear stats
		void Clear( void )
		{
			RecieveDeltaTime = 0 ;
			SecsInPast       = 0 ;
		}

		// Add stats
		net_sample& operator += ( const net_sample& S )
		{
			RecieveDeltaTime += S.RecieveDeltaTime ;
			SecsInPast		 += S.SecsInPast   ;
			return( *this );
		}

		// Divide for average
		net_sample& operator /= ( s32 D )
		{
			RecieveDeltaTime /= D ;
			SecsInPast		 /= D ;
			return( *this );
		}
	} ;

public:

    // Character info structure
    struct character_info
    {
        s32                 ShapeType ;             // Shape type to use
        const char*         MoveDefineFile ;        // File that contains the move defines
    } ;

    // Weapon info structure
    struct weapon_info
    {
        invent_type         AmmoType ;              // Ammo the weapon uses
        f32                 AmmoNeededToAllowFire ; // Must have this amount to even consider firing
        f32                 AmmoUsedPerFire ;       // Ammo used per fire
        f32                 FireDelay ;             // Delay between fires
        f32                 VelocityInheritance;    // (0-1)
        f32                 MuzzleSpeed;            // m/s
        f32                 MaxAge;                 // s
    } ;

    // Ammo info structure
    struct invent_info
    {
        s32                 ShapeType ;             // Associated shape type (if any)
        pickup::kind        PickupKind ;            // Kind of pickup (if any)
        s32                 MaxAmount ;             // Max amount any player can have
        f32                 Mass ;                  // Mass of object
    } ;


private:
    // Structure used for blending players
    struct blend_move
    {
        vector3     DeltaPos ;              // Pos to start from
        radian3     DeltaRot ;              // Rot to start from
        radian      DeltaViewFreeLookYaw ;  // View rot to start from
        
        f32         Blend ;     // Blend (0=all blend pos, 1=all world pos)
        f32         BlendInc ;  // Speed of blend

        // Initializes all members
        void Clear ( void )
        {
            DeltaPos.Zero() ;
            DeltaRot.Zero() ;
            DeltaViewFreeLookYaw = 0 ;

            Blend    = 0 ;
            BlendInc = 0 ;
        }
    };

    
//==========================================================================
// Static data
//==========================================================================
public:

    // Static data (defined in PlayerObject.cpp)
    static common_info      s_CommonInfo ;                                              // Common info for all players
    static character_info   s_CharacterInfo[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL] ;   // All character info
    static move_info        s_MoveInfo[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL] ;        // All character move info
    static weapon_info      s_WeaponInfo[INVENT_TYPE_WEAPON_TOTAL] ;                    // All weapon info
    static invent_info      s_InventInfo[INVENT_TYPE_TOTAL] ;                           // All invent info

    // Camera/weapon lookup data (defined in PlayerObject.cpp)
    // (3 character types, 3 armor types, 3 view types, 32 positions to blend between)
    static vector3          s_ViewPosLookup[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL][3][32] ;
    static vector3          s_WeaponPosLookup[CHARACTER_TYPE_TOTAL][ARMOR_TYPE_TOTAL][3][32] ;

    // General purpose collider (defined in PlayerObject.cpp)
    static collider         s_Collider ;

    // Collision defines (defined in PlayerLogic.cpp)
    static u32              PLAYER_LANDABLE_ATTRS ;
    static u32              PLAYER_COLLIDE_ATTRS ;
    static u32              WEAPON_COLLIDE_ATTRS ;
    static u32              PLAYER_PICKUPS_ATTRS ;

    // Key press defines (defined in PlayerLogic.cpp)
    static f32              GRENADE_THROW_DELAY_TIME ;
    static f32              GRENADE_THROW_MIN_TIME ;
    static f32              GRENADE_THROW_MAX_TIME ;

    static f32              MINE_THROW_DELAY_TIME ;
    static f32              MINE_THROW_MIN_TIME ;
    static f32              MINE_THROW_MAX_TIME ;

    static f32              SPAM_MIN_TIME ;
    static f32              SPAM_MAX_TIME ;
    static f32              SPAM_DELAY_TIME ;

    // Debug vars (defined in PlayerLogic.cpp)
    static f32              PLAYER_DEBUG_SPEED ;
    static xbool            PLAYER_DEBUG_GROUND_TRACK ;
    static xbool            PLAYER_DEBUG_3RD_PERSON ;

    // Movement defines (defined in PlayerLogic.cpp)
    static f32              PLAYER_SEND_MOVE_INTERVAL ;
    static f32              SPAWN_TIME ;                            
    static f32              MOVING_EPSILON ;
    static f32              MAX_STEP_HEIGHT ;
    static f32              VERTICAL_STEP_DOT ;
    static f32              MIN_FACE_DISTANCE ;
    static f32              TRACTION_DISTANCE ;
    static f32              NORMAL_ELASTICITY ;
    static s32              MOVE_RETRY_COUNT ;
    static f32              FALLING_THRESHOLD ;
    static f32              JUMP_SKIP_CONTACTS_MAX ;
    static f32              DEPLOY_DIST ;
    static f32              BOT_DEPLOY_DIST ;
    static f32              PLAYER_JUMP_SURFACE_LAST_CONTACT_TIME_MAX ;
    static f32              PLAYER_LAND_RECOVER_TIME_MAX ;
    static f32              PLAYER_LIP_TEST_DIST ;
    static f32              PLAYER_STEP_CLEAR_DIST ;
    static f32              PLAYER_CONTACT_THRESHOLD ;

	// Ground tracking defines
    static f32              GROUND_TRACKING_MAX_DIST ;
    static f32              GROUND_TRACKING_DOT ;

    // Contact info defines...
    static f32              PLAYER_CONTACT_DIST ;
    static f32              PLAYER_HIT_BY_PLAYER_DIST ;
    static f32              PLAYER_SURFACE_CHECK_DIST ;

    // Energy tweakables
    static f32              SHIELD_PACK_DRAIN_SCALER ;
    static f32              HEALTH_INCREASE_PER_SEC ;

    // Network defines (defines in PlayerNet.cpp)

    // Blending defines
    static  xbool           PLAYER_CLIENT_BLENDING ;
    static  f32             PLAYER_CLIENT_BLEND_TIME_SCALER ;
    static  f32             PLAYER_CLIENT_BLEND_TIME_MAX ;

    static  xbool           PLAYER_SERVER_BLENDING ;
    static  f32             PLAYER_SERVER_BLEND_TIME_SCALER ;
    static  f32             PLAYER_SERVER_BLEND_TIME_MAX ;

    static  f32             PLAYER_BLEND_DIST_MAX ;
                                                        
    // Max values                                           
    static  f32             PLAYER_VEL_MAX ;
    static  f32             PLAYER_ROT_MAX ;
    static  f32             PLAYER_ENERGY_MAX ;
    static  f32             PLAYER_HEALTH_MAX ;

    // Bits to use when sending to an input player machine
    static  s32             INPUT_PLAYER_VEL_BITS ;
    static  s32             INPUT_PLAYER_ROT_BITS ;
    static  s32             INPUT_PLAYER_ENERGY_BITS ;
    static  s32             INPUT_PLAYER_HEALTH_BITS ;
    static  s32             INPUT_PLAYER_SPAWN_TIME_BITS ;
    static  s32             PLAYER_JUMP_SURFACE_LAST_CONTACT_TIME_BITS ;
    static  s32             PLAYER_LAND_RECOVER_TIME_BITS ;

    // Bits to use when sending to a ghost player machine
    static  s32             GHOST_PLAYER_COMP_POS_BITS ;
    static  s32             GHOST_PLAYER_POS_BITS ;
    static  f32             GHOST_PLAYER_POS_PRECISION ;
    static  s32             GHOST_PLAYER_VEL_BITS ;
    static  s32             GHOST_PLAYER_ROT_BITS ;
    static  s32             GHOST_PLAYER_ENERGY_BITS ;
    static  s32             GHOST_PLAYER_HEALTH_BITS ;
    static  s32             GHOST_PLAYER_SPAWN_TIME_BITS ;
    
    // Prediction vars
    static  f32             PLAYER_TIME_STEP_MIN ;
    static  f32             GHOST_PLAYER_LATENCY_TIME_MAX ;
    static  f32             GHOST_PLAYER_LATENCY_TIME_STEP ;
    static  f32             GHOST_PLAYER_PREDICT_TIME_MAX ;
                            
    // misc                 
    static  xbool           PLAYER_NO_PRIORITY ;
	static  xbool			SHOW_NET_STATS ;


//==========================================================================
// Data
//==========================================================================
protected:

    // Instance vars            
    shape_instance              m_PlayerInstance ;                      // Instance for player
    shape_instance              m_WeaponInstance ;                      // Instance for attached weapon
    shape_instance              m_BackpackInstance ;                    // Instance for attached backpack
    shape_instance              m_FlagInstance ;                        // Instance for attached flag (if any)
                                
    // Character vars           
    character_type              m_CharacterType ;                       // Current character
    armor_type                  m_ArmorType ;                           // Current armor type
    skin_type                   m_SkinType ;                            // Current skin type
    voice_type                  m_VoiceType ;                           // Current voice type
    xwchar                      m_Name[NAME_LENGTH] ;                   // Name of player
    character_info              *m_CharacterInfo ;                      // Quick lookup pointer to char info
    move_info                   *m_MoveInfo ;                           // Quick lookup pointer to move info
    s32                         m_DefaultLoadoutType ;                  // Default loudout type
    loadout                     m_Loadout;                              // Current loadout
    loadout                     m_ClientLoadout;                        // Current client loadout
    loadout                     m_InventoryLoadout;                     // Loadout Inventory station will fill
    xbool                       m_InventoryLoadoutChanged ;             // Flags data should be sent to server/client
	xbool						m_ChangeTeam ;							// Flags the player wants to change teams

    // Weapon vars
    s32                         m_WeaponCurrentSlot;                    // Current slot
    invent_type                 m_WeaponCurrentType ;                   // Current weapon in hand
    invent_type                 m_WeaponRequestedType ;                 // Requested weapon
    weapon_state                m_WeaponState ;                         // Current weapon state
    weapon_state                m_WeaponLastState ;                     // State before current weapon state
    f32                         m_WeaponStateTime ;                     // Total time in current state
    f32                         m_WeaponLastStateTime ;                 // Last frames state time
    weapon_info                 *m_WeaponInfo ;                         // Weapon info of current weapon being used
    f32                         m_WeaponSpin ;                          // Current weapon rotation angle
    f32                         m_WeaponSpinSpeed ;                     // Current weapon speed
    f32                         m_WeaponOrientBlend ;                   // 0=all player, 1=all anim
    f32                         m_WeaponFireDelay[6] ;                  // If none zero, the player cannot fire!
    xbool                       m_WeaponHasFired ;                      // currently only used for chaingun
    vector3                     m_WeaponPullBackOffset ;                // Offset to add to weapon so it's not in walls etc

    // State vars                                                   
    player_state                m_PlayerState ;                         // Current state
    player_state                m_PlayerLastState ;                     // State before current state
    f32                         m_PlayerStateTime  ;                    // Time in current state
    f32                         m_Health ;                              // 0->100
    f32                         m_HealthIncrease ;                      // Health that's added over time
    f32                         m_Energy ;                              // 0->100
    f32                         m_Heat ;                                // Heat signature (0->1)
    s32                         m_ManiacState;                          // Maniac State

    // Spawn vars
    f32                         m_SpawnTime ;                           // Total time of spawn (makes player invunerable)

    // Damage info
    pain                        m_DamagePain  ;
    f32                         m_DamageWiggleRadius;
    f32                         m_DamageTotalWiggleTime;
    f32                         m_DamageWiggleTime;

    // Weapon target info
    object::id                  m_WeaponTargetID;                       // which player is being zapped
    vector3                     m_LocalWeaponTargetPt;                  // also for targeting laser
    xbool                       m_TargetingLaserIsValid;                // also for targeting laser
    xbool                       m_MissileLock;                          // TRUE when missile launcher locked to a target
    s32                         m_MissileTargetTrackCount ;             // >0 when being tracked by a missile launcer
    s32                         m_MissileTargetLockCount ;              // >0 when being locked by a missile
   
    // Vehicle vars
    object::id                  m_VehicleID ;                           // ID of vehicle (if in one)
    object::id                  m_LastVehicleID ;                       // ID of vehicle the player has just exited
    s32                         m_VehicleSeat ;                         // Vehicle seat number (if in one)
    f32                         m_VehicleDismountTime ;                 // Timer for player getting of a vehicle
    object::id                  m_LastVehiclePurchasedID ;              // Last vehicle spawned by this player

    // Movement vars                                                    
    f32                         m_PendingDeltaTime ;                    // Delta time not processed in logic
    radian3                     m_Rot ;                                 // Rotation direction
    vector3                     m_GhostCompPos ;                        // Current compression position (for ghosts)
    vector3                     m_Vel ;                                 // Current velocity
    f32                         m_Friction ;                            // Current friction
    vector3                     m_AnimMotionDeltaPos ;                  // Delta position from animation playback
    radian3                     m_AnimMotionDeltaRot ;                  // Delta rotation from animation playback
    contact_info                m_ContactInfo ;                         // Current plane contact info
    quaternion                  m_GroundRot ;                           // Ground rotation
    f32                         m_GroundOrientBlend ;                   // 0=ident,      1=all ground
    xbool                       m_ClearDisable ;                        // 1=Auto clear disable keys
    player_input                m_Input;                                // Current control input
    player_input                m_LastInput;                            // Control input of last tick
    player_input                m_DisabledInput;                        // Keys that are disabled until released
    player_input                m_InputMask;                            // The current input mask
    
    player_object::move         m_Move;                                 // Current move player is using
    s32                         m_MovesRemaining ;                      // Used to help auto aim prediction
    blend_move                  m_BlendMove;                            // Used to smooth out client and server player draw
    f32                         m_PredictionTime ;                      // Client ghost prediction time
    f32                         m_FrameRateDeltaTime ;                  // Constant frame rate delta time
    f32                         m_FrameWeaponStateTime ;                // Constant frame rate delta weapon state time
    f32                         m_LastRunFrame ;                        // Used for playing running sounds
    contact_info::surface_type  m_LastContactSurface ;                  // Used for playing land sound
    xbool                       m_LastOutOfBounds ;                     // Used for playing out of bounds sound
    xbool                       m_Jetting ;                             // TRUE if player is jetting
    xbool                       m_Falling ;                             // TRUE if player is falling
    xbool                       m_Jumping ;                             // TRUE if player is jumping
    xbool                       m_Skiing ;                              // TRUE if player is skiing
    f32                         m_JumpSurfaceLastContactTime ;          // Time since player has not been on a jump surface        
    f32                         m_LastContactTime ;                     // Time since last contact surface was hit
    f32                         m_PickNewMoveStateTime ;                // Time before picking a new move state
    f32                         m_LandRecoverTime ;                     // Time to recover from a hard land
    s32                         m_DeathAnimIndex ;                      // Index of death anim to play
    xbool                       m_Respawn ;                             // Allows player to magically resurrect!
    xbool                       m_Armed ;                               // If TRUE, player holds a weapon
    xbool                       m_RequestSuicide ;                      // If TRUE, player will commit suicide
    
    s32                         m_VelocityID;                           // Current slot in velocity table
    vector3                     m_AverageVelocity;                      // The computed average velocity
    vector3                     m_Velocities[ MAX_VELOCITIES ];         // Table of previous velocities (circular list)

    // Useful lookup vars
    vector3                     m_ViewPos ;                             // World position of camera for 1st person
    matrix4                     m_WeaponFireL2W ;                       // Local to world position of fire point
    radian3                     m_WeaponFireRot ;                       // World rot of weapon fire point

    // Results of "ComputeAutoAim" call
    object::id                  m_AutoAimTargetID ;                     // Object ID of current auto aim target
    f32                         m_AutoAimScore ;                        // Score of auto aim
    vector3                     m_AutoAimTargetCenter ;                 // Center of target
    vector3                     m_AutoAimDir ;                          // Dir to aim for
    vector3                     m_AutoAimPos ;                          // Pos to aim for
    vector3                     m_AutoAimArcPivot ;                     // Arc pivot to aim for

    // Lighting vars
    xcolor                      m_EnvAmbientColor ;
    xcolor                      m_EnvDirColor ;
    vector3                     m_EnvLightingDir ;

    // Collision impact vars
    vector3                     m_ImpactVel ;                           // Velocity before last collision impact
    f32                         m_ImpactSpeed ;                         // Impact speed into collision plane
    vector3                     m_GroundImpactVel ;                     // Impact vel with last ground plane
    f32                         m_GroundImpactSpeed ;                   // Impact speed with last ground plane
    s32                         m_NodeCollID ;                          // Used by node collision

    // View vars
    view*                       m_View ;                                // View to control if processing input
    view_type                   m_PlayerViewType ;                      // Current view type
    view_type                   m_VehicleViewType ;                     // Current view type
    xbool                       m_ViewChanged;                          // Has current view setting changed?
    f32                         m_ViewBlend ;                           // Blend between views
    f32                         m_ViewFreeLookYaw ;                     // Free look offset
    view_zoom_state             m_ViewZoomState ;                       // Current zoom state on or off
    f32                         m_ViewShowZoomTime ;                    // Time to show new zoom size
    
    radian3                     m_CamRotAng;                            // Rotation angle of spring camera  (current)
    radian3                     m_CamRotOld;                            // Rotation angle of spring camera  (previous)
    radian3                     m_CamTiltAng;                           // Rotation angle of camera tilting (current)
    radian3                     m_CamTiltOld;                           // Rotation angle of camera tilting (previous) 
    f32                         m_CamDistanceT;                         // Parametric distance of spring camera from look at point

    // Screen flash vars
    f32                         m_ScreenFlashTime ;                     // Total time for flash
    xcolor                      m_ScreenFlashColor ;                    // Color of flash
    f32                         m_ScreenFlashCount ;                    // Current count (0 = off, 1 = full)
    f32                         m_ScreenFlashSpeed ;                    // Speed of flash 1/Time

    // Sound vars
    s32                         m_SoundType ;                           // Used for net sending

    // Server/client vars       
    move_manager*               m_pMM;
                                
    // Input vars               
    xbool                       m_ProcessInput ;                        // TRUE if player should pay attention to input
    xbool                       m_HasInputFocus ;                       // TRUE if player has the input, else FALSE
    f32                         m_TVRefreshRate ;                       // Hz refresh rate of TV system player is controlled on
    s32                         m_LastMoveReceived;                     // Last move received on server
    s32                         m_ControllerID;                         // Mutli player controller ID
    
    f32                         m_GrenadeKeyTime ;                      // Time that grenade key was held down
    f32                         m_GrenadeThrowKeyTime ;                 // If > 0, player is trying to throw a grenade
    f32                         m_GrenadeDelayTime ;                    // Must be equal to zero to be able to throw a grenade

    f32                         m_MineKeyTime ;                         // Time that mine key was held down
    f32                         m_MineThrowKeyTime ;                    // If > 0, player is trying to throw a mine
    f32                         m_MineDelayTime ;                       // Must be equal to zero to be able to throw a mine
    
    f32                         m_PackKeyTime ;                         // Used for throwing packs (satchels)

    f32                         m_SpamTimer ;                           // Timer to stop play spamming
    xbool                       m_Muted;

    // Network vars
    net_sample					m_NetSample[NET_SAMPLES] ;		// Total samples to average over
    s32                         m_NetSampleFrame ;              // Current frame of net sample
    s32                         m_NetSampleIndex ;				// Current sample number to recieve
    net_sample                  m_NetSampleCurrent ;			// Current time passed since last recieve
    net_sample                  m_NetSampleAverage ;            // Current net sample average
    xbool                       m_NetMoveUpdate ;				// TRUE if recieving a client move update from the server

    // Sound vars
    f32                         m_FeedbackKickDelay;            // Used for force feedback. Initial "kick" of a weapon can be modified using this
    s32                         m_JetSfxID ;                    // Used for looping jet sfx

    s32                         m_WeaponSfxID ;                 // Used for looping weapon sfx
    s32                         m_WeaponSfxID2 ;                // Used for looping weapon sfx (
    
    s32                         m_MissileFirerSearchSfxID ;     // Sfx when firing missiles
    s32                         m_MissileFirerLockSfxID ;       // Sfx when firing missiles
    s32                         m_MissileTargetTrackSfxID ;     // Sfx when being tracked by missiles
    s32                         m_MissileTargetLockSfxID ;      // Sfx when being tracked by missiles

    s32                         m_ShieldPackLoopSfxID;          // Looping sound for shield pack
    s32                         m_ShieldPackStartSfxID;         // Starting sound for shield pack

    s32                         m_NetSfxID ;                    // ID of sound to play over the net
    u32                         m_NetSfxTeamBits ;              // Team bits to send over the net
    s32                         m_NetSfxTauntAnimType ;         // Taunt anim (if any ) to play over the net

    // Jet vars
    s32                         m_nJets ;                       // Number of jets on player shape
    hot_point*                  m_JetHotPoint[MAX_JETS] ;       // List of jet hot points for current player shape
    pfx_effect                  m_JetPfx[MAX_JETS] ;            // List of jet nozzles

    // "Glow" information
    xbool                       m_Glow;                         // Is the player glowing?
    f32                         m_GlowRadius;                   // Radius of glow
    s32                         m_PointLight;                   // Handle to a point light.
    s32                         m_FlagCount;                    // How many flags is player carrying?

    // Draw vars (which take blending into account)
    vector3                     m_DrawPos ;                     // Position player is drawn at
    radian3                     m_DrawRot ;                     // Rotation player is drawn at
    radian                      m_DrawViewFreeLookYaw ;         // Rotation of free look cam/player head
    bbox                        m_DrawBBox ;                    // Drawing bbox of player
    xbool                       m_PlayerInView ;                // Called once per view
    f32                         m_ScreenPixelSize ;             // Player pixel size on screen
    f32                         m_MaxScreenPixelSize ;          // Max screen player pixel size from last frame
    f32                         m_ForwardBackwardAnimFrame ;    // Forward/backward anim frame
    f32                         m_SideAnimFrame ;               // Side anim frame
    
    // Instance draw vars
    radian3                     m_PlayerRot ;                   // Player instance world rotation
    radian3                     m_PlayerDrawRot ;               // Player instance blended world rotation
    radian3                     m_WeaponRot ;                   // Weapon instance world rotation
    radian3                     m_WeaponDrawRot ;               // Weapon instance blended world rotation
    vector3                     m_WeaponPos ;                   // Weapon instance world position
    vector3                     m_WeaponDrawPos ;               // Weapon instance blended world position

    // Game manager variables
    s32                         m_PlayerIndex;

    // UI data
    s32                         m_UIUserID;                     // User ID for User Interface system
    s32                         m_HUDUserID;                    // User ID for HUD system
    s32                         m_WarriorID;                    // 0 or 1
    class ui_tabbed_dialog*     m_pPauseDialog;                 // Pointer to open Pause Dialog

    // Pack vars
    invent_type                 m_PackCurrentType ;             // Current type of pack that player has (if any)
    xbool                       m_PackActive ;                  // TRUE if pack has been activated
    object::id                  m_SatchelChargeID ;             // ID of satchel object (if any)

    // Sensor net vars
    u32                         m_SensedBits;                   // TeamBits of enemies which sense player

    // Target lock variables
    xbool                       m_TargetIsLocked;
    xbool                       m_LockAimSolved;
    object::id                  m_LockedTarget;
    f32                         m_LockBreakTimer;
    
	// Exchange vars
	pickup::kind				m_ExchangePickupKind ;			// Kind of pickup that can be exchanged

    // Vote vars
    xbool                       m_VoteCanStart ;                // Flags that player can start a vote
    xbool                       m_VoteCanCast ;                 // Flags that player can cast a vote
    vote_type                   m_VoteType ;                    // Type of vote that player is doing
    s32                         m_VoteData ;                    // Data associated with vote type


//==================================================================================
// Functions
//==================================================================================
public:

        //==========================================================================
        // Static functions in PlayerObject.cpp
        //==========================================================================

        // Loads defines from .txt files
static      void                             LoadDefines      ( void ) ;
static      void                             SetupViewAndWeaponLookupTable( void ) ;

static const player_object::character_info&  GetCharacterInfo ( character_type CharType, armor_type ArmorType ) ;
static const player_object::move_info&       GetMoveInfo      ( character_type CharType, armor_type ArmorType ) ;
static const player_object::common_info&     GetCommonInfo    ( void ) ;
static const player_object::weapon_info&     GetWeaponInfo    ( invent_type WeaponType ) ;


        //==========================================================================
        // Functions in PlayerObject.cpp
        //==========================================================================

        // Constructor/destructor
                    player_object   ( void );
                   ~player_object   ( void );

        // Initialize vars  
virtual void        Reset           ( void );

        // Misc useful functions
        void        ChooseSpawnPoint( void ) ;
virtual void        Respawn         ( void ) ;
        void        CommitSuicide   ( void ) ;
        void        CreateCorpse    ( void ) ;

        // Call from server
        void        Initialize      ( f32                           TVRefreshRate,
                                      character_type                CharType,
                                      skin_type                     SkinType,
                                      voice_type                    VoiceType,
                                      s32                           DefaultLoadoutType,
                                      const xwchar*                 Name = NULL) ;

        // Call on server to connect controls
        void        ServerInitialize( s32               ControllerID );

        // Call from client
        void        ClientInitialize( xbool             ProcessInput,
                                      s32               ControllerID,
                                      move_manager*     MoveManager) ;

        // Logic functions
virtual update_code AdvanceLogic                    ( f32 DeltaTime );
        update_code OnAdvanceLogic                  ( f32 DeltaTime );
        void        OnCollide                       ( u32 AttrBits, collider& Collider );
        matrix4*    GetNodeL2W                      ( s32 ID, s32 NodeIndex ) ;
        void        HitByPlayer                     ( player_object* pPlayer ) ;
        void        OnAdd                           ( void );
        void        OnRemove                        ( void );
       
        void        SetHealth       ( f32 Health );
        f32         GetHealth       ( void ) const;     // 0.0 thru 1.0
        f32         GetEnergy       ( void ) const;     // 0.0 thru 1.0
        xbool       IsShielded      ( void ) const;
        f32         GetPainRadius   ( void ) const;
        vector3     GetPainPoint    ( void ) const;
        const xwchar* GetLabel        ( void ) const;

        // Input functions
        const player_object::control_info&    GetControlInfo   ( void ) ;
        void        SetProcessInput     ( xbool ProcessInput );
        xbool       GetProcessInput     () const                    { return m_ProcessInput ;  }
        xbool       GetHasInputFocus    () const                    { return m_HasInputFocus ; }
        f32         GetTVRefreshRate    () const                    { return m_TVRefreshRate ; }
        xbool       GetRecievingMoveUpdate( void ) const            { return m_NetMoveUpdate ; }
        void        SetClearDisable( xbool  Flag )                  { m_ClearDisable = Flag ;  }
        s32         GetUserID           () const                    { return m_UIUserID ;      }
        
        player_object::player_input& GetInput           ( void )    { return m_Input;         }
        player_object::player_input& GetLastInput       ( void )    { return m_LastInput;     }
        player_object::player_input& GetDisabledInput   ( void )    { return m_DisabledInput; }
        player_object::player_input& GetInputMask       ( void )    { return m_InputMask;     }

        // Misc functions
        void        SetRespawn          ( xbool Respawn )           { m_Respawn = Respawn ; }
        void        SetArmed            ( xbool Armed )             { m_Armed   = Armed ;   }
        xbool       GetArmed            ( void )                    { return m_Armed; }

        // Flag carrying functions
        void        SetFlagCount        ( s32 Count );
        s32         GetFlagCount        ( void );
        xbool       HasFlag             ( void ) const;
        void        AttachFlag          ( s32 FlagTexture );
        vector3     DetachFlag          ( void );

        // "Glow" functions
        void        SetGlow             ( f32 Radius = 2.0f );
        void        ClearGlow           ( void );

        // Deployed satchel charge functions

        // Connects the player to the satchel charge and fizzles out any currently attached satchel charge
        void            AttachSatchelCharge ( object::id SatchelChargeID ) ;

        // Detaches from current satchel charge (if any) and fizzles it out upon request
        void            DetachSatchelCharge ( xbool Fizzle ) ;

        // Returns pointer to current attached satchel charge (if any)
        satchel_charge* GetSatchelCharge    ( void ) ;

        // Weapon target functions            
        void        SetWeaponTargetID           ( object::id ID ) ;
        object::id  GetWeaponTargetID           ( void ) ;
        vector3     GetLocalWeaponTargetPt      ( void ) { return m_LocalWeaponTargetPt; }

        // Missile functions
        void        SetMissileLock              ( xbool MissileLock ) ;
        void        UpdateMissileTargetCounts   ( void ) ;
virtual void        MissileTrack                ( void );
virtual void        MissileLock                 ( void );
        xbool       IsMissileTracking           ( void ) { return( m_MissileTargetTrackCount > 0 ); }
        xbool       IsMissileLockedOn           ( void ) { return( m_MissileTargetLockCount  > 0 ); }

        // Target lock functions
        xbool       IsTargetLocked  ( void )    { return m_TargetIsLocked; }
        object::id  GetLockedTarget ( void )    { return m_LockedTarget;   }
virtual void        UnbindTargetID  ( object::id TargetID );
                                            
        // Damage wiggle                    
        void        SetDamageWiggle         ( f32 Time, f32 Radius );

        // Name                             
const   xwchar*     GetName                 ( void ) const              { return m_Name; }
                                            
        // Dirty bit functions              
        void        SetDirtyBits            ( u32 DirtyBits )           { m_DirtyBits = DirtyBits ; }

        // Vehicle functions
        void            AttachToVehicle         ( vehicle* Vehicle, s32 Seat ) ;
        void            DetachFromVehicle       ( void ) ;
        void            InitSpringCamera        ( vehicle* pVehicle );

        vehicle_status  GetVehicleStatus        ( vehicle* Vehicle ) const ;

        // Vehicle queries
        vehicle*        IsAttachedToVehicle     ( void ) const ;
        vehicle*        IsVehiclePilot          ( void ) const ;
        vehicle*        IsVehiclePassenger      ( void ) const ;
        vehicle*        IsSatDownInVehicle      ( void ) const ;

        void            ResetVehicleDismountTime( void )     { m_VehicleDismountTime = 2.0f; }
        f32             GetVehicleDismountTime  ( void )     { return m_VehicleDismountTime ; }
        s32             GetVehicleSeat          ( void ) const;

        // Called by vehicles when they move so players can update themselves
        void            VehicleMoved            ( vehicle* Vehicle ) ;

        // Use by vehicle fade out logic
        void            SetLastVehiclePurchased ( object::id ID )  { m_LastVehiclePurchasedID = ID; }
        object::id      GetLastVehiclePurchased ( void )           { return m_LastVehiclePurchasedID; }

        // status functions
        xbool       OnGround                ( void ) const { return m_ContactInfo.ContactSurface ; }
        xbool       NearGround              ( void ) const { return m_ContactInfo.DistToSurface <= 2.0f; }
        xbool       IsDead                  ( void ) const { return (m_Health <= 0) || (m_PlayerState == PLAYER_STATE_DEAD); }
        
        // sensor net functions
        f32         GetSenseRadius          ( void ) const;
        void        ClearSensedBits         ( void )            { m_SensedBits  = 0x00; }
        void        AddSensedBits           ( u32 SensedBits )  { m_SensedBits |= SensedBits; }
        u32         GetSensedBits           ( void ) const      { return( m_SensedBits ); }

        //==========================================================================
        // Functions in PlayerNet.cpp
        //==========================================================================

        // Ghost player net functions
        void    UpdateGhostCompPos  ( void ) ;

        void    WriteGhostCompPos   ( bitstream& BitStream, const f32 Value ) ;
        void    ReadGhostCompPos    ( const bitstream& BitStream, f32& Value ) ;
        
        void    WriteGhostPos       ( bitstream& BitStream, const vector3& Value, u32& DirtyBits ) ;
        void    ReadGhostPos        ( const bitstream& BitStream, vector3& Value ) ;

        void    WriteGhostVel       ( bitstream& BitStream, const vector3& Value ) ;
        void    ReadGhostVel        ( const bitstream& BitStream, vector3& Value ) ;

        // Input player net functions
        void    Truncate            ( void ) ;
        
        void    TruncateVel         ( f32&     Value, s32 Bits ) ;
        void    TruncateVel         ( vector3& Value, s32 Bits ) ;
        
        void    WriteVel            ( bitstream& BitStream, const f32      Value, s32 Bits ) ;
        void    WriteVel            ( bitstream& BitStream, const vector3& Value, s32 Bits ) ;
        
        void    ReadVel             ( const bitstream& BitStream, vector3& Value, s32 Bits ) ;
        void    ReadVel             ( const bitstream& BitStream, f32&     Value, s32 Bits ) ;

        void    TruncateRot         ( radian&  Value, s32 Bits ) ;
        void    TruncateRot         ( radian3& Value, s32 Bits ) ;
        
        void    WriteRot            ( bitstream& BitStream, const radian   Value, s32 Bits ) ;
        void    WriteRot            ( bitstream& BitStream, const radian3& Value, s32 Bits ) ;
        
        void    ReadRot             ( const bitstream& BitStream, radian&  Value, s32 Bits ) ;
        void    ReadRot             ( const bitstream& BitStream, radian3& Value, s32 Bits ) ;

        void    UpdateNetSamples	( f32 SecsInPast ) ;

virtual void    WriteDirtyBitsData  (       bitstream& BitStream, u32& DirtyBits, f32 Priority, xbool MoveUpdate ) ;
virtual xbool   ReadDirtyBitsData   ( const bitstream& BitStream, f32 SecInPast ) ;

        // Logic functions
        update_code OnAcceptUpdate      ( const bitstream& BitStream, f32 SecInPast ) ;
        void        OnProvideUpdate     ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
        void        OnProvideMoveUpdate ( bitstream& BitStream, u32& DirtyBits ) ;
        void        OnAcceptInit        ( const bitstream& BitStream );
        void        OnProvideInit       (       bitstream& BitStream );

        // Move manager functions
        void        SetMoveManager      ( move_manager* pMM );
        void        ReceiveMove         ( player_object::move& Move, s32 MoveSeq );
        
        const move& GetMove             ( void ) const { return m_Move ; }
        s32         GetMovesRemaining   ( void ) const { return m_MovesRemaining ;  }
        void        SetMovesRemaining   ( s32 Moves )  { m_MovesRemaining = Moves ; }

        //==========================================================================
        // Functions in PlayerRender.cpp
        //==========================================================================

        // Render functions
        void        PreRender           ( void ) ;
        void        Render              ( void ) ;
        void        PostRender          ( void ) ;
        void        DebugRender         ( void ) ;
        xbool       IsBeingRendered     ( void ) { return m_PlayerInView; }

        //==========================================================================
        // Functions in PlayerLogic.cpp
        //==========================================================================

        // Shape functions
        protected:
        void        SetPlayerShape              (shape *PlayerShape, s32 SkinTexture) ;
        void        SetWeaponShape              (shape *WeaponShape) ;
        void        SetBackpackShape            (shape *BackpackShape) ;
                                                
        public:                                 
        void        SetCharacter                (character_type CharType,
                                                 armor_type     ArmorType,
                                                 skin_type      SkinType,
                                                 voice_type     VoiceType) ;

        character_type  GetCharacterType        ( void ) const;
        armor_type      GetArmorType            ( void ) const;
        s32             GetVoiceSfxBase         ( void ) const;
static  s32             GetVoiceSfxBase         ( voice_type VoiceType );

        // Position functions                   
        const vector3&  GetPos                  ( void ) const          { return m_WorldPos ; }
        void            WarpToPos               ( const vector3& Pos ) ;
        void            SetPos                  ( const vector3& Pos ) ;
        
        // Rotation functions
        const radian3&  GetRot                  ( void ) const          { return m_Rot ; }
        void            SetRot                  ( const radian3& Rot ) ;
        radian3         GetGroundRot            ( void ) const ;

        // Gets world rotation (taking being on a vehicle into account)
        void            GetWorldPitchYaw        ( f32& Pitch, f32& Yaw ) ;
        
        const vector3&  GetVel                  ( void ) const          { return m_Vel ; }
        void            SetVel                  ( const vector3& Vel ) ;
virtual void            OnAddVelocity           ( const vector3& Velocity ) ;

virtual vector3         GetBlendPos             ( void ) const          { return m_DrawPos ; }          
virtual radian3         GetBlendRot             ( void ) const          { return m_DrawRot ; }
virtual radian3         GetRotation             ( void ) const          { return m_Rot ; }
virtual vector3         GetVelocity             ( void ) const          { return m_Vel ; }
virtual f32             GetMass                 ( void ) const;
        
        vector3         GetAverageVelocity      ( void ) const          { return m_AverageVelocity ; }

        // Use these when drawing icons etc around the player
        const vector3&  GetDrawPos              () const                { return m_DrawPos ; }
        const radian3&  GetDrawRot              () const                { return m_DrawRot ; }

        // Net sfx functions
        void        SetNetSfxID                 ( s32 ID )              { m_NetSfxID = ID ;             }
        s32         GetNetSfxID                 ( void )                { return m_NetSfxID ;           }
        
        void        SetNetSfxTeamBits           ( u32 TeamBits )        { m_NetSfxTeamBits = TeamBits;  }
        u32         GetNetSfxTeamBits           ( void )                { return m_NetSfxTeamBits ;     }
        
        void        SetNetSfxTauntAnimType      ( s32 TauntAnimType )   { m_NetSfxTauntAnimType = TauntAnimType ; }
        s32         GetNetSfxTauntAnimType      ( void )                { return m_NetSfxTauntAnimType ;          }

        // Main loop functions
        void        UpdateDrawVars              ( void ) ;
        void        Advance                     ( f32 DeltaTime ) ;

        // Move functions
        void        CollectInput                ( f32 DeltaTime ) ;
        void        CreateMoveFromInput         ( f32 DeltaTime ) ;
        void        CreateMonkeyMove            ( f32 DeltaTime ) ;

        void        SetupContactInfo            ( const collider::collision& Collision, f32 HitDist ) ;
        void        SetupContactInfo            ( collider::collision* pCollision = NULL ) ;

        void        ApplyMove                   ( void ) ;
        void        CheckForThrowingAndFiring   ( f32 DeltaTime ) ;

		// Checks for any pickup that the player can exchange (weapons + packs) with.
		void		CheckForExchangePickup		( void ) ;

        // Pack functions
        void        SetPackCurrentType          ( player_object::invent_type Type ) ;
        void        SetPackActive               ( xbool Active ) ;
        void        CheckForUsingPacks          ( f32 DeltaTime ) ;

        void        DebugCheckValidPosition     ( void ) ;
        void        ProcessPermeableCollisions  ( collider& Collider ) ;

        xbool       Step                        ( const collider::collision& Collision,
                                                  const u32                  AttrMask,
                                                  const vector3&             Move, 
                                                  const vector3&             MoveDir,
                                                        collider&            Collider, 
                                                        f32&                 MaxStep ) ;

        void        KeepAboveTerrain            ( void ) ;

        void        ApplyPhysics                ( void ) ;
        void        UpdateLighting              ( void ) ;

        void        UpdateViewAndWeaponFirePos  ( void ) ;
        void        UpdateInstanceOrient        ( void ) ;

        // Bounds functions
        void        SetupBounds                 ( const vector3& Pos, bbox& BBox ) ;
        void        CalcBounds                  ( void ) ;
        xbool       IsOutOfBounds               ( void ) ;

        // View functions
        const char *GetViewZoomFactorString     ( void ) ;
        f32         GetViewZoomFactor           ( void ) ;

        f32         GetCurrentZoomFactor        ( void ) const;
        f32         GetCurrentFOV               ( void ) const;

        void        SetupView                   ( void ) ;
        void        SetupValidViewPos           ( const vector3& Start, vector3& ViewPos ) const;
        f32         Get3rdPersonView            ( vector3 &ViewPos, radian3 &ViewRot, f32 &ViewFOV ) const;
        f32         Get1stPersonView            ( vector3 &ViewPos, radian3 &ViewRot, f32 &ViewFOV ) const;
        void        GetView                     ( vector3 &ViewPos, radian3 &ViewRot, f32 &ViewFOV ) const;
        xbool       IsZoomed                    ( void ) const { return( m_ViewZoomState == VIEW_ZOOM_STATE_ON ); }

        void        ResetViewTypes              ( void ) ;
        view_type   GetPlayerViewType           ( void );
        void        SetPlayerViewType           ( view_type ViewType );
        view_type   GetVehicleViewType          ( void );
        void        SetVehicleViewType          ( view_type ViewType );

        view*       GetView                     ( void ) const;
        f32         GetCamDistanceT             ( void ) const { return( m_CamDistanceT ); }

        // Fast lookup functions
  const vector3&    GetViewPos                  ( void ) const ;
  const vector3&    GetWeaponPos                ( void ) const ;
  const matrix4&    GetWeaponFireL2W            ( void ) const ;
  const vector3     GetWeaponFirePos            ( void ) const ;
  const radian3&    GetWeaponFireRot            ( void ) const ;
  const vector3     GetWeaponFireDir            ( void ) const ;
        vector3     GetBlendFirePos             ( s32 Index = 0 );

  const matrix4     GetTweakedWeaponFireL2W     ( void ) const ;

        // Auto aim get functions
        object::id  GetAutoAimTargetID          ( void ) const { return m_AutoAimTargetID ;     }
        f32         GetAutoAimScore             ( void ) const { return m_AutoAimScore ;        }
  const vector3&    GetAutoAimTargetCenter      ( void ) const { return m_AutoAimTargetCenter ; }
  const vector3&    GetAutoAimDir               ( void ) const { return m_AutoAimDir ;          }
  const vector3&    GetAutoAimPos               ( void ) const { return m_AutoAimPos ;          }
  const vector3&    GetAutoAimArcPivot          ( void ) const { return m_AutoAimArcPivot ;     }

        // Pickup functions                     
        void        AddHealth                   ( f32 Amount ) ;
        
        // Energy functions
        void        AddEnergy                   ( f32 Amount ) ;
        void        SetEnergy                   ( f32 Amount ) ;

        // Heat functions
        f32         GetHeat                     ( void ) ;
        void        SetHeat                     ( f32 Heat ) ;
        
        // Inventory functions
        xbool       AddWeapon                   ( invent_type   WeaponType) ;
        f32         GetWeaponAmmoCount          ( invent_type   WeaponType ) const;

        xbool       HasPack                     ( void );
        xbool       HasGrenade                  ( void );
        xbool       AddInvent                   ( invent_type   InventType, s32 Amount ) ;
        void        SetInventCount              ( invent_type   InventType, s32 Count ) ;
        s32         GetInventCount              ( invent_type   InventType ) const;
        s32         GetInventMaxCount           ( invent_type   InventType ) const;
        invent_type GetCurrentPack              ( void ) const  { return m_PackCurrentType; }
        
        xbool       GetDeployAttemptData        ( vector3&      Point, 
                                                  vector3&      Normal,
                                                  object::type& ObjType );
//      void        AttemptDeployTurretBarrel   ( void );

        // Fx functions
        void        FlashScreen                 ( f32 TotalTime, const xcolor &Color );
        
        // Sound functions

        // Call from server (will send to client) or client to player on client/server controlled player
        void        PlaySound                   ( s32 SoundType ) ;
        
        // Will activate anim (if there is one for the sound)
        void        PlayVoiceMenuSound          ( s32 SfxID, u32 TeamBits, s32 HudVoiceID ) ;

        // Collision functions
        pickup*     CreatePickup                ( const vector3&        Pos,
                                                  const vector3&        Vel,
                                                        pickup::kind    Kind,
                                                        pickup::flags   Flags,
                                                        f32             Time ) ;

        xbool       HitPickup                   ( pickup*       pPickup ) ;

        // Weapon/ammo access functions
  const loadout&        GetLoadout              ( void ) const ;
  const loadout&        GetInventoryLoadout     ( void ) const ;
        void            SetInventoryLoadout     ( const loadout& Loadout );
        void            InventoryReload         ( xbool AtDeployedStation = FALSE );
        void            InventoryReset          ( void );
        s32             GetWeaponCurrentSlot    ( void ) const;
  const invent_type     GetWeaponCurrentType    ( void ) const;
  const shape_instance& GetWeaponShapeInst      ( void ) { return m_WeaponInstance; }
        pain::type      GetWeaponPainType       ( invent_type WeaponType ) const;
        s32             GetNumWeaponsHeld       ( void ) const;
        xbool           IsItemHeld              ( invent_type ItemType ) const;

        // UI / HUD Functions
        s32             GetWarriorID            ( void ) const { return m_WarriorID; }
        xbool           GetManiacMode           ( void ) const;
        xbool           IsLocal                 ( void ) const { return m_WarriorID != -1; }

        // Game manager player index
        void            SetPlayerIndex          ( s32 PlayerIndex ) { m_PlayerIndex = PlayerIndex; }
        s32             GetPlayerIndex          ( void ) { return m_PlayerIndex; }

		// Team change functions

		// Store request of team change - it will make it's way to the server...
		void			RequestChangeTeam		( void ) ;
		
		// Changes player team (only call from server!)
		void			ChangeTeam				( u32 TeamBits ) ;

        // Vote functions

        // Functions that can be called from the server/client ui player input dialog
        // (if on a client - it will be sent to the server)
        // Votes are processed on the server only - see player_object::ApplyMove
        void            VoteChangeMap           ( s32 MapIndex ) ;
        void            VoteKickPlayer          ( s32 PlayerIndex ) ;
        void            VoteYes                 ( void ) ;
        void            VoteNo                  ( void ) ;

        // Query functions that can be called by the server/client ui dialogs to see 
        // if start/cast voting can be enabled. NOTE: They are only valid for server/client
        // controlled players and not ghost players.
        xbool           GetVoteCanStart         ( void ) ;
        xbool           GetVoteCanCast          ( void ) ;

        // Vote functions that can be called by the server game manager
        // (the info will be sent to the client controlled players)
        void            SetVoteCanStart         ( xbool bCanStart ) ;
        void            SetVoteCanCast          ( xbool bCanCast  ) ;

        //==========================================================================
        // Functions in PlayerStates.cpp
        //==========================================================================

        // Useful state functions
        void         Player_AdvanceState                (f32 DeltaTime) ;
        void         Player_SetupState                  (player_state State, xbool SkipIfAlreadyInState = TRUE) ;
        player_state Player_GetState                    ( void ) const  { return m_PlayerState ; }
        const char*  Player_GetStateString              ( player_state State ) ;
        
        void         Player_PlaySoftLandSfx             ( void ) ;
        void         Player_PlayHardLandSfx             ( void ) ;
        void         Player_ChooseMovingState           ( f32 DeltaTime ) ;
        void         Player_ChooseSurfaceState          ( f32 DeltaTime ) ;

        // State functions
        void        Player_Setup_CONNECTING             ( void ) ;
        void        Player_Advance_CONNECTING           ( f32 DeltaTime ) ;
        
        void        Player_Setup_WAIT_FOR_GAME          ( void ) ;
        void        Player_Advance_WAIT_FOR_GAME        ( f32 DeltaTime ) ;
        void        Player_Exit_WAIT_FOR_GAME           ( void ) ;
       
        void        Player_Setup_TAUNT                  ( void ) ;
        void        Player_Advance_TAUNT                ( f32 DeltaTime ) ;

        void        Player_Setup_AT_INVENT              ( void ) ;
        void        Player_Advance_AT_INVENT            ( f32 DeltaTime ) ;

        void        Player_Setup_IDLE                   ( void ) ;
        void        Player_Advance_IDLE                 ( f32 DeltaTime ) ;
                                                
        void        Player_Setup_RUN                    ( void ) ;
        void        Player_Advance_RUN                  ( f32 DeltaTime ) ;
                                                
        void        Player_Setup_JUMP                   ( void ) ;
        void        Player_Advance_JUMP                 ( f32 DeltaTime ) ;
                                                
        f32         GetLandDamage                       ( void ) ;
        void        Player_Setup_LAND                   ( void ) ;
        void        Player_Advance_LAND                 ( f32 DeltaTime ) ;
                                                
        void        Player_Setup_JET_THRUST             ( void ) ;
        void        Player_Advance_JET_THRUST           ( f32 DeltaTime ) ;

        void        Player_Setup_JET_FALL               ( void ) ;
        void        Player_Advance_JET_FALL             ( f32 DeltaTime ) ;

        void        Player_Setup_FALL                   ( void ) ;
        void        Player_Advance_FALL                 ( f32 DeltaTime ) ;

        void        Player_Setup_VEHICLE                ( void ) ;
        void        Player_Exit_VEHICLE                 ( void ) ;
        void        Player_Advance_VEHICLE              ( f32 DeltaTime ) ;
                                                
virtual void        Player_Setup_DEAD                   ( void ) ;
        void        Player_Advance_DEAD                 ( f32 DeltaTime ) ;

        // Stimuli functions
        void        OnPain                              ( const pain& Pain ) ;
        void        ApplyConcussion                     ( f32 Distance ) ;

        //==========================================================================
        // Functions in WeaponStates.cpp
        //==========================================================================

        void        Weapon_CheckForUpdatingShape        ( void ) ;
        void        Weapon_AdvanceState                 ( f32 DeltaTime ) ;
        void        Weapon_SetupState                   ( weapon_state WeaponState, xbool SkipIfAlreadyInState = TRUE ) ;

        void        Weapon_CallAdvanceState             ( f32 DeltaTime ) ;
        void        Weapon_CallExitState                ( void ) ;
        void        Weapon_CallSetupState               ( weapon_state WeaponState ) ;
        void        Weapon_RenderFx                     ( void ) ;
        
        void        Weapon_Setup_ACTIVATE               ( void ) ;
        void        Weapon_Advance_ACTIVATE             ( f32 DeltaTime ) ;
                                                    
        void        Weapon_Setup_IDLE                   ( void ) ;
        void        Weapon_Advance_IDLE                 ( f32 DeltaTime ) ;
                                                    
        void        Weapon_Setup_FIRE                   ( void ) ;
        void        Weapon_Advance_FIRE                 ( f32 DeltaTime ) ;
                                                    
        void        Weapon_Setup_RELOAD                 ( void ) ;
        void        Weapon_Advance_RELOAD               ( f32 DeltaTime ) ;
                                                    
        void        Weapon_Setup_DEACTIVATE             ( void ) ;
        void        Weapon_Advance_DEACTIVATE           ( f32 DeltaTime ) ;


        //==========================================================================
        // Functions in DiskLauncher.cpp
        //==========================================================================
    
        void        DiskLauncher_AdvanceState          ( f32 DeltaTime ) ;
        void        DiskLauncher_ExitState             ( void ) ;
        void        DiskLauncher_SetupState            ( weapon_state WeaponState ) ;
                
        void        DiskLauncher_Setup_ACTIVATE        ( void ) ;
        void        DiskLauncher_Advance_ACTIVATE      ( f32 DeltaTime ) ;
                                                   
        void        DiskLauncher_Setup_IDLE            ( void ) ;
        void        DiskLauncher_Exit_IDLE             ( void ) ;
        void        DiskLauncher_Advance_IDLE          ( f32 DeltaTime ) ;
                                                   
        void        DiskLauncher_Setup_FIRE            ( void ) ;
        void        DiskLauncher_Advance_FIRE          ( f32 DeltaTime ) ;
                                                   
        void        DiskLauncher_Setup_RELOAD          ( void ) ;
        void        DiskLauncher_Advance_RELOAD        ( f32 DeltaTime ) ;
                                                   
        void        DiskLauncher_Setup_DEACTIVATE      ( void ) ;
        void        DiskLauncher_Advance_DEACTIVATE    ( f32 DeltaTime ) ;

        //==========================================================================
        // Functions in PlasmaGun.cpp
        //==========================================================================
    
        void        PlasmaGun_AdvanceState             ( f32 DeltaTime ) ;
        void        PlasmaGun_ExitState                ( void ) ;
        void        PlasmaGun_SetupState               ( weapon_state WeaponState ) ;
                                                       
        void        PlasmaGun_Setup_ACTIVATE           ( void ) ;
        void        PlasmaGun_Advance_ACTIVATE         ( f32 DeltaTime ) ;
                                                       
        void        PlasmaGun_Setup_IDLE               ( void ) ;
        void        PlasmaGun_Exit_IDLE                ( void ) ;
        void        PlasmaGun_Advance_IDLE             ( f32 DeltaTime ) ;
                                                       
        void        PlasmaGun_Setup_FIRE               ( void ) ;
        void        PlasmaGun_Advance_FIRE             ( f32 DeltaTime ) ;
                                                       
        void        PlasmaGun_Setup_RELOAD             ( void ) ;
        void        PlasmaGun_Advance_RELOAD           ( f32 DeltaTime ) ;
                                                       
        void        PlasmaGun_Setup_DEACTIVATE         ( void ) ;
        void        PlasmaGun_Advance_DEACTIVATE       ( f32 DeltaTime ) ;

        //==========================================================================
        // Functions in ChainGun.cpp
        //==========================================================================
    
        void        ChainGun_AdvanceState             ( f32 DeltaTime ) ;
        void        ChainGun_ExitState                ( void ) ;
        void        ChainGun_SetupState               ( weapon_state WeaponState ) ;
                                                       
        void        ChainGun_Setup_ACTIVATE           ( void ) ;
        void        ChainGun_Advance_ACTIVATE         ( f32 DeltaTime ) ;
                                                       
        void        ChainGun_Setup_IDLE               ( void ) ;
        void        ChainGun_Exit_IDLE                ( void ) ;
        void        ChainGun_Exit_FIRE                ( void ) ;
        void        ChainGun_Advance_IDLE             ( f32 DeltaTime ) ;
                                                       
        void        ChainGun_Setup_FIRE               ( void ) ;
        void        ChainGun_Advance_FIRE             ( f32 DeltaTime ) ;
                                                       
        void        ChainGun_Setup_RELOAD             ( void ) ;
        void        ChainGun_Advance_RELOAD           ( f32 DeltaTime ) ;
                                                       
        void        ChainGun_Setup_DEACTIVATE         ( void ) ;
        void        ChainGun_Advance_DEACTIVATE       ( f32 DeltaTime ) ;

        void        ChainGun_RenderFx                 ( void ) ;

        //==========================================================================
        // Functions in LaserRifle.cpp
        //==========================================================================
    
        void        LaserRifle_AdvanceState           ( f32 DeltaTime ) ;
        void        LaserRifle_ExitState              ( void ) ;
        void        LaserRifle_SetupState             ( weapon_state WeaponState ) ;
                                                      
        void        LaserRifle_Setup_ACTIVATE         ( void ) ;
        void        LaserRifle_Advance_ACTIVATE       ( f32 DeltaTime ) ;
                                                      
        void        LaserRifle_Setup_IDLE             ( void ) ;
        void        LaserRifle_Exit_IDLE              ( void ) ;
        void        LaserRifle_Advance_IDLE           ( f32 DeltaTime ) ;
                                                      
        void        LaserRifle_Setup_FIRE             ( void ) ;
        void        LaserRifle_Advance_FIRE           ( f32 DeltaTime ) ;
                                                      
        void        LaserRifle_Setup_RELOAD           ( void ) ;
        void        LaserRifle_Advance_RELOAD         ( f32 DeltaTime ) ;
                                                      
        void        LaserRifle_Setup_DEACTIVATE       ( void ) ;
        void        LaserRifle_Advance_DEACTIVATE     ( f32 DeltaTime ) ;

        //==========================================================================
        // Functions in MortarGun.cpp
        //==========================================================================
    
        void        MortarGun_AdvanceState            ( f32 DeltaTime ) ;
        void        MortarGun_ExitState               ( void ) ;
        void        MortarGun_SetupState              ( weapon_state WeaponState ) ;
                                                       
        void        MortarGun_Setup_ACTIVATE          ( void ) ;
        void        MortarGun_Advance_ACTIVATE        ( f32 DeltaTime ) ;
                                                       
        void        MortarGun_Setup_IDLE              ( void ) ;
        void        MortarGun_Exit_IDLE               ( void ) ;
        void        MortarGun_Advance_IDLE            ( f32 DeltaTime ) ;
                                                       
        void        MortarGun_Setup_FIRE              ( void ) ;
        void        MortarGun_Advance_FIRE            ( f32 DeltaTime ) ;
                                                       
        void        MortarGun_Setup_RELOAD            ( void ) ;
        void        MortarGun_Advance_RELOAD          ( f32 DeltaTime ) ;
                                                       
        void        MortarGun_Setup_DEACTIVATE        ( void ) ;
        void        MortarGun_Advance_DEACTIVATE      ( f32 DeltaTime ) ;

        //==========================================================================
        // Functions in GrenadeLauncher.cpp
        //==========================================================================
    
        void        GrenadeLauncher_AdvanceState       ( f32 DeltaTime ) ;
        void        GrenadeLauncher_ExitState          ( void ) ;
        void        GrenadeLauncher_SetupState         ( weapon_state WeaponState ) ;
                                                       
        void        GrenadeLauncher_Setup_ACTIVATE     ( void ) ;
        void        GrenadeLauncher_Advance_ACTIVATE   ( f32 DeltaTime ) ;
                                                       
        void        GrenadeLauncher_Setup_IDLE         ( void ) ;
        void        GrenadeLauncher_Exit_IDLE          ( void ) ;
        void        GrenadeLauncher_Advance_IDLE       ( f32 DeltaTime ) ;
                                                       
        void        GrenadeLauncher_Setup_FIRE         ( void ) ;
        void        GrenadeLauncher_Advance_FIRE       ( f32 DeltaTime ) ;
                                                       
        void        GrenadeLauncher_Setup_RELOAD       ( void ) ;
        void        GrenadeLauncher_Advance_RELOAD     ( f32 DeltaTime ) ;
                                                       
        void        GrenadeLauncher_Setup_DEACTIVATE   ( void ) ;
        void        GrenadeLauncher_Advance_DEACTIVATE ( f32 DeltaTime ) ;

        //==========================================================================
        // Functions in Blaster.cpp
        //==========================================================================
    
        void        Blaster_AdvanceState               ( f32 DeltaTime ) ;
        void        Blaster_ExitState                  ( void ) ;
        void        Blaster_SetupState                 ( weapon_state WeaponState ) ;
                                                               
        void        Blaster_Setup_ACTIVATE             ( void ) ;
        void        Blaster_Advance_ACTIVATE           ( f32 DeltaTime ) ;
                                                               
        void        Blaster_Setup_IDLE                 ( void ) ;
        void        Blaster_Exit_IDLE                  ( void ) ;
        void        Blaster_Advance_IDLE               ( f32 DeltaTime ) ;
                                                               
        void        Blaster_Setup_FIRE                 ( void ) ;
        void        Blaster_Advance_FIRE               ( f32 DeltaTime ) ;
                                                               
        void        Blaster_Setup_RELOAD               ( void ) ;
        void        Blaster_Advance_RELOAD             ( f32 DeltaTime ) ;
                                                               
        void        Blaster_Setup_DEACTIVATE           ( void ) ;
        void        Blaster_Advance_DEACTIVATE         ( f32 DeltaTime ) ;

        void        Blaster_RenderFx                   ( void ) ;

        //==========================================================================
        // Functions in Elf.cpp
        //==========================================================================
    
        void        Elf_AdvanceState                   ( f32 DeltaTime ) ;
        void        Elf_ExitState                      ( void ) ;
        void        Elf_SetupState                     ( weapon_state WeaponState ) ;
                                                                   
        void        Elf_Setup_ACTIVATE                 ( void ) ;
        void        Elf_Advance_ACTIVATE               ( f32 DeltaTime ) ;
                                                                   
        void        Elf_Setup_IDLE                     ( void ) ;
        void        Elf_Exit_IDLE                      ( void ) ;
        void        Elf_Advance_IDLE                   ( f32 DeltaTime ) ;
                                                                   
        void        Elf_Setup_FIRE                     ( void ) ;
        void        Elf_Advance_FIRE                   ( f32 DeltaTime ) ;
        void        Elf_Exit_FIRE                      ( void ) ;
                                                                   
        void        Elf_Setup_RELOAD                   ( void ) ;
        void        Elf_Advance_RELOAD                 ( f32 DeltaTime ) ;
                                                                   
        void        Elf_Setup_DEACTIVATE               ( void ) ;
        void        Elf_Advance_DEACTIVATE             ( f32 DeltaTime ) ;

        void        Elf_RenderFx                       ( void ) ;
        xbool       Elf_CalculateBeam                  ( vector3* pBeam, xbool FromRender ) ;

        //==========================================================================
        // Functions in RepairGun.cpp
        //==========================================================================
    
        void        RepairGun_AdvanceState             ( f32 DeltaTime ) ;
        void        RepairGun_ExitState                ( void ) ;
        void        RepairGun_SetupState               ( weapon_state WeaponState ) ;
                                                             
        void        RepairGun_Setup_ACTIVATE           ( void ) ;
        void        RepairGun_Advance_ACTIVATE         ( f32 DeltaTime ) ;
                                                             
        void        RepairGun_Setup_IDLE               ( void ) ;
        void        RepairGun_Exit_IDLE                ( void ) ;
        void        RepairGun_Advance_IDLE             ( f32 DeltaTime ) ;
                                                             
        void        RepairGun_Setup_FIRE               ( void ) ;
        void        RepairGun_Advance_FIRE             ( f32 DeltaTime ) ;
        void        RepairGun_Exit_FIRE                ( void ) ;
                                                             
        void        RepairGun_Setup_RELOAD             ( void ) ;
        void        RepairGun_Advance_RELOAD           ( f32 DeltaTime ) ;
                                                             
        void        RepairGun_Setup_DEACTIVATE         ( void ) ;
        void        RepairGun_Advance_DEACTIVATE       ( f32 DeltaTime ) ;

        void        RepairGun_RenderFx                 ( void ) ;

        //==========================================================================
        // Functions in TargetingLaser.cpp
        //==========================================================================
    
        void        TargetingLaser_AdvanceState        ( f32 DeltaTime ) ;
        void        TargetingLaser_ExitState           ( void ) ;
        void        TargetingLaser_SetupState          ( weapon_state WeaponState ) ;
                                                        
        void        TargetingLaser_Setup_ACTIVATE      ( void ) ;
        void        TargetingLaser_Advance_ACTIVATE    ( f32 DeltaTime ) ;
                                                        
        void        TargetingLaser_Setup_IDLE          ( void ) ;
        void        TargetingLaser_Exit_IDLE           ( void ) ;
        void        TargetingLaser_Advance_IDLE        ( f32 DeltaTime ) ;
                                                        
        void        TargetingLaser_Setup_FIRE          ( void ) ;
        void        TargetingLaser_Advance_FIRE        ( f32 DeltaTime ) ;
        void        TargetingLaser_Exit_FIRE           ( void ) ;
                                                        
        void        TargetingLaser_Setup_RELOAD        ( void ) ;
        void        TargetingLaser_Advance_RELOAD      ( f32 DeltaTime ) ;
                                                        
        void        TargetingLaser_Setup_DEACTIVATE    ( void ) ;
        void        TargetingLaser_Advance_DEACTIVATE  ( f32 DeltaTime ) ;

        void        TargetingLaser_RenderFx            ( void ) ;

        //==========================================================================
        // Functions in MissileLaunder.cpp
        //==========================================================================
    
        void        MissileLauncher_AdvanceState       ( f32 DeltaTime ) ;
        void        MissileLauncher_ExitState          ( void ) ;
        void        MissileLauncher_SetupState         ( weapon_state WeaponState ) ;
                                                       
        void        MissileLauncher_Setup_ACTIVATE     ( void ) ;
        void        MissileLauncher_Advance_ACTIVATE   ( f32 DeltaTime ) ;
                                                       
        void        MissileLauncher_Setup_IDLE         ( void ) ;
        void        MissileLauncher_Exit_IDLE          ( void ) ;
        void        MissileLauncher_Advance_IDLE       ( f32 DeltaTime ) ;
                                                       
        void        MissileLauncher_Setup_FIRE         ( void ) ;
        void        MissileLauncher_Advance_FIRE       ( f32 DeltaTime ) ;
        void        MissileLauncher_Exit_FIRE          ( void ) ;
                                                       
        void        MissileLauncher_Setup_RELOAD       ( void ) ;
        void        MissileLauncher_Advance_RELOAD     ( f32 DeltaTime ) ;
                                                       
        void        MissileLauncher_Setup_DEACTIVATE   ( void ) ;
        void        MissileLauncher_Advance_DEACTIVATE ( f32 DeltaTime ) ;

        void        MissileLauncher_RenderFx           ( void ) ;

protected:
};

//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

object*     PlayerObjectCreator( void );


//==============================================================================
#endif // PLAYEROBJECT_HPP
//==============================================================================

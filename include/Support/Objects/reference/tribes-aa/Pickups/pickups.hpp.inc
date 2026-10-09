//==============================================================================
//
//  Pickups.hpp
//
//==============================================================================

#ifndef PICKUPS_HPP
#define PICKUPS_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <xCore/x_files/x_color.hpp>
#include <Support/Shape/ShapeInstance.hpp>

//==============================================================================
//  ENUMS/DEFINES
//==============================================================================
#define PICKUP_WAIT         1.0f    // how long after spawn before allow retrieval

//==============================================================================
//  TYPES
//==============================================================================

class pickup : public object
{
//======================================================================
// DEFINES
//======================================================================
public:
    enum kind
    {
        KIND_NONE,

        // Weapons
        KIND_WEAPON_DISK_LAUNCHER,
        KIND_WEAPON_PLASMA_GUN,
        KIND_WEAPON_SNIPER_RIFLE,
        KIND_WEAPON_MORTAR_GUN,
        KIND_WEAPON_GRENADE_LAUNCHER,
        KIND_WEAPON_CHAIN_GUN,
        KIND_WEAPON_BLASTER,
        KIND_WEAPON_ELF_GUN,
        KIND_WEAPON_MISSILE_LAUNCHER,

        // Ammunition
        KIND_AMMO_DISC,
        KIND_AMMO_CHAIN_GUN,
        KIND_AMMO_PLASMA,
        KIND_AMMO_GRENADE_LAUNCHER,
        KIND_AMMO_GRENADE_BASIC,
        KIND_AMMO_MINE,
        KIND_AMMO_BOX,

        // Reapir stuff
        KIND_HEALTH_PATCH,
        KIND_HEALTH_KIT,

        // Armor packs
        KIND_ARMOR_PACK_AMMO,
        KIND_ARMOR_PACK_ENERGY,
        KIND_ARMOR_PACK_REPAIR,
        KIND_ARMOR_PACK_SENSOR_JAMMER,
        KIND_ARMOR_PACK_SHIELD,

        // Deployable packs
        KIND_DEPLOY_PACK_BARREL_AA,
        KIND_DEPLOY_PACK_BARREL_ELF,
        KIND_DEPLOY_PACK_BARREL_MORTAR,
        KIND_DEPLOY_PACK_BARREL_MISSILE,
        KIND_DEPLOY_PACK_BARREL_PLASMA,
        KIND_DEPLOY_PACK_INVENT_STATION,
        KIND_DEPLOY_PACK_PULSE_SENSOR,
        KIND_DEPLOY_PACK_TURRET_INDOOR,
        KIND_DEPLOY_PACK_TURRET_OUTDOOR,
        KIND_DEPLOY_PACK_SATCHEL_CHARGE,

        KIND_TOTAL
    };

    enum flags
    {
        #define bit(x)  (1 << (x))

        FLAG_ANIMATED         = bit( 0 ),
        FLAG_MOVABLE          = bit( 1 ),
        FLAG_IMMORTAL         = bit( 2 ),
        FLAG_GLOWS            = bit( 3 ),
        FLAG_RESPAWNS         = bit( 4 )
    };

    enum state
    {
        STATE_MOVING,
        STATE_IDLE,
        STATE_DYING,
        STATE_DEAD,
        STATE_LAZARUS
    };

    enum misc
    {
        DATA_ITEMS = 12,
    } ;

//======================================================================
// STRUCTURES
//======================================================================
public:

	// Information
	struct info
	{
		s32		    PickupKind ;	// Kind of pickup (for checking_
		s32		    ShapeType ;		// Shape type for drawing
		s32		    InventType ;	// Player inventory type
		s32		    InventCount ;	// Default amount to give player
	} ;

//======================================================================
// FUNCTIONS
//======================================================================
public:
    
                pickup          ( void );

    update_code AdvanceLogic    ( f32 DeltaTime );
    update_code OnAdvanceLogic  ( f32 DeltaTime );
    void        Initialize      ( const vector3& Position,                                   
                                  const vector3& Movement,
                                  pickup::kind   Kind,
                                  pickup::flags  Flags, 
                                  f32            Time );

    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );

    void        SetState        ( state State );

    void        Render          ( void );

    kind        GetKind         ( void ) const { return m_Kind; }
    state       GetState        ( void )     { return m_State; }

    xbool       IsPickuppable   ( void )     { if ( ( m_Age > PICKUP_WAIT ) && \
                                                   (( m_State != STATE_DEAD ) && \
                                                   ( m_State != STATE_LAZARUS )) ) \
                                                   return TRUE; else return FALSE; }

    u8          GetDataItem     ( s32 Index ) const ;
    void        SetDataItem     ( s32 Index, u8 Value ) ;
    
    void        SetFlags        ( flags Flags ) { m_Flags = Flags; }
    
    // Returns info about pickup
    const pickup::info& GetInfo ( void ) const ;

    // Returns info about requested pickup kind
    static const pickup::info& GetInfo ( pickup::kind Kind ) ;


//======================================================================
// DATA
//======================================================================
protected:
    vector3         m_Movement;
    shape_instance  m_Instance;
    f32             m_Age;
    f32             m_Life;                 // if respawns, this also represents delay before respawning
    f32             m_Update;               
    f32             m_Pending;              // for delaying movement updates if insufficient travel
                                            
    f32             m_Timer;                // for controlling the oscillation
    f32             m_YPos;                 // AGL elevation, for oscillation purposes only
    xbool           m_IsBouncing;           // for transitioning from at rest on ground to bouncing
                                            
    kind            m_Kind;                 
    flags           m_Flags;                
    state           m_State;                
    f32             m_Alpha;                // for fading out the object   
    
    u8              m_Data[DATA_ITEMS] ;    // Misc data (eg. for ammo pack this is the ammo totals)
};

//==============================================================================
#endif // PICKUPS_HPP
//==============================================================================

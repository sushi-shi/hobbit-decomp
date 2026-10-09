//==============================================================================
//
//  Pickups.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Pickups.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "../../AADS/Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "netlib/bitstream.hpp"
#include "Objects/Player/PlayerObject.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define P_GRAVITY         25.00f
#define FRICTION        0.25f
#define PU_RADIUS       1.0f
#define PU_RADIUS2      0.3f
#define MIN_MOVEMENT    4.0f
#define TOO_VERTICAL    0.05f
#define HEIGHT_ADJ      0.990f
#define LIFESPAN        40.0f
#define DYING_TIME      5.0f

#define BOUNCE_MIN      0.35f       // for during IDLE state, lazy li'l bounce
#define BOUNCE_MAX      0.50f

#define DIRTY_POS       ( 1 << 0 )
#define DIRTY_STATE     ( 1 << 1 )

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   PickupCreator;
    
obj_type_info   PickupTypeInfo( object::TYPE_PICKUP, 
                                "Pickup", 
                                PickupCreator, 
                                object::ATTR_PERMEABLE | object::ATTR_PICKUP );



                             
// Information table - add to this when ever you add new pickup kind
static pickup::info s_InfoTable[pickup::KIND_TOTAL] =
{
	// KIND_NONE
	{
		pickup::KIND_NONE,							            // Pickup kind (for checking)
		SHAPE_TYPE_NONE,							            // Shape type for drawing
		player_object::INVENT_TYPE_NONE,			            // Player inventory type
		0,											            // Default amount to give player
	},                                                          
                                                                
    //======================================================================
	// Weapons                                                  
    //======================================================================

	// KIND_WEAPON_DISK_LAUNCHER                                
	{                                                           
		pickup::KIND_WEAPON_DISK_LAUNCHER,					    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_DISC_LAUNCHER,			            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER,        // Player inventory type
		1,											            // Default amount to give player
	},

	// KIND_WEAPON_PLASMA_GUN
	{
		pickup::KIND_WEAPON_PLASMA_GUN,						    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_PLASMA_GUN,				            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_PLASMA_GUN,		    // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_WEAPON_SNIPER_RIFLE                                 
	{                                                           
		pickup::KIND_WEAPON_SNIPER_RIFLE,					    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_SNIPER_RIFLE,				            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_SNIPER_RIFLE,		    // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_WEAPON_MORTAR_GUN                                   
	{                                                           
		pickup::KIND_WEAPON_MORTAR_GUN,						    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_MORTAR_LAUNCHER,			            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_MORTAR_GUN,		    // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_WEAPON_GRENADE_LAUNCHER                             
	{                                                           
		pickup::KIND_WEAPON_GRENADE_LAUNCHER,				    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_GRENADE_LAUNCHER,			            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER,     // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_WEAPON_CHAIN_GUN                                    
	{                                                           
		pickup::KIND_WEAPON_CHAIN_GUN,						    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_CHAIN_GUN,				            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_CHAIN_GUN,            // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_WEAPON_BLASTER                                      
	{                                                           
		pickup::KIND_WEAPON_BLASTER,						    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_BLASTER,					            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_BLASTER,	            // Player inventory type
		1,											            // Default amount to give player
	},                                                          

	// KIND_WEAPON_ELF_GUN
	{
		pickup::KIND_WEAPON_ELF_GUN,						    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_ELF,						            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_ELF_GUN,			    // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_WEAPON_MISSILE_LAUNCHER                             
	{                                                           
		pickup::KIND_WEAPON_MISSILE_LAUNCHER,				    // Pickup kind (for checking)
		SHAPE_TYPE_WEAPON_MISSILE_LAUNCHER,			            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER,     // Player inventory type
		1,											            // Default amount to give player
	},                                                          
                                                                
                                                                
    //======================================================================
	// Ammunition                                               
    //======================================================================

	// KIND_AMMO_DISC                                           
	{                                                           
		pickup::KIND_AMMO_DISC,								    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_AMMO_DISC,				            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_AMMO_DISK,		    // Player inventory type
		20,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_AMMO_CHAIN_GUN                                      
	{                                                           
		pickup::KIND_AMMO_CHAIN_GUN,						    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_AMMO_CHAIN_GUN,			            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_AMMO_BULLET,		    // Player inventory type
		100,										            // Default amount to give player
	},                                                          
                                                                
	// KIND_AMMO_PLASMA                                         
	{                                                           
		pickup::KIND_AMMO_PLASMA,							    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_AMMO_PLASMA,				            // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_AMMO_PLASMA,		    // Player inventory type
		15,											            // Default amount to give player
	},                                                          
                                                                
	// KIND_AMMO_GRENADE_LAUNCHER                               
	{                                                           
		pickup::KIND_AMMO_GRENADE_LAUNCHER,					    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_AMMO_GRENADE_LAUNCHER,                // Shape type for drawing
		player_object::INVENT_TYPE_WEAPON_AMMO_GRENADE,		    // Player inventory type
		15,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_AMMO_GRENADE_BASIC                              
	{                                                       
		pickup::KIND_AMMO_GRENADE_BASIC,					    // Pickup kind (for checking)
		SHAPE_TYPE_PROJ_HANDGRENADE,			                // Shape type for drawing
		player_object::INVENT_TYPE_GRENADE_BASIC,			    // Player inventory type
		3,										                // Default amount to give player
	},                                                          
                                                                
	// KIND_AMMO_MINE                                           
	{                                                           
		pickup::KIND_AMMO_MINE,							        // Pickup kind (for checking)
		SHAPE_TYPE_PROJ_MINE,					                // Shape type for drawing
		player_object::INVENT_TYPE_MINE,			            // Player inventory type
		3,										                // Default amount to give player
	},                                                          
                                                                
	// KIND_AMMO_BOX
	{                                                           
		pickup::KIND_AMMO_BOX,								    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_AMMO_BOX,				                // Shape type for drawing
		player_object::INVENT_TYPE_NONE,            		    // Player inventory type
		0,											            // Default amount to give player
	},                                                          
                                                                
    //======================================================================
	// Repair stuff                                             
    //======================================================================

    // KIND_HEALTH_PATCH                                        
	{                                                           
		pickup::KIND_HEALTH_PATCH,							    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_HEALTH_PATCH,			                // Shape type for drawing
		player_object::INVENT_TYPE_NONE,		                // Player inventory type
		20,										                // Default amount to give player
	},                                                          
                                                                
	// KIND_HEALTH_KIT                                          
	{                                                           
		pickup::KIND_HEALTH_KIT,							    // Pickup kind (for checking)
		SHAPE_TYPE_PICKUP_HEALTH_KIT,			                // Shape type for drawing
		player_object::INVENT_TYPE_BELT_GEAR_HEALTH_KIT,        // Player inventory type
		50,										                // Default amount to give player
	},                                                          
                                                                
                                                                
    //======================================================================
	// Armor packs                                              
    //======================================================================

    // KIND_ARMOR_PACK_AMMO                                     
	{                                                           
		pickup::KIND_ARMOR_PACK_AMMO,						    // Pickup kind (for checking)
		SHAPE_TYPE_ARMOR_PACK_AMMO,				                // Shape type for drawing
		player_object::INVENT_TYPE_ARMOR_PACK_AMMO,             // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_ARMOR_PACK_ENERGY                               
	{                                                       
		pickup::KIND_ARMOR_PACK_ENERGY,						    // Pickup kind (for checking)
		SHAPE_TYPE_ARMOR_PACK_ENERGY,			                // Shape type for drawing
		player_object::INVENT_TYPE_ARMOR_PACK_ENERGY,           // Player inventory type
		1,										                // Default amount to give player
	},                                                          
                                                                
	// KIND_ARMOR_PACK_REPAIR                                   
	{                                                           
		pickup::KIND_ARMOR_PACK_REPAIR,						    // Pickup kind (for checking)
		SHAPE_TYPE_ARMOR_PACK_REPAIR,			                // Shape type for drawing
		player_object::INVENT_TYPE_ARMOR_PACK_REPAIR,           // Player inventory type
		1,									                    // Default amount to give player
	},                                                          
                                                                
	// KIND_ARMOR_PACK_SENSOR_JAMMER                            
	{                                                           
		pickup::KIND_ARMOR_PACK_SENSOR_JAMMER,				    // Pickup kind (for checking)
		SHAPE_TYPE_ARMOR_PACK_SENSOR_JAMMER,	                // Shape type for drawing
		player_object::INVENT_TYPE_ARMOR_PACK_SENSOR_JAMMER,    // Player inventory type
		1,										                // Default amount to give player
	},                                                          
                                                                
	// KIND_ARMOR_PACK_SHIELD                                   
	{                                                           
		pickup::KIND_ARMOR_PACK_SHIELD,						    // Pickup kind (for checking)
		SHAPE_TYPE_ARMOR_PACK_SHIELD,			                // Shape type for drawing
		player_object::INVENT_TYPE_ARMOR_PACK_SHIELD,           // Player inventory type
		1,								                        // Default amount to give player
	},                                                          
                                                                
                                                                
    //======================================================================
	// Deployable packs                                         
    //======================================================================

    // KIND_DEPLOY_PACK_BARREL_AA                               
	{                                                           
		pickup::KIND_DEPLOY_PACK_BARREL_AA,					    // Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_BARREL_AA,		                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_AA,       // Player inventory type
		1,										                // Default amount to give player
	},                                                          
                                                                
	// KIND_DEPLOY_PACK_BARREL_ELF                              
	{                                                           
		pickup::KIND_DEPLOY_PACK_BARREL_ELF,				    // Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_BARREL_ELF,		                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_ELF,      // Player inventory type
		1,										                // Default amount to give player
	},                                                          
                                                            
	// KIND_DEPLOY_PACK_BARREL_MORTAR                       
	{                                                       
		pickup::KIND_DEPLOY_PACK_BARREL_MORTAR,					// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_BARREL_MORTAR,                   // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_MORTAR,	// Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_BARREL_MISSILE                      
	{                                                       
		pickup::KIND_DEPLOY_PACK_BARREL_MISSILE,				// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_BARREL_MISSILE,	                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_MISSILE,  // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_BARREL_PLASMA                       
	{                                                       
		pickup::KIND_DEPLOY_PACK_BARREL_PLASMA,					// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_BARREL_PLASMA,                   // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_PLASMA,   // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_INVENT_STATION                      
	{                                                       
		pickup::KIND_DEPLOY_PACK_INVENT_STATION,				// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_INVENT_STATION,	                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION,  // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_PULSE_SENSOR                        
	{                                                       
		pickup::KIND_DEPLOY_PACK_PULSE_SENSOR,					// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_PULSE_SENSOR,	                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR,    // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_TURRET_INDOOR                       
	{                                                       
		pickup::KIND_DEPLOY_PACK_TURRET_INDOOR,					// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_TURRET_INDOOR,	                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR,	// Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_TURRET_OUTDOOR                      
	{                                                       
		pickup::KIND_DEPLOY_PACK_TURRET_OUTDOOR,				// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_TURRET_OUTDOOR,	                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_OUTDOOR,  // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
	// KIND_DEPLOY_PACK_SATCHEL_CHARGE                      
	{                                                       
		pickup::KIND_DEPLOY_PACK_SATCHEL_CHARGE,				// Pickup kind (for checking)
		SHAPE_TYPE_DEPLOY_PACK_SATCHEL_CHARGE,	                // Shape type for drawing
		player_object::INVENT_TYPE_DEPLOY_PACK_SATCHEL_CHARGE,  // Player inventory type
		1,										                // Default amount to give player
	},                                                      
                                                            
} ;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* PickupCreator( void )
{
    return( (object*)(new pickup) );
}

//==============================================================================

pickup::pickup( void )
{
    // Put into valid state
    m_Movement.Zero();
    m_Age           = 0;
    m_Life          = 0;
    m_Update        = 0;
    m_Pending       = 0;
    m_Kind          = (kind)-1;
    m_Flags         = (flags)0;
    m_State         = STATE_MOVING;
    m_Alpha         = 1.0f;

    m_Timer         = R_90;
    m_YPos          = 0.0f; 
    m_IsBouncing    = FALSE;
}

//==============================================================================

object::update_code pickup::OnAdvanceLogic( f32 DeltaTime )
{    
    // Update animation
    // SB - NOT NEEDED RIGHT NOW. MAY HAVE TO SPECIAL CASE PICKUPS
    m_Instance.Advance( DeltaTime ) ;

    // Increment the time since last update, but only if we're moving
    if( ( tgl.ServerPresent ) && (m_State == STATE_MOVING) )
    {
        m_Update += DeltaTime;

        // Time for an update?
        if( m_Update > 1.0f )
        {
            m_DirtyBits |= DIRTY_POS;
            m_Update     = x_frand( 0.0f, 0.2f );
        }
    }

    // Take care of that logic stuff.
    return( AdvanceLogic( DeltaTime ) );
}

//==============================================================================

object::update_code pickup::AdvanceLogic( f32 DeltaTime )
{       
    vector3 OldPos;
    vector3 NewPos;
    vector3 DeltaPos;
    xbool   Moved = FALSE;

    // Update age.  Artificially age faster when there are many pickups.
    s32 Count = GetTypeInfoPtr()->InstanceCount;
    if( Count < 60 )
        m_Age += DeltaTime;
    else
    {
        f32 Accel = 1.0f + ((Count - 60) * 0.1f);
        m_Age += DeltaTime * Accel;
    }

    if( m_State == STATE_MOVING )
        m_Movement.Y -= P_GRAVITY * DeltaTime;

    // Add in any unused time from previous logic runs.
    DeltaTime += m_Pending;
    m_Pending  = 0.0f;

    // Move the pickup through the world, colliding as we go!
    while( TRUE )
    {
        if( m_State == STATE_MOVING )
        {
            // make sure that we do transition to dying/dead states if appropriate
            if ( tgl.ServerPresent )
            {
                if ( (!(m_Flags & FLAG_IMMORTAL)) && 
                       (m_Age > ( m_Life - DYING_TIME )) )
                {
                    SetState( STATE_DYING );
                }
            }

            // update movement

            OldPos   = m_WorldPos;
            NewPos   = m_WorldPos + (m_Movement * DeltaTime);
            DeltaPos = NewPos - OldPos;

            // If not enough movement, just store the unused time for later.
            if( DeltaPos.LengthSquared() < 0.0001f )
            {
                m_Pending = DeltaTime;
                break;
            }

            //if ( !(m_Flags & FLAG_RESPAWNS) )
              //  x_printf("%f,%f,%f\n", m_Movement.X, m_Movement.Y, m_Movement.Z );

            Moved = TRUE;

            // Collision!
            collider Box;
            bbox     Bounds;

            Bounds.Set( m_WorldPos, PU_RADIUS2 );
            collider::collision Collision;
            Box.ExtSetup    ( this, Bounds, DeltaPos );
            ObjMgr.Collide  ( object::ATTR_SOLID_STATIC, Box );
            Box.GetCollision( Collision );

            // No collision?
            if( Collision.Collided == FALSE )
            {
                // All of DeltaTime is consumed.
                m_WorldPos = NewPos;
                break;
            }
            else
            {
                // See if the pickup should stop moving now
                if( Collision.Plane.Normal.Y > TOO_VERTICAL )
                {
                    // we're level enough, are we slow enough as well?
                    //x_DebugMsg( "Length: %f\n", m_Movement.LengthSquared() );
                    if( m_Movement.LengthSquared() < ( MIN_MOVEMENT * MIN_MOVEMENT ) )
                    {
                        if ( tgl.ServerPresent )
                        {
                            // we are!  ok, you're idle, buddy!
                            SetState(STATE_IDLE);
                            m_IsBouncing = FALSE;
                            m_Timer         = R_90;
                        }
                        break;
                    }
                }
            }

            // Bounce!
            m_WorldPos += DeltaPos * ( Collision.T * HEIGHT_ADJ );
            
            // adjust 
            m_Movement  = Collision.Plane.ReflectVector( m_Movement ) * FRICTION;
            DeltaTime  -= DeltaTime * Collision.T;
            break; 
        }
        else
        if( m_State == STATE_IDLE )
        {
            // switch to dying if we're not immortal and are too old
            m_Alpha = 1.0f;

            if ( (!(m_Flags & FLAG_IMMORTAL)) && 
                   (m_Age > ( m_Life - DYING_TIME )) )
            {
                if ( tgl.ServerPresent )
                {
                    SetState( STATE_DYING );
                }
            }
            break;
        }
        else
        if( m_State == STATE_DYING )
        {
            if ( m_Age > m_Life )
            {
                if ( tgl.ServerPresent )
                {
                    SetState( STATE_DEAD );
                }
            }
            else
                m_Alpha = (( m_Life - m_Age ) / DYING_TIME);
            break;
        }
        else
        if ( m_State == STATE_DEAD )
        {
            // completely transparent
            m_Alpha = 0.0f;

            // only destroy it if we're not going to respawn it
            if ( !( m_Flags & pickup::FLAG_RESPAWNS ))
            {
                if ( tgl.ServerPresent )
                {
                    return( DESTROY );
                }
            }
            else
            {
                // reset the age, and set it to the LAZARUS state
                m_Age = 0.0f;

                if ( tgl.ServerPresent )
                {
                    SetState( STATE_LAZARUS );
                }
            }
            // ...
            break;
        }
        else
        if ( m_State == STATE_LAZARUS )
        {
            // how long have I been dead?
            if ( tgl.ServerPresent )
            {
                if ( m_Age > m_Life )
                {
                    SetState( STATE_IDLE );
                    m_Alpha = 1.0f;
                }
            }

            m_Alpha = 0.0f;
            break;
        }
        else
        {
            ASSERT( FALSE );
        }
    }

    if( Moved )
    {
        // We are clear to move to the NewPos.
        m_WorldBBox.Set( m_WorldPos, PU_RADIUS );

        // Update object in the world.
        return( UPDATE );
    }
    return( NO_UPDATE );
}

//==============================================================================

void pickup::Initialize      ( const vector3& Position,                                
                               const vector3& Movement,
                               pickup::kind   Kind,
                               pickup::flags  Flags,
                               f32            Time )
{
    m_Movement   = Movement;
    m_Age        = 0.0f;
    m_Update     = 0.0f;
    m_Pending    = 0.0f;

    // Pickups spawned at mission load time should not move due to gravity.
    if( Flags & FLAG_RESPAWNS )
        SetState( STATE_IDLE );
    else
        SetState( STATE_MOVING );
    
    m_Life  = Time;
    m_Alpha = 1.0f;
    m_Kind  = Kind;
    m_Flags = Flags;

    m_WorldPos = Position;
    m_WorldBBox.Set( m_WorldPos, PU_RADIUS );

    // figure out what to draw for this pickup
    const pickup::info& Info = GetInfo() ;

    // Set the shape
    ASSERT(Info.ShapeType) ;
    m_Instance.SetShape( tgl.GameShapeManager.GetShapeByType( Info.ShapeType ) ) ;
    m_Instance.SetTextureAnimFPS( 10.0f );

    // ASSERT(m_Instance.GetShape()) ;
}

//==============================================================================

void pickup::Render( void )
{
    radian              Ang = 0;
    vector3             Pos;

    // f32 DeltaTime = (f32)(tgl.LogicTimeMs % 1000) / 1000.0f;
    f32 DeltaTime = tgl.DeltaLogicTime;

    if( m_Instance.GetShape() == NULL )
    {
        draw_BBox( m_WorldBBox, XCOLOR_YELLOW );
        return;
    }

    if( m_Kind == KIND_WEAPON_ELF_GUN )
        draw_Marker( m_WorldPos, XCOLOR_YELLOW );

    // move along, nothing to see here
    if ( m_Alpha == 0.0f )
        return;

    // don't want to render if we can't see it
    s32 InViewVolume = IsInViewVolume( m_WorldBBox );
    if( !InViewVolume )
        return;

    // make the color
    xcolor Col(255,255,255,(s32)(m_Alpha*255.0f));

    // happy little bobble if idling
    if ( ( m_State == STATE_IDLE ) ||
         ( m_State == STATE_DYING ) )
    {
        // if !m_IsBouncing, transition from at rest to BOUNCE_MAX
        if ( !m_IsBouncing )
        {
            if ( m_YPos < BOUNCE_MAX )
            {
                m_YPos += ( ( (BOUNCE_MAX + 0.2f) - m_YPos ) * DeltaTime );
            }
            else
            {
                m_YPos = BOUNCE_MAX;
                m_IsBouncing = TRUE;
            }

            Ang = R_90 * ( R_90 * 0.125f );
            Pos = m_WorldPos;
        }
        else
        {
            // calculate the current bounce
            m_YPos = BOUNCE_MIN + ( x_sin( m_Timer ) * (BOUNCE_MAX - BOUNCE_MIN) );

            // update angle and timer
            Ang = R_90 * (m_Timer * 0.125f);
            m_Timer += (DeltaTime * 4.0f);          
        }
    }
    else
    {
        Ang = R_90 * ( R_90 * 0.125f );
        Pos = m_WorldPos;
        m_YPos = 0.0f;
    }

    
    // calculate the spin and setup the L2W matrix
    Pos = vector3(m_WorldPos.X, m_WorldPos.Y + m_YPos, m_WorldPos.Z);

    // Setup draw flags
    u32 DrawFlags = shape::DRAW_FLAG_FOG | shape::DRAW_FLAG_TURN_OFF_LIGHTING ;
    
    // Clip?
    if (InViewVolume == view::VISIBLE_PARTIAL)
        DrawFlags |= shape::DRAW_FLAG_CLIP ;

    // Fade?
    if (Col.A != 255)
        DrawFlags |= shape::DRAW_FLAG_PRIME_Z_BUFFER | shape::DRAW_FLAG_SORT ;

    // draw the sphere in local space
    m_Instance.SetColor( Col );
    m_Instance.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
    m_Instance.SetPos( Pos );
    m_Instance.SetRot( radian3( 0, Ang, 0 ) );
    m_Instance.Draw( DrawFlags ) ;
}

//==============================================================================

void pickup::OnAcceptInit( const bitstream& BitStream )
{
    vector3 tPos, tMov;
    f32     Life;
    u32     Flags, Kind, State;

    BitStream.ReadVector    ( tPos );
    BitStream.ReadVector    ( tMov );
    BitStream.ReadF32       ( Life );
    BitStream.ReadU32       ( Flags, 4 ); 
    BitStream.ReadRangedU32 ( Kind, 0, KIND_TOTAL-1 );
    BitStream.ReadU32       ( State, 4 );

    Initialize( tPos, tMov, (kind)Kind, (flags)Flags, Life );

    // reset this, as Initialize overwrote it.
    m_State = (pickup::state)State;
}

//==============================================================================

void pickup::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteVector       ( m_WorldPos );
    BitStream.WriteVector       ( m_Movement );
    BitStream.WriteF32          ( m_Life );
    BitStream.WriteU32          ( (u32)m_Flags, 4 );
    BitStream.WriteRangedU32    ( (u32)m_Kind, 0, KIND_TOTAL-1 );
    BitStream.WriteU32          ( (u32)m_State, 4 );
}

//==============================================================================

object::update_code pickup::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    if ( BitStream.ReadFlag() )
    {
        BitStream.ReadVector( m_WorldPos );
        if( BitStream.ReadFlag() ) 
            BitStream.ReadRangedVector( m_Movement, 12, -30.0f, 30.0f );
        else
            BitStream.ReadVector( m_Movement );
    }

    if ( BitStream.ReadFlag() )
    {
        BitStream.ReadRangedS32( (s32&)m_State, STATE_MOVING, STATE_LAZARUS );
    }

    m_Pending = 0.0f;

    return( AdvanceLogic( SecInPast ) );
}

//==============================================================================

void pickup::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    if( BitStream.WriteFlag( DirtyBits & DIRTY_POS ) )
    {
        BitStream.WriteVector( m_WorldPos );
        if( BitStream.WriteFlag( m_Movement.InRange( -30.0f, 30.0f ) ) )
            BitStream.WriteRangedVector( m_Movement, 12, -30.0f, 30.0f );
        else
            BitStream.WriteVector( m_Movement );
    }

    if( BitStream.WriteFlag( DirtyBits & DIRTY_STATE ) )
    {
        BitStream.WriteRangedS32( m_State, STATE_MOVING, STATE_LAZARUS );
    }

    DirtyBits = 0;
}

//==============================================================================

void pickup::SetState( pickup::state State )
{
    m_State = State;
    m_DirtyBits |= DIRTY_STATE ;
    
    // just to make sure we're in sync at the state change
    m_DirtyBits |= DIRTY_POS ;          

    if ( ( State != STATE_DEAD ) &&
         ( State != STATE_LAZARUS ) )
        m_AttrBits |= object::ATTR_PERMEABLE;
    else
        m_AttrBits &= (~object::ATTR_PERMEABLE);
}

//==============================================================================

u8 pickup::GetDataItem( s32 Index ) const
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < DATA_ITEMS) ;

    return m_Data[Index] ;
}

//==============================================================================

void pickup::SetDataItem( s32 Index, u8 Value )
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < DATA_ITEMS) ;

    m_Data[Index] = Value ;
}

//==============================================================================

// Returns info about pickups
const pickup::info& pickup::GetInfo ( void ) const
{
    // Something went badly wrong...
    ASSERT(m_Kind >= KIND_NONE) ;
    ASSERT(m_Kind < KIND_TOTAL) ;

    // Info table is out of sync with pickup::kind types!
    ASSERT(s_InfoTable[m_Kind].PickupKind == m_Kind) ;

    // Return info
    return s_InfoTable[m_Kind] ;
}

//==============================================================================


// Returns info about requested pickup kind
const pickup::info& pickup::GetInfo ( pickup::kind Kind )
{
    // Something went badly wrong...
    ASSERT(Kind >= KIND_NONE) ;
    ASSERT(Kind < KIND_TOTAL) ;

    // Info table is out of sync with pickup::kind types!
    ASSERT(s_InfoTable[Kind].PickupKind == Kind) ;

    // Return info
    return s_InfoTable[Kind] ;
}

//==============================================================================

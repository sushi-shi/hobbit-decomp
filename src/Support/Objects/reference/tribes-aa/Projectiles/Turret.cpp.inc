//==============================================================================
//
//  Turret.cpp
//
//==============================================================================

// TO DO: Fire delay based on kind.
// TO DO: Store the pivot point (Center) directly in the turret in World space.

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Turret.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "..\AADS\Globals.hpp"
#include "ParticleObject.hpp"
#include "Objects/Projectiles/Aimer.hpp"
#include "Objects/Projectiles/Plasma.hpp"
#include "Objects/Projectiles/Blaster.hpp"
#include "Objects/Projectiles/Missile.hpp"
#include "Objects/Projectiles/Mortar.hpp"
#include "Objects/Projectiles/Generic.hpp"
#include "Objects\Vehicles\Vehicle.hpp"
#include "GameMgr\GameMgr.hpp" 
#include "LabelSets/Tribes2Types.hpp"
#include "AudioMgr/Audio.hpp"

#include <Support/StringMgr/StringMgr.hpp>
#include "AADS/Data/UI/ui_strings.h"

//==============================================================================
//  DEFINES
//==============================================================================

#define DIRTY_FIRE      0x01
#define DIRTY_STATE     0x02
#define DIRTY_TARGET    0x04
#define DIRTY_BARREL    0x08
#define DIRTY_ELF       0x10

//==============================================================================
//  TYPES
//==============================================================================

struct turret_attributes
{
    f32         RadiusSense;
    f32         RadiusFire;
    f32         TimeSense;
    f32         TimeActivate;
    f32         TimeReload;
    f32         TimeCool;
    f32         TimeDeploy;
    f32         RechargeRate;
    f32         Velocity;
    s32         Center;
    radian      AimRPSPitch;
    radian      AimRPSYaw;
    s32         SoundAmbient;
    s32         SoundActivate;
    s32         SoundDeactivate;
    s32         SoundReload;
    s32         ShapeBase;
    s32         CollisionBase;
    s32         ShapeBarrel;
    pain::type  PainType;
};

enum additive_anim_id
{
    ADDITIVE_ANIM_ID_PITCH,
    ADDITIVE_ANIM_ID_YAW,
    ADDITIVE_ANIM_ID_TOTAL,
};

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   TurretCreator;

//==============================================================================

obj_type_info   TurretFixedTypeInfo ( object::TYPE_TURRET_FIXED, 
                                      "Fixed Turret", 
                                      TurretCreator, 
                                      object::ATTR_UNBIND_ORIGIN |
                                      object::ATTR_UNBIND_TARGET |
                                      object::ATTR_SOLID_DYNAMIC | 
                                      object::ATTR_PERMANENT     |
                                      object::ATTR_LABELED       |
                                      object::ATTR_ASSET         | 
                                      object::ATTR_TURRET        | 
                                      object::ATTR_REPAIRABLE    | 
                                      object::ATTR_ELFABLE       |
                                      object::ATTR_DAMAGEABLE    |
                                      object::ATTR_HOT );

//==============================================================================

obj_type_info   TurretSentryTypeInfo( object::TYPE_TURRET_SENTRY, 
                                      "Sentry Turret", 
                                      TurretCreator, 
                                      object::ATTR_UNBIND_ORIGIN |
                                      object::ATTR_UNBIND_TARGET |
                                      object::ATTR_SOLID_DYNAMIC | 
                                      object::ATTR_PERMANENT     |
                                      object::ATTR_LABELED       |
                                      object::ATTR_ASSET         | 
                                      object::ATTR_TURRET        | 
                                      object::ATTR_REPAIRABLE    | 
                                      object::ATTR_ELFABLE       |
                                      object::ATTR_DAMAGEABLE );

//==============================================================================

obj_type_info   TurretClampTypeInfo ( object::TYPE_TURRET_CLAMP, 
                                      "Clamp Turret", 
                                      TurretCreator, 
                                      object::ATTR_UNBIND_ORIGIN |
                                      object::ATTR_UNBIND_TARGET |
                                      object::ATTR_SOLID_DYNAMIC | 
                                      object::ATTR_LABELED       |
                                      object::ATTR_ASSET         | 
                                      object::ATTR_TURRET        | 
                                      object::ATTR_REPAIRABLE    | 
                                      object::ATTR_ELFABLE       |
                                      object::ATTR_DAMAGEABLE );

//==============================================================================

static turret_attributes  Attr[] =
{
    {   // AA
        150.0f,                             //  f32         RadiusSense;    
        250.0f,                             //  f32         RadiusFire;     
          0.25f,                            //  f32         TimeSense;      
          0.5f,                             //  f32         TimeActivate;   
          1.0f,                             //  f32         TimeReload;     
          1.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.08f,                            //  f32         RechargeRate;
        100.0f,                             //  f32         Velocity;     
          0,                                //  s32         Center;         
        RADIAN( 720 ),                      //  radian      AimRPSPitch;    
        RADIAN( 720 ),                      //  radian      AimRPSYaw;      
        SFX_TURRET_HEAVY_IDLE_LOOP,         //  s32         SoundAmbient;   
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundActivate;  
        SFX_TURRET_HEAVY_DEACTIVATE,        //  s32         SoundDeactivate;
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_FIXED_BASE,       //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_FIXED_BASE_COLL,  //  s32         CollisionBase;      
        SHAPE_TYPE_TURRET_BARREL_AA,        //  s32         ShapeBarrel;  
        pain::EXPLOSION_TURRET_FIXED,       //  pain::type  PainType;
    },

    {   // ELF                                  
         80.0f,                             //  f32         RadiusSense;        
        100.0f,                             //  f32         RadiusFire;         
          0.1f,                             //  f32         TimeSense;          
          1.0f,                             //  f32         TimeActivate;       
          0.0f,                             //  f32         TimeReload;         
          1.0f,                             //  f32         TimeCool;           
          2.0f,                             //  f32         TimeDeploy;         
          0.08f,                            //  f32         RechargeRate;
          1.0f,                             //  f32         Velocity;     
          0,                                //  s32         Center;             
        RADIAN( 120 ),                      //  radian      AimRPSPitch;    
        RADIAN( 120 ),                      //  radian      AimRPSYaw;      
        SFX_TURRET_HEAVY_IDLE_LOOP,         //  s32         SoundAmbient;       
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundActivate;      
        SFX_TURRET_HEAVY_DEACTIVATE,        //  s32         SoundDeactivate;    
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundReload;        
        SHAPE_TYPE_TURRET_FIXED_BASE,       //  s32         ShapeBase;          
        SHAPE_TYPE_TURRET_FIXED_BASE_COLL,  //  s32         CollisionBase;      
        SHAPE_TYPE_TURRET_BARREL_ELF,       //  s32         ShapeBarrel;        
        pain::EXPLOSION_TURRET_FIXED,       //  pain::type  PainType;           
    },                                                                      

    {   // MISSILE
         80.0f,                             //  f32         RadiusSense;    
        250.0f,                             //  f32         RadiusFire;     
          0.25f,                            //  f32         TimeSense;      
          1.5f,                             //  f32         TimeActivate;   
          4.0f,                             //  f32         TimeReload;     
          2.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.08f,                            //  f32         RechargeRate;
        150.0f,                             //  f32         Velocity;     
          0,                                //  s32         Center;         
        RADIAN( 720 ),                      //  radian      AimRPSPitch;    
        RADIAN( 720 ),                      //  radian      AimRPSYaw;      
        SFX_TURRET_HEAVY_IDLE_LOOP,         //  s32         SoundAmbient;   
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundActivate;  
        SFX_TURRET_HEAVY_DEACTIVATE,        //  s32         SoundDeactivate;
        SFX_TURRET_HEAVY_ACTIVATE,            //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_FIXED_BASE,       //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_FIXED_BASE_COLL,  //  s32         CollisionBase;  
        SHAPE_TYPE_TURRET_BARREL_MISSILE,   //  s32         ShapeBarrel;    
        pain::EXPLOSION_TURRET_FIXED,       //  pain::type  PainType;       
    },

    {   // MORTAR
         80.0f,                             //  f32         RadiusSense;    
        160.0f,                             //  f32         RadiusFire;     
          0.5f,                             //  f32         TimeSense;      
          1.0f,                             //  f32         TimeActivate;   
          3.0f,                             //  f32         TimeReload;     
          1.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.08f,                            //  f32         RechargeRate;
        100.0f,                             //  f32         Velocity;     
          0,                                //  s32         Center;         
        RADIAN( 720 ),                      //  radian      AimRPSPitch;    
        RADIAN( 720 ),                      //  radian      AimRPSYaw;      
        SFX_TURRET_HEAVY_IDLE_LOOP,         //  s32         SoundAmbient;   
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundActivate;  
        SFX_TURRET_HEAVY_DEACTIVATE,        //  s32         SoundDeactivate;
        SFX_TURRET_HEAVY_ACTIVATE,            //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_FIXED_BASE,       //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_FIXED_BASE_COLL,  //  s32         CollisionBase;  
        SHAPE_TYPE_TURRET_BARREL_MORTAR,    //  s32         ShapeBarrel;    
        pain::EXPLOSION_TURRET_FIXED,       //  pain::type  PainType;       
    },

    {   // PLASMA
         80.0f,                             //  f32         RadiusSense;    
        120.0f,                             //  f32         RadiusFire;     
          0.2f,                             //  f32         TimeSense;      
          1.8f,                             //  f32         TimeActivate;   
          0.9f,                             //  f32         TimeReload;     
          2.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.08f,                            //  f32         RechargeRate;
         75.0f,                             //  f32         Velocity;     
          0,                                //  s32         Center;         
        RADIAN( 720 ),                      //  radian      AimRPSPitch;    
        RADIAN( 720 ),                      //  radian      AimRPSYaw;      
        SFX_TURRET_HEAVY_IDLE_LOOP,         //  s32         SoundAmbient;   
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundActivate;  
        SFX_TURRET_HEAVY_DEACTIVATE,        //  s32         SoundDeactivate;
        SFX_TURRET_HEAVY_ACTIVATE,            //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_FIXED_BASE,       //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_FIXED_BASE_COLL,  //  s32         CollisionBase;  
        SHAPE_TYPE_TURRET_BARREL_PLASMA,    //  s32         ShapeBarrel;    
        pain::EXPLOSION_TURRET_FIXED,       //  pain::type  PainType;       
    },

    {   // SENTRY
         60.0f,                             //  f32         RadiusSense;    
         60.0f,                             //  f32         RadiusFire;     
          0.2f,                             //  f32         TimeSense;      
          0.5f,                             //  f32         TimeActivate;   
          1.0f,                             //  f32         TimeReload;     
          2.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.10f,                            //  f32         RechargeRate;
        200.0f,                             //  f32         Velocity; 
          1,                                //  s32         Center;         
         R_360,                             //  radian      AimRPSPitch;    
         R_360,                             //  radian      AimRPSYaw;      
        0,                                  //  s32         SoundAmbient;   
        SFX_TURRET_LIGHT_ACTIVATE,          //  s32         SoundActivate;  
        SFX_TURRET_LIGHT_DEACTIVATE,        //  s32         SoundDeactivate;
        0,                                  //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_SENTRY,           //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_SENTRY_COLL,      //  s32         CollisionBase;  
        0,                                  //  s32         ShapeBarrel;    
        pain::EXPLOSION_TURRET_SENTRY,      //  pain::type  PainType;       
    },

    {   // DEPLOYED
         45.0f,                             //  f32         RadiusSense;    
         75.0f,                             //  f32         RadiusFire;     
          0.2f,                             //  f32         TimeSense;      
          1.0f,                             //  f32         TimeActivate;   
          0.125f,                           //  f32         TimeReload;     
          1.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.05f,                            //  f32         RechargeRate;
         90.0f,                             //  f32         Velocity;     
          2,                                //  s32         Center;         
         R_180,                             //  radian      AimRPSPitch;    
         R_180,                             //  radian      AimRPSYaw;      
        0,                                  //  s32         SoundAmbient;   
        SFX_TURRET_REMOTE_ACTIVATE,         //  s32         SoundActivate;  
        SFX_TURRET_REMOTE_DEACTIVATE,       //  s32         SoundDeactivate;
        0,                                  //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_DEPLOYED,         //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_DEPLOYED_COLL,    //  s32         CollisionBase;  
        0,                                  //  s32         ShapeBarrel;    
        pain::EXPLOSION_TURRET_CLAMP,       //  pain::type  PainType;       
    },

    {   // TAIL
        160.0f,                             //  f32         RadiusSense;    
        200.0f,                             //  f32         RadiusFire;     
          0.25f,                            //  f32         TimeSense;      
          0.5f,                             //  f32         TimeActivate;   
          1.0f,                             //  f32         TimeReload;     
          1.0f,                             //  f32         TimeCool;       
          2.0f,                             //  f32         TimeDeploy;     
          0.08f,                            //  f32         RechargeRate;
        150.0f,                             //  f32         Velocity;     
          3,                                //  s32         Center;         
        RADIAN( 720 ),                      //  radian      AimRPSPitch;    
        RADIAN( 720 ),                      //  radian      AimRPSYaw;      
        SFX_TURRET_HEAVY_IDLE_LOOP,         //  s32         SoundAmbient;   
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundActivate;  
        SFX_TURRET_HEAVY_DEACTIVATE,        //  s32         SoundDeactivate;
        SFX_TURRET_HEAVY_ACTIVATE,          //  s32         SoundReload;    
        SHAPE_TYPE_TURRET_TRANSPORT,        //  s32         ShapeBase;      
        SHAPE_TYPE_TURRET_DEPLOYED_COLL,    //  s32         CollisionBase;      
        0,                                  //  s32         ShapeBarrel;  
        pain::EXPLOSION_TURRET_FIXED,       //  pain::type  PainType;
    },
};

static vector3 Center[4] = 
{
    vector3( 0.00f, 2.50f, 0.50f ),    // Fixed   
    vector3( 0.00f,-0.50f, 0.00f ),    // Sentry  
    vector3( 0.00f, 0.75f, 0.00f ),    // Deployed
    vector3( 0.00f, 1.50f, 0.75f ),    // Tail
};

static f32 PainOffset[3] = { 1.0f, -0.20f, 0.25f };

static radian ELFFireCone = R_45;

static s32  ClampFire =  3;
static s32  ClampWait = 20;

static xbool Verbose = FALSE;

//==============================================================================
//  FUNCTION PROTOTYPES (functions defined in ELF.cpp)
//==============================================================================

void    CalcSplinePts       ( const matrix4& Start, 
                              const vector3& End, 
                                    vector3* pPoints, 
                                    s32      NumPoints );

// This functions is actually in Player\Elf.cpp !
xbool   CheckSplineCollision(       collider& Collider,
                                    vector3*  pPts, 
                                    s32       NumPts, 
                                    vector3&  ColPt, 
                                    s32       Exclude );

void    RenderELFBeam       (       vector3* pBeam );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* TurretCreator( void )
{
    return( (object*)(new turret) );
}

//==============================================================================

turret::turret( void )
{
    m_MPB            = FALSE;     
    m_ELFing         = FALSE;
    m_ELFAudioHandle = 0;
}

//==============================================================================

void turret::OnRemove( void )
{
    if( m_ELFAudioHandle != 0 )
    {
        audio_Stop( m_ELFAudioHandle );
        m_ELFAudioHandle = 0;
    }
}

//==============================================================================

object::update_code turret::ServerLogic( f32 DeltaTime )
{
    u32 OldTeamBits = m_TeamBits;

    asset::OnAdvanceLogic( DeltaTime );

    if( OldTeamBits != m_TeamBits )
    {
        // The team bits have changed!
        m_TargetID   = obj_mgr::NullID;
        m_TargetRank = 5;
    }

    if( (m_Health == 0.0f) && (m_Kind == DEPLOYED) )
    {
        return( DESTROY );
    }

    if( (!m_PowerOn) || (m_Disabled) )
    {
        if( m_ELFing )
            m_DirtyBits |= DIRTY_ELF;
        m_ELFing = FALSE;

        return( NO_UPDATE );
    }
    
    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            if( m_Timer >= Attr[m_Kind].TimeSense )
            {
                m_Timer = x_frand( 0.00f, 0.02f );
                if( LookForTarget() )
                {
                    // Target acquired.
                    EnterState( STATE_ACTIVATING );
                }
            }
            break;

        //----------------------------------------------------------------------
        case STATE_ACTIVATING:

            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);

            // If this is a missile turret, then "track" the target.
            {
                object* pObject = ObjMgr.GetObjectFromID( m_TargetID );
                if( pObject && (m_Kind == MISSILE) )
                    pObject->MissileTrack();
            }

            if( m_Timer >= Attr[m_Kind].TimeActivate )
            {
                if( LookForTarget() )
                {
                    EnterState( STATE_AIMING ); 
                    m_Reload = Attr[m_Kind].TimeReload;     // Init reload timer.
                }
                else
                {
                    EnterState( STATE_COOLING );
                }
            }
            break;

        //----------------------------------------------------------------------
        case STATE_AIMING:
        {
            s32 Status = ServerTrack( DeltaTime );

            // On target and ready to fire?
            if( Status == 2 ) 
            {
                // Debugging
                /*
                {
                    Verbose = TRUE;
                    radian  Pitch, Yaw;
                    object* pObject = ObjMgr.GetObjectFromID( m_TargetID );

                    // If we can't find the target, TARGET LOST.
                    if( pObject )
                    {
                        TrackTarget( pObject, Pitch, Yaw );
                    }

                    Verbose = FALSE;
                }
                */

                Fire( DeltaTime );
                
                // Reconsider the current target.
            //  if( m_Kind != ELF )
                {
                    if( !LookForTarget() )
                    {
                        // No target available.
                        EnterState( STATE_COOLING );
                    }
                }
            } 

            // Did we lose sight of the target?
            if( (Status == 0) || ((m_Kind == ELF) && (!m_ELFing)) )
            {    
                // Lost sight of the original target.
                // See if another target is available.
                if( !LookForTarget() )
                {
                    // No target available.
                    EnterState( STATE_COOLING );
                }
            }

            m_Reload += DeltaTime;
        }
        break;

        //----------------------------------------------------------------------
        case STATE_COOLING:
            m_Reload += DeltaTime;
            if( m_Timer > Attr[m_Kind].TimeCool )
            {
                EnterState( STATE_UNAIMING );
            }
            else
            {
                if( (x_irand( 1, 10 ) == 1) && LookForTarget() )
                {
                    EnterState( STATE_AIMING );
                }
            }
            break;

        //----------------------------------------------------------------------
        case STATE_UNAIMING:
        {
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            radian Yaw = (m_Kind == DEPLOYED) ? m_Yaw : R_0;
            if( UpdateAim( DeltaTime, R_0, Yaw ) )
            {
                EnterState( STATE_DEACTIVATING );
            }
            break;
        }

        //----------------------------------------------------------------------
        case STATE_DEACTIVATING:
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            if( m_Timer > Attr[m_Kind].TimeActivate )
            {
                EnterState( STATE_IDLE );
            }
            break;

        //----------------------------------------------------------------------
        case STATE_BARREL_CHANGING:
            if( m_Timer > Attr[m_Kind].TimeDeploy )
            {
                // Don't use enter state here to go back to previous state.  
                // Don't want to kick off sounds and such.
                m_Timer = 0.0f;
                m_State = m_PreChangeState;
                m_DirtyBits |= DIRTY_STATE;
            }
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            if( m_Timer > Attr[m_Kind].TimeDeploy )
            {
                EnterState( STATE_IDLE );
            }
            break;
    }

    m_Pitch = x_ModAngle2( m_Pitch );
    m_Yaw   = x_ModAngle ( m_Yaw   );

    return( NO_UPDATE );
}

//==============================================================================

object::update_code turret::ClientLogic( f32 DeltaTime )
{    
    asset::OnAdvanceLogic( DeltaTime );

    if( (!m_PowerOn) || (m_Disabled) )
        return( NO_UPDATE );

    // State setup on client.
    if( m_State != m_PreChangeState )
    {
        EnterState( m_State );
        m_PreChangeState = m_State;
    }

    // State behavior on client.
    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            break;

        //----------------------------------------------------------------------
        case STATE_ACTIVATING:
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            if( m_Timer >= Attr[m_Kind].TimeActivate )
            {
                EnterState( STATE_AIMING ); 
            }
            break;

        //----------------------------------------------------------------------
        case STATE_AIMING:
            ClientTrack( DeltaTime );
            break;

        //----------------------------------------------------------------------
        case STATE_COOLING:
            if( m_Timer > Attr[m_Kind].TimeCool )
            {
                EnterState( STATE_UNAIMING );
            }
            break;

        //----------------------------------------------------------------------
        case STATE_UNAIMING:
        {
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            radian Yaw = (m_Kind == DEPLOYED) ? m_Yaw : R_0;
            if( UpdateAim( DeltaTime, R_0, Yaw ) )
            {
                EnterState( STATE_DEACTIVATING );
            }
            break;
        }

        //----------------------------------------------------------------------
        case STATE_DEACTIVATING:
            if( m_Kind == DEPLOYED )
                m_Yaw += (R_120 * DeltaTime);
            if( m_Timer > Attr[m_Kind].TimeActivate )
            {
                EnterState( STATE_IDLE );
            }
            break;

        //----------------------------------------------------------------------
        case STATE_BARREL_CHANGING:
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            if( m_Timer > Attr[m_Kind].TimeDeploy )
            {
                EnterState( STATE_IDLE );
            }
            break;
    }

    m_Pitch = x_ModAngle2( m_Pitch );
    m_Yaw   = x_ModAngle ( m_Yaw   );

    return( NO_UPDATE );
}

//==============================================================================

void turret::EnterState( turret::state State )
{
    m_Timer = 0.0f;

    if( State == m_State )
        return;
  
    m_State = State;

    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            m_Pitch = R_0;
            if( m_Kind != DEPLOYED )
                m_Yaw = R_0;
            m_Shape.SetAnimByType( ANIM_TYPE_TURRET_ACTIVATE );
            m_Shape.GetCurrentAnimState().SetFrameParametric( 0.0f );
            m_DirtyBits |= DIRTY_STATE;   
            break;

        //----------------------------------------------------------------------
        case STATE_ACTIVATING:
            m_DirtyBits |= DIRTY_STATE;   
            m_Shape.GetCurrentAnimState().SetSpeed( 0.0f );
            audio_Play( Attr[m_Kind].SoundActivate, &m_WorldPos );
            break;

        //----------------------------------------------------------------------
        case STATE_AIMING:
            m_ShotCount  = -1;
            m_DirtyBits |= DIRTY_STATE;   
            break;

        //----------------------------------------------------------------------
        case STATE_COOLING:
            m_DirtyBits |= DIRTY_STATE; 
            if( m_Kind == ELF )
            {
                if( m_ELFing )
                {
                    m_ELFing = FALSE;
                    m_DirtyBits |= DIRTY_ELF; 
                }
                if( m_ELFAudioHandle != 0 )
                {
                    audio_Stop( m_ELFAudioHandle );
                    m_ELFAudioHandle = 0;
                }
            }
            break;

        //----------------------------------------------------------------------
        case STATE_UNAIMING:
            break;

        //----------------------------------------------------------------------
        case STATE_DEACTIVATING:
            m_Pitch = R_0;
            if( m_Kind != DEPLOYED )
                m_Yaw = R_0;
            m_Shape.GetCurrentAnimState().SetSpeed( 0.0f );
            audio_Play( Attr[m_Kind].SoundDeactivate, &m_WorldPos );
            break;

        //----------------------------------------------------------------------
        case STATE_BARREL_CHANGING:
            m_DirtyBits |= DIRTY_STATE;
            m_DirtyBits |= DIRTY_BARREL;
            m_Barrel.SetAnimByType( ANIM_TYPE_TURRET_DEPLOY );
            audio_Play( SFX_PACK_TURRET_BARREL_DEPLOY, &m_WorldPos );
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            m_DirtyBits |= DIRTY_STATE;
            m_Shape.SetAnimByType( ANIM_TYPE_TURRET_DEPLOY );
            audio_Play( SFX_PACK_SPIDER_DEPLOY, &m_WorldPos );
            break;
    }
}

//==============================================================================

object::update_code turret::OnAdvanceLogic( f32 DeltaTime )
{    
    if( (m_Health > 0.0f) && 
        (m_Shape.GetAnimByType() == ANIM_TYPE_TURRET_DESTROYED) )
    {   
        m_Shape.SetAnimByType( ANIM_TYPE_TURRET_ACTIVATE );
    }

    m_Shape.Advance( DeltaTime );
    if( m_Kind < SENTRY )
        m_Barrel.Advance( DeltaTime );

    m_Timer += DeltaTime;

    if( tgl.ServerPresent )
        return( ServerLogic( DeltaTime ) );
    else
        return( ClientLogic( DeltaTime ) );
}

//==============================================================================

void turret::DebugRender( void )
{
    asset::DebugRender();

    if( !g_DebugAsset.ShowExtra )
        return;

    if( m_MPB )
    {
        matrix4 L2W;
        L2W.Identity();
        L2W.SetRotation( m_WorldRot );
        L2W.SetTranslation( m_WorldPos );
        draw_SetL2W( L2W );
        draw_Axis( 5.0f );
    }

    draw_ClearL2W();
    draw_Point( m_WorldPos, XCOLOR_BLUE );
    xcolor Color = XCOLOR_RED;

    if( (m_State == STATE_AIMING) || 
        (m_State == STATE_UNAIMING) || 
        (m_State == STATE_ACTIVATING) || 
        (m_State == STATE_DEACTIVATING) || 
        (m_State == STATE_COOLING) )
    {
        vector3 Point1;
        vector3 Point2;

        Point1 = Center[ Attr[m_Kind].Center ];
        Point1.Rotate( m_WorldRot );
        Point1 += m_WorldPos;

        Point2( 0, 0, Attr[m_Kind].RadiusFire );
        Point2.Rotate( radian3( m_Pitch, m_Yaw, R_0 ) );
        Point2 += Center[ Attr[m_Kind].Center ];
        Point2.Rotate( m_WorldRot );
        Point2 += m_WorldPos;

        if( m_State == STATE_ACTIVATING )
            Color.R = u8(255 * (m_Timer / Attr[m_Kind].TimeActivate));

        if( m_State == STATE_DEACTIVATING )
            Color.R = u8(255 * (1.0f - (m_Timer / Attr[m_Kind].TimeActivate)));

        draw_Line( Point1, Point2, Color );

        if( m_State == STATE_AIMING )
        {
            object* pObject = ObjMgr.GetObjectFromID( m_TargetID );
            
            if( pObject )
            {
                Point2 = pObject->GetBlendPainPoint();
                Point2 = m_W2L.Transform( Point2 );
                Point2.Rotate( m_WorldRot );
                Point2 += m_WorldPos;
                Color.Random();
                draw_Point( Point2, Color );
            }
        }

        {
            Color.Random();
            draw_Point( Point1, Color );
        }
    }

    switch( m_Kind )
    {
        case AA:
        {
            vector3 Pos;
            Pos = m_Barrel.GetHotPointByIndexWorldPos( 0 );
            draw_Point( Pos, XCOLOR_RED );
            Pos = m_Barrel.GetHotPointByIndexWorldPos( 1 );
            draw_Point( Pos, XCOLOR_BLUE );
            break;
        }
        case TAIL:
        {
            vector3 Pos;
            Pos = m_Shape.GetHotPointByIndexWorldPos( 0 );
            draw_Point( Pos, XCOLOR_RED );
            Pos = m_Shape.GetHotPointByIndexWorldPos( 1 );
            draw_Point( Pos, XCOLOR_BLUE );
            break;
        }
        case ELF:
        case MISSILE:
        case MORTAR:
        case PLASMA:
        {
            vector3 Pos;
            Pos = m_Barrel.GetHotPointByIndexWorldPos( 0 );
            draw_Point( Pos, XCOLOR_RED );
            break;
        }
        case SENTRY:
        case DEPLOYED:
        {
            vector3 Pos;
            Pos = m_Shape.GetHotPointByIndexWorldPos( 0 );
            draw_Point( Pos, XCOLOR_RED );
            break;
        }
    }
}

//==============================================================================

void turret::UpdateInstance( void )
{
    // Set the animation.
    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            m_Shape.GetCurrentAnimState().SetFrameParametric( 0.0f );
            break;

        //----------------------------------------------------------------------
        case STATE_ACTIVATING:
            m_Shape.GetCurrentAnimState().SetFrameParametric( m_Timer / Attr[m_Kind].TimeActivate );
            break;

        //----------------------------------------------------------------------
        case STATE_AIMING:
        case STATE_COOLING:
        case STATE_UNAIMING:
            m_Shape.GetCurrentAnimState().SetFrameParametric( 1.0f );
            break;

        //----------------------------------------------------------------------
        case STATE_DEACTIVATING:
            m_Shape.GetCurrentAnimState().SetFrameParametric( 1.0f - (m_Timer / Attr[m_Kind].TimeActivate) );
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            m_Shape.GetCurrentAnimState().SetFrameParametric( m_Timer / Attr[m_Kind].TimeDeploy );
            break;

        //----------------------------------------------------------------------
        default:
            break;
    }

    // Aim the turret.
    {
        f32 Frame;
        anim_state* pAnimState;

        pAnimState = m_Shape.GetAdditiveAnimState( ADDITIVE_ANIM_ID_PITCH );
        ASSERT( pAnimState );
        Frame = (m_Pitch + R_90) / R_180;    
        pAnimState->SetFrameParametric( Frame );

        pAnimState = m_Shape.GetAdditiveAnimState( ADDITIVE_ANIM_ID_YAW );
        ASSERT( pAnimState );
        Frame = m_Yaw / R_360;
        pAnimState->SetFrameParametric( Frame );
    }        
}

//==============================================================================

void turret::Render( void )
{
    // Special case.
    if( (m_Kind == DEPLOYED) && (m_Health == 0.0f) )
        return;

    if( m_Kind == TAIL )
    {
        // Can't use render prep function on TAIL turrets 'cause they move.
        m_ShapeDrawFlags = shape::DRAW_FLAG_FOG | shape::DRAW_FLAG_CLIP;
        m_Shape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
    }
    else
    {
        if( !RenderPrep() )
            return;
    }

    UpdateInstance();

    // Draw barrel.
    if( m_Kind < SENTRY )
    {
        m_Barrel.SetColor( m_Shape.GetColor() );
        m_Barrel.SetFogColor( m_Shape.GetFogColor() );
        m_Barrel.Draw( m_ShapeDrawFlags, m_DmgTexture, m_DmgClutHandle );
    }

    // Draw main shape last so dirty bounds works for barrel.
    m_Shape.Draw( m_ShapeDrawFlags, m_DmgTexture, m_DmgClutHandle );

    if( g_DebugAsset.DebugRender )
        DebugRender();
}

//==============================================================================

void turret::RenderEffects( void )
{
    // Draw the ELF beam if needed.
    if( m_ELFing && (m_State == STATE_AIMING) )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_TargetID );
        if( pObject )
        {
            matrix4 L2W;
            radian3 Rotation;
            vector3 Position;
            vector3 Direction( m_Pitch, m_Yaw );

            Direction.Rotate( m_WorldRot );
            Direction.GetPitchYaw( Rotation.Pitch, Rotation.Yaw );
            Rotation.Roll = R_0;    

            Position = m_Barrel.GetHotPointByIndexWorldPos( 0 );

            L2W.Identity();
            L2W.SetRotation( Rotation );
            L2W.SetTranslation( Position );
            
            vector3 Spline[16];
            vector3 Target = pObject->GetBlendPainPoint();

            CalcSplinePts( L2W, Target, Spline, 16 );
            RenderELFBeam( Spline );
        }
    }
}

//==============================================================================
// Used for Fixed and Sentry Turrets when loaded from Mission.txt file.

void turret::Initialize( const vector3& WorldPos,
                         const radian3& WorldRot,
                               s32      Switch,
                               s32      Power,
                         turret::kind   Kind,
                         const xwchar*  pLabel )
{   
    vector3 Pos = WorldPos;

    // "Drop" the turret on the surface it is against.
    {
        collider::collision Collision;
        collider            Collider;
        f32                 Delta = 0.5f;
    
        if( Kind == turret::SENTRY )
            Delta = -0.5f;
    
        // Get local Y-axis of turret.
        matrix4 M( WorldRot );
        vector3 X, Y, Z;
    
        M.GetColumns( X, Y, Z );
    
        Y *= Delta;
    
        vector3 S = WorldPos + Y;
        vector3 E = WorldPos - Y;
    
        Collider.RaySetup( NULL, S, E );
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Collider );
    
        if( Collider.GetCollision( Collision ) )
        {
            Pos = Collision.Point;
        }
        else
        {
        //  ASSERT( FALSE );
        }
    }

    asset::Initialize( Pos, WorldRot, Switch, Power, pLabel );
    
    m_TargetID   = obj_mgr::NullID;
    m_TargetRank = 5;
    m_Pitch      = R_0;
    m_Yaw        = R_0;
    m_Kind       = Kind;

    CommonInit();

    m_State = STATE_UNDEFINED;
    EnterState( STATE_IDLE );
}

//==============================================================================
// Used on server to create Deployed Turrets.

void turret::Initialize( const vector3&   WorldPos,
                         const vector3&   Normal,
                         const object::id PlayerID,
                               u32        TeamBits )
{
    radian3 WorldRot;
    Normal.GetPitchYaw( WorldRot.Pitch, WorldRot.Yaw );
    WorldRot.Roll   = R_0;
    WorldRot.Pitch += R_90;

    asset::Initialize( WorldPos, WorldRot, -1, 0, StringMgr( "ui", IDS_REMOTE_TURRET ) );

    m_TeamBits   = TeamBits;
    m_TargetID   = obj_mgr::NullID;
    m_TargetRank = 5;
    m_Pitch      = R_0;
    m_Yaw        = R_0;
    m_Kind       = DEPLOYED;
    m_OriginID   = PlayerID;

    CommonInit();

    m_State = STATE_UNDEFINED;
    EnterState( STATE_DEPLOYING );
}

//==============================================================================

void turret::MPBInitialize( const matrix4 L2W,
                                  u32     TeamBits )
{
    vector3 Pos = L2W.GetTranslation();
    radian3 Rot = L2W.GetRotation();
    
    asset::Initialize( Pos, Rot, -1, 0, NULL );
    m_RechargeRate = 10.0f;

    m_TeamBits   = TeamBits;
    m_TargetID   = obj_mgr::NullID;
    m_TargetRank = 5;
    m_Pitch      = R_0;
    m_Yaw        = R_0;
    m_Kind       = TAIL;
    m_MPB        = TRUE;

    CommonInit();

    m_State = STATE_UNDEFINED;
    EnterState( STATE_IDLE );
}

//==============================================================================

void turret::MPBSetL2W( const matrix4& L2W )
{
    asset::MPBSetL2W( L2W );

    m_W2L.Identity();
    m_W2L.Rotate( m_WorldRot );
    m_W2L.Translate( m_WorldPos );
    m_W2L.InvertRT();
}

//==============================================================================

void turret::OnAcceptInit( const bitstream& BitStream )
{
    s32 State;

    asset::OnAcceptInit( BitStream );

    BitStream.ReadRangedS32( State, STATE_UNDEFINED, STATE_DEPLOYING );
    BitStream.ReadObjectID ( m_TargetID );
    BitStream.ReadRangedF32( m_Pitch, 8, -R_180, R_180 );
    BitStream.ReadRangedF32( m_Yaw,   9,  R_0  , R_360 );
    m_MPB = BitStream.ReadFlag();

    if( m_MPB )
        m_Kind = TAIL;
    else
    {
        m_Kind = DEPLOYED;             // Can be only "deployed".
        x_wstrcpy( m_Label, StringMgr( "ui", IDS_REMOTE_TURRET ) );
    }
    
    CommonInit();

    m_State = STATE_UNDEFINED;
    EnterState( (state)State );
}

//==============================================================================

void turret::CommonInit( void )
{
    m_W2L.Identity();
    m_W2L.Rotate( m_WorldRot );
    m_W2L.Translate( m_WorldPos );
    m_W2L.InvertRT();

    m_PreChangeState = STATE_UNDEFINED;
    m_Timer = 0.0f;

    if( m_MPB )
        m_AttrBits &= ~( object::ATTR_PERMANENT  |
                         object::ATTR_LABELED    |
                         object::ATTR_REPAIRABLE | 
                         object::ATTR_ELFABLE    |
                         object::ATTR_DAMAGEABLE |
                         object::ATTR_HOT );

    if( m_Kind == DEPLOYED )
        m_AttrBits &= ~( object::ATTR_PERMANENT  |
                         object::ATTR_HOT );

    // Prepare for ambient audio.
    m_AmbientID = Attr[m_Kind].SoundAmbient;

    // Setup shape.
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( Attr[m_Kind].ShapeBase ) );
    ASSERT( m_Shape.GetShape() );
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );
    m_Shape.SetAnimByType( ANIM_TYPE_TURRET_ACTIVATE );
    m_Shape.GetCurrentAnimState().SetSpeed( 0.0f );

    anim_state* PitchAnimState = m_Shape.AddAdditiveAnim( ADDITIVE_ANIM_ID_PITCH );
    ASSERT( PitchAnimState );
    PitchAnimState->SetAnim( m_Shape.GetShape()->GetAnimByType(ANIM_TYPE_TURRET_PITCH) );
    ASSERT( PitchAnimState->GetAnim() );
    PitchAnimState->SetSpeed( 0 );

    anim_state* YawAnimState = m_Shape.AddAdditiveAnim( ADDITIVE_ANIM_ID_YAW );
    ASSERT( YawAnimState );
    YawAnimState->SetAnim( m_Shape.GetShape()->GetAnimByType(ANIM_TYPE_TURRET_YAW) );
    ASSERT( YawAnimState->GetAnim() );
    YawAnimState->SetSpeed( 0 );

    // Setup collision.
    m_Collision.SetShape( tgl.GameShapeManager.GetShapeByType( Attr[m_Kind].CollisionBase ) );
    ASSERT(m_Collision.GetShape()) ;
    m_Collision.SetPos( m_WorldPos );
    m_Collision.SetRot( m_WorldRot );

    // Barrel setup if needed.
    if( m_Kind < SENTRY )
    {
        m_Barrel.SetShape( tgl.GameShapeManager.GetShapeByType( Attr[m_Kind].ShapeBarrel ) );
        ASSERT( m_Barrel.GetShape() );
        m_Barrel.SetParentInstance( &m_Shape, HOT_POINT_TYPE_TURRET_BARREL_MOUNT );
        m_Barrel.SetAnimByType( ANIM_TYPE_TURRET_IDLE );

        if( (m_PowerOn) && (!m_Disabled) )
        {
            m_Barrel.SetTextureAnimFPS( 8.0f );
        }
        else
        {
            m_Barrel.SetTextureAnimFPS( 0.0f );
            m_Barrel.SetTextureFrame( 0 );
        }
    }

    // Set final world bbox.
    m_WorldBBox  = m_Shape.GetWorldBounds();
    m_WorldBBox += m_Collision.GetWorldBounds() ;

    // Compute the zone set.
    ComputeZoneSet( m_ZoneSet, m_WorldBBox, TRUE );

    // Recharge rate.
    m_RechargeRate = Attr[m_Kind].RechargeRate;

    // Set the pain point.
    m_PainPoint( 0, PainOffset[Attr[m_Kind].Center], 0 );
    m_PainPoint.Rotate( m_WorldRot );
    m_PainPoint += m_WorldPos;

    // Set up the bubble effects values.
    switch( m_Kind )
    {
    case AA:
    case ELF:
    case MISSILE:
    case MORTAR:
    case PLASMA:
        m_BubbleOffset( 0.00f, 1.25f, 0.75f );
        m_BubbleScale ( 4.00f, 6.00f, 5.00f );
        break;
    case SENTRY:
        m_BubbleOffset( 0.00f,-0.50f, 0.00f );
        m_BubbleScale ( 1.50f, 1.50f, 1.50f );
        break;
    case TAIL:
    case DEPLOYED:
        m_BubbleOffset( 0.00f, 0.75f, 0.00f );
        m_BubbleScale ( 1.25f, 2.00f, 1.25f );
        break;
    }
}

//==============================================================================

void turret::OnProvideInit( bitstream& BitStream )
{
    asset::OnProvideInit( BitStream );

    BitStream.WriteRangedS32( m_State, STATE_UNDEFINED, STATE_DEPLOYING );
    BitStream.WriteObjectID ( m_TargetID );
    BitStream.WriteRangedF32( m_Pitch, 8, -R_180, R_180 );
    BitStream.WriteRangedF32( m_Yaw,   9,  R_0  , R_360 );
    BitStream.WriteFlag     ( m_MPB );
}

//==============================================================================

object::update_code turret::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    s32     Int;
    xbool   NewELFing;

    //x_DebugMsg( "\n{%d}", m_ObjectID.Slot );

    asset::OnAcceptUpdate( BitStream, SecInPast );

    //x_DebugMsg( "[%d]", BitStream.GetCursor() );

    if( BitStream.ReadFlag() )
    {
        if( m_Kind < SENTRY )
            m_Barrel.SetAnimByType( ANIM_TYPE_TURRET_FIRE );
        if( m_Kind == TAIL )
            m_Shape.SetAnimByType( ANIM_TYPE_TURRET_FIRE );
    }

    NewELFing = BitStream.ReadFlag();

    if( BitStream.ReadFlag() )
    {
        BitStream.ReadS32( Int, 4 );  
        if( m_Kind < SENTRY )
            ChangeBarrel( (kind)Int );
    }

    if( BitStream.ReadFlag() )
    {
        m_PreChangeState = m_State;
        BitStream.ReadS32( Int, 5 );  
        EnterState( (state)Int );
        //x_DebugMsg( "(%d)", (s32)m_State );
    }

    if( BitStream.ReadFlag() )
    {
        BitStream.ReadObjectID( m_TargetID );
        //x_DebugMsg( "<%d>", m_TargetID.Slot );
    }

    if( NewELFing && !m_ELFing )
    {
        m_ELFAudioHandle = audio_Play( SFX_WEAPONS_REPAIR_USE, &m_WorldPos );
    }

    if( !NewELFing && m_ELFing && (m_ELFAudioHandle != 0) )
    {
        audio_Stop( m_ELFAudioHandle );
        m_ELFAudioHandle = 0;
    }

    m_ELFing = NewELFing;

    return( ClientLogic( SecInPast ) );
}

//==============================================================================

void turret::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    if( DirtyBits == 0xFFFFFFFF )
        DirtyBits =  0x8000001F;

    //x_DebugMsg( "{%d}", m_ObjectID.Slot );

    //if( DirtyBits & 0x80000000 )
        //x_DebugMsg( "+" );

    asset::OnProvideUpdate( BitStream, DirtyBits, Priority );

    //x_DebugMsg( "[%d,%d]", BitStream.GetCursor(), DirtyBits );

    BitStream.WriteFlag( DirtyBits & DIRTY_FIRE );
    BitStream.WriteFlag( m_ELFing );

    if( BitStream.WriteFlag( DirtyBits & DIRTY_BARREL ) )
    {
        BitStream.WriteS32( m_Kind, 4 );
    }

    if( BitStream.WriteFlag( DirtyBits & DIRTY_STATE ) )
    {
        BitStream.WriteS32( m_State, 5 );
        //x_DebugMsg( "(%d)", m_State );
    }

    if( BitStream.WriteFlag( DirtyBits & DIRTY_TARGET ) )
    {
        BitStream.WriteObjectID( m_TargetID );
        //x_DebugMsg( "<%d>", m_TargetID.Slot );
    }

    //x_DebugMsg( "\n" );

    DirtyBits &= ~0x1F;
}

//==============================================================================

xbool turret::LookForTarget( void )
{
    xbool Selection = FALSE;

    // Rank for targets:
    //  1 - Enemy player
    //  2 - Enemy vehicle
    //  3 - Enemy in a friendly vehicle
    //  4 - Enemy damageable object of any kind
    //  5 - No target

    // Targetting an enemy player?
    if( m_TargetRank == 1 )
    {   
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_TargetID );

        // Look for reasons to invalidate this target:
        //  - Dead.
        //  - Not found.
        //  - Not sensed by turret's team.
        //  - On same team as turret.
        //  - Not hot for Missile.
        //  - Not hot for Tail.
        //  - Not hot for AA.
        //  - In a vehicle.
        //  - Out of range.
        //  - No line of sight.
        
        if( (pPlayer == NULL) ||
            (pPlayer->IsDead()) ||
            (!(pPlayer->GetSensedBits() & m_TeamBits)) ||
            (pPlayer->GetTeamBits() & m_TeamBits) ||
            ((m_Kind == MISSILE) && !(pPlayer->GetAttrBits() & ATTR_HOT)) ||
            ((m_Kind == TAIL   ) && !(pPlayer->GetAttrBits() & ATTR_HOT)) ||
            ((m_Kind == AA     ) && !(pPlayer->GetAttrBits() & ATTR_HOT)) ||
            (pPlayer->IsAttachedToVehicle()) ||
            ((pPlayer->GetBlendPainPoint() - m_WorldPos).Length() > Attr[m_Kind].RadiusFire) ||
            (!CheckLOS( pPlayer )) )
        {
            // Target is invalid for some reason.
            m_TargetID   = obj_mgr::NullID;
            m_TargetRank = 5;
            m_DirtyBits |= DIRTY_TARGET;
        }
    }

    // Look for an enemy player to target.
    if( m_TargetRank > 1 )
    {          
        object* pObject;

        // Select all players and vehicles within radius of fire.  Note that
        // this selection may potentially be used repeatedly in this function.
        ObjMgr.Select( ATTR_PLAYER | ATTR_VEHICLE, m_WorldPos, Attr[m_Kind].RadiusFire );
        Selection = TRUE;

        // Iterate thru the currently selected objects.
        ObjMgr.RestartSelection();
        while( (pObject = ObjMgr.GetNextSelected()) )
        {
            // Look for reasons to invalidate this target candidate:

            //  - Not a player.
            if( !(pObject->GetAttrBits() & ATTR_PLAYER) )
                continue;

            //  - Dead.
            if( ((player_object*)pObject)->IsDead() )
                continue;

            //  - Not sensed by turret's team.
            if( !(pObject->GetSensedBits() & m_TeamBits) )
                continue;

            //  - On same team as turret.
            if( pObject->GetTeamBits() & m_TeamBits )
                continue;

            //  - Not hot for Missile.
            if( (m_Kind == MISSILE) && !(pObject->GetAttrBits() & ATTR_HOT) )
                continue;

            //  - Not hot for Tail.
            if( (m_Kind == TAIL) && !(pObject->GetAttrBits() & ATTR_HOT) )
                continue;

            //  - Not hot for AA.
            if( (m_Kind == AA) && !(pObject->GetAttrBits() & ATTR_HOT) )
                continue;

            //  - In a vehicle.
            if( ((player_object*)pObject)->IsAttachedToVehicle() )
                continue;

            //  - No line of sight.
            if( !CheckLOS( pObject ) )
                continue;

            // If we are here, then the target candidate is acceptable.
            m_TargetID   = pObject->GetObjectID();
            m_TargetRank = 1;
            m_DirtyBits |= DIRTY_TARGET;
            break;
        }
    }

    // Currently targetting an enemy vehicle?
    if( m_TargetRank == 2 )
    {
        vehicle* pVehicle = (vehicle*)ObjMgr.GetObjectFromID( m_TargetID );

        // Look for reasons to invalidate this target:
        //  - Not found.
        //  - Not sensed by turret's team.
        //  - On same team as turret.
        //  - Out of range.
        //  - No line of sight.
        
        if( (pVehicle == NULL) ||
            (!(pVehicle->GetSensedBits() & m_TeamBits)) ||
            (pVehicle->GetTeamBits() & m_TeamBits) ||
            ((pVehicle->GetBlendPainPoint() - m_WorldPos).Length() > Attr[m_Kind].RadiusFire) ||
            (!CheckLOS( pVehicle )) )
        {
            // Target is invalid for some reason.
            m_TargetID   = obj_mgr::NullID;
            m_TargetRank = 5;
            m_DirtyBits |= DIRTY_TARGET;
        }
    }

    // Look for an enemy vehicle to target.
    if( m_TargetRank > 2 )
    {          
        object* pObject;

        // Iterate thru the currently selected objects.
        ObjMgr.RestartSelection();
        while( (pObject = ObjMgr.GetNextSelected()) )
        {
            // Look for reasons to invalidate this target candidate:

            //  - Not a vehicle.
            if( !(pObject->GetAttrBits() & ATTR_VEHICLE) )
                continue;

            //  - Not sensed by turret's team.
            if( !(pObject->GetSensedBits() & m_TeamBits) )
                continue;

            //  - On same team as turret.
            if( pObject->GetTeamBits() & m_TeamBits )
                continue;

            //  - No line of sight.
            if( !CheckLOS( pObject ) )
                continue;

            // If we are here, then the target candidate is acceptable.
            m_TargetID   = pObject->GetObjectID();
            m_TargetRank = 2;
            m_DirtyBits |= DIRTY_TARGET;
            break;
        }
    }

    // Currently targetting enemy in friendly vehicle?
    if( m_TargetRank == 3 )
    {
        player_object* pPlayer  = (player_object*)ObjMgr.GetObjectFromID( m_TargetID );
        vehicle*       pVehicle = NULL;

        // Look for reasons to invalidate this target:
        //  - Not found.
        //  - Not sensed by turret's team.
        //  - On same team as turret.
        //  - NOT in a vehicle.
        //  - VEHICLE out of range.
        //  - No line of sight to VEHICLE.
        
        if( (pPlayer == NULL) ||
            (!(pPlayer->GetSensedBits() & m_TeamBits)) ||
            (pPlayer->GetTeamBits() & m_TeamBits) ||
            ((pVehicle = pPlayer->IsAttachedToVehicle()) == NULL) ||
            ((pVehicle->GetBlendPainPoint() - m_WorldPos).Length() > Attr[m_Kind].RadiusFire) ||
            (!CheckLOS( pVehicle )) )
        {
            // Target is invalid for some reason.
            m_TargetID   = obj_mgr::NullID;
            m_TargetRank = 5;
            m_DirtyBits |= DIRTY_TARGET;
        }
    }

    // Look for an enemy in a friendly vehicle to target.
    if( m_TargetRank > 3 )
    {  
        object*  pObject;
        vehicle* pVehicle;

        // Iterate thru the currently selected objects.
        ObjMgr.RestartSelection();
        while( (pObject = ObjMgr.GetNextSelected()) )
        {
            // Look for reasons to invalidate this target candidate:
            // (Don't need to check team of vehicle.  If it was an enemy 
            // vehicle, it would have been targetted above.)
        
            //  - Not a player.
            if( !(pObject->GetAttrBits() & ATTR_PLAYER) )
                continue;

            //  - Not sensed by turret's team.
            if( !(pObject->GetSensedBits() & m_TeamBits) )
                continue;

            //  - On same team as turret.
            if( pObject->GetTeamBits() & m_TeamBits )
                continue;

            //  - NOT in a vehicle.
            if( (pVehicle = ((player_object*)pObject)->IsAttachedToVehicle()) == NULL )
                continue;

            //  - No line of sight to VEHICLE.
            if( !CheckLOS( pVehicle ) )
                continue;

            // If we are here, then the target candidate is acceptable.
            m_TargetID   = pObject->GetObjectID();
            m_TargetRank = 3;
            m_DirtyBits |= DIRTY_TARGET;
            break;
        }
    }

    /* NEW CODE */

    // Currently targetting an enemy satchel charge?
    if( m_TargetRank == 4 )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_TargetID );

        // Look for reasons to invalidate this target:
        //  - Not found.

        if( pObject == NULL )
        {
            // Target is invalid for some reason.
            m_TargetID   = obj_mgr::NullID;
            m_TargetRank = 5;
            m_DirtyBits |= DIRTY_TARGET;
        }
    }

    // Sentry and remote turrets can shoot at satchel charges.
    if( (m_TargetRank > 4) && 
        ((m_Kind == SENTRY) || (m_Kind == DEPLOYED)) )
    {
        object*  pObject;

        if( Selection )
            ObjMgr.ClearSelection();

        ObjMgr.Select( ATTR_DAMAGEABLE, m_WorldPos, Attr[m_Kind].RadiusSense / 2.0f );
        Selection = TRUE;

        // Iterate thru the currently selected objects.
        ObjMgr.RestartSelection();
        while( (pObject = ObjMgr.GetNextSelected()) )
        {
            //  - Not a satchel charge.
            if( pObject->GetType() != TYPE_SATCHEL_CHARGE )
                continue;

            //  - On same team as turret.
            if( pObject->GetTeamBits() & m_TeamBits )
                continue;

            //  - No line of sight.
            if( !CheckLOS( pObject ) )
                continue;

            // If we are here, then we got one!  Shoot it!
            m_TargetID   = pObject->GetObjectID();
            m_TargetRank = 4;
            m_DirtyBits |= DIRTY_TARGET;
            break;
        }
    }

    /* NEW CODE */

    if( Selection )
        ObjMgr.ClearSelection();

    return( m_TargetRank < 5 );
}

//==============================================================================

void turret::ClientTrack( f32 DeltaTime )
{
    radian  Pitch, Yaw;
    object* pObject = ObjMgr.GetObjectFromID( m_TargetID );

    if( !pObject )
        return;

    if( m_TargetID.Seq == -1 )
        m_TargetID.Seq = pObject->GetObjectID().Seq;

    TrackTarget( pObject, Pitch, Yaw );
    UpdateAim( DeltaTime, Pitch, Yaw );
}

//==============================================================================

s32 turret::ServerTrack( f32 DeltaTime )
{
    radian  Pitch, Yaw;
    xbool   OnTarget;
    object* pObject = ObjMgr.GetObjectFromID( m_TargetID );

    // If we can't find the target, TARGET LOST.
    if( !pObject )
        return( 0 );

    // If "enemy in friendly vehicle" gets out of the vehicle, TARGET LOST.
    if( m_TargetRank == 3 )
    {
        pObject = ((player_object*)pObject)->IsAttachedToVehicle();
        if( !pObject )
            return( 0 );
    }

    // If "hot target" isn't hot anymore, TARGET LOST.
    if( (m_Kind == AA) || (m_Kind == TAIL) || (m_Kind == MISSILE) )
    {
        if( (pObject->GetAttrBits() & object::ATTR_HOT) == 0x00 )
            return( 0 );
    }

    // If this is a missile turret, then "track" the target.
    if( m_Kind == MISSILE )
        pObject->MissileTrack();

    TrackTarget( pObject, Pitch, Yaw );

    OnTarget = UpdateAim( DeltaTime, Pitch, Yaw );

    if( OnTarget )
    {
        // Special case.  ELF!
        if( m_Kind == ELF )
        {
            return( 2 );        // ON TARGET AND READY TO FIRE
        }

        // Special case.  The AA and tail turrets fire twice per "shot".  The
        // second shot takes place a little while after the first.  So...
        // If it is an AA/tail turret, and the reload time just passed the 1/3
        // way mark, FIRE!
        if( (m_Kind == AA) || (m_Kind == TAIL) )
        {
            f32 Next = Attr[m_Kind].TimeReload * 0.34f;
            if( (m_Reload > Next) && ((m_Reload - DeltaTime) <= Next) )
            {   
                return( 2 );    // ON TARGET AND READY TO FIRE
            }
        }

        // We are on target.  How about reload time?
        if( m_Reload > Attr[m_Kind].TimeReload )
        {
            // On target, weapon reloaded... Line of Sight?
            if( CheckLOS( pObject ) )
                return( 2 );    // ON TARGET AND READY TO FIRE
            else
                return( 0 );    // TARGET LOST!
        }
        else
        {
            return( 1 );        // ON TARGET, BUT RELOADING
        }
    }
    else
    {
        return( 1 );            // TARGET IDENTIFIED, BUT AIM NOT GOOD YET
    }
}

//==============================================================================

void turret::TrackTarget( const object* pObject, radian& Pitch, radian& Yaw )
{
    vector3 Velocity;
    vector3 Position;
    xbool   Success = FALSE;
    vector3 Aim;
    vector3 Target;
    vector3 Muzzle;

    // Need position of the target.
    Position = pObject->GetPainPoint();

    // Use average velocity to mortar players.
    if( (m_Kind == MORTAR) && (pObject->GetAttrBits() & ATTR_PLAYER) )
        Velocity = ((player_object*)pObject)->GetAverageVelocity();
    else
        Velocity = pObject->GetVelocity();

    // If logic hasn't happend to the target object yet, then predict another frame
    if (!(pObject->GetAttrBits() & object::ATTR_LOGIC_APPLIED))
        Position += Velocity * tgl.DeltaLogicTime ;
/*
    // Add some time randomness
    Position += Velocity * x_frand(-1.0f, 1.0f) * tgl.DeltaLogicTime ;
*/
    // Get the muzzle position.
    Muzzle = Center[ Attr[m_Kind].Center ];
    Muzzle.Rotate( m_WorldRot );
    Muzzle += m_WorldPos;

    // Ready... Aim...
    switch( m_Kind )
    {
    case AA:
    case SENTRY:
    case DEPLOYED:
        Success = CalculateLinearAimDirection( Position,
                                               Velocity,
                                               Muzzle,
                                               vector3(0,0,0),
                                               0.0f,
                                               Attr[m_Kind].Velocity,
                                               3000.0f,
                                               Aim,
                                               Target, Verbose );
        break;

    case MISSILE:
        break;

    case MORTAR:
        Success = CalculateArcingAimDirection( Position,
                                               Velocity,
                                               Muzzle,
                                               vector3(0,0,0),
                                               0.0f,
                                               Attr[m_Kind].Velocity,
                                               180000.0f,
                                               Aim,
                                               Target );
        break;

    case PLASMA:
        Success = CalculateLinearAimDirection( Position,
                                               Velocity,
                                               Muzzle,
                                               vector3(0,0,0),
                                               0.0f,
                                               Attr[m_Kind].Velocity,
                                               3000.0f,
                                               Aim,
                                               Target );
        break;

    case ELF:
        Success = CalculateLinearAimDirection( Position,
                                               Velocity,
                                               Muzzle,
                                               vector3(0,0,0),
                                               0.0f,
                                               Attr[m_Kind].Velocity,
                                               3000.0f,
                                               Aim,
                                               Target );
    }

    // Aim right at da fool!
    if( !Success )
    {
        Target = Position;  // TO DO - Use distance + velocity to guess at lead.
    }

    // Compensate for fire point.
    Position  = m_W2L.Transform( Target );
    Position -= Center[ Attr[m_Kind].Center ];
    Position.GetPitchYaw( Pitch, Yaw );

    Pitch = x_ModAngle2( Pitch );
    Yaw   = x_ModAngle ( Yaw   );

    // Tweak the aim point to make the ELF beam bend and look cool.
    if( m_Kind == ELF )
    {
        if( Pitch < -R_45 )  
            Pitch += R_10;
        else
            Pitch -= R_10;
    }
}

//==============================================================================

xbool turret::UpdateAim( f32 DeltaTime, radian Pitch, radian Yaw )
{
    s32    OnTarget = 0;
    radian DeltaP, DeltaY;
    radian TurnP,  TurnY;

    DeltaP = x_MinAngleDiff( Pitch, m_Pitch );
    DeltaY = x_MinAngleDiff( Yaw,   m_Yaw   );
    TurnP  = Attr[m_Kind].AimRPSPitch * DeltaTime;
    TurnY  = Attr[m_Kind].AimRPSYaw   * DeltaTime;

    if( x_abs( DeltaP ) < TurnP )
    {
        m_Pitch = Pitch;
        OnTarget++;
    }
    else
    {
        if( DeltaP > R_0 )  m_Pitch += TurnP;
        else                m_Pitch -= TurnP;
    }

    if( x_abs( DeltaY ) < TurnY )
    {
        m_Yaw = Yaw;
        OnTarget++;
    }
    else
    {
        if( DeltaY > R_0 )  m_Yaw += TurnY;
        else                m_Yaw -= TurnY;
    }

    // Special case.  Allow ELF to fire if aim is within 30 degrees.
    if( (m_Kind == ELF) && (m_State == STATE_AIMING) )
    {
        vector3 Aim   ( m_Pitch, m_Yaw );
        vector3 Target(   Pitch,   Yaw );
        if( v3_AngleBetween( Aim, Target ) < ELFFireCone )
            return( TRUE );
    }

    return( OnTarget == 2 );
}

//==============================================================================

xbool turret::ChangeBarrel( turret::kind Kind )
{
    ASSERT(   Kind < SENTRY );
    ASSERT( m_Kind < SENTRY );

    if( m_Health < m_EnableLevel )
        return( FALSE );

    if( m_Kind == ELF )
    {
        if( m_ELFing )
        {
            m_ELFing = FALSE;
            m_DirtyBits |= DIRTY_ELF; 
        }
        if( m_ELFAudioHandle != 0 )
        {
            audio_Stop( m_ELFAudioHandle );
            m_ELFAudioHandle = 0;
        }
    }

    m_Kind = Kind;
    m_PreChangeState = m_State;

    m_Barrel.SetShape( tgl.GameShapeManager.GetShapeByType( Attr[m_Kind].ShapeBarrel ) );

    EnterState( STATE_BARREL_CHANGING );
    return( TRUE );
}

//==============================================================================

void turret::Destroyed( object::id OriginID )
{
    if( m_Explode )
    {
        SpawnExplosion( Attr[m_Kind].PainType,
                        m_WorldBBox.GetCenter(),
                        vector3(0,1,0),
                        OriginID );
    }
    asset::Destroyed( OriginID );

    if( m_Kind == SENTRY )
    {
        m_Pitch = R_0;
        m_Yaw   = R_0;
        m_Shape.SetAnimByType( ANIM_TYPE_TURRET_DESTROYED );
    }
}

//==============================================================================

void turret::Enabled( object::id OriginID )
{
    asset::Enabled( OriginID );
    m_Barrel.SetTextureAnimFPS( 8.0f );
}

//==============================================================================

void turret::Disabled( object::id OriginID )
{
    asset::Disabled( OriginID );
    m_Barrel.SetTextureAnimFPS( 0.0f );
    m_Barrel.SetTextureFrame( 0 );

    if( m_ELFAudioHandle != 0 )
    {
        audio_Stop( m_ELFAudioHandle );
        m_ELFAudioHandle = 0;
    }
    m_ELFing = FALSE;
}

//==============================================================================

turret::kind turret::GetKind( void ) const 
{
    return( m_Kind );
}
    
//==============================================================================

f32 turret::GetSenseRadius( void ) const
{
    if( (m_PowerOn) && (!m_Disabled) )
        return( Attr[m_Kind].RadiusSense );
    else
        return( 0.0f );
}

//==============================================================================

xbool turret::UsesBarrel( void )
{
    return( (m_Kind < SENTRY) && (!m_MPB) );
}

//==============================================================================

void turret::UnbindOriginID( object::id OriginID )
{
    if( m_OriginID == OriginID )
        m_OriginID = obj_mgr::NullID;
}

//==============================================================================

void turret::UnbindTargetID( object::id TargetID )
{
    if( m_TargetID == TargetID )
        m_TargetID = obj_mgr::NullID;
}

//==============================================================================

vector3 turret::GetPainPoint( void ) const
{
    return( m_PainPoint );
}

//==============================================================================

xbool turret::CheckLOS( void )
{
    return( CheckLOS( ObjMgr.GetObjectFromID( m_TargetID ) ) );
}

//==============================================================================

xbool turret::CheckLOS( object* pObject )
{
    if( !pObject )
        return( FALSE );

    vector3 From = Center[ Attr[m_Kind].Center ];
    From.Rotate( m_WorldRot );
    From += m_WorldPos;

    vector3 To = pObject->GetBlendPainPoint();

    // Tail turrets are restricted to a single hemisphere of fire.
    if( m_Kind == TAIL )
    {
        vector3 Hemisphere( 0, 1, 1 );
        Hemisphere.Rotate( m_WorldRot );
        if( Hemisphere.Dot( To - From ) < 0.0f )
            return( FALSE );
    }

    collider Collider;
    Collider.RaySetup( NULL, 
                       From, 
                       To, 
                       pObject->GetObjectID().GetAsS32(), 
                       TRUE );
    ObjMgr.Collide( ATTR_SOLID_STATIC, Collider );

    return( !Collider.HasCollided() );
}

//==============================================================================

vector3 turret::GetBlendFirePos( s32 Index )
{
    vector3 Position;

    switch( m_Kind )
    {
        case AA:
            Position = m_Barrel.GetHotPointByIndexWorldPos( Index );
            break;

        case TAIL:
            Position = m_Shape.GetHotPointByIndexWorldPos( Index );
            break;

        case ELF:
        case MISSILE:
        case MORTAR:
        case PLASMA:
            Position = m_Barrel.GetHotPointByIndexWorldPos( 0 );
            break;

        case SENTRY:
        case DEPLOYED:
            Position = m_Shape.GetHotPointByIndexWorldPos( 0 );
            break;
    }

    return( Position );
}

//==============================================================================

void turret::Fire( f32 DeltaTime )
{   
    s32 ShotIndex = 1;

    if( m_Kind == DEPLOYED )
    {
        m_ShotCount++;

        if( m_ShotCount >= (ClampFire + ClampWait) )
            m_ShotCount = 0;

        if( m_ShotCount >= ClampFire )
            return;
    }

    matrix4 L2W;
    radian3 Rotation;
    vector3 Position;
    vector3 Direction( m_Pitch, m_Yaw );

    Direction.Rotate( m_WorldRot );
    Direction.GetPitchYaw( Rotation.Pitch, Rotation.Yaw );
    Rotation.Roll = R_0;    

    UpdateInstance();

    switch( m_Kind )
    {
        case AA:
        case TAIL:
            if( m_Reload > Attr[m_Kind].TimeReload )
                ShotIndex = 0;                
            Position = GetBlendFirePos( ShotIndex );
            break;

        case ELF:
        case MISSILE:
        case MORTAR:
        case PLASMA:
        case SENTRY:
        case DEPLOYED:
            Position = GetBlendFirePos( 0 );
            break;
    }

    L2W.Identity();
    L2W.SetRotation( Rotation );
    L2W.SetTranslation( Position );

    if( m_Kind != ELF )
        m_DirtyBits |= DIRTY_FIRE;

    if( (m_Kind < SENTRY) && (m_Reload > Attr[m_Kind].TimeReload) )
        m_Barrel.SetAnimByType( ANIM_TYPE_TURRET_FIRE );

    if( m_Reload > Attr[m_Kind].TimeReload )
        m_Reload = 0.0f;

    switch( m_Kind )
    {
        //----------------------------------------------------------------------
        case AA:
        case TAIL:
        {
	        generic_shot* pShot = (generic_shot*)ObjMgr.CreateObject( object::TYPE_GENERICSHOT );
            pShot->Initialize( L2W, m_TeamBits, generic_shot::AA_TURRET, m_ObjectID );
            pShot->SetShotIndex( ShotIndex );
            pShot->m_TargetID = m_TargetID;
            ObjMgr.AddObject( pShot, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
        break;

        //----------------------------------------------------------------------
        case ELF:
        {
            object* pObject = ObjMgr.GetObjectFromID( m_TargetID );
            if( !pObject )
                break;

            vector3 Spline[16];
            vector3 Impact;
            vector3 Target;

            Target = pObject->GetPainPoint();

            collider Collider ;
            CalcSplinePts( L2W, Target, Spline, 16 );
            if( !CheckSplineCollision( Collider, Spline, 16, Impact, m_TargetID.GetAsS32() ) )
            {
                // Spline path is clear.  Fire!
                if( !m_ELFing )
                {
                    m_DirtyBits |= DIRTY_ELF;
                    m_ELFAudioHandle = audio_Play( SFX_WEAPONS_REPAIR_USE, &m_WorldPos );
                }
                m_ELFing = TRUE;
                ObjMgr.InflictPain( pain::TURRET_ELF, 
                                    pObject, 
                                    Spline[15], 
                                    m_ObjectID, 
                                    m_TargetID, 
                                    TRUE,
                                    DeltaTime );
            }
            else
            {
                // Spline path is obstructed.  Stop firing!
                if( m_ELFing )
                {
                    m_DirtyBits |= DIRTY_ELF;
                    if( m_ELFAudioHandle != 0 )
                    {
                        audio_Stop( m_ELFAudioHandle );
                        m_ELFAudioHandle = 0;
                    }
                }
                m_ELFing = FALSE;
            }
        }
        break;

        //----------------------------------------------------------------------
        case MISSILE:
        {
	        missile* pMissile = (missile*)ObjMgr.CreateObject( object::TYPE_MISSILE );
            pMissile->Initialize( L2W, m_TeamBits, m_ObjectID, m_TargetID );
            ObjMgr.AddObject( pMissile, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
        break;

        //----------------------------------------------------------------------
        case PLASMA:
        {
	        generic_shot* pShot = (generic_shot*)ObjMgr.CreateObject( object::TYPE_GENERICSHOT );
            pShot->Initialize( L2W, m_TeamBits, generic_shot::PLASMA_TURRET, m_ObjectID );
            ObjMgr.AddObject( pShot, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
        break;

        //----------------------------------------------------------------------
        case SENTRY:
        {
	        blaster* pBlast = (blaster*)ObjMgr.CreateObject( object::TYPE_BLASTER );
            pBlast->Initialize( L2W, m_TeamBits, m_ObjectID );
            ObjMgr.AddObject( pBlast, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
        break;

        //----------------------------------------------------------------------
        case DEPLOYED:
        {
	        generic_shot* pShot = (generic_shot*)ObjMgr.CreateObject( object::TYPE_GENERICSHOT );
            pShot->Initialize( L2W, m_TeamBits, generic_shot::CLAMP_TURRET, m_ObjectID, m_ObjectID );
            pShot->SetScoreID( m_OriginID );
            ObjMgr.AddObject( pShot, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
        break;

        //----------------------------------------------------------------------
        case MORTAR:
        {
            mortar* pMortar = (mortar*)ObjMgr.CreateObject( object::TYPE_MORTAR );
            pMortar->Initialize( Position,
                                 Direction,
                                 m_ObjectID,
                                 m_ObjectID,
                                 m_TeamBits,
                                 1 );
            ObjMgr.AddObject( pMortar, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
        break;
    }
}

//==============================================================================

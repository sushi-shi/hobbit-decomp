//==============================================================================
//
//  VehiclePad.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "VehiclePad.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "GameMgr\GameMgr.hpp"
#include "AudioMgr/Audio.hpp"
#include "Objects\Projectiles\Bubble.hpp"
#include "Asset.hpp"

#include "Objects\Vehicles\GravCycle.hpp"
#include "Objects\Vehicles\Shrike.hpp"
#include "Objects\Vehicles\Transport.hpp"
#include "Objects\Vehicles\Bomber.hpp"

#include <Support/StringMgr/StringMgr.hpp>
#include "AADS/Data/UI/ui_strings.h"

//==============================================================================
//  DEFINES
//==============================================================================
/*
static f32  s_BeamWidth = 0.15f;
static f32  s_BeamRand  = 0.5f;
static f32  s_GlowSize  = 5.0f;

#define     NUM_ARM_SEGS        8
#define     BOLT_RANGE          ( x_frand( -s_BeamRand, s_BeamRand ) )
*/
//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   VehiclePadCreator;

obj_type_info   VehiclePadTypeInfo( object::TYPE_VPAD, 
                                    "VehiclePad", 
                                    VehiclePadCreator, 
                                    object::ATTR_SOLID_DYNAMIC |
									object::ATTR_VPAD |
                                    object::ATTR_UNBIND_TARGET );
/*
static s32      s_GlowBeam = -1;
static s32      s_GlowCore = -1;
*/
//==============================================================================

static vector3 vpad_BubbleScale[6] =
{
    vector3(  3.5f,  3.5f,  5.0f ),
    vector3( 12.0f,  8.0f, 15.0f ),
    vector3( 18.0f, 12.0f, 20.0f ),
    vector3(  8.0f,  7.0f, 10.0f ),
    vector3( 10.0f, 10.0f, 15.0f ),
    vector3( 15.0f, 15.0f, 18.0f ),
};

static f32 vpad_VOffset[6] = { 4, 6, 6, 6, 6, 6 };

//==============================================================================
//  PROTOTYPES
//==============================================================================

xbool IntersectsOBB( const bbox& BBox, const vector3* pOBBVerts );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* VehiclePadCreator( void )
{
    return( (object*)(new vehicle_pad) );
}

//==============================================================================

vehicle_pad::vehicle_pad( void )
{
    m_Handle = 0;
}

//==============================================================================

object::update_code vehicle_pad::OnAdvanceLogic( f32 DeltaTime )
{    
    m_Shape.Advance( DeltaTime );
    m_Timer += DeltaTime;

    // Update team bits based on the state of the switch circuit.
    if( m_Switch != -1 )
        m_TeamBits = GameMgr.GetSwitchBits( m_Switch );

    if( m_State == BUSY )
    {
        // Shall we give a little boost to clear out players?
        if( m_Timer < 3.0f )
        {
            // TO DO: Use dedicated pain for this.
            ObjMgr.InflictPain( pain::MISC_FLIP_FLOP_FORCE, 
                                m_WorldPos, 
                                m_ObjectID,
                                obj_mgr::NullID,
                                (m_Timer / 3.0f) );
        }

        // Time to pop the vehicle in?
        if( (tgl.ServerPresent) && 
            (m_Timer > 3.0f) && 
            ((m_Timer - DeltaTime) <= 3.0f) )
        {
            CreateVehicle();
        }

        // Time to become 'un'busy?
        if( m_Timer > 6.0f )
        {
            m_State = IDLE;
            m_Timer = 0.0f;
        }
    }

    // Been at "alert" for 15 seconds?  Stand down.
    if( (m_State == IDLE) && (m_Handle != 0) && (m_Timer > 15.0f) )
    {
        audio_Stop( m_Handle );
        m_Handle = 0;
    }

    return( NO_UPDATE );
}

//==============================================================================

void vehicle_pad::Initialize( const vector3& WorldPos,
                              const radian3& WorldRot,
                                    s32      Switch,
                              const xwchar*  pLabel )
{   
    // Set position and bbox.
    m_WorldPos = WorldPos;
    m_WorldRot = WorldRot;

    // embed the vehicle pad inside the building
    m_WorldPos -= vector3( 0, 2.3f, 0 );

    m_Shape    .SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_VEHICLE_PAD ) );
    m_Collision.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_VEHICLE_PAD_COLL ) );

    ASSERT( m_Shape.GetShape() );
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );
    m_Shape.SetAnimByType( ANIM_TYPE_INVENT_IDLE );
    m_Shape.SetTextureAnimFPS( 8.0f );

    ASSERT( m_Collision.GetShape() );
    m_Collision.SetPos( m_WorldPos );
    m_Collision.SetRot( m_WorldRot );

    // Set final world bbox.
    m_WorldBBox  = m_Shape.GetWorldBounds();
    m_WorldBBox += m_Collision.GetWorldBounds() ;

    // Set the label.
    if( pLabel )
        x_wstrncpy( m_Label, pLabel, 31 );
    else
    {
        x_wstrcpy( m_Label, StringMgr( "ui", IDS_VEHICLE_PAD ) );
    }
    m_Label[31] = '\0';

    // Initialize state.
    m_State = IDLE;
    m_Timer = 0.0f;

    m_Switch = Switch;
}

//==============================================================================

void vehicle_pad::DebugRender( void )
{
    if( g_DebugAsset.ShowBBox )
    {
        vector3 P = m_WorldPos;
        P.Y += 8.0f;

        draw_BBox( m_WorldBBox, XCOLOR_YELLOW );
        draw_Point( m_WorldPos, XCOLOR_BLUE );
        draw_Point( P, XCOLOR_RED );
        draw_Sphere( P, 12.0f, XCOLOR_RED );
    }

    if( g_DebugAsset.ShowCollision )    
        m_Collision.DrawCollisionModel();

    if( IN_RANGE( TYPE_VEHICLE_WILDCAT, m_VehicleType, TYPE_VEHICLE_HAVOC ) )
    {
        s32     Index    = (s32)(m_VehicleType - TYPE_VEHICLE_WILDCAT);
        vector3 Position = m_WorldPos;
        Position.Y += vpad_VOffset[ Index ];

        matrix4 L2W;
        vector3 Vertex[8] = 
        {
            vector3(  0.5f, 0.5f, 0.5f ),
            vector3(  0.5f, 0.5f,-0.5f ),
            vector3( -0.5f, 0.5f,-0.5f ),
            vector3( -0.5f, 0.5f, 0.5f ),
            vector3(  0.5f,-0.5f, 0.5f ),
            vector3(  0.5f,-0.5f,-0.5f ),
            vector3( -0.5f,-0.5f,-0.5f ),
            vector3( -0.5f,-0.5f, 0.5f ),
        };

        L2W.Setup( vpad_BubbleScale[ Index ] * 0.75f, m_WorldRot, Position );
/*
        L2W.Identity();
        L2W.Scale( vpad_BubbleScale[ Index ] * 0.75f );
        L2W.Rotate( m_WorldRot );
        L2W.Translate( Position );
*/
        L2W.Transform( Vertex, Vertex, 8 );

        draw_Line( Vertex[0], Vertex[1], XCOLOR_BLUE );
        draw_Line( Vertex[1], Vertex[2], XCOLOR_BLUE );
        draw_Line( Vertex[2], Vertex[3], XCOLOR_BLUE );
        draw_Line( Vertex[3], Vertex[0], XCOLOR_BLUE );

        draw_Line( Vertex[4], Vertex[5], XCOLOR_BLUE );
        draw_Line( Vertex[5], Vertex[6], XCOLOR_BLUE );
        draw_Line( Vertex[6], Vertex[7], XCOLOR_BLUE );
        draw_Line( Vertex[7], Vertex[4], XCOLOR_BLUE );

        draw_Line( Vertex[0], Vertex[4], XCOLOR_BLUE );
        draw_Line( Vertex[1], Vertex[5], XCOLOR_BLUE );
        draw_Line( Vertex[2], Vertex[6], XCOLOR_BLUE );
        draw_Line( Vertex[3], Vertex[7], XCOLOR_BLUE );

        draw_Point( Position, XCOLOR_WHITE );
    }
}

//==============================================================================

void vehicle_pad::Render( void )
{
    s32 Visible = IsVisible( m_WorldBBox );
    if( !Visible )
        return;

    // Setup draw flags.
    u32 ShapeDrawFlags = shape::DRAW_FLAG_FOG;
    if( Visible == view::VISIBLE_PARTIAL )
        ShapeDrawFlags |= shape::DRAW_FLAG_CLIP;

    // Lookup damage texture and clut.
    texture* DamageTexture;
    s32      DamageClut;
    tgl.Damage.GetTexture( 1.0f, DamageTexture, DamageClut );

    // Setup fog color.
    m_Shape.SetFogColor( tgl.Fog.ComputeFog( m_WorldPos ) );

    // Render the shape.
    m_Shape.Draw( ShapeDrawFlags, DamageTexture, DamageClut );

    if( g_DebugAsset.DebugRender )
        DebugRender();
}

//==============================================================================

object::update_code vehicle_pad::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    u32 Bits;

    BitStream.ReadU32( Bits, 2 );

    if( Bits & 0x01 )
    {
        Alert();
    }

    if( Bits & 0x02 )
    {
        BeginSequence();
    }

    OnAdvanceLogic( SecInPast );

    return( NO_UPDATE );
}

//==============================================================================

void vehicle_pad::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    if( DirtyBits == 0xFFFFFFFF )
        DirtyBits =  0x00000000;

    BitStream.WriteU32( DirtyBits, 2 );
    DirtyBits = 0x00;
}

//==============================================================================

void vehicle_pad::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;
    m_Collision.ApplyCollision( Collider );
}

//==============================================================================

void vehicle_pad::UnbindTargetID( object::id TargetID )
{
    if( m_PlayerID == TargetID )
        m_PlayerID = obj_mgr::NullID;
}

//==============================================================================

void vehicle_pad::OnRemove( void )
{
    if( m_Handle != 0 )
    {
        audio_Stop( m_Handle );
        m_Handle = 0;
    }
}

//==============================================================================

const xwchar* vehicle_pad::GetLabel( void ) const
{
    return( m_Label );
}

//==============================================================================

void vehicle_pad::Alert( void )
{
    if( (m_Handle == 0) && (m_State == IDLE) )
    {
        m_Timer      = 0.0f;
        m_DirtyBits |= 0x01;

        // Queue up the streaming sound.
        m_Handle = audio_Play( SFX_POWER_VEHICLE_APPEAR, &m_WorldPos, AUDFLAG_DEFER_PLAY );
    }
}

//==============================================================================

void vehicle_pad::BeginSequence( void )
{
    if( tgl.ServerPresent )
    {
        m_State      = BUSY;
        m_Timer      = 0.0f;
        m_DirtyBits |= 0x02;

        // Start the effect.
        s32 Index = (s32)(m_VehicleType - TYPE_VEHICLE_WILDCAT);
        vector3  Position = m_WorldPos;
        radian3  Rotation = m_WorldRot;
        Position.Y  += vpad_VOffset[Index];
        Rotation.Yaw = x_ModAngle2( Rotation.Yaw + R_180 );
        /*
        CreateVehicleSpawnEffect( Position,
                                  Rotation,
                                  vpad_BubbleScale[Index] );
        */
    }

    // Start the animation.
    m_Shape.SetAnimByType( ANIM_TYPE_INVENT_ACTIVATE );

    // Set the streaming sound in motion.
    if( m_Handle == 0 )
    {
        m_Handle = audio_Play( SFX_POWER_VEHICLE_APPEAR, 
                               &m_WorldPos, 
                               AUDFLAG_DEFER_PLAY );
    }
    audio_SetPitch( m_Handle, 1.0f );
    m_Handle = 0;
}

//==============================================================================

vehicle_pad::vpad_code vehicle_pad::CreateVehicle( object::type VehicleType, object::id PlayerID )
{
    s32 Max, Count;

    // See if vpad is busy.
    if( m_State == BUSY )
    {
        return( VPAD_BUSY );
    }

    // See if there is a vehicle available in the pool.
    GameMgr.GetDeployStats( VehicleType, m_TeamBits, Count, Max );
    if( Count >= Max )
    {
        return( VPAD_VEHICLE_NOT_AVAILABLE );
    }

    // See if vpad is clear of other vehicles.
    vector3 P = m_WorldPos;
    P.Y += 8.0f;
    ObjMgr.Select( ATTR_VEHICLE, P, 12.0f );
    object* pObject = ObjMgr.GetNextSelected();
    ObjMgr.ClearSelection();
    if( pObject )
    {
        return( VPAD_BLOCKED );
    }

    // If we are here, then its all clear to create the vehicle.
    m_VehicleType = VehicleType;
    m_PlayerID    = PlayerID;
    BeginSequence();

    return( VPAD_OK );
}

//==============================================================================

void vehicle_pad::CreateVehicle( void )
{
    s32     Index    = (s32)(m_VehicleType - TYPE_VEHICLE_WILDCAT);
    radian  Yaw      = x_ModAngle2( m_WorldRot.Yaw + R_180 );
    vector3 Position = m_WorldPos;

    Position.Y += vpad_VOffset[ Index ];

    //
    // Nuke anything which is in the oriented box around the new vehicle.
    //
    {
        object* pObject;
        matrix4 L2W;
        vector3 Vertex[8] = 
        {
            vector3(  0.5f, 0.5f, 0.5f ),
            vector3(  0.5f, 0.5f,-0.5f ),
            vector3( -0.5f, 0.5f,-0.5f ),
            vector3( -0.5f, 0.5f, 0.5f ),
            vector3(  0.5f,-0.5f, 0.5f ),
            vector3(  0.5f,-0.5f,-0.5f ),
            vector3( -0.5f,-0.5f,-0.5f ),
            vector3( -0.5f,-0.5f, 0.5f ),
        };

        L2W.Setup( vpad_BubbleScale[ Index ] * 0.75f, m_WorldRot, Position );
        L2W.Transform( Vertex, Vertex, 8 );

        ObjMgr.Select( object::ATTR_DAMAGEABLE, m_WorldBBox );
        while( (pObject = ObjMgr.GetNextSelected()) )
        {
            if( IntersectsOBB( pObject->GetBBox(), Vertex ) )
            {
                ObjMgr.InflictPain( pain::MISC_VEHICLE_SPAWN,
                                    pObject,
                                    m_WorldPos,
                                    m_ObjectID,
                                    pObject->GetObjectID() );
            }           
        }
        ObjMgr.ClearSelection();
    }

    //
    // Now, let's make a vehicle!
    //

    gnd_effect* pVehicle = (gnd_effect*)ObjMgr.CreateObject( m_VehicleType );
    pVehicle->Initialize( Position, Yaw, m_TeamBits );
    ObjMgr.AddObject( pVehicle, obj_mgr::AVAILABLE_SERVER_SLOT );

    //
    // Player, meet vehicle.  Vehicle, meet player.
    // 
    if( m_PlayerID.Slot >= 0 )
    {
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_PlayerID );
        if( pPlayer )
        {
            pPlayer->SetLastVehiclePurchased( pVehicle->GetObjectID() );
            pVehicle->SetOwner( m_PlayerID );
            pVehicle->SetTeamBits( pPlayer->GetTeamBits() );
        }
    }
}

//==============================================================================

void vehicle_pad::PowerOff( void )
{
    m_Shape.SetTextureAnimFPS( 0.0f );
    m_Shape.SetTextureFrame( 0 );
}

//==============================================================================

void vehicle_pad::PowerOn( void )
{
    m_Shape.SetTextureAnimFPS( 8.0f );
}

//==============================================================================

void vehicle_pad::RenderSpawnFX( void )
{
/*
    s32     i, j;
    vector3 SpawnPoint;
    f32     Angle;

    SpawnPoint = m_Shape.GetHotPointWorldPos( HOT_POINT_TYPE_VEHICLE_STATION_SPAWN );

    // render the spawn fx only if spinning
    if ( m_State != BUSY )
        return;

    // initial load if necessary
    if ( s_GlowBeam == -1 )
    {
	    s_GlowBeam = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName("[A]elf_beam") ;
        s_GlowCore = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName("[A]HotCore") ;
    }

    // draw glowy center
    draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_SetClamping( FALSE );
    gsreg_End();
#endif
    
    // Activate the texture
    draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(s_GlowCore).GetXBitmap() );

    Angle = m_Timer * R_45;
    // Render the core
    draw_SpriteUV( SpawnPoint, vector2(s_GlowSize,s_GlowSize), vector2(0,0),vector2(1,1), XCOLOR_WHITE, Angle );
    draw_SpriteUV( SpawnPoint, vector2(s_GlowSize,s_GlowSize), vector2(0,0),vector2(1,1), XCOLOR_WHITE, -Angle );

    draw_End();

    // prepare to render beams
    draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_SetClamping( FALSE );
    gsreg_End();
#endif
    
    // Activate the texture
    draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(s_GlowBeam).GetXBitmap() );

    // find all the hotpoints
    for ( i = 0; i < m_Shape.GetModel()->GetHotPointCount(); i++ )
    {
        hot_point* pHotPoint = m_Shape.GetHotPointByIndex(i);

        // if this is one, draw the beam
        if ( pHotPoint->GetType() == HOT_POINT_TYPE_INVENT_RAY )
        {
            vector3 Segs[ NUM_ARM_SEGS ];
            vector3 Tmp = m_Shape.GetHotPointByIndexWorldPos(i);
            vector3 Delta = ( SpawnPoint - Tmp ) / NUM_ARM_SEGS;

            // calculate the points (straight line, but perturbed for effect)
            for ( j = 0; j < NUM_ARM_SEGS; j++ )
            {
                Segs[j] = Tmp + vector3( BOLT_RANGE, BOLT_RANGE, BOLT_RANGE );
                Tmp+=Delta;
            }

            // it remains only to draw it
            draw_OrientedStrand( Segs, NUM_ARM_SEGS, vector2(0,0), vector2(1,1), XCOLOR_WHITE, XCOLOR_WHITE, s_BeamWidth );
        }
    }

    // all done with bolts, de-initialize
    draw_End();

#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif
*/
}

//==============================================================================

//==============================================================================
//
//  Bubble.cpp
//
//==============================================================================

//==============================================================================
//  DEFINITIONS
//==============================================================================


//==============================================================================
//  INCLUDES
//==============================================================================

#include "Bubble.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Vehicles\Vehicle.hpp"

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BubbleCreator;

obj_type_info   BubbleTypeInfo( object::TYPE_BUBBLE, 
                                "Bubble", 
                                BubbleCreator,
                                0 );

//==============================================================================

static f32  ShieldTime = 1.0f;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* BubbleCreator( void )
{
    return( (object*)(new bubble) );
}

//==============================================================================

object::update_code bubble::OnAdvanceLogic( f32 DeltaTime )
{    
    xbool NeedUpdate = FALSE;

    // Advance the timer.
    m_Timer -= DeltaTime;
    if( m_Timer <= 0.0f )
        return( DESTROY );

    // Ready?
    if( !m_Ready )
    {
        // Try to get ready.
        Initialize( m_AnchorID, m_Intensity );

        if( !m_Ready )
            return( NO_UPDATE );
        else
            NeedUpdate = TRUE;
    }

    object* pObject = ObjMgr.GetObjectFromID( m_AnchorID );
    if( !pObject )
        return( DESTROY );
    if( m_AnchorID.Seq == -1 )
        m_AnchorID.Seq = pObject->GetObjectID().Seq;

    // Follow mobile parent object if any.
    if( m_Mobile )
    {
        // Update position and bbox.  Use conservative bbox.

        m_WorldPos = pObject->GetBlendPos();

        vector3 Size;
        m_WorldBBox = pObject->GetBBox();
        Size = m_WorldBBox.GetSize();
        m_WorldBBox.Min -= Size;
        m_WorldBBox.Max += Size;

        // Make sure ObjMgr updates this object.
        NeedUpdate = TRUE;
    }

    if( NeedUpdate )
        return( UPDATE );
    else
        return( NO_UPDATE );
}

//==============================================================================

void bubble::Initialize( const object::id  AnchorID,
                               f32         Intensity )
{   
    // Record vital values.
    m_Intensity = Intensity;
    m_AnchorID  = AnchorID;
    m_Timer     = ShieldTime;
    m_Mobile    = FALSE;

    // Attempt to acquire the anchor object.
    object* pObject = ObjMgr.GetObjectFromID( AnchorID );
    if( !pObject )
    {
        m_Ready = FALSE;
        return;
    }

    // Get a bbox for the object.
    bbox ObjBBox = pObject->GetBBox();

    switch( pObject->GetType() )
    {
    case TYPE_STATION_FIXED:
    case TYPE_STATION_VEHICLE:
        m_LocalOffset( 0, 1, 0 );
        m_LocalScale ( 6, 6, 6 );
        break;
    case TYPE_STATION_DEPLOYED:
        m_LocalOffset( 0.0f, 1.0f, 0.0f );
        m_LocalScale ( 2.5f, 2.5f, 2.5f );
        break;
    case TYPE_TURRET_FIXED:
        m_LocalOffset( 0.00f, 1.25f, 0.75f );
        m_LocalScale ( 4.00f, 6.00f, 5.00f );
        break;
    case TYPE_TURRET_SENTRY:
        m_LocalOffset( 0.00f,-0.50f, 0.00f );
        m_LocalScale ( 1.50f, 1.50f, 1.50f );
        break;
    case TYPE_TURRET_CLAMP:
        m_LocalOffset( 0.00f, 0.75f, 0.00f );
        m_LocalScale ( 1.25f, 2.00f, 1.25f );
        break;
    case TYPE_SENSOR_REMOTE:
        m_LocalOffset( 0.0f, 0.3f, 0.0f );
        m_LocalScale ( 1.5f, 1.5f, 1.5f );
        break;                                  
    case TYPE_SENSOR_MEDIUM:
        m_LocalOffset( 0.0f, 2.5f, 0.0f );
        m_LocalScale ( 7.0f, 8.0f, 7.0f );
        break;
    case TYPE_SENSOR_LARGE:
        m_LocalOffset( 0.0f, 3.5f, 0.0f );
        m_LocalScale ( 7.5f,12.0f, 7.5f );
        break;
    case TYPE_GENERATOR:
        m_LocalOffset( 0.0f,  1.5f, -1.75f );
        m_LocalScale ( 5.0f,  5.0f,  5.00f );
        break;
    case TYPE_VEHICLE_WILDCAT:     
        m_LocalOffset( 0.0f, 0.0f, 0.0f );
        m_LocalScale ( 3.5f, 3.5f, 5.0f );
        m_Mobile = TRUE;
        break;
    case TYPE_VEHICLE_SHRIKE:      
        m_LocalOffset( 0, 0,  0 );
        m_LocalScale ( 8, 7, 10 );
        m_Mobile = TRUE;
        break;
    case TYPE_VEHICLE_THUNDERSWORD:
        m_LocalOffset(  0,  0,  0 );
        m_LocalScale ( 10, 10, 15 );
        m_Mobile = TRUE;
        break;
    case TYPE_VEHICLE_HAVOC:       
        m_LocalOffset(  0,  0,  0 );
        m_LocalScale ( 15, 15, 18 );
        m_Mobile = TRUE;
        break;
    case TYPE_PLAYER:
    case TYPE_BOT:
    default:
        m_LocalOffset = ObjBBox.GetCenter() - pObject->GetPosition();
        m_LocalScale  = ObjBBox.GetSize() * 1.75f;
        m_Mobile = TRUE;
        break;   
    }

    // Setup the shape instance.
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_MISC_SHIELD ) );

    // Establish position and bounding box.
    if( m_Mobile )
    {
        // Use a conservative bbox for mobile shields.
        // The shape itself will be positioned in the render function.
        m_WorldPos = pObject->GetBlendPos();
        vector3 Size = ObjBBox.GetSize();
        m_WorldBBox = ObjBBox;
        m_WorldBBox.Min -= Size;
        m_WorldBBox.Max += Size;
    }
    else
    {
        // Since this bubble is not mobile, set everything up here and now.
        m_WorldPos = pObject->GetBlendPos();
        m_WorldRot = pObject->GetRotation();
        vector3 Offset = m_LocalOffset;
        Offset.Rotate( m_WorldRot );
        m_Shape.SetRot( m_WorldRot );
        m_Shape.SetPos( m_WorldPos + Offset );
        m_WorldBBox = m_Shape.GetWorldBounds();
    }

    // Ready!
    m_Ready = TRUE;
}

//==============================================================================

object::update_code bubble::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    BitStream.ReadRangedF32( m_Timer,     8, 0.0f, 1.0f );
    BitStream.ReadRangedF32( m_Intensity, 8, 0.0f, 1.0f );

    m_Timer -= SecInPast;

    return( NO_UPDATE );
}

//==============================================================================

void bubble::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    BitStream.WriteRangedF32( m_Timer,     8, 0.0f, 1.0f );
    BitStream.WriteRangedF32( m_Intensity, 8, 0.0f, 1.0f );

    DirtyBits = 0x00;
}

//==============================================================================

void bubble::OnAcceptInit( const bitstream& BitStream )
{
    object::id  AnchorID;
    f32         Intensity;

    BitStream.ReadObjectID ( AnchorID );
    BitStream.ReadRangedF32( Intensity, 8, 0.0f, 1.0f );

    Initialize( AnchorID, Intensity );
}

//==============================================================================

void bubble::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteObjectID ( m_AnchorID );
    BitStream.WriteRangedF32( m_Intensity, 8, 0.0f, 1.0f );
}

//==============================================================================

void bubble::DebugRender( void )
{
    draw_Point( m_WorldPos, XCOLOR_YELLOW );

    bbox BBox( -m_LocalScale / 2.0f, m_LocalScale / 2.0f );
    BBox.Translate( m_WorldPos );
    BBox.Translate( m_LocalOffset );
    draw_BBox( BBox, XCOLOR_WHITE );
}

//==============================================================================

void bubble::Render( void )
{
    // Ready?
    if( !m_Ready )
        return;

    // Any time left?
    if( m_Timer <= 0.0f )
        return;

    // Visible?
    s32 Visible = IsVisible( m_WorldBBox );
    if( !Visible )
        return;

    object* pObject = ObjMgr.GetObjectFromID( m_AnchorID );
    if( !pObject )
        return;
    if( m_AnchorID.Seq == -1 )
        m_AnchorID.Seq = pObject->GetObjectID().Seq;

    // Follow mobile parent object if any.
    if( m_Mobile )
    {
        // Update shape position and rotation.
        if( pObject->GetAttrBits() & ATTR_PLAYER )
        {
            player_object* pPlayer = (player_object*)pObject;

            m_WorldRot = pPlayer->GetGroundRot();

            bbox ObjBBox  = pObject->GetBBox();

            m_LocalOffset = ObjBBox.GetCenter() - pObject->GetPosition();
            m_LocalScale  = ObjBBox.GetSize() * 1.75f;

            m_Shape.SetRot( m_WorldRot );
            m_Shape.SetPos( m_WorldPos + m_LocalOffset );
        }
        else
        {
            m_WorldRot = pObject->GetBlendRot();    // Blended rotation!

            vector3 Offset = m_LocalOffset;
            Offset.Rotate( m_WorldRot );

            m_Shape.SetRot( m_WorldRot );
            m_Shape.SetPos( m_WorldPos + Offset );
        }
    }

    // Setup flags.
    m_DrawFlags = shape::DRAW_FLAG_FOG | shape::DRAW_FLAG_PRIME_Z_BUFFER;
    if( Visible == view::VISIBLE_PARTIAL )
        m_DrawFlags |= shape::DRAW_FLAG_CLIP;

    // Setup fog color.
    m_Shape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
    
    xcolor Color( 191, 191, 255, (u8)(127 * m_Intensity * (m_Timer / ShieldTime) ) );
    m_Shape.SetColor( Color );

    m_Shape.SetScale( m_LocalScale * (1.25f - ((m_Timer / ShieldTime) * 0.25f) ) );
    
    m_Shape.Draw( m_DrawFlags |
                  shape::DRAW_FLAG_REF_RANDOM | 
                  shape::DRAW_FLAG_TURN_OFF_LIGHTING );
}

//==============================================================================

object::id bubble::GetAnchorID( void )
{
    return( m_AnchorID );
}

//==============================================================================

void bubble::RestartTimer( void )
{
    m_Timer = ShieldTime;
    m_DirtyBits |= 0x01;
}

//==============================================================================

void bubble::SetIntensity( f32 Intensity )
{
    m_Intensity = Intensity;
}

//==============================================================================

void bubble::SetDirection( const vector3& Direction )
{
    m_Direction = Direction;
}

//==============================================================================

void CreateShieldEffect( const object::id ObjectID, 
                               f32        Intensity )
{
    if( !tgl.ServerPresent )
        return;

    bubble* pBubble;

    // If there is already a shield bubble around the object, just restart its
    // timer.

    ObjMgr.StartTypeLoop( object::TYPE_BUBBLE );
    while( (pBubble = (bubble*)ObjMgr.GetNextInType()) )
    {
        if( pBubble->GetAnchorID() == ObjectID )
            break;
    }
    ObjMgr.EndTypeLoop();

    if( pBubble )
    {
        pBubble->RestartTimer();
        pBubble->SetIntensity( Intensity );
    }
    else
    {
        pBubble = (bubble*)ObjMgr.CreateObject( object::TYPE_BUBBLE );
        pBubble->Initialize( ObjectID, Intensity );
        ObjMgr.AddObject( pBubble, obj_mgr::AVAILABLE_SERVER_SLOT );
    }
}

//==============================================================================

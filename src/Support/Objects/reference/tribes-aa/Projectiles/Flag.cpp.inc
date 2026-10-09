//==============================================================================
//
//  Flag.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Flag.hpp"
#include "GameMgr\GameMgr.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS\Globals.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include <Support/PointLight/PointLight.hpp>
#include "netlib/bitstream.hpp"


//#define FLAG_TIMER

// Includes for special case flag wobbling (function needs vert format)
#ifdef TARGET_PC
#include <Support/Shape/Shaders/D3D_Shape.hpp>
#endif

#ifdef TARGET_PS2
#include <Support/Shape/MCode/PS2_Shape.hpp>
#endif

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   FlagCreator;

obj_type_info   FlagTypeInfo( object::TYPE_FLAG, 
                              "Flag", 
                              FlagCreator, 
                              object::ATTR_DAMAGEABLE |
                              object::ATTR_PERMEABLE );

static f32 FlagTimeOutAge = 30.0f;
static bbox FlagBBox( vector3( -1.00f, 0.2f, -1.00f ), 
                      vector3(  1.00f, 2.0f,  1.00f ) );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* FlagCreator( void )
{
    return( (object*)(new flag) );
}

//==============================================================================
//  SPECIAL CASE SHAPE ANIMATION FUNCTION - CURRENT TIME IS 0.26ms
//==============================================================================

static f32 s_Angle0=0 ;
static f32 s_Angle1=0 ;
static f32 s_Angle2=0 ;
static f32 s_Angle3=0 ;
static f32 s_Angle4=0 ;
static f32 s_Angle5=0 ;
static s32 s_Frame=0 ;

model *AnimateFlagShape( shape* FlagShape, s32 Frame, f32 DeltaTime )
{
    ASSERT(FlagShape) ;
    ASSERT(FlagShape->GetModelCount() == 3) ;

    // Get models
    model* SourceModel = FlagShape->GetModelByIndex(2) ;
    model* DestModel   = FlagShape->GetModelByIndex(Frame & 1) ;

    // Aready animated this frame?
    if (s_Frame == Frame)
        return DestModel ;
    
    // Flag animated this frame
    s_Frame = Frame ;

    // Advance angles
    s_Angle0 += DeltaTime * R_2 ;    // dir1 angle
    s_Angle1 += DeltaTime * R_3 ;    // dir2 angle
    f32 CosAngle0 = x_cos(s_Angle0) ;
    f32 CosAngle1 = x_cos(s_Angle1) ;
    s_Angle2 += DeltaTime * R_130 * CosAngle0 ;    // dist angle1
    s_Angle3 += DeltaTime * R_140 * CosAngle1 ;    // dist angle2
    s_Angle4 += DeltaTime * R_150 * CosAngle0 ;    // dist scale angle
    s_Angle5 += DeltaTime * R_160 * CosAngle1 ;    // angle scale angle

    // Update flag verts
    s32 TotalVerts = 0;

#ifdef FLAG_TIMER
    xtimer Timer ;
    Timer.Start() ;
#endif

    material& DestMat    = DestModel->GetMaterial(0) ;
    material& SourceMat  = SourceModel->GetMaterial(0) ;
    f32       DistScale  = (0.02f + (0.01f * x_cos(s_Angle4))) * FlagShape->GetModelByIndex(0)->GetPrecisionScale() ;
    f32       AngleScale = (5.0f + (0.5f * x_cos(s_Angle5))) / FlagShape->GetModelByIndex(0)->GetPrecisionScale() ;
    f32       AngleOffset, Dist, NormRot ;
    radian    S,C ;
    vector3   TempNorm ;  
    s32       i,j,v ;

    for (i = 0 ; i < SourceMat.GetDListCount() ; i++)
    {
        material_dlist& SourceDList = SourceMat.GetDList(i) ;
        material_dlist& DestDList   = DestMat.GetDList(i) ;

        for (j = 0 ; j < SourceDList.GetStripCount() ; j++)
        {
            dlist_strip& SourceStrip = SourceDList.GetStrip(j) ;
            dlist_strip& DestStrip   = DestDList.GetStrip(j) ;

            v = SourceStrip.GetVertCount() ;
            ASSERT(SourceStrip.GetVertCount() == DestStrip.GetVertCount()) ;

            #if defined(TARGET_PC)||defined(TARGET_LINUX)
                d3d_vert *SourceVert = (d3d_vert*) SourceStrip.GetPosList() ;
                d3d_vert *DestVert =   (d3d_vert*) DestStrip.GetPosList() ;
                while(v--)
                {
                    // Calc angle offset given position, and calc dist to move
                    AngleOffset = (SourceVert->vPos.X + SourceVert->vPos.Y) * AngleScale ;
                    Dist        = x_sin(s_Angle2 + AngleOffset) + x_sin(s_Angle3 + AngleOffset)  ; // range -2 to 2

                    // Move the vert in and out on the z axis
                    DestVert->vPos.Z = SourceVert->vPos.Z + (DistScale * Dist) ;
                    
                    // Rotate normal to give fudged lighting effect 
                    NormRot = R_20 * Dist ;
                    x_sincos( NormRot, S, C );
                    
                    // RotateX
                    TempNorm.X = SourceVert->vNormal.X ;
                    TempNorm.Y = (C * SourceVert->vNormal.Y) - (S * SourceVert->vNormal.Z) ;
                    TempNorm.Z = (C * SourceVert->vNormal.Z) + (S * SourceVert->vNormal.Y) ;

                    // RotateY
                    DestVert->vNormal.X = ((C * TempNorm.X) + (S * TempNorm.Z)) ;
                    DestVert->vNormal.Y = TempNorm.Y ;
                    DestVert->vNormal.Z = ((C * TempNorm.Z) - (S * TempNorm.X)) ;

                    // Next vert
                    DestVert++ ;
                    SourceVert++ ;
                    TotalVerts++ ;
                }
            #endif

            #ifdef TARGET_PS2
                ps2_pos    *SourcePos  = (ps2_pos*) SourceStrip.GetPosList() ;
                ps2_norm   *SourceNorm = (ps2_norm*)SourceStrip.GetNormalList() ;
                ps2_pos    *DestPos    = (ps2_pos*) DestStrip.GetPosList() ;
                ps2_norm   *DestNorm   = (ps2_norm*)DestStrip.GetNormalList() ;
                while(v--)
                {
                    // Calc angle offset given position, and calc dist to move
                    AngleOffset = (SourcePos->x + SourcePos->y) * AngleScale ;
                    Dist        = x_sin(s_Angle2 + AngleOffset) + x_sin(s_Angle3 + AngleOffset)  ; // range -2 to 2

                    // Move the vert in and out on the z axis
                    DestPos->z = SourcePos->z + (s16)(DistScale * Dist) ;
                    
                    // Rotate normal to give fudged lighting effect 
                    NormRot = R_20 * Dist ;
                    x_sincos( NormRot, S, C );
                    
                    // RotateX
                    TempNorm.X = (f32)SourceNorm->x ;
                    TempNorm.Y = (C * (f32)SourceNorm->y) - (S * (f32)SourceNorm->z) ;
                    TempNorm.Z = (C * (f32)SourceNorm->z) + (S * (f32)SourceNorm->y) ;

                    // RotateY
                    DestNorm->x = (s8) ((C * TempNorm.X) + (S * TempNorm.Z)) ;
                    DestNorm->y = (s8) TempNorm.Y ;
                    DestNorm->z = (s8) ((C * TempNorm.Z) - (S * TempNorm.X)) ;

                    // Next vert
                    DestPos++ ;
                    DestNorm++ ;
                    SourcePos++ ;
                    SourceNorm++ ;
                    TotalVerts++ ;
                }
            #endif
        }
    }

#ifdef FLAG_TIMER
    Timer.Stop() ;

    //x_printfxy(0,10, "FlagTime:%f Verts:%d", Timer.ReadMs(), TotalVerts) ;
#endif

    return DestModel ;
}

//==============================================================================
//  FLAG CLASS FUNCTIONS
//==============================================================================

void flag::Initialize( const vector3&   Position,
                             radian     Yaw,
                             u32        TeamBits )
{
    CommonInit( Position, Yaw, vector3(0,0,0), obj_mgr::NullID, TeamBits, 3 );

    m_TimeOut    = FALSE;
    m_TimeOutAge = FlagTimeOutAge;
    m_State      = STATE_SETTLED;
}

//==============================================================================

void flag::Initialize( const vector3&   Position,
                             radian     Yaw,
                       const vector3&   Velocity,
                             object::id OriginID,
                             u32        TeamBits,
                             s32        Value )
{
    CommonInit( Position, Yaw, Velocity, OriginID, TeamBits, Value );

    m_TimeOut    = TRUE;
    m_TimeOutAge = FlagTimeOutAge;
}

//==============================================================================

void flag::CommonInit( const vector3&   Position,
                             radian     Yaw,
                       const vector3&   Velocity,
                             object::id OriginID,
                             u32        TeamBits,
                             s32        Value )
{
    arcing::Initialize( Position, Velocity, OriginID, OriginID, TeamBits );

    m_WorldBBox( Position + FlagBBox.Min,
                 Position + FlagBBox.Max );
    m_Yaw   = Yaw;
    m_Value = Value;

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_MISC_FLAG ) );
    m_Shape.SetTextureAnimFPS( 0.0f );
    m_Shape.SetTextureFrame( (f32)m_TeamBits );
    ASSERT( m_Shape.GetShape() );

    if( m_TeamBits != 0x00 )
    {
        const xwchar* pName = GameMgr.GetTeamName( m_TeamBits );
        if( pName && pName[0] )
        {
            xwstring Flag( " Flag" );
            x_wstrcpy( m_Label, pName );
            x_wstrcat( m_Label, (const xwchar*)Flag );
        }
        else
        {
            xwstring Flag( "Flag" );
            x_wstrcpy( m_Label, pName );
        }

        m_AttrBits |= ATTR_LABELED;
    }
}

//==============================================================================

object::update_code flag::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    update_code Result;

    m_TimeOut = BitStream.ReadFlag();
    Result    = arcing::OnAcceptUpdate( BitStream, SecInPast );
    return( Result );
}

//==============================================================================

void flag::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    if( DirtyBits & 0x01 )
    {
        BitStream.WriteFlag( m_TimeOut );
        arcing::OnProvideUpdate( BitStream, DirtyBits, Priority );
    }
}

//==============================================================================

void flag::OnAcceptInit( const bitstream& BitStream )
{
    arcing::OnAcceptInit( BitStream );
    
    BitStream.ReadRangedF32( m_Yaw, 8, R_0, R_360 );
    BitStream.ReadTeamBits ( m_TeamBits );
    m_TimeOut = BitStream.ReadFlag();
    BitStream.ReadRangedS32( m_Value, 0, 3 );

    m_TimeOutAge = FlagTimeOutAge;
    m_WorldBBox( m_WorldPos + FlagBBox.Min,
                 m_WorldPos + FlagBBox.Max );

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_MISC_FLAG ) );
    m_Shape.SetTextureAnimFPS( 0.0f );
    m_Shape.SetTextureFrame( (f32)m_TeamBits );
    ASSERT( m_Shape.GetShape() );

    if( m_TeamBits != 0x00 )
    {
        const xwchar* pName = GameMgr.GetTeamName( m_TeamBits );
        if( pName && pName[0] )
        {
            xwstring Flag( " Flag" );
            x_wstrcpy( m_Label, pName );
            x_wstrcat( m_Label, (const xwchar*)Flag );
        }
        else
        {
            xwstring Flag( "Flag" );
            x_wstrcpy( m_Label, Flag );
        }

        m_AttrBits |= ATTR_LABELED;
    }
}

//==============================================================================

void flag::OnProvideInit( bitstream& BitStream )
{   
    m_Yaw = x_ModAngle( m_Yaw );

    arcing::OnProvideInit( BitStream );
    BitStream.WriteRangedF32( m_Yaw, 8, R_0, R_360 );
    BitStream.WriteTeamBits ( m_TeamBits );
    BitStream.WriteFlag     ( m_TimeOut );
    BitStream.WriteRangedS32( m_Value, 0, 3 );
}

//==============================================================================

void flag::OnAdd( void )
{
    arcing::OnAdd();

    xcolor Color[] = { XCOLOR_RED, XCOLOR_BLUE, XCOLOR_GREEN, XCOLOR_WHITE };

    vector3 Position = m_WorldPos;
    Position.Y += 1.0f;

    ASSERT( IN_RANGE( 0, m_Value, 3 ) );

    m_PointLight = ptlight_Create( Position, 2.0f, Color[m_Value] );
}

//==============================================================================

void flag::OnRemove( void )
{
    ptlight_Destroy( m_PointLight );
    arcing::OnRemove();
}

//==============================================================================

object::update_code flag::AdvanceLogic( f32 DeltaTime )
{
    object::update_code Result;
    if( (m_TimeOut) && (m_Age > m_TimeOutAge) )
    {
        pGameLogic->FlagTimedOut( m_ObjectID );
        return( DESTROY );
    }

    // Let the ancestor class do its thing.
    Result = arcing::AdvanceLogic( DeltaTime );

    // Update the position of the point light.
    {
        vector3 Position = m_WorldPos;
        Position.Y += 1.0f;
        ptlight_SetPosition( m_PointLight, Position );
        ptlight_SetRadius( m_PointLight, 3.0f + x_sin( m_Age + m_Yaw ) );
    }

    return( Result );
}

//==============================================================================

xbool flag::Impact( const collider::collision& Collision )
{   
    object* pObjectHit = (object*)Collision.pObjectHit;
    ASSERT( pObjectHit );

    if( pObjectHit->GetAttrBits() & object::ATTR_PLAYER )
    {
        player_object* pPlayer = (player_object*)pObjectHit;
        return( pGameLogic->FlagHitPlayer( m_ObjectID, pPlayer->GetObjectID() ) );
    }

    return( FALSE );
}

//==============================================================================

void flag::Render( void )
{
    // Setup shape ready for render pre
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( radian3( R_0, m_Yaw, R_0 ) );

    // Visible?
    if( !RenderPrep() )
        return;

    // Animate shape and draw
    m_Shape.SetModel( AnimateFlagShape( m_Shape.GetShape(), tgl.NRenders, tgl.DeltaLogicTime ) );
    m_Shape.Draw( m_ShapeDrawFlags );
}

//==============================================================================

s32 flag::GetValue( void ) const
{
    return( m_Value );
}

//==============================================================================

radian flag::GetYaw( void ) const
{
    return( m_Yaw );
}

//==============================================================================

f32 flag::GetPainRadius( void ) const
{
    return( 1.0f );
}

//==============================================================================

vector3 flag::GetPainPoint( void ) const
{
    vector3 Result = m_WorldPos;
    Result.Y += 1.0f;
    return( Result );
}

//==============================================================================

void flag::OnPain( const pain& Pain )
{
    // Don't bother with a flag on a stand.
    if( !m_TimeOut )
        return;

    if( Pain.LineOfSight )
    {
        f32     Mass;
        vector3 Point = m_WorldPos;
        Point.Y += 1.0f;

        if( GameMgr.GetGameType() == GAME_CTF )   Mass = 55.0f;
        else                                      Mass = 75.0f;

        vector3 ForceDir = Point - Pain.Center;
        ForceDir.NormalizeAndScale( Pain.Force / Mass );

        m_Velocity  += ForceDir;
        m_State      = STATE_FALLING;
        m_DirtyBits |= 0x01;
    }
}

//==============================================================================

const xwchar* flag::GetLabel( void ) const
{
    return( m_Label );
}

//==============================================================================

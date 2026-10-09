//==============================================================================
//
//  Terrain.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES    
//==============================================================================

#include "Terrain.hpp"

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   TerrainCreator;

obj_type_info   TerrainTypeInfo( object::TYPE_TERRAIN, 
                                 "Terrain", 
                                 TerrainCreator, 
                                 object::ATTR_SOLID_STATIC | 
                                 object::ATTR_PERMANENT | 
                                 object::ATTR_GLOBAL );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* TerrainCreator( void )
{
    return( (object*)(new terrain) );
}

//==============================================================================

void terrain::Initialize( const char* pFileName, const fog* pFog )
{
    m_Terrain.Load( pFileName );
    m_Terrain.SetFog( pFog );
}

//==============================================================================

terrain::~terrain( void )
{
    m_Terrain.Kill();
}

//==============================================================================

void terrain::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;
    m_Terrain.Collide( Collider );
}

//==============================================================================

void terrain::GetLighting( const vector3& Pos, xcolor& C, vector3& TerrPt )
{
    f32 Y;
    m_Terrain.GetLighting( Pos.Z, Pos.X, C, Y );
    TerrPt.X = Pos.X;
    TerrPt.Y =     Y;
    TerrPt.Z = Pos.Z;

    // Watch for underground
    if( Pos.Y < TerrPt.Y )
    {
        C = XCOLOR_BLACK;
        TerrPt.Y = -100000.0f;
    }
}

//==============================================================================

void terrain::GetSphereFaceKeys ( const vector3&      EyePos,
                                  const vector3&      Pos,
                                        f32           Radius,
                                        s32&          NKeys,
                                        fcache_key*   pKey )
{
    (void)EyePos;
    m_Terrain.GetSphereFaceKeys( Pos, Radius, m_ObjectID.Slot, NKeys, pKey );
}
                
//==============================================================================

void terrain::GetPillFaceKeys ( const vector3&    StartPos,
                              const vector3&    EndPos,
                                    f32         Radius,
                                    s32&        NKeys,
                                    fcache_key* pKey )
{
    m_Terrain.GetPillFaceKeys( StartPos, EndPos, Radius, m_ObjectID.Slot, NKeys, pKey );
}

//==============================================================================

void terrain::ConstructFace   ( fcache_face& Face )
{
    m_Terrain.ConstructFace( Face );
}

//==============================================================================



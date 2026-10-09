//==============================================================================
//
//  Terrain.hpp
//
//==============================================================================

#ifndef TERRAIN_HPP
#define TERRAIN_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/Terrain/Terr.hpp>
#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Fog/Fog.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class terrain : public object
{

//------------------------------------------------------------------------------
//  Public Functions

public:

               ~terrain         ( void );
    void        Initialize      ( const char* pFileName, const fog* pFog );
    void        OnCollide       ( u32 AttrBits, collider& Collider );
    terr&       GetTerr         ( void ) { return m_Terrain; };
    void        GetLighting     ( const vector3& Pos, xcolor& C, vector3& TerrPt );

    void        GetSphereFaceKeys ( const vector3&      EyePos,
                                    const vector3&      Pos,
                                          f32           Radius,
                                          s32&          NKeys,
                                          fcache_key*   pKey );
                   
    void        GetPillFaceKeys ( const vector3&    StartPos,
                                  const vector3&    EndPos,
                                        f32         Radius,
                                        s32&        NKeys,
                                        fcache_key* pKey );
    
    void        ConstructFace   ( fcache_face& Face );

//------------------------------------------------------------------------------
//  Private Data

//protected:

    terr        m_Terrain;

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     TerrainCreator( void );

//==============================================================================
#endif // TERRAIN_HPP
//==============================================================================

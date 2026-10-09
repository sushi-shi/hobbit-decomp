//==============================================================================
//
//  PathGenerator.hpp
//
//==============================================================================
#ifndef PATHGENERATOR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "CellRef.hpp"
#include <Support/ObjectMgr/ObjectMgr.hpp>

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================

//==============================================================================
//  FUNCTIONS
//==============================================================================

class path_generator
{
public:
    static const s32 NUM_VERTS_HORIZ;
    static const f32 VERTEX_SPACING;
    static void GenerateTerrainVertices( obj_mgr& ObjMgr );
    static s32 GraphIndex( s32 X, s32 Z ) { return Z * NUM_VERTS_HORIZ + X; };
    static void Load( cell_attribute* GraphArray, obj_mgr& ObjMgr );
private:
    static void Save( const char* pFileName, const cell_attribute* GraphArray );
    static xstring GetFileName( obj_mgr& ObjMgr );
};

#endif //ndef PATHGENERATOR_HPP

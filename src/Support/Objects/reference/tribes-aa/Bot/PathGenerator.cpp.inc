//==============================================================================
//
//  PathGenerator.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/Entropy.hpp>
#include "PathGenerator.hpp"
#include "CellRef.hpp"
#include "Objects/Terrain/Terrain.hpp"
#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "BotLog.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

const           s32 path_generator::NUM_VERTS_HORIZ = 256;
static const    s32 NUM_VERTS = path_generator::NUM_VERTS_HORIZ
                    * path_generator::NUM_VERTS_HORIZ;
const           f32 path_generator::VERTEX_SPACING  = 8.0f;

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================

//==============================================================================
//  FUNCTIONS
//==============================================================================

void path_generator::GenerateTerrainVertices( obj_mgr& ObjMgr ) 
{
    ObjMgr.StartTypeLoop( object::TYPE_TERRAIN );
    terrain* Terrain = (terrain*)(ObjMgr.GetNextInType());
    ObjMgr.EndTypeLoop();
    ASSERT( Terrain );

    //----------------------------------------------------------------------
    // Create the graph vertices from the terrain vertices
    //----------------------------------------------------------------------
    cell_attribute* Graph = new cell_attribute[NUM_VERTS];

    for ( s32 X = 0; X < 256; ++X )
    {
        for ( s32 Z = 0; Z < 256; ++Z ) 
        {
            cell_attribute& CurCellAttribute = Graph[GraphIndex(X, Z)];

            //--------------------------------------------------------------
            // Check to see if we are in lava or water
            //--------------------------------------------------------------

            
            //--------------------------------------------------------------
            // Check to see if we are under a building
            //--------------------------------------------------------------
            // loop through the buildings
            CurCellAttribute.BLOCKED = 0;
            object *CurBuilding;

            const f32       WorldX = X * VERTEX_SPACING;
            const f32       WorldZ = Z * VERTEX_SPACING;
            //const f32       WorldY = Terrain.GetVertHeight( Z, X );
            //const vector3   WorldPos(WorldX, 0.0f, WorldZ );
            //const vector3   AboveWorldPos( WorldPos + vector3( 0, 100.0f, 0 ) );

            ObjMgr.StartTypeLoop( object::TYPE_BUILDING );
            while( (CurBuilding = ObjMgr.GetNextInType()) ) 
            {
                const bbox& BBox = CurBuilding->GetBBox();
                
                if ( BBox.Intersect(
                    vector3( WorldX, BBox.GetCenter()[1], WorldZ ) ) )
                {
                    CurCellAttribute.BLOCKED = 1;
                }
            }
            ObjMgr.EndTypeLoop();
            
            //--------------------------------------------------------------
            // Check visibility with enemy objects
            //--------------------------------------------------------------

            CurCellAttribute.VISIBLE = 0;
        }
    }

    //----------------------------------------------------------------------
    // Generate the filename
    //----------------------------------------------------------------------
    Save( GetFileName( ObjMgr ), Graph );
    delete [] Graph;
}

void path_generator::Save( const char* pFileName
    , const cell_attribute* GraphArray )
{
    X_FILE* pGraphFile = x_fopen( pFileName, "w" );
    ASSERT( pGraphFile );

    x_fwrite( GraphArray, sizeof(cell_attribute), NUM_VERTS, pGraphFile );
    x_fclose( pGraphFile );
}

void path_generator::Load( cell_attribute* GraphArray
    , obj_mgr& ObjMgr)
{
    ASSERT(GraphArray);
    
    X_FILE* pGraphFile = x_fopen( GetFileName( ObjMgr ), "r" );
    ASSERT( pGraphFile );

    x_fread( GraphArray, sizeof(cell_attribute), NUM_VERTS, pGraphFile );
    x_fclose( pGraphFile );

    s32 Blocked = 0;
    for ( s32 i = 0; i < NUM_VERTS; ++i )
    {
        if ( GraphArray[i].BLOCKED )
        {
            ++Blocked;
        }
    }
//    bot_log::Log( "path_generator::Load() detected %i blocked cells\n", Blocked );
}

xstring path_generator::GetFileName( obj_mgr& ObjMgr )
{
    // Grab the Terrain
    ObjMgr.StartTypeLoop( object::TYPE_TERRAIN );
    terrain* Terrain = (terrain*)(ObjMgr.GetNextInType());
    ObjMgr.EndTypeLoop();
    ASSERT( Terrain );

    // Produce the filename from the terrain file name
    xstring GraphFileName( Terrain->GetTerr().GetName() );
    

    const s32 len = GraphFileName.GetLength();
    GraphFileName.Delete( len - 3, 3 );
    GraphFileName += "pth";
    return GraphFileName;
}





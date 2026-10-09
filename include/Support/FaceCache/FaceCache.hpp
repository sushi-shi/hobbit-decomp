//=========================================================================
//
// FACECACHE.HPP
//
//=========================================================================
#ifndef FACECACHE_HPP
#define FACECACHE_HPP

#include <xCore/x_files/x_math.hpp>

//=========================================================================

#define MAX_FCACHE_VERTS    4

typedef u32 fcache_key;

struct fcache_face
{
    fcache_key  Key;
    bbox        BBox;
    plane       Plane;
    s32         NVerts;
    vector3     Vert[MAX_FCACHE_VERTS];

    s32         HashNext;
    s32         HashPrev;
    s32         AgeNext;
    s32         AgePrev;
};

//=========================================================================

void fcache_Init        ( void );
void fcache_Kill        ( void );
void fcache_Flush       ( void );

const fcache_face&  fcache_GetFace( fcache_key Key );

//=========================================================================
#endif
//=========================================================================

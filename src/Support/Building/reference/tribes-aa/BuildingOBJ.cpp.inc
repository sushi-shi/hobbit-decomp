
//==============================================================================
// INCLUDES
//==============================================================================

#include "BuildingOBJ.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "GameMgr\GameMgr.hpp"

//==============================================================================
// VARIABLES
//==============================================================================

static obj_type_info   s_BuildingTypeInfo( object::TYPE_BUILDING, 
                                           "Building", 
                                           BuildingCreator, 
                                           object::ATTR_SOLID_STATIC | 
                                           object::ATTR_BUILDING | 
                                           object::ATTR_PERMANENT );

//==============================================================================

static xbool    s_CollectLighting = FALSE;
static xcolor   s_CollectedLighting;

static s32   s_Cell     = -1;   // which Cell in the Grid
static s32   s_CellNGon = -1;   // which NGon in the Cell
static s32   s_Edge     = -1;   // which edge in the NGon
static xbool s_Outline  = TRUE; // render only NGon outline or not

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* BuildingCreator( void )
{
    return( (object*)(new building_obj) );
}

//==============================================================================

void building_obj::Initialize( const char* pFileName, const matrix4& L2W, s32 Power )
{
    ASSERT( pFileName );

    xbool LoadAlarmMaps = ( Power == 0 ) ? FALSE : TRUE;

    m_Instance.LoadBuilding( pFileName, L2W, LoadAlarmMaps );
    m_Power = Power;

    m_WorldPos = L2W.GetTranslation();
    m_Instance.GetBBox( m_WorldBBox );

    // SB
    // Verify collision data is as expected by the function "StripToNGon" -
    // that is only the first 2 verts in any NGon strip have the adc bit set,
    // all others must be zero.
#ifdef X_ASSERT
    const building::building_grid& Grid = m_Instance.GetBuilding().m_CGrid ;
    for (s32 i = 0 ; i < Grid.m_nNGons ; i++)
    {
        // Get NGon
        building::building_grid::ngon& NGon = Grid.m_pNGon[i] ;

        // Get NGon verts
        vector4* pVert = NGon.pVert ;
        ASSERT(NGon.nVerts >= 3) ;

        // Make sure start of strip is good
        ASSERT( *((u32*)&pVert[0].W) & (1<<15) );
        ASSERT( *((u32*)&pVert[1].W) & (1<<15) );
        ASSERT( (*((u32*)&pVert[2].W) & (1<<15))==0 );

        // Make sure rest of verts are good
        for (s32 j = 3 ; j < NGon.nVerts ; j++)
        {
            ASSERT( (*((u32*)&pVert[j].W) & (1<<15))==0 );
        }
    }
#endif
}

//==============================================================================

building_obj::~building_obj( void )
{
}

//==============================================================================

object::update_code building_obj::OnAdvanceLogic( f32 DeltaTime )
{
    (void)DeltaTime;
    SetAlarmLighting( !GameMgr.GetPower( m_Power ) );
    return( NO_UPDATE );
}

//==============================================================================

static
s32 StripToNGon( vector3* pDst, vector4* pSrc, s32 MaxVerts )
{
    // SB.
    // Simply copies verts from strip into final position of NGon verts

    // Make sure start of strip is valid
    ASSERT( *((u32*)&pSrc[0].W) & (1<<15) );
    ASSERT( *((u32*)&pSrc[1].W) & (1<<15) );
    ASSERT( (*((u32*)&pSrc[2].W) & (1<<15))==0 );

    // Setup trace pointers
    vector3 *pOddDst  = &pDst[0] ;
    vector3 *pEvenDst = &pDst[MaxVerts-1] ;
    vector4 *pVert    = pSrc ;

    // Odd number of verts?
    if (MaxVerts & 1)
    {
        // eg. Strip verts are 0,1,2,3,4,5,6
        //     Output NGon will be 1,3,5,6,4,2,0

        // Fill middle vert ready for even loop
        pEvenDst[0].Set( pVert[0].X, pVert[0].Y, pVert[0].Z ) ;
        pEvenDst-- ;
        pVert++ ;

        // Copy rest
        while(pEvenDst > pOddDst)
        {
            pOddDst->Set ( pVert[0].X, pVert[0].Y, pVert[0].Z ) ;
            pEvenDst->Set( pVert[1].X, pVert[1].Y, pVert[1].Z ) ;
            pOddDst++ ;
            pEvenDst-- ;
            pVert += 2 ;
        }
    }
    else
    {
        // eg. Strip verts are 0,1,2,3,4,5
        //     Output NGon will be 1,3,5,4,2,0

        // Copy rest
        while(pEvenDst > pOddDst)
        {
            pEvenDst->Set( pVert[0].X, pVert[0].Y, pVert[0].Z ) ;
            pOddDst->Set ( pVert[1].X, pVert[1].Y, pVert[1].Z ) ;
            pEvenDst-- ;
            pOddDst++ ;
            pVert += 2 ;
        }
    }

    // Make sure we got all the verts
    ASSERT(pEvenDst == (pOddDst-1)) ;       // Pointers will have crossed
    ASSERT(pVert    == &pSrc[MaxVerts]) ;   // Should have copied all verts

    return MaxVerts ;
    
    /*
    vector3 O[64];
    vector3 E[64];
    s32 o=0,e=0;

    ASSERT( *((u32*)&pSrc[0].W) & (1<<15) );
    ASSERT( *((u32*)&pSrc[1].W) & (1<<15) );
    ASSERT( (*((u32*)&pSrc[2].W) & (1<<15))==0 );

    for( s32 c=0, j=0; j<MaxVerts; j++, c++ )
    {
        if( *((u32*)&pSrc[j].W) & (1<<15) )
        {
            if( o )
                break;

            E[0].Set( pSrc[j+0].X, pSrc[j+0].Y, pSrc[j+0].Z );
            O[0].Set( pSrc[j+1].X, pSrc[j+1].Y, pSrc[j+1].Z );
            E[1].Set( pSrc[j+2].X, pSrc[j+2].Y, pSrc[j+2].Z );

            j+=2;
            c = 0;
            o = 1;
            e = 2; 
        }
        else
        {
            if( c&1 )
            {
                O[ o++ ].Set( pSrc[j-0].X, pSrc[j-0].Y, pSrc[j-0].Z );
            }
            else
            {
                E[ e++ ].Set( pSrc[j-0].X, pSrc[j-0].Y, pSrc[j-0].Z );
            }
        }
    }

    //
    // Put face in the colided
    //

    // Copy all the off facets into the even list
    while( --e >= 0 ) O[o++] = E[e];

    // Move verts into dest
    for( s32 i=0; i<o; i++ )
        pDst[i] = O[i];

    return o;
    */
}

//==============================================================================

void CollideWithGrid( const building::building_grid& Grid, collider& Collider )
{
    vector3 Vert[64];
    s32 NNGonsChecked = 0;
    s32 NNGonsLooped = 0;
    s32 NUniqueNGonsLooped = 0;
    s32 NCellsVisited=0;
    Grid.m_Sequence++;

    s32 MinX,MaxX;
    s32 MinY,MaxY;
    s32 MinZ,MaxZ;
    bbox BBox = Collider.GetVolumeBBox();

    MinX = (s32)x_floor( (BBox.Min.X-Grid.m_BBox.Min.X) / BUILDING_GRID_CELL_SIZE );
    MaxX = (s32)x_floor( (BBox.Max.X-Grid.m_BBox.Min.X) / BUILDING_GRID_CELL_SIZE );
    MinY = (s32)x_floor( (BBox.Min.Y-Grid.m_BBox.Min.Y) / BUILDING_GRID_CELL_SIZE );
    MaxY = (s32)x_floor( (BBox.Max.Y-Grid.m_BBox.Min.Y) / BUILDING_GRID_CELL_SIZE );
    MinZ = (s32)x_floor( (BBox.Min.Z-Grid.m_BBox.Min.Z) / BUILDING_GRID_CELL_SIZE );
    MaxZ = (s32)x_floor( (BBox.Max.Z-Grid.m_BBox.Min.Z) / BUILDING_GRID_CELL_SIZE );

    if( MaxX < 0 ) return;
    if( MaxY < 0 ) return;
    if( MaxZ < 0 ) return;
    if( MinX >= Grid.m_GridSizeX ) return;
    if( MinY >= Grid.m_GridSizeY ) return;
    if( MinZ >= Grid.m_GridSizeZ ) return;

    MinX = MAX( 0, MinX );
    MinY = MAX( 0, MinY );
    MinZ = MAX( 0, MinZ );
    MaxX = MIN( Grid.m_GridSizeX-1, MaxX );
    MaxY = MIN( Grid.m_GridSizeY-1, MaxY );
    MaxZ = MIN( Grid.m_GridSizeZ-1, MaxZ );

    //x_printf("*****\n");
    for( s32 Z=MinZ; Z<=MaxZ; Z++ )
    for( s32 Y=MinY; Y<=MaxY; Y++ )
    for( s32 X=MinX; X<=MaxX; X++ )
    {
        NCellsVisited++;
        building::building_grid::cell* pCell = &Grid.m_pCell[ X + Y*Grid.m_YOffset + Z*Grid.m_ZOffset ];

        u16* pRef = &Grid.m_pNGonRef[ pCell->RefIndex ];
        //x_printf("%1d] (%1d,%1d,%1d) %1d\n",NCellsVisited,X,Y,Z,pCell->nNGons);
        for( s32 i=0; i<pCell->nNGons; i++ )
        {
            NNGonsLooped++;

            // Get ngon and check to see if already looked at
            building::building_grid::ngon* pNGon = &Grid.m_pNGon[ pRef[i] ];
            if( pNGon->Seq == Grid.m_Sequence )
                continue;
            pNGon->Seq = Grid.m_Sequence;

            // Do quick bbox check
            NUniqueNGonsLooped++;
            if( BBox.Intersect( pNGon->BBox ) )
            {
                // Build verts and apply
                NNGonsChecked++;
                VERIFY( StripToNGon( Vert, pNGon->pVert, pNGon->nVerts ) == pNGon->nVerts );
                //draw_NGon(Vert,pNGon->nVerts,XCOLOR_RED,FALSE);
                Collider.ApplyNGon( Vert, pNGon->nVerts, pNGon->Plane );

                // Early out?
                if (Collider.GetEarlyOut())
                    return ;
            }
        }
    }

    //x_printfxy(20,0,"NNGonsLooped %1d",NNGonsLooped);
    //x_printfxy(20,1,"NNGonsULooped %1d",NUniqueNGonsLooped);
    //x_printfxy(20,2,"NNGonsChecked %1d",NNGonsChecked);
    //x_printfxy(20,3,"NCellsVisited %1d",NCellsVisited);
}

//==============================================================================

void building_obj::RenderGridNGons( void )
{
    /*
    vector3 Vert[64];
    random  Random;
    const   building::building_grid& m_CollGrid = m_Instance.GetBuilding().m_CGrid;

    draw_SetL2W( m_Instance.GetL2W() );

    for( s32 i=0;i<m_CollGrid.m_nNGons; i++ )
    {
        VERIFY( StripToNGon( Vert, m_CollGrid.m_pNGon[i].pVert, m_CollGrid.m_pNGon[i].nVerts ) == 
            m_CollGrid.m_pNGon[i].nVerts );

        draw_NGon( Vert, m_CollGrid.m_pNGon[i].nVerts, Random.color(), FALSE );
    }
    */
}

//==============================================================================

void building_obj::RenderRayNGons( void )
{
    /*
    vector3 Vert[64];
    random  Random;
    const   building::building_grid& Grid = m_Instance.GetBuilding().m_CGrid;

    draw_SetL2W( m_Instance.GetL2W() );

    for( s32 i=0;i<Grid.m_nNGons; i++ )
    {
        VERIFY( StripToNGon( Vert, Grid.m_pNGon[i].pVert, Grid.m_pNGon[i].nVerts ) == 
            Grid.m_pNGon[i].nVerts );

        if( Grid.m_pNGon[i].Seq == Grid.m_Sequence )
            draw_NGon( Vert, Grid.m_pNGon[i].nVerts, Random.color(), FALSE );
    }
    */
}

//==============================================================================
/*

void draw_Line2( const vector3& P0, const vector3& P1, xcolor Color )
{
    draw_Begin( DRAW_LINES, DRAW_NO_ZBUFFER );
    draw_Color( Color );
    draw_Vertex( P0 );
    draw_Vertex( P1 );
    draw_End();
}

void draw_NGonOutline( const vector3* pPoint, s32 NPoints )
{
    for( s32 i=0; i<NPoints; i++ )
    {
        if( i == s_Edge )
        {
            draw_Line2( pPoint[i], pPoint[ (i+1) % NPoints ], XCOLOR_WHITE );
        }
        else
        {
            draw_Line2( pPoint[i], pPoint[(i+1)%NPoints], XCOLOR_GREEN );
        }
    }
}

*/
//==============================================================================

static s32 s_RenderCellDelay = 0;

void building_obj::RenderCellNGons( const vector3& Pos )
{
    /*
    vector3 Vert[64];
    const   building::building_grid& Grid = m_Instance.GetBuilding().m_CGrid;
    random  Random;

    draw_SetL2W( m_Instance.GetL2W() );

    //
    // determine which cell the point is in
    //

    vector3 P = m_Instance.GetW2L() * Pos;

    f32 Factor = 1.0f / BUILDING_GRID_CELL_SIZE;
    P -= Grid.m_BBox.Min;
    P.Scale( Factor );

    s32 X = (s32)x_floor( P.X );
    s32 Y = (s32)x_floor( P.Y );
    s32 Z = (s32)x_floor( P.Z );

    s32 CellNum = X + Y*Grid.m_YOffset + Z*Grid.m_ZOffset;

    s_RenderCellDelay++;
    if( s_RenderCellDelay == 60 )
    {
        s_RenderCellDelay = 0;
        x_DebugMsg( "Cell = %08X [%d]\n", CellNum, CellNum );
    }

    building::building_grid::cell* pCell = &Grid.m_pCell[ CellNum ];

    if( s_Cell != -1 )
    {
        s_Cell = MAX( MIN( s_Cell, Grid.m_nGridCells - 1 ), 0 );
        pCell = &Grid.m_pCell[ s_Cell ];
    }

    building::building_grid::cell& Cell = *pCell;

    #ifdef TARGET_PS2
    if( Cell.nNGons > 100 )
        return;
    #endif

    //
    // render the ngons in this cell
    //

    for( s32 i=0; i<Cell.nNGons; i++ )
    {
        if( s_CellNGon != -1 )
        {
            if( i != s_CellNGon ) continue;
        }
        
        if( Cell.RefIndex >= Grid.m_nNGonRefs )
            return;
    
        u16* pRef = &Grid.m_pNGonRef[ Cell.RefIndex ];

        building::building_grid::ngon& NGon = Grid.m_pNGon[ pRef[i] ];

        VERIFY( StripToNGon( Vert, NGon.pVert, NGon.nVerts ) == NGon.nVerts );

        if( s_Outline )
        {
            draw_NGonOutline( Vert, NGon.nVerts );
        }
        else
        {
            draw_NGon( Vert, NGon.nVerts, Random.color(), FALSE );
        }
    }
    */
}

//==============================================================================

void building_obj::RenderGrid( void )
{
    /*
    s32 i,j;

    const building::building_grid& Grid = m_Instance.GetBuilding().m_CGrid;

    draw_ClearL2W();
    draw_BBox(m_WorldBBox,XCOLOR_WHITE);

    draw_SetL2W( m_Instance.GetL2W() );
    draw_BBox(Grid.m_BBox,XCOLOR_YELLOW);

    draw_Begin(DRAW_LINES);

    // X
    draw_Color( XCOLOR_RED );
    for( i=0; i<=Grid.m_GridSizeY; i++ )
    for( j=0; j<=Grid.m_GridSizeZ; j++ )
    {
        vector3 Start;
        vector3 End;

        Start.X = Grid.m_BBox.Min.X + 0*BUILDING_GRID_CELL_SIZE;
        Start.Y = Grid.m_BBox.Min.Y + i*BUILDING_GRID_CELL_SIZE;
        Start.Z = Grid.m_BBox.Min.Z + j*BUILDING_GRID_CELL_SIZE;
        End.X   = Grid.m_BBox.Min.X + (Grid.m_GridSizeX * BUILDING_GRID_CELL_SIZE);
        End.Y   = Start.Y;
        End.Z   = Start.Z;
        draw_Vertex( Start );
        draw_Vertex( End );
    }

    // Y
    draw_Color( XCOLOR_GREEN );
    for( i=0; i<=Grid.m_GridSizeX; i++ )
    for( j=0; j<=Grid.m_GridSizeZ; j++ )
    {
        vector3 Start;
        vector3 End;

        Start.X = Grid.m_BBox.Min.X + i*BUILDING_GRID_CELL_SIZE;
        Start.Y = Grid.m_BBox.Min.Y + 0*BUILDING_GRID_CELL_SIZE;
        Start.Z = Grid.m_BBox.Min.Z + j*BUILDING_GRID_CELL_SIZE;
        End.X   = Start.X;
        End.Y   = Grid.m_BBox.Min.Y + (Grid.m_GridSizeY * BUILDING_GRID_CELL_SIZE);
        End.Z   = Start.Z;
        draw_Vertex( Start );
        draw_Vertex( End );
    }

    // Z
    draw_Color( XCOLOR_BLUE );
    for( i=0; i<=Grid.m_GridSizeX; i++ )
    for( j=0; j<=Grid.m_GridSizeY; j++ )
    {
        vector3 Start;
        vector3 End;

        Start.X = Grid.m_BBox.Min.X + i*BUILDING_GRID_CELL_SIZE;
        Start.Y = Grid.m_BBox.Min.Y + j*BUILDING_GRID_CELL_SIZE;
        Start.Z = Grid.m_BBox.Min.Z + 0*BUILDING_GRID_CELL_SIZE;
        End.X   = Start.X;
        End.Y   = Start.Y;
        End.Z   = Grid.m_BBox.Min.Z + (Grid.m_GridSizeZ * BUILDING_GRID_CELL_SIZE);
        draw_Vertex( Start );
        draw_Vertex( End );
    }
    draw_End();
    */
}

//==============================================================================

void building_obj::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    Collider.SetContext( this, m_Instance.GetL2W(), m_Instance.GetW2L());

    // React based on type of collider.
    if( Collider.GetType() == collider::RAY )
    {
        RayCollide( Collider, m_Instance.GetBuilding().m_CGrid );
    }
    else
    if( Collider.GetType() == collider::EXTRUSION )
    {
        CollideWithGrid( m_Instance.GetBuilding().m_CGrid, Collider );  
    }
}

//==============================================================================

void building_obj::RayCollide( collider& Collider, 
                               const building::building_grid& Grid )
{
    f32     t0 = 0.0f;
    f32     t1 = 1.0f;

    vector3 LP0  ( Collider.RayGetStart() );    // Local space P0.
    vector3 LP1  ( Collider.RayGetEnd()   );    // Local space P1.
    vector3 GP0  ( LP0 );                       // Grid  space P0.
    vector3 GP1  ( LP1 );                       // Grid  space P1.

    // Need to move the GP0/1 points from Local to Grid space.
    {
        f32 Factor = 1.0f / BUILDING_GRID_CELL_SIZE;
        GP0 -= Grid.m_BBox.Min;
        GP1 -= Grid.m_BBox.Min;
        GP0.Scale( Factor );
        GP1.Scale( Factor );
    }

    // Now we can make our grid space bounding box.
    bbox    GBBox( GP0, GP1 );

    // Get a delta between the points in grid space.
    vector3 GDelta = GP1 - GP0;

    // And the delta in local space.
    vector3 LDelta = LP1 - LP0;

    // Click up the grid's sequence number.
    Grid.m_Sequence++;

    //x_printfxy( 0, 0, "P0: %8.3f, %8.3f, %8.3f", P0.X, P0.Y, P0.Z );
    //x_printfxy( 0, 1, "P1: %8.3f, %8.3f, %8.3f", P1.X, P1.Y, P1.Z );
    //x_printfxy( 0, 2, "  : %8.3f, %8.3f, %8.3f", P1.X-P0.X, P1.Y-P0.Y, P1.Z-P0.Z );

    //
    // The vast majority of rays into a building will be "short" and probably
    // land completely within a single grid cell or 2 or 4.  So the path for 
    // such cases is pretty light.
    //

    //
    // We need to clip the ray, or rather, its t values, to the grid bounds.
    //
    {
        f32     t;
        u8      P0Mask = 0;
        u8      P1Mask = 0;
        u8      Bit;
        s32     i;
        vector3 Hi( (f32)Grid.m_GridSizeX, 
                    (f32)Grid.m_GridSizeY, 
                    (f32)Grid.m_GridSizeZ );
                                
        if( GP0.X < 0.0f )       P0Mask |= 0x01;
        if( GP0.Y < 0.0f )       P0Mask |= 0x02;
        if( GP0.Z < 0.0f )       P0Mask |= 0x04;
        if( GP0.X > Hi.X )       P0Mask |= 0x08;
        if( GP0.Y > Hi.Y )       P0Mask |= 0x10;
        if( GP0.Z > Hi.Z )       P0Mask |= 0x20;

        if( GP1.X < 0.0f )       P1Mask |= 0x01;
        if( GP1.Y < 0.0f )       P1Mask |= 0x02;
        if( GP1.Z < 0.0f )       P1Mask |= 0x04;
        if( GP1.X > Hi.X )       P1Mask |= 0x08;
        if( GP1.Y > Hi.Y )       P1Mask |= 0x10;
        if( GP1.Z > Hi.Z )       P1Mask |= 0x20;

        // Do we need to clip?
        if( P0Mask & P1Mask )
        {
            // The line segment is completely out of the grid.  Reject it!
            return;
        }
        else
        if( P0Mask | P1Mask )
        {
            Bit = 0x01;

            // Clip on the 0,0,0 side.
            for( i = 0; i < 3; i++ )
            {
                // Clip entry on X,Y,Z = 0.
                if( (P0Mask & Bit) && !(P1Mask & Bit) )
                {
                    t = (0.0f - GP0[i]) / (GDelta[i]);
                    if( t > t0 )  t0 = t;
                }

                // Clip exit on X,Y,Z = 0.
                if( !(P0Mask & Bit) && (P1Mask & Bit) )
                {
                    t = (0.0f - GP0[i]) / (GDelta[i]);
                    if( t < t1 )  t1 = t;
                }

                Bit <<= 1;
            }

            // Clip on the Hi side.
            for( i = 0; i < 3; i++ )
            {
                // Clip entry on Hi.X,Y,Z.
                if( (P0Mask & Bit) && !(P1Mask & Bit) )
                {
                    t = (Hi[i] - GP0[i]) / (GDelta[i]);
                    if( t > t0 )  t0 = t;
                }

                // Clip exit on Hi.X,Y,Z.
                if( !(P0Mask & Bit) && (P1Mask & Bit) )
                {
                    t = (Hi[i] - GP0[i]) / (GDelta[i]);
                    if( t < t1 )  t1 = t;
                }

                Bit <<= 1;
            }
        }
        else
        {
            // If we are here, then we didn't need to clip at all.  See if we
            // have a short ray and can do this "quick and dirty".

            if( GDelta.LengthSquared() < 16.0f )
            {
                // SHORT RAY!  WOOHOO!

                s32 MinX, MaxX;
                s32 MinY, MaxY;
                s32 MinZ, MaxZ;
                s32 x, y, z;

                MinX = (s32)GBBox.Min.X;
                MaxX = (s32)GBBox.Max.X;
                            
                MinY = (s32)GBBox.Min.Y;
                MaxY = (s32)GBBox.Max.Y;
                            
                MinZ = (s32)GBBox.Min.Z;
                MaxZ = (s32)GBBox.Max.Z;

                for( z = MinZ; z <= MaxZ; z++ )
                for( y = MinY; y <= MaxY; y++ )
                for( x = MinX; x <= MaxX; x++ )
                {
                    xbool ValidCell = (IN_RANGE( 0, x, Grid.m_GridSizeX-1 )) &&
                                      (IN_RANGE( 0, y, Grid.m_GridSizeY-1 )) &&
                                      (IN_RANGE( 0, z, Grid.m_GridSizeZ-1 ));
                    if( ValidCell )
                    {
                        s32 Cell = (x) + (y * Grid.m_YOffset) + (z * Grid.m_ZOffset);
                        RayCollideWithCell( Collider, Grid, Cell, LP0, LP1, 0.0f, 1.0f );

                        // Early out?
                        if (Collider.GetEarlyOut())
                            return ;
                    }
                } 
            
                // Done!
                return;
            }
        }

        // Did we clip the whole line segment away?
        if( t0 > t1 )
            return;      
    }

    //
    // We now have parametric values t0 and t1 which are clipped to the grid.
    // Walk t0 towards t1 until we get to the cell containing the end point.  
    // Also, bail out if we hit at some t value less than our stepping t0.
    //
    {
        s32     GridX0,   GridY0,   GridZ0;
        s32     GridX1,   GridY1,   GridZ1;
        s32     StepCX,   StepCY,   StepCZ;     // Step CELL
        s32     StepGX,   StepGY,   StepGZ;     // Step GRID
        f32     StepTX,   StepTY,   StepTZ;     // Step T
        f32     NextTX,   NextTY,   NextTZ;
        s32     GridCell, StopCell, NextCell;

        vector3 Clip0;
        vector3 Clip1;
        vector3 Segment0;
        vector3 Segment1;
        vector3 Fudge;

        Fudge = Collider.RayGetDir() * 0.05f;

        //draw_SetL2W( m_Instance.GetL2W() );
        //draw_Label( LP0 + (LP1-LP0) * t0, XCOLOR_BLACK, "X" );
        //draw_Label( LP0 + (LP1-LP0) * t1, XCOLOR_BLACK, "X" );
        //draw_ClearL2W();

        // Clip P0 and P1 into Clip0 and Clip1.
        if( (t0 > 0.0f) || (t1 < 1.0f) )
        {
            Clip0 = GP0 + GDelta * t0;
            Clip1 = GP0 + GDelta * t1;
        }
        else
        {
            Clip0 = GP0;
            Clip1 = GP1;
        }

        // Determine starting and ending grid sections.  The clipped values are
        // always positive, so casting to s32 is good enough for a floor 
        // operation.

        GridX0 = (s32)Clip0.X;
        GridY0 = (s32)Clip0.Y;
        GridZ0 = (s32)Clip0.Z;
        GridX1 = (s32)Clip1.X;
        GridY1 = (s32)Clip1.Y;
        GridZ1 = (s32)Clip1.Z;

        GridX0 = MIN( GridX0, Grid.m_GridSizeX-1 );
        GridY0 = MIN( GridY0, Grid.m_GridSizeY-1 );
        GridZ0 = MIN( GridZ0, Grid.m_GridSizeZ-1 );
        GridX1 = MIN( GridX1, Grid.m_GridSizeX-1 );
        GridY1 = MIN( GridY1, Grid.m_GridSizeY-1 );
        GridZ1 = MIN( GridZ1, Grid.m_GridSizeZ-1 );

        // Setup stepping in X.

        if( GridX0 != GridX1 )
        {
            StepTX = x_abs( 1.0f / GDelta.X );
            if( GDelta.X > 0.0f )
            {
                NextTX = ((GridX0+1) - GP0.X) / (GDelta.X);
                StepCX = 1;
                StepGX = 1;
            }
            else
            {
                NextTX = ((GridX0  ) - GP0.X) / (GDelta.X);
                StepCX = -1;
                StepGX = -1;
            }
        }
        else
        {
            NextTX = 2.0f;
            StepTX = 0.0f;
            StepCX = 0;
            StepGX = 0;
        }

        // Setup stepping in Y.

        if( GridY0 != GridY1 )
        {
            StepTY = x_abs( 1.0f / GDelta.Y );
            if( GDelta.Y > 0.0f )
            {
                NextTY = ((GridY0+1) - GP0.Y) / (GDelta.Y);
                StepCY = Grid.m_YOffset;
                StepGY = 1;
            }
            else
            {
                NextTY = ((GridY0  ) - GP0.Y) / (GDelta.Y);
                StepCY = -Grid.m_YOffset;
                StepGY = -1;
            }
        }
        else
        {
            NextTY = 2.0f;
            StepTY = 0.0f;
            StepCY = 0;
            StepGY = 0;
        }

        // Setup stepping in Z.

        if( GridZ0 != GridZ1 )
        {
            StepTZ = x_abs( 1.0f / GDelta.Z );
            if( GDelta.Z > 0.0f )
            {
                NextTZ = ((GridZ0+1) - GP0.Z) / (GDelta.Z);
                StepCZ = Grid.m_ZOffset;
                StepGZ = 1;
            }
            else
            {
                NextTZ = ((GridZ0  ) - GP0.Z) / (GDelta.Z);
                StepCZ = -Grid.m_ZOffset;
                StepGZ = -1;
            }
        }
        else
        {
            NextTZ = 2.0f;
            StepTZ = 0.0f;
            StepCZ = 0;
            StepGZ = 0;
        }

        GridCell = (GridX0) + (GridY0 * Grid.m_YOffset) + (GridZ0 * Grid.m_ZOffset);
        StopCell = (GridX1) + (GridY1 * Grid.m_YOffset) + (GridZ1 * Grid.m_ZOffset);

        // Here's the big show.  Remember, we are walking t0 towards t1 until we
        // get to the stop cell, or we have a collision at some t value less 
        // than t0.

        Segment0 = LP0 + LDelta * t0;

        while( t0 < Collider.GetCollisionT() )
        {
            // Just to be absolutely safe (and guard against floating point
            // problems due to slight inaccuracies, see if we are currently in a
            // valid cell.

            xbool ValidCell = (IN_RANGE( 0, GridX0, Grid.m_GridSizeX-1 )) &&
                              (IN_RANGE( 0, GridY0, Grid.m_GridSizeY-1 )) &&
                              (IN_RANGE( 0, GridZ0, Grid.m_GridSizeZ-1 ));

            // Do the computations for taking a step.  (Don't actually take the
            // step, just think about it so we know where we leave the grid 
            // cell.)

            if( NextTX < NextTY )
            {
                if( NextTX < NextTZ )
                {
                    // Step in X.
                    t1        = NextTX;
                    NextTX   += StepTX;
                    GridX0   += StepGX;
                    NextCell  = GridCell + StepCX;
                }
                else
                {
                    // Step in Z.
                    t1        = NextTZ;
                    NextTZ   += StepTZ;
                    GridZ0   += StepGZ;
                    NextCell  = GridCell + StepCZ;
                }
            }
            else
            {
                if( NextTY < NextTZ )
                {
                    // Step in Y.
                    t1        = NextTY;
                    NextTY   += StepTY;
                    GridY0   += StepGY;
                    NextCell  = GridCell + StepCY;
                }
                else
                {
                    // Step in Z.
                    t1        = NextTZ;
                    NextTZ   += StepTZ;
                    GridZ0   += StepGZ;
                    NextCell  = GridCell + StepCZ;
                }
            }

            // At this point we already have Segment0 (from before loop or 
            // below), we need Segment1.
            Segment1 = LP0 + (LDelta * t1);

            // Collide the ngons in the cell with the Segment.
            if( ValidCell )
            {
                RayCollideWithCell( Collider, 
                                    Grid, 
                                    GridCell, 
                                    Segment0, 
                                    Segment1 + Fudge, 
                                    t0, t1 );

                // Early out?
                if (Collider.GetEarlyOut())
                    return ;
            }

            // If we are in the final cell, bail out.
            if( GridCell == StopCell )
                break;

            // Update values for next time around.
            GridCell = NextCell;
            Segment0 = Segment1;
            t0       = t1;
        }
    }
}

//==============================================================================

#define MAX_VERTS_PER_NGON  64

void building_obj::RayCollideWithCell( collider&        Collider, 
                                      const building::building_grid&   Grid,
                                       s32              GridCell,
                                       const vector3&   P0, 
                                       const vector3&   P1,
                                       f32              t0, 
                                       f32              t1 )
{
    // Be sure we have a valid cell
    ASSERT( (GridCell>=0) && (GridCell<Grid.m_nGridCells) );
    if( (GridCell<0) || (GridCell>=Grid.m_nGridCells) )
        return;

    s32                  i, j;
    f32                  D0, D1, T;
    vector3              Vert[MAX_VERTS_PER_NGON];
    vector3              Ray, P, Edge;
    collider::collision  Collision;
    building::building_grid::cell& Cell = Grid.m_pCell[ GridCell ];
    u16*                 pRef = &Grid.m_pNGonRef[ Cell.RefIndex ];

    /*
    if( TRUE )
    {
        s32 G  = GridCell;
        s32 z  = (G / Grid.m_ZOffset);  
            G -= (z * Grid.m_ZOffset);
        s32 y  = (G / Grid.m_YOffset);  
            G -= (y * Grid.m_YOffset);
        s32 x  = G;
        ASSERT( x < Grid.m_GridSizeX );
        ASSERT( y < Grid.m_GridSizeY );
        ASSERT( z < Grid.m_GridSizeZ );
        vector3 Position( x * 4.0f + 2, y * 4.0f + 2, z * 4.0f + 2 );
        Position += Grid.m_BBox.Min;
    //  draw_Label( m_Instance.GetL2W() * Position, XCOLOR_YELLOW, "X" );
        draw_Label( m_Instance.GetL2W() * Position, XCOLOR_YELLOW, xfs("%d",Cell.nNGons) );    
    }
    */

    Ray = P1 - P0;

    for( i = 0; i < Cell.nNGons; i++ )
    {
        // Get the ngon.
        building::building_grid::ngon& NGon = Grid.m_pNGon[ pRef[i] ];

        // Set the sequence number, but do NOT use it to prevent testing.  
        // (It is possible to miss the ngon in the current grid cell, but hit it
        // in the next one, since the ray is clipped to the grid cell.)
        NGon.Seq = Grid.m_Sequence;

        // Do the collision manually so we can use the given points rather than
        // those that are already in the Collider.

        // Backface rejection check.
        if( v3_Dot( NGon.Plane.Normal, Ray ) >= 0.0f )
            continue;

        // Get distance of points to plane.
        D0 = NGon.Plane.Distance( P0 );
        D1 = NGon.Plane.Distance( P1 );

        // If both on same side, reject.
        if( (D0 <= 0) && (D1 <= 0) )  continue;
        if( (D0 >= 0) && (D1 >= 0) )  continue;

        ASSERT( NGon.nVerts < MAX_VERTS_PER_NGON );

        VERIFY( StripToNGon( Vert, NGon.pVert, NGon.nVerts ) == NGon.nVerts );

        // Find intersection point on plane.
        T = D0 / (D0-D1);
        P = P0 + Ray * T;

        const f32 E = 0.0001f;

        // Test if this ngon is convex or concave.
        // The convex flag is stored in bit 0 of the Min.X in the bounding box.

        if(( *(u32*)&NGon.BBox.Min.X ) & 0x1 )
        {
            // See if point is inside the convex ngon.
            for( j = 0; j < NGon.nVerts; j++ )
            {
                Edge = NGon.Plane.Normal.Cross( Vert[ (j+1) % NGon.nVerts ] - Vert[j] );
                
                if( Edge.Dot( P - Vert[j] ) < -E )
                    break;
            }
    
            // If the loop did not complete, then there was no hit.
            if( j < NGon.nVerts )
                continue;
        }
        else
        {
            vector3& P0 = Vert[0];
            vector3& N  = NGon.Plane.Normal;
            vector3  V  = P - P0;

            // See if point is inside the concave ngon.
            for( j = 1; j < NGon.nVerts-1; j++ )
            {
                vector3& P1 = Vert[ j ];
                vector3& P2 = Vert[j+1];
                
                vector3  V0 = N.Cross( P1 - P0 );
                vector3  V1 = N.Cross( P2 - P1 );
                vector3  V2 = N.Cross( P0 - P2 );

                if( V0.Dot(        V      ) < -E ) continue;
                if( V1.Dot( P - Vert[ j ] ) < -E ) continue;
                if( V2.Dot( P - Vert[j+1] ) < -E ) continue;
                
                break;
            }

            // If the loop completed, then there was no hit.
            if( j == ( NGon.nVerts - 1 ))
                continue;
        }
        

        // The loop completed, so there is a hit.
        Collision.Collided   = TRUE;
        Collision.Plane      = NGon.Plane;
        Collision.Point      = P;
        Collision.T          = t0 + T * (t1-t0);
        Collider.ApplyCollision( Collision );

        // Early out?
        if (Collider.GetEarlyOut())
            return ;

        // If collecting lighting and new collision
        if( ( s_CollectLighting ) &&
            ( Collider.GetCollisionT() == Collision.T ) )
        {
            ASSERT( 0 );
//            s_CollectedLighting = m_Instance.GetBuilding().LookupLighting( NGon, P, m_Instance.m_HashAdd, m_Instance.m_AlarmLighting );
        }
    }
}

//==============================================================================

void building_obj::GetLighting( const vector3& Pos, const vector3& RayDir, xcolor& C, vector3& BuildingPt )
{
    ASSERT( FALSE );

    /*

    collider Collider;
	
	vector3 Start = Pos;
    vector3 End   = Start + RayDir;

    s_CollectLighting = TRUE;

	Collider.RaySetup( NULL, Start, End ) ;
    this->OnCollide( 0, Collider );

    if( Collider.HasCollided() )
    {
        collider::collision Coll;
        Collider.GetCollision( Coll );
        C = s_CollectedLighting;
        BuildingPt = Coll.Point;
    }
    else
    {
        C = XCOLOR_WHITE;
        BuildingPt = Start;
        BuildingPt.Y = -100000.0f;
    }

    s_CollectLighting = FALSE;

    */
}

//==============================================================================

void building_obj::GetSphereFaceKeys ( const vector3& EyePos,
                                       const vector3& Pos,
                                       f32            Radius,
                                       s32&           NKeys,
                                       fcache_key*    pKey )
{
    /*
//    #ifdef X_ASSERT
    s32 MaxKeys = NKeys;
    //#endif

    NKeys = 0;

    const building::building_grid& Grid = m_Instance.GetBuilding().m_CGrid;
//  s32 NNGonsChecked = 0;
    s32 NNGonsLooped = 0;
    s32 NUniqueNGonsLooped = 0;
    s32 NCellsVisited=0;
    s32 MinX,MaxX;
    s32 MinY,MaxY;
    s32 MinZ,MaxZ;

    Grid.m_Sequence++;
    vector3 LocalEye = m_Instance.GetW2L() * EyePos;
    vector3 Center = m_Instance.GetW2L() * Pos;
    bbox BBox( Center, Radius );

    MinX = (s32)x_floor( (BBox.Min.X-Grid.m_BBox.Min.X) / BUILDING_GRID_CELL_SIZE );
    MaxX = (s32)x_floor( (BBox.Max.X-Grid.m_BBox.Min.X) / BUILDING_GRID_CELL_SIZE );
    MinY = (s32)x_floor( (BBox.Min.Y-Grid.m_BBox.Min.Y) / BUILDING_GRID_CELL_SIZE );
    MaxY = (s32)x_floor( (BBox.Max.Y-Grid.m_BBox.Min.Y) / BUILDING_GRID_CELL_SIZE );
    MinZ = (s32)x_floor( (BBox.Min.Z-Grid.m_BBox.Min.Z) / BUILDING_GRID_CELL_SIZE );
    MaxZ = (s32)x_floor( (BBox.Max.Z-Grid.m_BBox.Min.Z) / BUILDING_GRID_CELL_SIZE );

    if( MaxX < 0 ) return;
    if( MaxY < 0 ) return;
    if( MaxZ < 0 ) return;
    if( MinX >= Grid.m_GridSizeX ) return;
    if( MinY >= Grid.m_GridSizeY ) return;
    if( MinZ >= Grid.m_GridSizeZ ) return;

    MinX = MAX( 0, MinX );
    MinY = MAX( 0, MinY );
    MinZ = MAX( 0, MinZ );
    MaxX = MIN( Grid.m_GridSizeX-1, MaxX );
    MaxY = MIN( Grid.m_GridSizeY-1, MaxY );
    MaxZ = MIN( Grid.m_GridSizeZ-1, MaxZ );

    for( s32 Z=MinZ; Z<=MaxZ; Z++ )
    for( s32 Y=MinY; Y<=MaxY; Y++ )
    for( s32 X=MinX; X<=MaxX; X++ )
    {
        NCellsVisited++;
        building::building_grid::cell* pCell = &Grid.m_pCell[ X + Y*Grid.m_YOffset + Z*Grid.m_ZOffset ];

        u16* pRef = &Grid.m_pNGonRef[ pCell->RefIndex ];
        //x_printf("%1d] (%1d,%1d,%1d) %1d\n",NCellsVisited,X,Y,Z,pCell->nNGons);
        for( s32 i=0; i<pCell->nNGons; i++ )
        {
            NNGonsLooped++;

            // Get ngon and check to see if already looked at
            building::building_grid::ngon* pNGon = &Grid.m_pNGon[ pRef[i] ];
            if( pNGon->Seq == Grid.m_Sequence )
                continue;
            pNGon->Seq = Grid.m_Sequence;
            NUniqueNGonsLooped++;

            // Check if eye is behind the plane
            if( pNGon->Plane.Distance( LocalEye ) > 0.0f )
            {
                // Do quick bbox check
                if( BBox.Intersect( pNGon->BBox ) )
                {
                    // Do plane sphere check
                    f32 D = pNGon->Plane.Distance( Center );
                    //if( D<0 ) D = -D;
                    //if( D <= Radius )
                    if( (D>0) && (D<=Radius) )
                    {
                        // Add keys
                        for( s32 j=0; j<pNGon->nVerts-2; j++ )
                        if( NKeys < MaxKeys )
                        {
                            s32 PrivateInfo = (((s32)pRef[i])<<6) | (j&((1<<6)-1));
                            fcache_ConstructKey( pKey[NKeys], m_ObjectID.Slot, PrivateInfo );
                            NKeys++;
                        }
                    }
                }
            }
        }
    } 
    */
}
                
//==============================================================================

void building_obj::ConstructFace   ( fcache_face& Face )
{
    /*
    // Determine ngon and triangle we want
    s32 ObjectSlot;
    s32 PrivateInfo;
    s32 NGonID,TriID;
    fcache_DestructKey( Face.Key, ObjectSlot, PrivateInfo );
    u32 PrivateI = (u32)PrivateInfo;
    NGonID = PrivateI >> 6;
    TriID  = PrivateI & ((1<<6)-1);

    // Get ptr to ngon
    const building::building_grid& Grid = m_Instance.GetBuilding().m_CGrid;
    ASSERT( (NGonID>=0) && (NGonID<Grid.m_nNGons) );
    building::building_grid::ngon* pNGon = &Grid.m_pNGon[ NGonID ];

    // Build verts and apply
    vector3 Vert[64];
    VERIFY( StripToNGon( Vert, pNGon->pVert, pNGon->nVerts ) == pNGon->nVerts );
    const matrix4& L2W = m_Instance.GetL2W();

    ASSERT( (TriID>=0) && (TriID<pNGon->nVerts-2) );

    Face.Pos[0] = L2W * Vert[0];
    Face.Pos[1] = L2W * Vert[TriID+1];
    Face.Pos[2] = L2W * Vert[TriID+2];

    Face.Plane = pNGon->Plane;
    Face.Plane.Transform( L2W );
    //Face.Plane.Setup( Face.Pos[0], Face.Pos[1], Face.Pos[2] );

    Face.NVerts = 3;
    Face.BBox.Clear();
    Face.BBox.AddVerts( Face.Pos, 3 );
    Face.Plane.GetOrthoVectors( Face.OrthoS, Face.OrthoT );
    Face.OrthoST[0].X = Face.OrthoS.Dot( Face.Pos[0] );
    Face.OrthoST[0].Y = Face.OrthoT.Dot( Face.Pos[0] );
    Face.OrthoST[1].X = Face.OrthoS.Dot( Face.Pos[1] );
    Face.OrthoST[1].Y = Face.OrthoT.Dot( Face.Pos[1] );
    Face.OrthoST[2].X = Face.OrthoS.Dot( Face.Pos[2] );
    Face.OrthoST[2].Y = Face.OrthoT.Dot( Face.Pos[2] );
    */
}

//==============================================================================

const char* building_obj::GetBuildingName( void )
{
    return g_BldManager.GetBuildingName( *m_Instance.m_pBuilding );
}

//==============================================================================

matrix4 building_obj::GetW2L( void )
{
    return m_Instance.m_W2L;
}

//==============================================================================
matrix4 building_obj::GetL2W( void )
{
    return m_Instance.m_L2W;
}

//==============================================================================

s32 building_obj::FindZoneFromPoint( const vector3& Point )
{
    return m_Instance.m_pBuilding->FindZoneFromPoint( Point );
}

//==============================================================================

void building_obj::SetAlarmLighting( xbool State )
{
    m_Instance.SetAlarmLighting( State );
}

//==============================================================================
//
// Light all building objects
//
//==============================================================================

#if 0
void LightBuildings( void )
{

    building_obj* pBuildingObj = NULL;
/*
    const view *pView = eng_GetActiveView( 0 );
    g_BldManager.SetLightPos( pView->GetPosition() );
*/

    x_DebugMsg( "***********************************\n" );
    x_DebugMsg( "*** Lighting Building Instances ***\n" );
    x_DebugMsg( "***********************************\n" );
    
    ObjMgr.StartTypeLoop( object::TYPE_BUILDING );
    
    while( (pBuildingObj = (building_obj*)ObjMgr.GetNextInType()))
    {
        bld_instance& Instance = pBuildingObj->m_Instance;
        const char* pName = pBuildingObj->GetBuildingName();
    
        if( Instance.m_ID && SkipBuilding( pName ))
        {
            x_DebugMsg( "Skipping the lighting for instance %d of building \"%s\"!\n", Instance.m_ID, pName );
        }
        else
        {
            x_DebugMsg( "\nLighting building \"%s\"\n", pName );
            pBuildingObj->m_Instance.LightBuilding();
        }
    }
    
    ObjMgr.EndTypeLoop();
}
#endif

//==============================================================================

xbool IsPointInside( const vector3& P, const vector3* pVert, s32 NumVert )
{
    vector3 Normal = vector3( 0, 0, 0 );
    vector3 Centre = pVert[0];
    
    for( s32 i=1; i<NumVert-1; i++ )
    {
        vector3 V0 = pVert[ i ] - pVert[0];
        vector3 V1 = pVert[i+1] - pVert[0];
        vector3 V2 = V0.Cross( V1 );
        
        Normal += V2;
        Centre += pVert[i];
    }

    Centre += pVert[ NumVert-1 ];
    Centre  = Centre / (f32)NumVert;
    
    if( Normal.Length() < 0.0001f )
        return( FALSE );
    
    Normal.Normalize();
    plane Plane( Normal, -Normal.Dot( Centre ));

    if( x_abs( Plane.Distance( P )) > 0.001f )
        return( FALSE );

    const f32 E = 0.0001f;
    const vector3& P0 = pVert[0];
    vector3         V = P - P0;
    s32 j;

    for( j=1; j<NumVert-1; j++ )
    {
        const vector3& P1 = pVert[ j ];
        const vector3& P2 = pVert[j+1];
    
        vector3 V0 = Normal.Cross( P1 - P0 );
        vector3 V1 = Normal.Cross( P2 - P1 );
        vector3 V2 = Normal.Cross( P0 - P2 );

        if( V0.Dot(         V      ) < -E ) continue;
        if( V1.Dot( P - pVert[ j ] ) < -E ) continue;
        if( V2.Dot( P - pVert[j+1] ) < -E ) continue;
    
        break;
    }

    if( j == ( NumVert - 1 ))
        return( FALSE );
        
    return( TRUE );
}

//==============================================================================

struct dlist_uv
{
    s16 TU,TV;
    u16 LU,LV;
};

s32 StripToNGonWithUVs( vector3* pDstVert, vector4* pSrcVert, 
                        vector2* pDstUV,   dlist_uv* pSrcUV,
                        s32 MaxVerts );

xbool FindNGon( const vector3& Point, const building::dlist& DList, s32& NumVerts, vector3* pVert )
{
    s32 nVertices = DList.nVertices;
    
    #ifdef TARGET_PS2
    vector4*  pVt = (vector4* )( DList.pData + 32 );        // +32 skips over DMA tag and header
    #else
    byte* PS2Data = (byte*)(((building::dlist::pc_data*)DList.pData)->pDList);
    vector4*  pVt = (vector4* )( PS2Data + 32 );
    #endif

    dlist_uv* pUV = (dlist_uv*)( pVt + nVertices  ) + 2;    // + 2 skips over 128-bit UV header
    vector2 UV[64];

    NumVerts = 0;
    
    //
    // run through all vertices in display list and gather information for each ngon
    //

    for ( s32 i=0; i<nVertices-1; i++ )
    {
        u32 t0 = *(u32*)&pVt[ i ].W;
        u32 t1 = *(u32*)&pVt[i+1].W;

        // wait until the start of the next ngon by looking at the ADC bit

        if (( t0 & 0x8000 ) && ( t1 & 0x8000 ))
        {
            //
            // search forward until the start of the next ngon (or no more vertices)
            //

            s32 NumV = 0;

            for ( s32 j=i; j<nVertices; j++ )
            {
                // skip over first 2 vertices which we know have the ADC bit set
                
                if ( j > i+2 )
                {
                    t0 = *(u32*)&pVt[j].W;
                    
                    // loop until the start of a new ngon
                    
                    if ( t0 & 0x8000 )
                    {
                        break;
                    }
                }
                
                // increment number of vertices in this strip
                
                NumV++;
            }

            //
            // convert triangle strip to a triangle fan
            //

            ASSERT( NumV >=  3 );
            ASSERT( NumV <= 64 );
            
            VERIFY( StripToNGonWithUVs( pVert, &pVt[i], UV, &pUV[i], NumV ) == NumV );
            
            if( IsPointInside( Point, pVert, NumV ))
            {
                NumVerts = NumV;
                return( TRUE );
            }
        }
    }

    return( FALSE );
}

//==============================================================================

static s32 s_CastRayDelay = 0;

void CastRayDebug( const building* pBuilding, const matrix4& L2W )
{
    const view* pView     = eng_GetActiveView( 0 );
    vector3     Point;
    f32         Dist;

    xbool Hit = pBuilding->CastRay( pView->GetPosition(), pView->GetPosition() + ( pView->GetViewZ() * 500.0f ), Dist, Point );
    
    if( Hit == FALSE )
        return;
    
    draw_Point( Point, XCOLOR_RED );
    draw_SetL2W( L2W );
    
    matrix4 W2L = L2W;
    W2L.Invert();
    
    vector3 LocalPoint = W2L * Point;

    for( s32 i=0; i<pBuilding->m_nDLists; i++ )
    {
        s32     NumVerts;
        vector3 Vertices[64];

        vector3 BBMin = pBuilding->m_pDList[i].BBMin;
        vector3 BBMax = pBuilding->m_pDList[i].BBMax;
        vector3 Vec   = BBMax - BBMin;

        const f32 E = 0.1f;
        
        if( x_abs( Vec.X ) < E ) BBMin.X -= E, BBMax.X += E;
        if( x_abs( Vec.Y ) < E ) BBMin.Y -= E, BBMax.Y += E;
        if( x_abs( Vec.Z ) < E ) BBMin.Z -= E, BBMax.Z += E;

        Vec = BBMax - BBMin;
        Vec.Normalize();
        
        BBMin -= Vec * 0.1f;
        BBMax += Vec * 0.1f;
        
        bbox BBox( BBMin, BBMax );

        if( BBox.Intersect( LocalPoint ))
        {
            if( FindNGon( LocalPoint, pBuilding->m_pDList[i], NumVerts, Vertices ))
            {
                draw_NGon( Vertices, NumVerts, XCOLOR_RED, TRUE );

                s_CastRayDelay++;
                
                if( s_CastRayDelay == 60 )
                {
                    s_CastRayDelay = 0;
                    x_DebugMsg( "=================================\n" );
                
                    for( s32 j=0; j<NumVerts; j++ )
                    {
                        x_DebugMsg( "%f %f %f\n", Vertices[j].X, Vertices[j].Y, Vertices[j].Z );
                    }
                }
                
                break;
            }
        }
    }
    
    draw_ClearL2W();
}

//==============================================================================

void RenderBuildingDebug( void )
{
    /*
    if(( g_BldDebug.ShowGrid         == FALSE ) &&
       ( g_BldDebug.ShowCellNGons    == FALSE ) &&
       ( g_BldDebug.ShowPortals      == FALSE ) &&
       ( g_BldDebug.ShowLookupColors == FALSE ) &&
       ( g_BldDebug.ShowRayCast      == FALSE ))
    {
        return;
    }

    building_obj*  pBuildingObj;
    building_obj*  pBuilding = NULL;
    vector3 Pos  = eng_GetActiveView( 0 )->GetPosition();
    f32     Dist = 10000.0f;

    ObjMgr.StartTypeLoop( object::TYPE_BUILDING );
    
    while( (pBuildingObj = (building_obj*)ObjMgr.GetNextInType()) )
    {
        f32 D = ( pBuildingObj->GetPosition() - Pos ).Length();
        
        if( D < Dist )
        {
            Dist = D;
            pBuilding = pBuildingObj;
        }
    }
    
    ObjMgr.EndTypeLoop();

    const matrix4& L2W = pBuilding->GetL2W();
    
    if( pBuilding )
    {
        if( g_BldDebug.ShowGrid )
        {
            pBuilding->RenderGrid();
        }

        if( g_BldDebug.ShowCellNGons )
        {
            pBuilding->RenderCellNGons( Pos );
        }
        
        if( g_BldDebug.ShowPortals )
        {
            pBuilding->m_Instance.m_pBuilding->RenderPortals( L2W );
        }
        
        if( g_BldDebug.ShowLookupColors )
        {
            RenderLookupColors();
        }
        
        if( g_BldDebug.ShowRayCast )
        {
            CastRayDebug( pBuilding->m_Instance.m_pBuilding, L2W );
        }
    }
    */
}



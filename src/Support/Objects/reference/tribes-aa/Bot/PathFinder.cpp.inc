//==============================================================================
//
//  PathFinder.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "PathFinder.hpp"
#include "CellRef.hpp"
#include "FibHeap.hpp"
#include <Support/Terrain/Terr.hpp>
#include "BotLog.hpp"
#include "PathGenerator.hpp"

//==============================================================================
//  STORAGE
//==============================================================================

//==============================================================================
//  VALUES
//==============================================================================
static const s32 MAX_NUM_VERTS = 256 * 256;
static const f32 ASCENT_COST_WEIGHT = 0.0f;
static const f32 LATERAL_SLOPE_COST_WEIGHT = 0.5f;
static const f32 PATH_TURN_COST = 8.0f;
    
static const byte NUM_EDGE_DIRECTIONS = 8;

static const f32 INFINITY = F32_MAX / 2.0f;
static const f32 DESIRED_AVG_UPDATE_MS_PER_FRAME = 0.005f;
static const f32 MAX_SEARCH_MS_PER_UPDATE = 0.5f;
static const f32 MS_BETWEEN_UPDATES
                = MAX_SEARCH_MS_PER_UPDATE / DESIRED_AVG_UPDATE_MS_PER_FRAME;

static const byte INVALID_EDGE = 255;

static const cell_ref WALK_EDGE_DELTA[8] = {
    cell_ref( 0, -1 )       // N
    , cell_ref( 1, -1 )     // NE
    , cell_ref( 1, 0 )      // E
    , cell_ref( 1, 1 )      // SE
    , cell_ref( 0, 1 )      // S
    , cell_ref( -1, 1 )     // SW
    , cell_ref( -1, 0 )     // W
    , cell_ref( -1, -1 )    // NW
};

static const byte OPPOSITE_EDGE[8] = {
     4 // N -> S
    ,5 // NE -> SW
    ,6 // E -> W
    ,7 // SE -> NW
    ,0 // S -> N
    ,1 // SW -> NE
    ,2 // W -> E
    ,3 // NW -> NE
};

static const xbool EDGES_ARE_90_OR_MORE[8][8] = {
    /* N  */  { false, false, true, true, true, true, true, false }
    /* NE */, { false, false, false, true, true, true, true, true }
    /* E  */, { true, false, false, false, true, true, true, true }
    /* SE */, { true, true, false, false, false, true, true, true }
    /* S  */, { true, true, true, false, false, false, true, true }
    /* SW */, { true, true, true, true, false, false, false, true }
    /* W  */, { true, true, true, true, true, false, false, false }
    /* NW */, { false, true, true, true, true, true, false, false }
};

    


//----------------------------------------------------------------------
// Given an edge direction, these walk left or right of that direction
static const cell_ref WALK_LEFT_DELTA[8] =
{
    cell_ref( -1, 0 )       // N    ->  W
    , cell_ref( -1, -1 )    // NE   ->  NW
    , cell_ref( 0, -1 )     // E    ->  N
    , cell_ref( 1, -1 )     // SE   ->  NE
    , cell_ref( 1, 0 )      // S    ->  E
    , cell_ref( 1, 1 )      // SW   ->  SE
    , cell_ref( 0, 1 )      // W    ->  S
    , cell_ref( -1, 1 )     // NW   ->  SW
};
    
static const cell_ref WALK_RIGHT_DELTA[8] =
{
    cell_ref( 1, 0 )        // N    ->  E
    , cell_ref( 1, 1 )      // NE   ->  SE
    , cell_ref( 0, 1 )      // E    ->  S
    , cell_ref( -1, 1 )     // SE   ->  SW
    , cell_ref( -1, 0 )     // S    ->  W
    , cell_ref( -1, -1 )    // SW   ->  NW
    , cell_ref( 0, -1 )     // W    ->  N
    , cell_ref( 1, -1 )     // NW   ->  NE
};

//----------------------------------------------------------------------
    

//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

//==============================================================================
//  CLASS FUNCTIONS
//==============================================================================
path_finder::path_finder( void ) :
    m_pTerrain( NULL )
    , m_Nodes( NULL )
    , m_Ed( NULL )
    , m_pPQ( NULL )
    , m_notvisited( NULL )
    , m_retired( NULL )
    , m_pPath( NULL )
    , m_FindingPath( false )
    , m_CellAttributes( NULL )
{
}

path_finder::~path_finder( void )
{
    if ( m_Nodes ) delete [] m_pPQ;
    if ( m_Ed ) delete [] m_pPQ;
    if ( m_pPQ ) delete m_pPQ;
    if ( m_notvisited ) delete m_notvisited;
    if ( m_retired ) delete m_retired;
    if ( m_pPath ) delete m_pPath;
    if ( m_CellAttributes ) delete [] m_CellAttributes;
}

void path_finder::Initialize( terr& Terrain )
{
    m_pTerrain = &Terrain;

    m_CellAttributes = new cell_attribute[MAX_NUM_VERTS];
    ASSERT( m_CellAttributes );

    path_generator::Load( m_CellAttributes, ObjMgr );

#if DEBUG_DRAW_BLOCKED_CELLS
    // build an xarray of cell_refs that are blocked
    for ( s32 i = 0; i < MAX_NUM_VERTS; ++i )
    {
        if ( m_CellAttributes[i].BLOCKED )
        {
            m_BlockedCells.Append( cell_ref( i % 256, i / 256 ) );
        }
    }
#endif
    
}

//==============================================================================
// CostNorm
// Returns an underestimate of the path cost from Src to Dst
f32 path_finder::CostNorm( const cell_ref& Src, const cell_ref& Dst ) const
{
    const f32 DX = (f32)ABS( Src.X - Dst.X );
    const f32 DZ = (f32)ABS( Src.Z - Dst.Z );
    return m_pPath->GetMinDiagCost()
        * MIN( DX, DZ ) + m_pPath->GetMinAxialCost() * ABS( DX - DZ );
}

xbool path_finder::InitializeFindPath( s16 VertexDensity
    , const vector3& From
    , const vector3& To )
{
    if ( m_FindingPath )
    {
//        bot_log::Log( "UNABLE TO FIND PREVIOUS PATH!\n\n" );
    }
        
    ASSERT( From != To );
/*    bot_log::Log( "InitializeFindPath:\n" );
    bot_log::Log( "    Density: %i\n", VertexDensity );
    bot_log::Log( "    From:    ( %7.2f, %7.2f, %7.2f ) To: (%7.2f, %7.2f, %7.2f ) )\n"
        , From[0], From[1], From[2], To[0], To[1], To[2] );
    bot_log::Log( "    Distance: %7.2f  --  about %i nodes\n"
        , (From - To).Length()
        , (int)((From - To).Length() / (VertexDensity * 8)) );

    bot_log::Log( "\n" );
    
 */   if ( m_pPath ) delete m_pPath;
    m_pPath = new bot_path( VertexDensity );
    ASSERT( m_pPath );

    // Find the beginning and ending vertices
    m_From = From;
    m_To = To;
    GetNearestVertex( m_crBeg, m_From, m_pPath->GetDensity() );
    GetNearestVertex( m_crEnd, m_To, m_pPath->GetDensity() );

    if ( m_crBeg == m_crEnd )
    {
        // The path shouldn't have any nodes -- just move btw. From and To
        return true;
    }
    
    m_Timer.Stop();
    m_Timer.Reset();
    m_Timer.Start();
    m_TimeLastUpdateEndedMs = -MS_BETWEEN_UPDATES; // force first update
    
#if TIMING_PATH_FINDER
    x_printf( "InitializeFindPath() called...\n" );
    m_DebugTimer.Stop();
    m_DebugTimer.Reset();
    m_DebugTimer.Start();
    m_DebugNumUpdates = 0;
    m_DebugNumExtractedNodes = 0;
#endif
    
    if ( m_Nodes ) delete [] m_Nodes;
    m_Nodes = new PFibHeapNode[m_pPath->GetTotalCells()];
    ASSERT( m_Nodes );
    
    if ( m_Ed ) delete [] m_Ed;
    m_Ed = new edge_dir[m_pPath->GetTotalCells()];
    ASSERT( m_Ed );
    
    m_pU = m_pV = m_pReuse = NULL;

    // dummy placeholders for notvisited and retired "keywords"
    if ( m_notvisited ) delete m_notvisited;
    m_notvisited = new FibHeapNode;
    ASSERT( m_notvisited );

    if ( m_retired ) delete m_retired;
    m_retired = new FibHeapNode;
    ASSERT( m_retired );


    s32 i;
    const s32 NumCells = m_pPath->GetTotalCells();

    // Set all node ptrs to notvisited
    for ( i = 0; i < NumCells; i++ )
    {
        m_Nodes[i] = m_notvisited;
        m_Ed[i] = INVALID_EDGE;
    }

    // Insert the original node
    m_Nodes[IndexOf( m_crBeg )] = m_pU = new FibHeapNode;
    m_pU->cr = m_crBeg;
    m_pU->g = 0;
    m_pU->h = CostNorm( m_crBeg, m_crEnd );
    if ( m_pPQ ) delete m_pPQ;
    m_pPQ = new FibHeap;
    ASSERT( m_pPQ );

    m_pPQ->Insert( m_pU );

    m_FindingPath = true;

    return false;
}

xbool path_finder::Update( void )
{
    if ( !m_FindingPath )
    {
        // we shouldn't have been called
        //ASSERT( 0 );
        return false;
    }

    f32 StartTimeMs = m_Timer.ReadMs();

    if ( StartTimeMs - m_TimeLastUpdateEndedMs < MS_BETWEEN_UPDATES )
    {
        // gotta wait longer
        return false;
    }
    
#if TIMING_PATH_FINDER
    ++m_DebugNumUpdates;
#endif

    FibHeapNode temp;
    graph::node PETempNode;

    // While there are still nodes to visit, visit them
    while ( m_Timer.ReadMs() - StartTimeMs <= MAX_SEARCH_MS_PER_UPDATE
        && (m_pU = m_pPQ->ExtractMin()) != NULL ) 
    {
        // If we're at the destination, then exit the while loop
        m_crU = m_pU->cr;

#if DEBUG_DRAW_EXTRACTED_NODES
        m_ExtractedNodes.Append( m_crU );
#endif

#if TIMING_PATH_FINDER
        ++m_DebugNumExtractedNodes;
#endif

        if ( m_crU == m_crEnd ) {
            m_Cost = m_pU->g;               // Final path cost!
            if ( m_pU ) { delete m_pU; m_pU = NULL; }
            break;
        }

        m_Nodes[IndexOf( m_crU )] = m_retired;

        // Examine possible paths from U to its neighbors
        for ( edge_dir edp = 0; edp < NUM_EDGE_DIRECTIONS; edp++ )
        {
            // Compute new least cost path candidate
            m_Cost = m_pU->g + EdgeCost(
                m_crV
                , m_crU
                , edp
                , m_Ed[IndexOf( m_crU )] );

            // If it's off limits (e.g. outside the map), then skip it
            if (m_Cost >= INFINITY)
            {
                continue;
            }

            m_iV = IndexOf( m_crV );
            m_pV = m_Nodes[m_iV];

            if ( m_pV == m_retired )
            {
                continue;   // Can't improve it!
            }

            if ( m_pV == m_notvisited )
            {       // First time,add to queue
                if ( m_pReuse != NULL )
                {
                    m_pV = m_pReuse;
                    m_pReuse = NULL;
                }
                else
                {
                    m_pV = new FibHeapNode;
                    ASSERT( m_pV );
                }
                
                m_Nodes[m_iV] = m_pV;
                m_Ed[m_iV] = edp;
                m_pV->cr = m_crV;
                m_pV->g = m_Cost;
                m_pV->h = CostNorm( m_crV, m_crEnd );
                m_pPQ->Insert( m_pV );
            }
            else if ( m_Cost < m_pV->g )        // Can be improved!?
            {   
                m_Ed[m_iV] = edp;               // Update neighbor
                temp = *m_pV;
                temp.g = m_Cost;

                m_pPQ->DecreaseKey( m_pV, temp );
            }
        }
    }

    if ( m_Timer.ReadMs() - StartTimeMs <= MAX_SEARCH_MS_PER_UPDATE ) 
    {
        edge_dir* pedEdges;

        if ( m_crU == m_crEnd )
        {               // We did find a path!
            m_FindingPath = false;
            // Count the number of edges by backtracking the path
            s32 NumEdges;
            for ( NumEdges = 0; m_crU != m_crBeg; NumEdges++ )
            {
                WalkEdge( m_crU, m_crU, OPPOSITE_EDGE[m_Ed[IndexOf( m_crU )]] );
                m_pPath->Append();
            }

            // Construct the edge path by backtracking & reversing...
            pedEdges = new edge_dir[NumEdges];
            edge_dir *ped = pedEdges + NumEdges;
            s32 index = NumEdges - 1;
            for ( m_crU = m_crEnd; m_crU != m_crBeg; )
            {
                --ped;
                *ped = m_Ed[IndexOf( m_crU )];
                WalkEdge( m_crU, m_crU, OPPOSITE_EDGE[*ped] );
                PETempNode.Pos.X = m_crU.X * (f32)m_pPath->GetVertexSpacing();
                PETempNode.Pos.Y = m_pTerrain->GetVertHeight(
                    m_crU.Z * m_pPath->GetDensity()
                    , m_crU.X * m_pPath->GetDensity() );
                PETempNode.Pos.Z = m_crU.Z * (f32)m_pPath->GetVertexSpacing();
                
                (*m_pPath)[index] = PETempNode;
                --index;
            }

        }
        else
        {
            // No path found
//            bot_log::Log( "Unable to find path\n" );
            if ( m_Ed ) {
                delete[] m_Ed;
                m_Ed = NULL;
            }
            
            pedEdges = NULL;
            m_Cost = 0.0f;

            // Let's see if we can find a path with smaller vertex spacing
            if ( m_pPath->GetDensity() > 1 )
            {
//                bot_log::Log( "Reinitializing...\n\n" );
                InitializeFindPath( m_pPath->GetDensity() / 2, m_From, m_To );
            }
            else
            {
                ASSERT( 0 ); // We need to handle this case: no path possible
            }
            
            return false;
        }

        // Free up some memory (PQ frees up its queued nodes itself)
        if ( m_pReuse ) { delete m_pReuse; m_pReuse = NULL; }
        if ( m_Nodes ) { delete[] m_Nodes; m_Nodes = NULL; }
        if ( m_Ed ) { delete[] m_Ed; m_Ed = NULL; }
        if ( m_retired ) { delete m_retired; m_retired = NULL; }
        if ( m_notvisited ) { delete m_notvisited; m_notvisited = NULL; }
#if TIMING_PATH_FINDER
        x_printf( "FindPath(): %fms\n", m_DebugTimer.StopMs() );
        //x_printf( "Update was called %d times\n", m_DebugNumUpdates );
        //x_printf( "Final Path Nodes: %d\n", m_pPath->GetCount() );
        x_printf( "Checked %d nodes\n", m_DebugNumExtractedNodes );
#endif
        return (m_Cost != 0.0f);
    }
    
    return false;
}

void path_finder::GetNearestVertex(
    cell_ref& Cell
    , const vector3& Point
    , s32 Density ) const
{
    // try the "ideal" nearest cell first
    Cell.X = (s32)(Point[0] + 0.5f) / (s32)m_pPath->GetAxialDist();
    Cell.Z = (s32)(Point[2] + 0.5f) / (s32)m_pPath->GetAxialDist();

    if ( !CellIsBlocked( Cell, Density ) ) return;

    // if this cell is blocked, try cells spiraling outward until we find one
    // that isn't blocked
    s32 CenterX = Cell.X;
    s32 CenterZ = Cell.Z;
    
    s32 SearchRadius = 0;
    s32 SearchRadius2;
    s32 i, DX=0, DZ=0;
    byte SearchSide; // N, E, S, W

    while ( CellIsBlocked( Cell, Density ) )
    {
        ++SearchRadius;
        SearchRadius2 = SearchRadius * 2;

        // Check sides of a square
        for ( SearchSide = 0
            ; CellIsBlocked( Cell, Density ) && SearchSide < 4
            ; ++SearchSide )
        {
            // Set up X, Y, DX, and DZ to loop through a side of the square
            switch ( SearchSide )
            {
            case 0: // North
                Cell.X = CenterX - SearchRadius;
                Cell.Z = CenterZ - SearchRadius;

                DX = 1;
                DZ = 0;
                break;
            case 1: // East
                Cell.X = CenterX + SearchRadius;
                Cell.Z = CenterZ - SearchRadius;

                DX = 0;
                DZ = 1;
                break;
            case 2: // South
                Cell.X = CenterX + SearchRadius;
                Cell.Z = CenterZ + SearchRadius;

                DX = -1;
                DZ = 0;
                break;
            case 3: // West
                Cell.X = CenterX - SearchRadius;
                Cell.Z = CenterZ + SearchRadius;

                DX = 0;
                DZ = -1;
                break;
            default:
                ASSERT( 0 ); // invalid axial direction
            }

            // Check the cells in a side
            for ( i = 0; (CellIsBlocked( Cell, Density ) || !IsCellRefValid( Cell, Density ))
                      && i < SearchRadius2
                      ; ++i )
            {
                if ( CellIsBlocked( Cell, Density ) || !IsCellRefValid( Cell, Density ) )
                {
                    Cell.X += DX;
                    Cell.Z += DZ;
                }
            }
        }
    }

    ASSERT( !CellIsBlocked( Cell, Density ) );
    ASSERT( IsCellRefValid( Cell, Density ) );
}

//======================================================================
// EdgeCost()
// Calculate the cost function
//======================================================================
f32 path_finder::EdgeCost(
    cell_ref &crDst          // Out: destination cell for edge
    , const cell_ref& crSrc  // In:  source cell
    , edge_dir ed            // In:  edge direction
    , edge_dir PrevDir       // In:  direction into source cell
    )
{
    // Walk to the destination cell index
    crDst = crSrc + WALK_EDGE_DELTA[ed];    // 2d index + operator

    // Border cell modifier, return INFINITY if outside out graph
    if ( !IsCellRefValid( crDst, m_pPath->GetDensity() )
        || CellIsBlocked( crDst, m_pPath->GetDensity() )
        || EdgeIsBlocked( crSrc, crDst, m_pPath->GetDensity() ) )
    {
        return INFINITY;
    }
    
    f32 Cost = m_pPath->GetCost( ed & 1 );

    //----------------------------------------------------------------------
    // account for forward slope
    //----------------------------------------------------------------------
    const f32 HeightDifference
        = m_pTerrain->GetVertHeight(
                    crDst.Z * m_pPath->GetDensity()
                    , crDst.X * m_pPath->GetDensity() )
                - m_pTerrain->GetVertHeight(
                    crSrc.Z * m_pPath->GetDensity()
                    , crSrc.X * m_pPath->GetDensity() )
            + GetTotalHeightClimbed( crDst );

    if ( HeightDifference > 0 )
    {
        Cost += (HeightDifference * ASCENT_COST_WEIGHT);
    }

    //----------------------------------------------------------------------
    // account for lateral slope (across Dst vert & perp to this edge)
    //----------------------------------------------------------------------
    const cell_ref crLeftCell = crDst + WALK_LEFT_DELTA[ed];
    const cell_ref crRightCell = crDst + WALK_RIGHT_DELTA[ed];
    const s32 LeftX = crLeftCell.X;
    const s32 LeftZ = crLeftCell.Z;
    const s32 RightX = crRightCell.X;
    const s32 RightZ = crRightCell.Z;
    
    const f32 LateralHeightDifference = ABS(
        m_pTerrain->GetVertHeight( LeftZ, LeftX )
        - m_pTerrain->GetVertHeight( RightZ, RightX ) );

    Cost += LateralHeightDifference * LATERAL_SLOPE_COST_WEIGHT;

    //----------------------------------------------------------------------
    // account for path turning
    //----------------------------------------------------------------------
    if ( PrevDir != ed )
    {
        // This is an elbow
        if ( EDGES_ARE_90_OR_MORE[PrevDir][ed] )
        {
            Cost += PATH_TURN_COST * 100;
        }
        else 
        {
            Cost += PATH_TURN_COST;
        }
    }
    
    
    //----------------------------------------------------------------------
    // Clamp to infinity
    //----------------------------------------------------------------------
    if ( Cost > INFINITY ) 
    {
        Cost = INFINITY;
    }

    
#if 0  // Original code from Markus' paper
    CellAttr    &caSrc = AttributesFor( crSrc );
    CellAttr    &caDst = AttributesFor( crDst );
    
    // Apply road modifier
    if ((caSrc.cRoadType != noroad) && (caDst.cRoadType != noroad))
    {
        Cost = dwRoadEdgeCost[ed&1][caSrc.cRoadType];
    }
    else
    {
        // else apply slope modifier
        if ( ABS( caSrc.wHeight-caDst.wHeight ) > dwMaxHeightDiff[ed&1] )
        {
            return INFINITY;
        }
        // else compute the basic terrain cost
        Cost = dwTerrainHalfEdgeCost[ed&1][caSrc.cTerrainType] +
            dwTerrainHalfEdgeCost[ed&1][caDst.cTerrainType]; 
    }

    // Apply enemy modifier
    if ( !caDst.bHaveVisCalc )
    { // Lazy evaluation of the visibility
        caDst.bVisible = IsObserved(crDst); // Uses DDA trace alg.
        caDst.bHaveVisCalc = true;
    }
    if ( caDst.bVisible ) {
        Cost += dwVisibilityCost;
        if ( Cost > MAXCOST ) Cost = MAXCOST;
    }
#endif

    // Done!
    return Cost;
}

void path_finder::WalkEdge( cell_ref& Dst
    , const cell_ref& Src
    , edge_dir Dir ) const
{
    Dst = Src + WALK_EDGE_DELTA[Dir];
}

void path_finder::CellRefOf( cell_ref& Cell, s32 Index ) const
{
    Cell.Z = Index / m_pPath->GetNumVertsHoriz();
    Cell.X = Index - (Cell.Z * m_pPath->GetNumVertsHoriz());
}


#if DEBUG_DRAW_EXTRACTED_NODES
void path_finder::DrawExtractedNodes( void ) const
{
    if ( m_FindingPath ) return;
    
    eng_Begin();
    for ( s32 i = 0; i < m_ExtractedNodes.GetCount(); ++ i ) 
    {
        draw_Point(
        vector3( (f32)m_ExtractedNodes[i].X * cell_ref::VERTEX_SPACING
            , m_pTerrain->GetVertHeight(
                m_ExtractedNodes[i].Z * m_pPath->GetDensity()
                , m_ExtractedNodes[i].X * m_pPath->GetDensity() )
            + 1.0f
            , (f32)m_ExtractedNodes[i].Z * cell_ref::VERTEX_SPACING )
        , XCOLOR_RED );

    }
    eng_End();
}
#endif

xbool path_finder::IsCellRefValid( cell_ref Cell, s32 Density) const
{
    const s32 NumVerts = 256 / Density;
    return ( Cell.X >= 0 && Cell.Z >= 0
        && Cell.X < NumVerts
        && Cell.Z < NumVerts );
}

xbool path_finder::CellIsBlocked( const cell_ref& Cell, s32 Density ) const
{
    return ( m_CellAttributes[(Cell.Z * Density) * 256
        + (Cell.X * Density)].BLOCKED );
}

xbool path_finder::EdgeIsBlocked( const cell_ref& Src
    , const cell_ref& Dst
    , s32 Density ) const
{
    // ONLY CHECKS CELLS UNDER THE EDGE, NOT THE ENDPOINTS
    const s32 DX = Src.X < Dst.X
        ? 1
        : ((Src.X > Dst.X)
            ? -1
            : 0);
    const s32 DZ = Src.Z < Dst.Z
        ? 1
        : ((Src.Z > Dst.Z)
            ? -1
            : 0);
    
    s32 X = (Src.X * Density) + DX;
    s32 Z = (Src.Z * Density) + DZ;
    const s32 EndX = Dst.X * Density;
    const s32 EndZ = Dst.Z * Density;

    while ( X != EndX && Z != EndZ )
    {
        if ( m_CellAttributes[Z * 256 + X].BLOCKED )
        {
            return TRUE;
        }
        X += DX;
        Z += DZ;
    }

    return FALSE;
}

f32 path_finder::GetTotalHeightClimbed( const cell_ref& Cell ) const 
{
    (void)Cell;
    return( 0.0f );
}

f32 path_finder::GetElapsedTime( void )
{
    return( m_Timer.ReadMs() );
}

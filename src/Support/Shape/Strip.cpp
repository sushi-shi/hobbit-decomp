#include <Support/Shape/Strip.hpp>



//==============================================================================

// Constructor
strip::strip()
{
    // Init
    m_Processed = FALSE ;
    m_Orient      = 0 ;    
    m_Node      = -1 ;
    m_Verts     = NULL ;
    m_nVerts    = 0 ;
}

//==============================================================================

// Destructor
strip::~strip()
{
    // Free verts
    if (m_Verts)
        delete [] m_Verts ;
}

//==============================================================================

// Allocated verts
void strip::SetVertCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old verts
    if (m_Verts)
    {
        delete [] m_Verts ;
        m_Verts = NULL ;
    }

    // Allocate new verts
    if (Count)
    {
        m_Verts = new strip_vert[Count] ;
        ASSERT(m_Verts) ;
    }

    m_nVerts = Count ;
}

//==============================================================================

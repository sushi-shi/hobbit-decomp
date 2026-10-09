#include <Support/Shape/Node.hpp>




//==============================================================================
// DEFINES
//==============================================================================
#define NODE_MIN_BOUNDS_AXIS_DIST   0.02f  // Min thickness of a node bounds axis



//==============================================================================
// FUNCTIONS
//==============================================================================

//==============================================================================

// Constructor
node::node()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(node) & 15) == 0) ;

    // Init
    m_Type              = 0 ;
    m_Parent            = -1 ;

    //m_mBind.Identity() ;
    //m_mInvBind.Identity() ;
    
    //m_vBindScale        = vector3(1.0f, 1.0f, 1.0f) ;
    //m_vBindPos          = vector3(0.0f, 0.0f, 0.0f) ;
    //m_qBindRot.Identity() ;
    
    m_Flags.ReflectionMapped = FALSE ;
    m_Flags.DoNotDelete      = FALSE ;

    m_UserData          = NULL ;
    m_UserDataSize      = 0 ;
}

//==============================================================================

// Copy a node
node&  node::operator =  ( node& Node )
{
    x_memcpy((void *)this, (void *)&Node, sizeof(node)) ;
    return *this ;
}

//==============================================================================

// Calculates binding matrices (ie. the orientation when the skin was attached)
//void node::CalcBindMatrices()
//{
    //matrix4 mScale, mRot, mPos ;
    
    // Build orientation matrices
    //mScale.Identity() ;
    //mScale.SetScale(m_vBindScale) ;
    
    //mRot.Identity() ;
    //mRot.Setup(m_qBindRot) ;

    //mPos.Identity() ;
    //mPos.SetTranslation(m_vBindPos) ;

    // Calc bind matrix
    //m_mBind = mPos * mScale * mRot ; // xfiles correct (Right->Left)!
    
    // Calc inverse bind matrix
    //m_mInvBind = m_mBind ;
    //m_mInvBind.Invert() ;
//}

//==============================================================================

// Sets up bounds of node
void node::CalculateBounds(s32 NodeIndex, vert Verts[], s32 nVerts)
{
    s32 i ;
    xbool HasGeometry = FALSE ;

    m_Bounds.Clear() ;

    // Loop through all verts in model
    for (i = 0 ; i < nVerts ; i++)
    {
        vert &Vert = Verts[i] ;
        
        // Is vert attached to this node?
        if (Vert.GetNode() == NodeIndex)
        {
            // Add to bounds
            m_Bounds += Vert.GetPos() ;

            // Flags is has geometry
            HasGeometry = TRUE ;
        }
    }

    // If no geometry is attached to this node, make a valid bound
    if (!HasGeometry)
        m_Bounds += vector3(0.0f, 0.0f, 0.0f) ;

    // Make sure bounds have some thickness on every axis for collision
    for (i = 0 ; i < 3 ; i++)
    {
        // Axis too thin?
        if ((m_Bounds.Max[i] - m_Bounds.Min[i]) < NODE_MIN_BOUNDS_AXIS_DIST)
        {
            // Expand axis
            m_Bounds.Min[i] -= NODE_MIN_BOUNDS_AXIS_DIST / 2.0f ;
            m_Bounds.Max[i] += NODE_MIN_BOUNDS_AXIS_DIST / 2.0f ;
        }
    }
}

//==============================================================================

void node::SetupUserData(void *UserData, s32 UserDataSize)
{
    ASSERT(UserData) ;
    ASSERT(UserDataSize) ;

    // Allocate user data
    m_UserData = new byte[UserDataSize] ;
    ASSERT(m_UserData) ;
    m_UserDataSize = UserDataSize ;

    // Copy user data
    x_memcpy(m_UserData, UserData, UserDataSize) ;
}

//==============================================================================

void node::ReadWrite(shape_bin_file &File)
{
    // Read/Write attributes
    File.ReadWrite(m_Type) ;
    File.ReadWrite(m_Parent) ;
    //File.ReadWrite(m_vBindScale) ;
    //File.ReadWrite(m_vBindPos) ;
    //File.ReadWrite(m_qBindRot) ;
    File.ReadWrite(m_Flags) ;
    File.ReadWrite(m_Bounds) ;

    // User data
    File.ReadWriteStructArray(m_UserData, m_UserDataSize) ;

    // Get ready...
    //CalcBindMatrices() ;
}

//==============================================================================

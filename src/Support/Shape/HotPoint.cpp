// HotPoint.cpp
//

#include <Support/Shape/HotPoint.hpp>

   

//==============================================================================
// Functions
//==============================================================================

// Default constructor
hot_point::hot_point()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(hot_point) & 15) == 0) ;

    m_Node = -1 ;
	m_vPos = vector3(0.0f, 0.0f, 0.0f) ;
    m_qDir.Identity() ;
}


//==============================================================================

void hot_point::ReadWrite(shape_bin_file &File)
{
    // ReadWrite attributes
    File.ReadWrite(m_Node) ;
    File.ReadWrite(m_Type) ;
    File.ReadWrite(m_vPos) ;
    File.ReadWrite(m_qDir) ;
}

//==============================================================================


#ifndef __HOT_POINT_H__
#define __HOT_POINT_H__


#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/ShapeBinFile.hpp>


//==============================================================================
// Classes
//==============================================================================

// Hot point - a point attached to the hierarchy
class hot_point : public shape_bin_file_class
{
	
// Members
private:
    quaternion  m_qDir ;            // Local direction
    vector3     m_vPos ;            // Local pos
    s32         m_Node ;			// Node point is attached too
    s32         m_Type ;            // Type of hot point

    byte        PADDING[8] ;        // Align to 16 bytes for shape_bin_file_class

// Functions
public:
    // Default constructor
    hot_point() ;

    // Inline functions
    s32         GetNode ()                          { return m_Node ; }
    s32         GetType ()                          { return m_Type ; }
    vector3     GetPos  ()                          { return m_vPos ; }
    quaternion  GetDir  ()                          { return m_qDir ; }
                                                
    void        SetNode (s32 Node)                  { m_Node = Node ; }
    void        SetType (s32 Type)                  { m_Type = Type ; }
    void        SetPos  (const vector3& vPos)       { m_vPos = vPos ; }
    void        SetDir  (const quaternion& qDir)    { m_qDir = qDir ; }

    // File IO
    void        ReadWrite               (shape_bin_file &File) ;

} ;


#endif	//#define __HOT_POINT_H__

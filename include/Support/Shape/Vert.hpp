#ifndef __VERT_HPP__
#define __VERT_HPP__

#include <xCore/x_files/x_files.hpp>


//==============================================================================

// Vertex
class vert
{
// Data
private:
    vector3 m_vPos ;        // Local/world co-ordinated
    s32     m_Node ;        // Node, vertex is attached to
    vector3 m_vNormal ;     // Normal
    vector2 m_vTexCoord ;   // Texture co-ordinates

// Functions
public:
    
    // Inline functions
    s32     MemoryUsed  ()                          { return sizeof(vert) ; }
                                                    
    vector3 GetPos      ()                          { return m_vPos ;       }
    vector3 GetNormal   ()                          { return m_vNormal ;    }
    vector2 GetTexCoord ()                          { return m_vTexCoord ;  }
    s32     GetNode     ()                          { return m_Node ;       }

    void    SetPos      (const vector3& vPos)       { m_vPos      = vPos ;      }
    void    SetNormal   (const vector3& vNormal)    { m_vNormal   = vNormal ;   }
    void    SetTexCoord (const vector2& vTexCoord)  { m_vTexCoord = vTexCoord ; }
    void    SetNode     (s32 Node)                  { m_Node      = Node ;      }
    
} ;

//==============================================================================


#endif  //#ifndef __VERT_HPP__

#ifndef __POLY_HPP__
#define __POLY_HPP__


#include <xCore/x_files/x_files.hpp>


//==============================================================================


// Poly
class poly
{
// Data
private:
    s32 m_Verts[3] ;    // Vertex indices
    s32 m_Material ;    // Material index

// Functions
public:

    // Inline functions

    // Returns memory used
    s32     MemoryUsed      ()      { return sizeof(poly) ; }
    
    // Sets vertex index
    void SetVert(s32 Index, s32 VertIndex)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < 3) ;

        m_Verts[Index] = VertIndex ;
    }

    // Returns vertex index
    s32 GetVert(s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < 3) ;

        return m_Verts[Index] ;            
    }

    // Sets material index
    void SetMaterial(s32 Index)
    {
        ASSERT(Index != -1) ;
        m_Material = Index ;
    }

    // Returns material index
    s32 GetMaterial()
    {
        return m_Material ;
    }

} ;

//==============================================================================


#endif  //#ifndef __POLY_HPP__

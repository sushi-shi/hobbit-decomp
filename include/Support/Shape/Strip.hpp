#ifndef __STRIP_HPP__
#define __STRIP_HPP__

#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/Vert.hpp>

//==============================================================================

// Strip vert
class strip_vert
{
// Data
private:
    s32     m_VertIndex ;   // Vert index
    xbool   m_NoKick ;      // No kick (don't trigger a triangle draw) flag
    xbool   m_Orient ;      // Orient direction

// Functions
public:

    void    SetVertIndex(s32 VertIndex)   {   m_VertIndex = VertIndex ;   }
    void    SetNoKick(xbool NoKick)       {   m_NoKick    = NoKick ;      }
    void    SetOrient(xbool Orient)       {   m_Orient    = Orient ;      }

    s32     GetVertIndex()                {   return m_VertIndex ;        }
    xbool   GetNoKick()                   {   return m_NoKick ;           }
    xbool   GetOrient()                   {   return m_Orient ;           }
} ;


//==============================================================================

// Strip
class strip
{
// Data
private:
    xbool       m_Processed ;   // TRUE if strip has been processed
    s32         m_Node ;        // Node strip is attached too
    s32         m_Orient ;        // Orienting direction    
    s32         m_nVerts ;      // Number of vertices in strip
    strip_vert  *m_Verts ;      // Array of strip verts

// Functions
public:
    strip() ;
    ~strip() ;

    // Allocated verts
    void    SetVertCount    (s32 nVerts) ;

    // Inline functions
    void    SetProcessed(xbool Processed) { m_Processed = Processed ; }
    xbool   IsProcessed()                 { return m_Processed ; }

    void    SetOrient(s32 Orient)     { m_Orient = Orient ; }
    s32     GetOrient()             { return m_Orient ; }

    void    SetNode(s32 Node)     { m_Node = Node ; }
    s32     GetNode()             { return m_Node ; }

    // Returns memory used
    s32     MemoryUsed      ()      { return sizeof(strip) + (m_nVerts*sizeof(s32)) ; }
    
    // Returns number of vertices in strip
    s32     GetVertCount    ()      { return m_nVerts ; }

    // Returns vertex index
    strip_vert &GetVert     (s32 Vert)
    {
        ASSERT(Vert >= 0) ;
        ASSERT(Vert < m_nVerts) ;
        return m_Verts[Vert] ;
    }

    // Sets vert
    void    SetVert(s32 Vert, s32 VertIndex, xbool NoKick, xbool Orient)
    {
        ASSERT(Vert >= 0) ;
        ASSERT(Vert < m_nVerts) ;
        m_Verts[Vert].SetVertIndex(VertIndex) ;
        m_Verts[Vert].SetNoKick(NoKick) ;
        m_Verts[Vert].SetOrient(Orient) ;
    }

} ;

//==============================================================================



#endif  //#ifndef __STRIP_HPP__

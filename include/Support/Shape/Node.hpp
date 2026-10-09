#ifndef __NODE_HPP__
#define __NODE_HPP__


#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/Vert.hpp>
#include <Support/Shape/ShapeBinFile.hpp>


//==============================================================================
// Defines
//==============================================================================


//==============================================================================
// Classes
//==============================================================================

// Node class
class node : public shape_bin_file_class
{
// Structures
private:

	// Material flags
	struct flags
	{
        u32 DoNotDelete:1 ;         // Does not get deleted from models
        u32 ReflectionMapped:1 ;    // Needs reflection map matrix
	} ;

// Data
private:
    
    // NOTE: If you use vu0 with these matrices, the must be 16 byte aligned
    //matrix4     m_mBind ;                   // Binding orientation for skin
    //matrix4     m_mInvBind ;                // Inverse of bind matrix

    s32         m_Type ;                    // Type of node
    s32         m_Parent ;                  // Parent node index, or -1 if non
    flags       m_Flags ;                   // Misc flags
    
    bbox        m_Bounds ;                  // Bounds of node

    byte        *m_UserData ;               // Special case user data can be attached here
    s32         m_UserDataSize ;            // Length (in bytes) of user data (if any)

    //vector3     m_vBindScale, m_vBindPos ;  // Bind orientation
    //quaternion  m_qBindRot ;                // Bind orientation

    //byte        PADDING[16] ;                // Align to 16 bytes for shape_bin_file_class


// Functions
public:

    // Constructor/destructor
    node() ;

    // Operators
    node&  operator =  ( node& Node );

    // Builds bind and inverse bind matrix
    //void        CalcBindMatrices() ;

    // Sets up bounds of node
    void        CalculateBounds         (s32 NodeIndex, vert Verts[], s32 nVerts) ;

    // Allocates and copies user data
    void        SetupUserData(void *UserData, s32 UserDataSize) ;

    // Inline functions

    s32         MemoryUsed              ()                  { return sizeof(node) ;         }

    s32         GetType                 () const            { return m_Type ;               }
    xbool       IsReflectionMapped      ()                  { return m_Flags.ReflectionMapped ;  }
    xbool       DoNotDelete             ()                  { return m_Flags.DoNotDelete ;  }
    s32         GetParent               ()                  { return m_Parent ;             }

    //matrix4     &GetBindOrient          ()                  { return m_mBind ;              }
    //matrix4     &GetInvBindOrient       ()                  { return m_mInvBind ;           }

    //vector3     GetBindScale            ()                  { return m_vBindScale ;         }
    //vector3     GetBindPos              ()                  { return m_vBindPos ;           }
    //quaternion  GetBindRot              ()                  { return m_qBindRot ;           }

    void        SetType                 (s32 Type)          { m_Type = Type ;               }
    void        SetReflectionMapped     (xbool Value)       { m_Flags.ReflectionMapped = Value ; }
    void        SetDoNotDelete          (xbool Value)       { m_Flags.DoNotDelete = Value ; }
    void        SetParent               (s32 Parent)        { m_Parent = Parent ;           }
               
    //void        SetBindScale            (vector3 vScale)    { m_vBindScale = vScale ;       }
    //void        SetBindPos              (vector3 vPos)      { m_vBindPos = vPos ;           }
    //void        SetBindRot              (quaternion qRot)   { m_qBindRot = qRot ;           }

    bbox        &GetBounds              ()                  { return m_Bounds ;             }

    void        *GetUserData            ()                  { return m_UserData ;           }

    // File IO
    void        ReadWrite               (shape_bin_file &File) ;
} ;


//==============================================================================
// Structures used when rendering
//==============================================================================

// Used for environment mapping on PS2 
// (Since there is no translation, there's a register saved)
struct render_ref_matrix
{
// Data
    f32     M[3][4];

// Functions
    f32     operator () ( s32 Column, s32 Row ) const { return M[Column][Row] ; }
    f32&    operator () ( s32 Column, s32 Row ) { return M[Column][Row] ; }

} ;

// Simple light struct
struct render_light
{
    vector4 Dir ;  // Light direction in world space (w=0)
    vector4 Col ;  // Light color (w=0)
    vector4 Amb ;  // Light ambient color (w=alpha)
} ;

// Collection of info needed to transfrom verts for each node
struct render_node
{
    matrix4         L2W ;       // Local to world matrix
    render_light    Light ;     // Render light
} ;


//==============================================================================
//  Types
//==============================================================================

// User function for messing with node orientation
typedef void node_fn(const void *Param, const node &Node, matrix4 &mOrient) ;

// User function for messing with render node
typedef void render_node_fn(const void *Param, const node &Node, render_node &RenderNode) ;



//==============================================================================




#endif  //#ifndef __NODE_HPP__

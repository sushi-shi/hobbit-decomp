#ifndef __MODEL_HPP__
#define __MODEL_HPP__

#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/Node.hpp>
#include <Support/Shape/Vert.hpp>
#include <Support/Shape/Poly.hpp>
#include <Support/Shape/Texture.hpp>
#include <Support/Shape/Material.hpp>
#include <Support/Shape/Anim.hpp>
#include <Support/Shape/HotPoint.hpp>



//==============================================================================
// Forward references
//==============================================================================
class shape ;
class shape_instance ;
struct shape_load_settings ;

//==============================================================================
// Defines
//==============================================================================

#define MODEL_MAX_FNAME 48


//==============================================================================
// Classes
//==============================================================================

// Model class
class model : public shape_bin_file_class
{
// Data
private:
    char                m_Name[MODEL_MAX_FNAME] ;       // Filename
    time_stamp          m_TimeStamp ;                   // Time stamp from .skel file
                        
    s32                 m_Type ;                        // Type of model
    f32                 m_Weight ;                      // Weight used for choosing same types
                        
    s32                 m_nNodes ;                      // Number of nodes
    node                *m_Nodes ;                      // Array of nodes
                        
    s32                 m_nVerts ;                      // Number of verts
    vert                *m_Verts ;                      // Array of verts
                                                        
    s32                 m_nPolys ;                      // Number of polys
    poly                *m_Polys ;                      // Array of poly
                        
    s32                 m_nHotPoints ;                  // Number of hot points
    hot_point           *m_HotPoints ;                  // Array of hot points

    s32                 m_nTextureRefs ;                // Number of texture refs
    shape_texture_ref   *m_TextureRefs ;                // Array of texture references
    
    s32                 m_nMaterials ;                  // Number of materials
    material            *m_Materials ;                  // Array of materials
                                                       
    xbool               m_bReflectionMapped ;           // TRUE if this model has any reflection mapped material on it
                        
    f32                 m_PrecisionScale ;              // Scale matrices down by this
                        
    bbox                m_Bounds ;                      // Bounds of model
                        
    f32                 m_WorldPixelSize ;              // Used for mip-mapping
                        
    s32                 m_nDListVerts, m_nDListPolys ;  // Display list stats

    byte                PADDING[4] ;                    // Align to 16 bytes for shape_bin_file_class

// Functions
public:
    // Constructor/destructor
    model() ;
    ~model() ;

    // Initialization functions
    void        SetName                     (const char *Name) ;
    void        SetTimeStamp                (time_stamp TimeStamp)  { m_TimeStamp = TimeStamp ; }
                
    void        SetType                     (s32 Type)          { m_Type = Type ;           }
    void        SetWeight                   (f32 Weight)        { m_Weight = Weight ;       }
    void        SetNodeCount                (s32 Count) ;
    void        SetVertCount                (s32 Count) ;
    void        SetPolyCount                (s32 Count) ;
    void        SetHotPointCount            (s32 Count) ;
    void        SetTextureRefCount          (s32 Count) ;
    s32         AddTextureRef               (const char *Name) ;
    void        SetMaterialCount            (s32 Count) ;

    xbool       IsNodeUsed                  (s32 Index ) ;        
    void        DeleteNode                  (s32 Index) ;

    void        CleanupUnusedMemory         () ;
    s32         MemoryUsed                  () ;
    void        Init                        () ;

	void		SortMaterials				(texture_manager& TextureManager) ;
    
    void        LoadTextures                (texture_manager&               TextureManager, 
                                             const shape_load_settings&     LoadSettings) ;

    // Returns array of node matrices, given the anim and frame (and optional parent transformation matrix)
    matrix4*    GetNodeMatrices             (anim& Anim, f32 Frame, matrix4* ParentTM=NULL) ;
    
    void        CalculateBounds             (shape& Shape, shape_bin_file_class::target Target) ;
    void        ComputeWorldPixelSize       (texture_manager& TextureManager) ;
    void        BuildDLists                 (shape_bin_file_class::target Target) ;
    
    void        InitializeForDrawing        (shape&                         Shape, 
                                             texture_manager&               TextureManager, 
                                             shape_bin_file_class::target   Target) ;

    void        Draw                        (material_draw_info &DrawInfo) ;

    // Inline functions
    const char *GetName             ()      { return m_Name ;           }
    time_stamp  GetTimeStamp        ()      { return m_TimeStamp ;      }

    s32         GetType             ()      { return m_Type ;           }
    f32         GetWeight           ()      { return m_Weight ;         }

    s32         GetNodeCount        ()      { return m_nNodes ;         }
    s32         GetVertCount        ()      { return m_nVerts ;         }
    s32         GetPolyCount        ()      { return m_nPolys ;         }
    s32         GetHotPointCount    ()      { return m_nHotPoints ;     }
    s32         GetTextureRefCount  ()      { return m_nTextureRefs ;   }
    s32         GetMaterialCount    ()      { return m_nMaterials ;     }

    f32         GetPrecisionScale   ()      { return m_PrecisionScale ; }
    bbox        GetBounds           ()      { return m_Bounds ;         } 
    f32         GetWorldPixelSize   ()      { return m_WorldPixelSize ; }

    s32         GetDListVertCount   ()      { return m_nDListVerts ;    }
    s32         GetDListPolyCount   ()      { return m_nDListPolys ;    }

    // Debug collision functions
    s32         GetCollisionPolys   () ;

    // Fills array with nodes in hierarchical order
    void        GetOrderedNodes(s32 NodeIndices[], s32 ArraySize, s32 NodeIndex, s32& Index) ;
    void        GetOrderedNodes(s32 NodeIndices[], s32 ArraySize) ;

    // Returns index of first node with given type if found, else -1
    s32         GetNodeIndexByType  (s32 Type) ;

    // Fills in list of textures applied to a node
    s32         GetNodeTextures(s32 NodeIndex, s32 TextureIndices[], s32 ArraySize) ;

    // Returns number of children that a node has
	s32			GetNodeChildCount(s32 NodeIndex) ;

	// Returns given child index of node
	s32			GetNodeChildIndex(s32 NodeIndex, s32 ChildIndex) ;

    // Returns list of node children indices for given node
    s32         GetNodeChildren(s32 NodeIndex, s32 ChildrenIndices[], s32 ArraySize) ;

	// Returns number of descendent nodes (children, and their children's children etc)
	s32			GetNodeDescendentCount(s32 NodeIndex) ;

	// Returns list of node children indices for given node
private:
	s32			GetNodeDescendents(s32 NodeIndex, s32 DescendentIndices[], s32 ArraySize, s32 &ArrayIndex) ;

public:
	s32			GetNodeDescendents(s32 NodeIndex, s32 DescendentIndices[], s32 ArraySize) ;

    // Hot Point functions
    void        SortHotPointsByType(void) ;

    // Returns requested hot point
    hot_point   *GetHotPointByIndex(s32 Index) ;

    // Returns pointer to given hot point if found, else NULL
    hot_point   *GetHotPointByType(s32 Type) ;

    // Returns index of first node with given name if found, else -1
    s32         GetMaterialIndexByName(const char *Name) ;

    // Returns vertex
    node        &GetNode            (s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nNodes) ;
        return m_Nodes[Index] ;
    }

    // Returns vertex
    vert        &GetVert            (s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nVerts) ;
        return m_Verts[Index] ;
    }

    // Returns polygon
    poly        &GetPoly            (s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nPolys) ;
        return m_Polys[Index] ;
    }

    // Returns hot_pointgon
    hot_point  &GetHotPoint         (s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nHotPoints) ;
        return m_HotPoints[Index] ;
    }

    // Returns material
    material& GetMaterial        (s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nMaterials) ;
        return m_Materials[Index] ;
    }

    // Returns texture name
    shape_texture_ref& GetTextureRef(s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nTextureRefs) ;
        return m_TextureRefs[Index] ;
    }

    // File IO
    void        ReadWrite               (shape_bin_file &File) ;

} ;


//==============================================================================




#endif  //#ifndef __MODEL_HPP__

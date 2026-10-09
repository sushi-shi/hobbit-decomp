#ifndef __SHAPE_MANAGER_H__
#define __SHAPE_MANAGER_H__

  
#include <Support/Shape/Shape.hpp>
#include <Support/Shape/TextureManager.hpp>
#include <Support/Shape/LabelSet.hpp>
#include <Support/Shape/Material.hpp>


//==============================================================================
// Classes
//==============================================================================

// A simple class to consolidate shape loading
class shape_manager
{
// Defines
public:

    // Misc defines
    enum shape_manager_defines
    {
        MAX_SHAPES       = 200,     // Currently set in stone... could be changed
    } ;

    // Load modes
    enum mode
    {
        MODE_LOAD_BINARY,       // Just load .SHP's
        MODE_LOAD_ASCII,        // Just load .SKEL's
        MODE_BUILD_BINARY       // Check and update SHP's from .SKEL's
    } ;


// Data
private:
    shape               *m_Shapes[MAX_SHAPES] ;     // List of shapes
    s32                 m_nShapes ;                 // # of shapes
                        
    texture_manager     m_TextureManager ;          // Texture manager for this set of shapes

    s32                 m_NextMaterialID ;          // Next valid material ID
    s32                 m_NextMaterialDrawListID ;  // Next valid material draw list ID

// Functions
public:
	shape_manager() ;
	~shape_manager() ;

	// Loads list of files from a text file
    s32 AddAndLoadShapes        (const char*                    Filename, 
                                 shape_manager::mode            Mode,
                                 shape_bin_file_class::target   Target = shape_bin_file_class::DEFAULT) ;

    // Returns texture manager
    texture_manager &GetTextureManager()    { return m_TextureManager ; }

    // Returns number of shapes
    s32     GetShapeCount   ()       { return m_nShapes ; }

    // Returns shape, given the index
    shape  &GetShape        (s32 Index) ;

    // Returns shape, given the index
    shape  *GetShapeByIndex (s32 Index) ;

    // Sorts shapes by type for faster lookup
    void    SortShapesByType() ;

    // Returns shape, given the type
    shape  *GetShapeByType (s32 Type) ;

    // Removes all shape from memory
    void    DeleteAllShapes () ;

    // Assigns material ID's
    void    AssignMaterialIDs(s32 ID = 0, s32 DrawListID = 0) ;

    // Returns the next available material ID to assign for other materials...
    s32     GetNextMaterialID           ( void ) ;
    s32     GetNextMaterialDrawListID   ( void ) ;

} ;


#endif	//#define __SHAPE_MANAGER_H__

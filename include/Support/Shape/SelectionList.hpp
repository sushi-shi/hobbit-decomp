#ifndef __SELECTION_LIST_HPP__
#define __SELECTION_LIST_HPP__


#include <xCore/x_files/x_files.hpp>



//==============================================================================
//
// Class: selection_list
//
// A simple list class to which elements can be added, along with a weight.
// Taking the weights into account, elements can then be randomly choosen.
//
//==============================================================================




//==============================================================================
// Defines
//==============================================================================

#define MAX_SELECTION_LIST_ENTRIES  32


//==============================================================================
// Structures
//==============================================================================

struct entry
{
    s32 Element ;   // Element ID
    f32 Weight ;    // Weight
} ;


//==============================================================================
// Classes
//==============================================================================

class selection_list
{
// Data
private:
    entry   m_Entries[MAX_SELECTION_LIST_ENTRIES] ; // List of entries     
    s32     m_nEntries ;                            // Total entries in list
    f32     m_TotalWeight ;                         // Total weight of entries

// Functions
public:
    // Default constructor
    selection_list() ;
    
    // Removes all elements from list
    void Clear() ;
    
    // Returns # of elements in the list
    s32 GetCount() ;

    // Adds an element to the selection list
    void AddElement(s32 Element, f32 Weight) ;
    
    // Randomly chooses an element, taking the weights into account
    s32  ChooseElement() ;
} ;


//==============================================================================


#endif  //#ifndef __SELECTION_LIST_HPP__

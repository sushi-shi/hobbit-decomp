#include <Support/Shape/SelectionList.hpp>


//==============================================================================
// Functions
//==============================================================================


// Default constructor
selection_list::selection_list()
{
    Clear() ;
}

//==============================================================================

// Removes all elements from list
void selection_list::Clear()
{
    m_nEntries      = 0 ;
    m_TotalWeight   = 0.0f ;
}

//==============================================================================

// Returns # of elements in the list
s32 selection_list::GetCount()
{
    return m_nEntries ;
}

//==============================================================================

// Adds an element to the selection list
void selection_list::AddElement(s32 Element, f32 Weight)
{
    ASSERT(m_nEntries < MAX_SELECTION_LIST_ENTRIES) ;

    // Add to end of list
    m_Entries[m_nEntries].Element     = Element ;
    m_Entries[m_nEntries++].Weight    = Weight ;

    // Update total weight
    m_TotalWeight += Weight ;
}

//==============================================================================

// Randomly chooses an element, taking the weights into account
s32  selection_list::ChooseElement()
{
    s32 i ;
    f32 Weight, TotalWeight ;

    ASSERT(m_nEntries) ;

    // Choose random weight
    Weight      = x_frand(0.0f, m_TotalWeight) ;
    TotalWeight = 0.0f ;
    
    // Loop through entries, until we get to the entry within the weight
    for (i = 0 ; i < m_nEntries ; i++)
    {
        TotalWeight += m_Entries[i].Weight ;
        if (Weight <= TotalWeight)
            return m_Entries[i].Element ;
    }

    // Should never get here if there are entries
    ASSERT(0) ;
    return -1 ;
}

//==============================================================================



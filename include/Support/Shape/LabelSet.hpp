#ifndef __LABEL_SET_HPP__
#define __LABEL_SET_HPP__



#include <xCore/x_files/x_files.hpp>




//=========================================================================
// Classes
//=========================================================================

// Single label
class label
{
// Defines
public:
    enum
    {
        MAX_NAME_LENGTH = 256,     // Max # of chars in label name
        MAX_LINE_LENGTH = 256      // Max # of chars in text file line
    } ;

// Data
private:
    char            m_Name[MAX_NAME_LENGTH] ;   // C++ name of label
    char            m_Desc[MAX_NAME_LENGTH] ;   // Text description of label
    s32             m_Value ;                   // Value of label
   
// Functions
public:
    label() ;
    s32         Read_LABEL          (char* &CurrentParam, s32 &ID) ;
    s32         Read_LABEL_VALUE    (char* &CurrentParam, s32 &ID) ;

    const char *GetName             ()                      { return m_Name ;  }
    const char *GetDesc             ()                      { return m_Desc ;  }
    s32         GetValue            ()                      { return m_Value ; }

} ;

//=========================================================================

// Collections of labels
class label_set
{
// Data
public:
    xarray<label>   m_LabelTab ;                // List of labels

// Functions
public:

    label_set() ;

    // Parses file and adds labels to set
    s32 Read(const char *Filename) ;

    // Returns index of label with give name, if found, else -1
    s32 FindLabel(const char *Name) ;

    // Returns value of label with give name, if found, else -1
    s32 FindLabelValue(const char *Name) ;

    // Returns index of label with given value, if found, else -1
    s32 FindLabel(s32 Value) ;

    // Returns label of given index
    label &GetLabel(s32 Index) ;
} ;

//=========================================================================

#endif  // #ifdef __LABEL_SET_HPP__
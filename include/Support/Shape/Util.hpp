#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>


//==============================================================================
// Global functions
//==============================================================================

// Skips spaces in char stream
void UtilSkipWhiteSpace(char* &CurrentParam) ;

// Reads s32 in char stream
xbool UtilRead(char* &CurrentParam, s32 *V) ;

// Reads f32 in char stream
xbool UtilRead(char* &CurrentParam, f32 *V) ;

// Reads string in char stream
xbool UtilRead(char* &CurrentParam, char *V) ;

// Skips end of line characters
void UtilSkipEndOfLine(char* &CurrentParam) ;

// Returns TRUE if file exists, else FALSE
xbool UtilDoesFileExist(const char* FileName) ;

//==============================================================================




#endif  //#ifndef __UTIL_HPP__

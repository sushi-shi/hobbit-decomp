#include <Support/Shape/Util.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_plus.hpp>


//==============================================================================
// Private functions
//==============================================================================

static xbool IsEndOfLineChar(char c)
{
	return ((c == '\r') || (c == '\n') || (c == 0)) ;
}

static xbool IsParamChar(char c)
{
    // Lower case letter?
    if ((c >= 'a') && (c <= 'z'))
        return TRUE ;

    // Lower case letter?
    if ((c >= 'A') && (c <= 'Z'))
        return TRUE ;

    // Numberic character?
    if ((c >= '0') && (c <= '9'))
        return TRUE ;

    // Misc characters
    if (    (c == '_')  ||
            (c == '?')  ||
            (c == '#')  ||
            (c == '"')  ||
            (c == '.')  ||
            (c == '-')  ||
            (c == ':')  ||
            (c == '\\') ||
            (c == '/')  ||
            (c == '[')  ||
            (c == ']')  ||
            (c == '<')  ||
            (c == '>') )
        return TRUE ;

    // Not found!
    return FALSE ;
}


//==============================================================================
// Utility functions for parsing text file
//==============================================================================

// Skips spaces in char stream
void UtilSkipWhiteSpace(char* &CurrentParam)
{
    // Advance to first valid character
    while( (!IsParamChar(*CurrentParam)) && (!IsEndOfLineChar(*CurrentParam)) )
        CurrentParam++ ;
}

//==============================================================================

// Skips spaces in char stream
void UtilSkipParam(char* &CurrentParam)
{
    char *InCurrentParam = CurrentParam ;

    // Advance to first valid character
    while( (IsParamChar(*CurrentParam)) && (!IsEndOfLineChar(*CurrentParam)) )
        CurrentParam++ ;

    // Must skip at least one character!
    if ( (InCurrentParam == CurrentParam) && (!IsEndOfLineChar(*CurrentParam)) )
        CurrentParam++ ;
}

//==============================================================================

// Reads s32 in char stream
xbool UtilRead(char* &CurrentParam, s32 *V)
{
    UtilSkipWhiteSpace(CurrentParam) ;

	// Fail if end of line
	if (IsEndOfLineChar(*CurrentParam))
		return FALSE ;

    *V = x_atoi(CurrentParam) ;

    UtilSkipParam(CurrentParam) ;

    return TRUE ;
}

//==============================================================================

// Reads f32 in char stream
xbool UtilRead(char* &CurrentParam, f32 *V)
{
    UtilSkipWhiteSpace(CurrentParam) ;

	// Fail if end of line
	if (IsEndOfLineChar(*CurrentParam))
		return FALSE ;

    *V = x_atof(CurrentParam) ;

    UtilSkipParam(CurrentParam) ;

    return TRUE ;
}

//==============================================================================

// Reads string in char stream
xbool UtilRead(char* &CurrentParam, char *V)
{
    UtilSkipWhiteSpace(CurrentParam) ;

	// Fail if end of line
	if (IsEndOfLineChar(*CurrentParam))
		return FALSE ;
    
    // Reading a string in quotes?
    if (*CurrentParam == '"')
    {
        // Skip the opening quote
        CurrentParam++ ;
        
        // Copy string up to closing quote
        while(*CurrentParam != '"')
            *V++ = *CurrentParam++ ;

        // Skip the closing quote ready for the next param
        CurrentParam++ ;
    }
    else
    {
        // Copy param
        while(IsParamChar(*CurrentParam))
            *V++ = *CurrentParam++ ;
    }

    // Terminate the string
    *V = 0 ;

    return TRUE ;
}

//==============================================================================

// Skips end of line characters
void UtilSkipEndOfLine(char* &CurrentParam)
{
	// Skip space
    UtilSkipWhiteSpace(CurrentParam) ;

    // Skip end of line characters
    while(IsEndOfLineChar(*CurrentParam))
        CurrentParam++ ;
}

//==============================================================================

// Returns TRUE if file exists, else FALSE
xbool UtilDoesFileExist(const char* FileName)
{
	// Try open file
    X_FILE *File = x_fopen(FileName, "rb") ;
    if (!File)
        return FALSE ;

    // Close the file
    x_fclose(File) ;

    // It exists!
    return TRUE ;
}

//==============================================================================

#include <Support/Shape/LabelSet.hpp>
#include <Support/Shape/Util.hpp>




//=========================================================================

static s32 s_LabelSetMaxSize = 0 ;

// Default constructor
label::label()
{
    x_strcpy(m_Name, "NULL") ;    // Define name
    x_strcpy(m_Desc, "NULL") ;    // Define description
    m_Value = -1 ;                // Value of label
}

//=========================================================================

// Reads label from line of params
s32 label::Read_LABEL(char* &CurrentParam, s32 &ID)
{
    // Read name and value
    UtilRead(CurrentParam, &m_Name[0]) ;
    ASSERT(x_strlen(m_Name) < (s32)sizeof(m_Name)) ;

    UtilRead(CurrentParam, &m_Desc[0]) ;
    ASSERT(x_strlen(m_Desc) < (s32)sizeof(m_Desc)) ;

    // Set value
    m_Value = ID++ ;

    // Success!
    return TRUE ;
}

//=========================================================================

// Reads label from line of params
s32 label::Read_LABEL_VALUE(char* &CurrentParam, s32 &ID)
{
    // Read name and value
    UtilRead(CurrentParam, &m_Name[0]) ;
    ASSERT(x_strlen(m_Name) < (s32)sizeof(m_Name)) ;

    UtilRead(CurrentParam, &ID) ;

    UtilRead(CurrentParam, &m_Desc[0]) ;
    ASSERT(x_strlen(m_Desc) < (s32)sizeof(m_Desc)) ;

    // Set value
    m_Value = ID++ ;

    // Success!
    return TRUE ;
}

//=========================================================================

// Default constructor
label_set::label_set()
{
	// Should be enough to stop re-allocs!
	m_LabelTab.SetCapacity(700) ;
}

//=========================================================================

// Parses file and adds labels to set
s32 label_set::Read(const char *Filename)
{
    label   L ;
    s32     ID=0;

	// Try open file
    X_FILE *File = x_fopen(Filename, "rt") ;  // read text mode

    // Fail?
	if (!File)
	{
		ASSERT(0) ;
        return FALSE ;
	}

    // Allocate buffer to read file in one go
    x_fseek( File, 0, X_SEEK_END );
    s32 FileLen = x_ftell(File);
    x_fseek( File, 0, X_SEEK_SET );
    char *Buffer = new char[FileLen] ;
    if (!Buffer)
    {
        // Close file
        x_fclose(File) ;

        // Exit
		return FALSE ;
    }

    // Read file in one go...
    FileLen = x_fread((void *)&Buffer[0], 1, FileLen, File) ;

    // End the file buffer
    Buffer[FileLen-1] = 0 ;

    // Prepare to parse text file
    char    *CurrentParam = Buffer ;
    char    Param[label::MAX_LINE_LENGTH] ;
    
    // Search all file
    while((CurrentParam < (Buffer+FileLen)) && (*CurrentParam))
    {
        // Try read a param
        if (UtilRead(CurrentParam, Param))
        {
            // Label found?
            if (x_strncmp(Param, "LABEL", 5) == 0)
            {
                // LABEL_VALUE found?
                if (x_strncmp(Param, "LABEL_VALUE", 11) == 0)
                {
                    // Add to set if read is successful
                    if (L.Read_LABEL_VALUE(CurrentParam, ID))
                        m_LabelTab.Append(L) ;
                }
                else
                {
                    // Add to set if read is successful
                    if (L.Read_LABEL(CurrentParam, ID))
                        m_LabelTab.Append(L) ;
                }
            }

            // End of set found?
            if (x_strncmp(Param, "END_LABEL_SET", 13) == 0)
                break ;
        }

		// Next line
		UtilSkipEndOfLine(CurrentParam) ;
    }

    // Close file
    x_fclose(File) ;

    // Free buffer 
    delete [] Buffer ;

    // Keep stats
    s_LabelSetMaxSize = MAX(s_LabelSetMaxSize, m_LabelTab.GetCount()) ;

    // Success!
    return TRUE ;
}

//=========================================================================

// Returns index of label with give name, if found, else -1
s32 label_set::FindLabel(const char *Name)
{
    // Perform slow linear search!
    for (int i = 0 ; i < m_LabelTab.GetCount() ; i++)
    {
        // Found?
        if (x_strcmp(m_LabelTab[i].GetName(), Name) == 0)
            return i ;
    }

    // If you reach here, then you have not included a label set file 
    // or the .skel file is out of date with the label set files!
    ASSERT(0) ;

    // Not found
    return -1 ;
}

//=========================================================================

// Returns value of label with give name, if found, else -1
s32 label_set::FindLabelValue(const char *Name)
{
    // Search for label?
    int Index = FindLabel(Name) ;
    
    // Found?
    if (Index != -1)
        return m_LabelTab[Index].GetValue() ;
    else
        return -1 ;
}

//=========================================================================

// Returns index of label with given value, if found, else -1
s32 label_set::FindLabel(s32 Value)
{
    // Perform slow linear search!
    for (int i = 0 ; i < m_LabelTab.GetCount() ; i++)
    {
        // Found?
        if (m_LabelTab[i].GetValue() == Value)
            return i ;
    }

    // If you reach here, then you have not included a label set file 
    // or the .skel file is out of date with the label set files!
    ASSERT(0) ;

    // Not found
    return -1 ;
}

//=========================================================================

// Returns label of given index
label &label_set::GetLabel(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_LabelTab.GetCount()) ;

    return m_LabelTab[Index] ;
}

//=========================================================================

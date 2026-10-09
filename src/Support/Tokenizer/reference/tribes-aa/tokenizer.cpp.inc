//==============================================================================
//  
//  tokenizer.cpp
//
//==============================================================================

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_memory.hpp>
#include "tokenizer.hpp"

//==============================================================================

token_stream::token_stream    (void)
{
    m_FileBuffer = NULL;
    m_FileSize   = 0;
    m_LineNumber = 1;
    m_pAllocFunction = NULL;

    // Fill out delimiters
    x_strcpy(m_DelimiterStr,TOKEN_DELIMITER_STR);
    x_strcpy(m_NumberStr,"0123456789-+Ee.#QNABIF");
}

//==============================================================================

token_stream::~token_stream   (void)
{
    if (m_pAllocFunction)
    {
        m_pAllocFunction(TOKEN_ALLOC_FREE,(s32)m_FileBuffer);
    }
    else
    {
        x_free(m_FileBuffer);
    }
    m_FileBuffer = NULL;
}

//==============================================================================

void     token_stream::SetDelimiterString  ( const char* pString )
{
    // Increase TOKEN_DELIMITER_STR_MAX in tokenizer.hpp if this asserts!
    ASSERTS( x_strlen(pString) < TOKEN_DELIMITER_STR_MAX, "Increase TOKEN_DELIMITER_STR_MAX!" ) ;
    
    // Copy the string
    x_strcpy(m_DelimiterStr, pString) ;
}

//==============================================================================
xbool    token_stream::OpenFile    ( const char* pFileName, token_allocator *pAllocFunction )
{
    ASSERT(!m_pAllocFunction);
    m_pAllocFunction = pAllocFunction;
    return OpenFile(pFileName);
}

xbool    token_stream::OpenFile    ( const char* pFileName )
{
    X_FILE* fp;

    // Clear class
    m_FileSize   = 0;
    m_FilePos    = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;

    if (x_strlen(pFileName)>63)
    {
        x_strcpy(m_Filename,pFileName+x_strlen(pFileName)-63);
    }
    else
    {
        x_strcpy(m_Filename,pFileName);
    }
    // Open file
    fp = x_fopen( pFileName, "rb" );
//    ASSERT(fp);           // The application should assert
    if( fp == NULL )
        return FALSE;

    // Find how large the file is
    m_FileSize = x_flength(fp);

    // Allocate a buffer to hold the file
    if (m_pAllocFunction)
    {
        m_FileBuffer = (char*)m_pAllocFunction(TOKEN_ALLOC_ALLOCATE,m_FileSize+1);
    }
    else
    {
        m_FileBuffer = (char*)x_malloctop(m_FileSize+1);
    }
    if( m_FileBuffer == NULL )
        return FALSE;

    // Load the entire file into the buffer
    m_FileBuffer[m_FileSize] = 0;
    if( x_fread( m_FileBuffer, 1, m_FileSize, fp ) != m_FileSize )
    {
        if (m_pAllocFunction)
        {
            m_pAllocFunction(TOKEN_ALLOC_FREE,(s32)m_FileBuffer);
        }
        else
        {
            x_free(m_FileBuffer);
        }
        return FALSE;
    }
    
    // Close the input file
    x_fclose( fp );

    Rewind();

    return( TRUE );
}

//==============================================================================

void    token_stream::CloseFile   ( void )
{
    if (m_pAllocFunction)
    {
        m_pAllocFunction(TOKEN_ALLOC_FREE,(s32)m_FileBuffer);
    }
    else
    {
        x_free(m_FileBuffer);
    }
    m_FileBuffer = NULL;
    m_FileSize = 0;
}

//==============================================================================

void    token_stream::Rewind      ( void )
{
    m_FilePos          = 0;
    m_LineNumber       = 1;
    m_Type             = TOKEN_NONE;
    m_Delimiter        = ' ';
    m_Float            = 0.0f;
    m_Int              = 0;
    m_String[0]        = 0;
}

//==============================================================================

void    token_stream::SetCursor      ( s32 Pos )
{
    ASSERT( (Pos>=0) && (Pos<m_FilePos) );
    m_FilePos          = Pos;
    m_LineNumber       = 1;
    m_Type             = TOKEN_NONE;
    m_Delimiter        = ' ';
    m_Float            = 0.0f;
    m_Int              = 0;
    m_String[0]        = 0;
}

//==============================================================================

s32    token_stream::GetCursor      ( void )
{
    return m_FilePos;
}

//==============================================================================

s32    token_stream::GetFileSize    ( void )
{
    return m_FileSize;
}

//==============================================================================

xbool   token_stream::Find        ( const char* TokenStr, xbool FromBeginning )
{
    if( FromBeginning )
        Rewind();

    Read();

    while( m_Type != TOKEN_EOF )
    {
        if( x_stricmp(m_String,TokenStr)==0 ) return TRUE;
        Read();
    }

    return FALSE;
}

//==============================================================================

void token_stream::SkipWhitespace( void )
{
    while( 1 )
    {
        if( m_FilePos >= m_FileSize )
            return;

        // Skip whitespace and count EOLNs
        if( m_FileBuffer[m_FilePos]<=32 )
        {
            while( (m_FileBuffer[m_FilePos]<=32) && (m_FilePos<m_FileSize))
            {
                if( m_FileBuffer[m_FilePos]=='\n' ) 
                {
                    m_LineNumber++;
                }
                m_FilePos++;
            }

            continue;
        }

        // Watch for line comment
        if( (m_FileBuffer[m_FilePos+0] == '/') && (m_FileBuffer[m_FilePos+1] == '/') )
        {
            m_FilePos += 2;

            while( (m_FileBuffer[m_FilePos]!='\n') && (m_FilePos<m_FileSize))
                m_FilePos++;

            if( m_FileBuffer[m_FilePos]=='\n' ) 
            {
                m_LineNumber++;
                m_FilePos++;
            }

            continue;
        }

        // Watch for block comment
        if( (m_FileBuffer[m_FilePos+0] == '/') && (m_FileBuffer[m_FilePos+1] == '*') )
        {
            m_FilePos += 2;

            while( m_FilePos <= m_FileSize-1 )
            {
                if( (m_FileBuffer[m_FilePos+0] == '*') && (m_FileBuffer[m_FilePos+1] == '/') )
                {
                    m_FilePos += 2;
                    break;
                }

                if( m_FileBuffer[m_FilePos]=='\n' ) 
                {
                    m_LineNumber++;
                }

                m_FilePos++;
            }

            continue;
        }

        // No whitespace found
        return;
    }
}

//==============================================================================

token_stream::type    token_stream::Read        ( s32 NTokens )
{
    while( NTokens-- )
    {
        s32     i,j;
        char    ch;

        // Clear the current settings
        m_Type          = TOKEN_NONE;
        m_Delimiter     = ' ';
        m_Float         = 0.0f;
        m_Int           = 0;
        m_IsFloat       = FALSE;
        m_String[0]     = 0;

        ASSERT(m_FilePos<=m_FileSize);

        // Skip whitespace and comments
        SkipWhitespace();

        // Watch for end of file
        if( m_FilePos >= m_FileSize )
        {
            m_Type = TOKEN_EOF;
            continue;
        }

        // Look for number first since we load a lot of these
        ch = m_FileBuffer[m_FilePos];
        if(((ch>='0') && (ch<='9')) || (ch=='-') || (ch=='+'))
        {
            // Copy number into string buffer
            i=0;
            while( 1 )
            {
                ch = m_FileBuffer[m_FilePos];
                j=0;
                while( m_NumberStr[j] && (ch != m_NumberStr[j]) ) j++;
                if( m_NumberStr[j]==0 ) break;
                m_String[i] = ch;
                m_FilePos++;
                i++;
            }
            
            // Generate float version
            m_String[i] = 0;
            m_Float     = x_atof( m_String );
            m_Int       = (s32)m_Float;

            // Decide on token type
            if( m_Float == (f32)m_Int )
            {
                m_Type      = TOKEN_NUMBER;
                m_IsFloat   = FALSE;
            }
            else
            {
                m_Type      = TOKEN_NUMBER;
                m_IsFloat   = TRUE;
            }

            continue;
        }

        // Watch for string
        if( m_FileBuffer[m_FilePos] == '"' )
        {
            m_FilePos++;        
            i=0;
            while( m_FileBuffer[m_FilePos]!='"' )
            {
                // Check for illegal ending of a string
                ASSERT((m_FilePos < m_FileSize) && "EOF in quote");
                ASSERT((i<TOKEN_STRING_SIZE-1) && "Quote too long");
                ASSERT((m_FileBuffer[m_FilePos]!='\n') && "EOLN in quote");
            
                m_String[i] = m_FileBuffer[m_FilePos];
                i++;
                m_FilePos++;
            }

            m_FilePos++;
            m_String[i]  = 0;
            m_Type       = TOKEN_STRING;

            continue;
        }

        // Look for delimiter
        ch = m_FileBuffer[m_FilePos];
        i=0;
        while( m_DelimiterStr[i] && (ch != m_DelimiterStr[i]) ) 
            i++;
        if( m_DelimiterStr[i] )
        {
            m_FilePos++;
            m_Type           = TOKEN_DELIMITER;
            m_Delimiter      = ch;
            m_String[0]      = ch;
            m_String[1]      = 0;
            continue;
        }

        // Treat this as a raw symbol
        {
            i=0;
            while( m_FilePos<m_FileSize )
            {
                j=0;
                if( i==TOKEN_STRING_SIZE-1 ) break;
                if( m_FileBuffer[m_FilePos]<=32 ) break;
                while( m_DelimiterStr[j] && (m_FileBuffer[m_FilePos] != m_DelimiterStr[j]) ) j++;
                if( m_DelimiterStr[j] ) break;

                m_String[i] = (char)m_FileBuffer[m_FilePos];
                i++;
                m_FilePos++;
            }

            m_String[i] = 0;
            m_Type      = TOKEN_SYMBOL;
            continue;
        }
    }

    return m_Type;
}

//==============================================================================

f32     token_stream::ReadFloat   ( void )
{
    SkipWhitespace();

    char ch = m_FileBuffer[m_FilePos];
    ASSERT(((ch>='0') && (ch<='9')) || (ch=='-') || (ch=='+'));

    // Copy number into string buffer
    s32 j,i=0;
    while( 1 )
    {
        ch = m_FileBuffer[m_FilePos];
        j=0;
        while( m_NumberStr[j] && (ch != m_NumberStr[j]) ) j++;
        if( m_NumberStr[j]==0 ) break;
        m_String[i] = ch;
        m_FilePos++;
        i++;
    }
    
    // Generate float version
    m_String[i] = 0;
    m_Float     = x_atof( m_String );
    m_Int       = (s32)m_Float;
    m_Type      = TOKEN_NUMBER;
    m_IsFloat   = TRUE;

    return m_Float;
}

//==============================================================================

s32     token_stream::ReadInt     ( void )
{
    SkipWhitespace();

    char ch = m_FileBuffer[m_FilePos];
    ASSERT(((ch>='0') && (ch<='9')) || (ch=='-') || (ch=='+'));

    // Copy number into string buffer
    s32 j,i=0;
    while( 1 )
    {
        ch = m_FileBuffer[m_FilePos];
        j=0;
        while( m_NumberStr[j] && (ch != m_NumberStr[j]) ) j++;
        if( m_NumberStr[j]==0 ) break;
        m_String[i] = ch;
        m_FilePos++;
        i++;
    }
    
    // Generate int version
    m_String[i] = 0;
    m_Int       = x_atoi( m_String );
    m_Float     = (f32)m_Int;
    m_Type      = TOKEN_NUMBER;
    m_IsFloat   = FALSE;

    return m_Int;
}

//==============================================================================

s32     token_stream::ReadHex     ( void )
{
    SkipWhitespace();

    char ch = m_FileBuffer[m_FilePos++];
    ASSERT( ch == '0' );
    ch = m_FileBuffer[m_FilePos++];
    ASSERT( ch == 'x' );

    // Copy number into string buffer
    s32 Number = 0;
    s32 i=2;
    m_String[0] = '0';
    m_String[1] = 'x';
    while( 1 )
    {
        ch = x_toupper(m_FileBuffer[m_FilePos]);
        if( x_ishex( ch ) )
        {
            s32 Digit = ch - '0';
            if( Digit >= 10 ) Digit -= 'A'-'0'-10;
            Number *= 10;
            Number += Digit;
            m_FilePos++;
            m_String[i++] = ch;
        }
        else
            break;
    }
    
    // Generate int version
    m_String[i] = 0;
    m_Int       = Number;
    m_Float     = (f32)m_Int;
    m_Type      = TOKEN_NUMBER;
    m_IsFloat   = FALSE;

    return m_Int;
}

//==============================================================================

char*   token_stream::ReadSymbol  ( void )
{
    s32 i,j;

    SkipWhitespace();

    i=0;
    while( m_FilePos<m_FileSize )
    {
        j=0;
        if( i==TOKEN_STRING_SIZE-1 ) break;
        if( m_FileBuffer[m_FilePos]<=32 ) break;
        while( m_DelimiterStr[j] && (m_FileBuffer[m_FilePos] != m_DelimiterStr[j]) ) j++;
        if( m_DelimiterStr[j] ) break;

        m_String[i] = (char)m_FileBuffer[m_FilePos];
        i++;
        m_FilePos++;
    }

    m_String[i] = 0;
    m_Type      = TOKEN_SYMBOL;
    return m_String;
}

//==============================================================================

char*   token_stream::ReadString  ( void )
{
    SkipWhitespace();

    ASSERT( m_FileBuffer[m_FilePos] == '"' );

    m_FilePos++;        
    s32 i=0;
    while( m_FileBuffer[m_FilePos]!='"' )
    {
        // Check for illegal ending of a string
        ASSERT((m_FilePos < m_FileSize) && "EOF in quote");
        ASSERT((i<TOKEN_STRING_SIZE-1) && "Quote too long");
        ASSERT((m_FileBuffer[m_FilePos]!='\n') && "EOLN in quote");
    
        m_String[i] = m_FileBuffer[m_FilePos];
        i++;
        m_FilePos++;
    }

    m_FilePos++;
    m_String[i]  = 0;
    m_Type       = TOKEN_STRING;
    return m_String;
}

//==============================================================================

char*   token_stream::ReadLine  ( void )
{
    s32 i;

    // Copy from current position...
    i=0;
    while(( m_FilePos<m_FileSize ) && (i < (TOKEN_STRING_SIZE-1)))
    {
        // Hit end of line character(s)?
        if( (m_FileBuffer[m_FilePos] == 10) ||
            (m_FileBuffer[m_FilePos] == 13) )
        {
            // Skip past EOL.
            if( m_FileBuffer[m_FilePos]=='\r' )
                m_FilePos++;
            if( m_FileBuffer[m_FilePos]=='\n' )
            {
                m_FilePos++;
                m_LineNumber++; // Update line number.
            }

            // Jump out of while loop
            break;
        }

        // Copy character and go to next
        m_String[i++] = (char)m_FileBuffer[m_FilePos++];
    }

    // Return as a symbol
    m_String[i] = 0 ;
    m_Type      = TOKEN_SYMBOL;
    return m_String;
}

//==============================================================================


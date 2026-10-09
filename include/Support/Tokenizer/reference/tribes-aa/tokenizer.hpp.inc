//==============================================================================
//  
//  tokenizer.hpp
//
//==============================================================================

#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

//==============================================================================

#include <xCore/x_files/x_math.hpp>

//==============================================================================
#define TOKEN_STRING_SIZE           512
#define TOKEN_DELIMITER_STR         ",{}()<>;=[]"   // Default delimiter string
#define TOKEN_DELIMITER_STR_MAX     16              // Max size of delimiter string

#define TOKEN_ALLOC_ALLOCATE        0
#define TOKEN_ALLOC_FREE            1

typedef void * (token_allocator)(s32 key, s32 length);

class token_stream
{   
public:

    enum type
    {
        TOKEN_NONE,
        TOKEN_NUMBER,
        TOKEN_DELIMITER,
        TOKEN_SYMBOL,
        TOKEN_STRING,
        TOKEN_EOF = 0x7FFFFFFF,
    };

private:

    token_allocator *m_pAllocFunction;

    s32     m_FileSize;
    char*   m_FileBuffer;
    s32     m_FilePos;
    s32     m_LineNumber;

    char    m_Filename[64];
    char    m_DelimiterStr[TOKEN_DELIMITER_STR_MAX];
    char    m_NumberStr[32];

    type    m_Type;
    char    m_String[TOKEN_STRING_SIZE];
    f32     m_Float;
    s32     m_Int;
    xbool   m_IsFloat;
    char    m_Delimiter;

    void    SkipWhitespace( void );

public:

    token_stream    (void);
    ~token_stream   (void);

    // Start
    void    SetDelimiterString  ( const char* pString ) ;
    xbool   OpenFile            ( const char* pFileName );
    xbool   OpenFile            ( const char* pFileName, token_allocator *pAllocFunction );
    void    CloseFile           ( void );

    // Move through tokens in file
    void    Rewind      ( void );
    type    Read        ( s32 NTokens=1 );
    f32     ReadFloat   ( void );
    s32     ReadInt     ( void );
    s32     ReadHex     ( void );
    char*   ReadSymbol  ( void );
    char*   ReadString  ( void );
    char*   ReadLine    ( void );

    // Complex requests
    xbool   Find        ( const char* TokenStr, xbool FromBeginning=FALSE );
    s32     GetFileSize ( void );
    s32     GetCursor   ( void );
    void    SetCursor   ( s32 Pos );
    s32     GetLineNumber(void) {return m_LineNumber;}
    char    *GetFilename(void) {return m_Filename;}

    // Interrogate about current token
    type    Type        ( void ) {return m_Type;}
    f32     Float       ( void ) {return m_Float;}
    s32     Int         ( void ) {return m_Int;}
    char    Delimiter   ( void ) {return m_Delimiter;}
    char*   String      ( void ) {return m_String;}

};

//==============================================================================
#endif
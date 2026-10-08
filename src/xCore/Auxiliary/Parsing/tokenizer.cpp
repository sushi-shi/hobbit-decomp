#include <rva.h>

#include <xCore/Auxiliary/Parsing/tokenizer.hpp>

#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

RVA(0x002436b0, 0x82)
char token_stream::CHAR(int position) {
    int chunk = position / (1024 * 1024);
    m_CurBufferStart = chunk * (1024 * 1024);
    int size =
        1024 * 1024 < m_FileSize - m_CurBufferStart ? 1024 * 1024 : m_FileSize - m_CurBufferStart;
    m_CurBufferEnd = m_CurBufferStart + size - 1;
    x_fseek(m_pFile, m_CurBufferStart + m_StartPosition, 0);
    x_fread(m_CurBuffer, size, 1, m_pFile);
    return m_CurBuffer[position - m_CurBufferStart];
}

RVA(0x00243740, 0xd7)
int token_stream::OpenFile(const char* name) {
    m_FileSize = 0;
    m_FilePos = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;
    if (x_strlen(name) > 63) {
        x_strcpy(m_Filename, name + x_strlen(name) - 63);
    } else {
        x_strcpy(m_Filename, name);
    }
    m_pFile = x_fopen(name, "rb");
    if (!m_pFile) {
        return 0;
    }
    m_FileSize = x_flength(m_pFile);
    m_bBuffered = 1;
    m_CurBufferStart = -1;
    m_CurBufferEnd = -1;
    m_CurBuffer = static_cast<char*>(x_malloc_fn(
        1024 * 1024 < m_FileSize ? 1024 * 1024 : m_FileSize,
        "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp",
        99
    ));
    Rewind();
    m_StartPosition = 0;
    return 1;
}

RVA(0x00243820, 0xfc)
int token_stream::OpenFile(X_FILE* file, int buffered) {
    m_FileSize = 0;
    m_FilePos = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;
    m_pFile = file;
    if (!m_pFile) {
        return 0;
    }
    m_FileSize = x_flength(m_pFile);
    m_StartPosition = x_ftell(m_pFile);
    m_FileSize -= x_ftell(m_pFile);
    if (buffered) {
        m_bBuffered = 1;
        m_CurBufferStart = -1;
        m_CurBufferEnd = -1;
        m_CurBuffer = static_cast<char*>(x_malloc_fn(
            1024 * 1024 < m_FileSize ? 1024 * 1024 : m_FileSize,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp",
            145
        ));
        Rewind();
        return 1;
    } else {
        m_bBuffered = 0;
        m_FileBuffer = static_cast<char*>(x_malloc_fn(
            m_FileSize,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp",
            151
        ));
        x_fread(m_FileBuffer, m_FileSize, 1, m_pFile);
        Rewind();
        return 1;
    }
}

RVA(0x00243920, 0xd7)
token_stream::token_stream() {
    m_FileBuffer = 0;
    m_FileSize = 0;
    m_LineNumber = 1;
    m_CurBuffer = 0;
    m_bBuffered = 0;
    m_CurBufferStart = -1;
    m_CurBufferEnd = -1;
    m_pFile = 0;
    x_strcpy(m_DelimiterStr, ",{}()<>;");
    int i;
    char integers[] = "0123456789-+";
    char floats[] = "Ee.#QNABIFD";
    for (i = 0; i < 256; ++i) {
        m_IsCharNumber[i] = 0;
    }
    i = 0;
    while (integers[i]) {
        m_IsCharNumber[integers[i]] = 1;
        ++i;
    }
    i = 0;
    while (floats[i]) {
        m_IsCharNumber[floats[i]] = 2;
        ++i;
    }
}

RVA(0x00243a00, 0x5b)
token_stream::~token_stream() {
    x_free_fn(
        m_FileBuffer,
        "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp",
        324
    );
    m_FileBuffer = 0;
    x_free_fn(m_CurBuffer, "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp", 326);
    m_CurBuffer = 0;
    m_bBuffered = 0;
    m_CurBufferStart = -1;
    m_CurBufferEnd = -1;
}

RVA(0x00243a60, 0x11)
int token_stream::IsEOF() const {
    return m_FilePos >= m_FileSize;
}
RVA(0x00243a80, 0x4)
char* token_stream::GetDelimeter() {
    return m_DelimiterStr;
}
RVA(0x00243a90, 0x14)
void token_stream::SetDelimeter(char* delimiter) {
    x_strcpy(m_DelimiterStr, delimiter);
}

RVA(0x00243ab0, 0x87)
void token_stream::CloseFile() {
    if (m_bBuffered) {
        if (m_CurBuffer) {
            x_free_fn(
                m_CurBuffer,
                "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp",
                364
            );
            m_CurBuffer = 0;
            m_CurBufferStart = -1;
            m_CurBufferEnd = -1;
            if (m_pFile) {
                x_fclose(m_pFile);
            }
            m_pFile = 0;
        }
    } else {
        x_free_fn(
            m_FileBuffer,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\tokenizer.cpp",
            375
        );
        m_FileBuffer = 0;
        m_FileSize = 0;
    }
}

RVA(0x00243b40, 0x2c)
void token_stream::Rewind() {
    m_FilePos = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;
    m_Delimiter = ' ';
    m_Float = 0;
    m_Int = 0;
    m_String[0] = 0;
}
RVA(0x00243b70, 0x32)
void token_stream::SetCursor(int position) {
    m_FilePos = position;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;
    m_Delimiter = ' ';
    m_Float = 0;
    m_Int = 0;
    m_String[0] = 0;
}
RVA(0x00243bb0, 0x4)
int token_stream::GetCursor() {
    return m_FilePos;
}
RVA(0x00243bc0, 0x67)
int token_stream::Find(const char* token, int fromBeginning) {
    if (fromBeginning) {
        Rewind();
    }
    Read();
    while (m_Type != TOKEN_EOF) {
        if (x_stricmp(m_String, token) == 0) {
            return 1;
        }
        Read();
    }
    return 0;
}

// PC inlines the buffered-range check and calls CHAR only for a refill.
#define TOKEN_CHAR(position) GetChar(position)

RVA(0x00243c30, 0x321)
void token_stream::SkipWhitespace() {
    while (1) {
        if (m_FilePos >= m_FileSize) {
            return;
        }
        if (TOKEN_CHAR(m_FilePos) <= 32) {
            while (m_FilePos < m_FileSize && TOKEN_CHAR(m_FilePos) <= 32) {
                if (TOKEN_CHAR(m_FilePos) == '\n') {
                    ++m_LineNumber;
                }
                ++m_FilePos;
            }
            continue;
        }
        if (TOKEN_CHAR(m_FilePos + 0) == '/' && TOKEN_CHAR(m_FilePos + 1) == '/') {
            m_FilePos += 2;
            while (TOKEN_CHAR(m_FilePos) != '\n' && m_FilePos < m_FileSize) {
                ++m_FilePos;
            }
            if (TOKEN_CHAR(m_FilePos) == '\n') {
                ++m_LineNumber;
                ++m_FilePos;
            }
            continue;
        }
        if (TOKEN_CHAR(m_FilePos + 0) == '/' && TOKEN_CHAR(m_FilePos + 1) == '*') {
            m_FilePos += 2;
            while (m_FilePos <= m_FileSize - 1) {
                if (TOKEN_CHAR(m_FilePos + 0) == '*' && TOKEN_CHAR(m_FilePos + 1) == '/') {
                    m_FilePos += 2;
                    break;
                }
                if (TOKEN_CHAR(m_FilePos) == '\n') {
                    ++m_LineNumber;
                }
                ++m_FilePos;
            }
            continue;
        }
        return;
    }
}

RVA(0x00243f60, 0x492)
token_stream::type token_stream::Read(int tokens) {
    while (tokens--) {
        int i, j;
        char ch;
        m_Type = TOKEN_NONE;
        m_Delimiter = ' ';
        m_Float = 0;
        m_Int = 0;
        m_IsFloat = 0;
        m_String[0] = 0;
        SkipWhitespace();
        if (m_FilePos >= m_FileSize) {
            m_Type = TOKEN_EOF;
            continue;
        }
        ch = TOKEN_CHAR(m_FilePos);
        if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+') {
            if (ch == '0' && m_FilePos + 1 < m_FileSize && TOKEN_CHAR(m_FilePos + 1) == 'x') {
                ReadHex();
                continue;
            }
            i = 0;
            m_IsFloat = 0;
            while (1) {
                ch = TOKEN_CHAR(m_FilePos);
                if (!m_IsCharNumber[ch]) {
                    break;
                }
                m_String[i] = ch;
                m_IsFloat |= m_IsCharNumber[ch];
                ++m_FilePos;
                ++i;
            }
            m_IsFloat >>= 1;
            m_String[i] = 0;
            if (m_IsFloat) {
                m_Float = x_atof(m_String);
                m_Int = static_cast<int>(m_Float);
                m_Type = TOKEN_NUMBER;
            } else {
                m_Int = x_atoi(m_String);
                m_Float = static_cast<float>(m_Int);
                m_Type = TOKEN_NUMBER;
            }
            continue;
        }
        if (TOKEN_CHAR(m_FilePos) == '"') {
            ++m_FilePos;
            i = 0;
            while (TOKEN_CHAR(m_FilePos) != '"') {
                m_String[i] = TOKEN_CHAR(m_FilePos);
                ++i;
                ++m_FilePos;
            }
            ++m_FilePos;
            m_String[i] = 0;
            m_Type = TOKEN_STRING;
            continue;
        }
        ch = TOKEN_CHAR(m_FilePos);
        i = 0;
        while (m_DelimiterStr[i] && ch != m_DelimiterStr[i]) {
            ++i;
        }
        if (m_DelimiterStr[i]) {
            ++m_FilePos;
            m_Type = TOKEN_DELIMITER;
            m_Delimiter = ch;
            m_String[0] = ch;
            m_String[1] = 0;
            continue;
        }
        i = 0;
        while (m_FilePos < m_FileSize) {
            j = 0;
            if (i == 1023) {
                break;
            }
            if (TOKEN_CHAR(m_FilePos) <= 32) {
                break;
            }
            while (m_DelimiterStr[j] && TOKEN_CHAR(m_FilePos) != m_DelimiterStr[j]) {
                ++j;
            }
            if (m_DelimiterStr[j]) {
                break;
            }
            m_String[i] = TOKEN_CHAR(m_FilePos);
            ++i;
            ++m_FilePos;
        }
        m_String[i] = 0;
        m_Type = TOKEN_SYMBOL;
        continue;
    }
    return m_Type;
}

RVA(0x00244400, 0xc2)
float token_stream::ReadFloat() {
    SkipWhitespace();
    char ch = TOKEN_CHAR(m_FilePos);
    int i = 0;
    while (1) {
        ch = TOKEN_CHAR(m_FilePos);
        if (!m_IsCharNumber[ch]) {
            break;
        }
        m_String[i] = ch;
        ++m_FilePos;
        ++i;
    }
    m_String[i] = 0;
    m_Float = x_atof(m_String);
    m_Int = static_cast<int>(m_Float);
    m_Type = TOKEN_NUMBER;
    m_IsFloat = 1;
    return m_Float;
}

RVA(0x002444d0, 0xc8)
int token_stream::ReadInt() {
    SkipWhitespace();
    char ch = TOKEN_CHAR(m_FilePos);
    int i = 0;
    while (1) {
        ch = TOKEN_CHAR(m_FilePos);
        if (!m_IsCharNumber[ch]) {
            break;
        }
        m_String[i] = ch;
        ++m_FilePos;
        ++i;
    }
    m_String[i] = 0;
    m_Int = x_atoi(m_String);
    m_Float = static_cast<float>(m_Int);
    m_Type = TOKEN_NUMBER;
    m_IsFloat = 0;
    return m_Int;
}

RVA(0x002445a0, 0x12b)
int token_stream::ReadHex() {
    SkipWhitespace();
    char ch = TOKEN_CHAR(m_FilePos);
    ++m_FilePos;
    ch = TOKEN_CHAR(m_FilePos);
    ++m_FilePos;
    int number = 0;
    int i = 2;
    m_String[0] = '0';
    m_String[1] = 'x';
    while (1) {
        // Hobbit's observed inline case/class lookup narrows the table index.
        ch = x_UpperCase[static_cast<unsigned char>(TOKEN_CHAR(m_FilePos))];
        if (x_CharacterClass[static_cast<unsigned char>(ch)] & 8) {
            int digit = ch - '0';
            if (digit >= 10) {
                digit -= 'A' - '0' - 10;
            }
            number *= 16;
            number += digit;
            ++m_FilePos;
            m_String[i++] = ch;
        } else {
            break;
        }
    }
    m_String[i] = 0;
    m_Int = number;
    m_Float = static_cast<float>(m_Int);
    m_Type = TOKEN_NUMBER;
    m_IsFloat = 0;
    return m_Int;
}
RVA(0x002446d0, 0x146)
char* token_stream::ReadSymbol() {
    int i, j;
    SkipWhitespace();
    i = 0;
    while (m_FilePos < m_FileSize) {
        j = 0;
        if (i == 1023) {
            break;
        }
        if (TOKEN_CHAR(m_FilePos) <= 32) {
            break;
        }
        while (m_DelimiterStr[j] && TOKEN_CHAR(m_FilePos) != m_DelimiterStr[j]) {
            ++j;
        }
        if (m_DelimiterStr[j]) {
            break;
        }
        m_String[i] = TOKEN_CHAR(m_FilePos);
        ++i;
        ++m_FilePos;
    }
    m_String[i] = 0;
    m_Type = TOKEN_SYMBOL;
    return m_String;
}
RVA(0x00244820, 0xe3)
char* token_stream::ReadString() {
    SkipWhitespace();
    ++m_FilePos;
    int i = 0;
    int badString = 0;
    while (TOKEN_CHAR(m_FilePos) != '"') {
        m_String[i] = TOKEN_CHAR(m_FilePos);
        if (m_String[i] < 32) {
            badString = 1;
        }
        ++i;
        ++m_FilePos;
    }
    ++m_FilePos;
    m_String[i] = 0;
    m_Type = TOKEN_STRING;
    if (badString) {
        x_strcpy(m_String, "!!!BADSTRING!!!");
    }
    return m_String;
}

RVA(0x00244910, 0x2f5)
int token_stream::IsEOL() {
    int position = m_FilePos;
    while (1) {
        if (position >= m_FileSize) {
            return 1;
        }
        if (TOKEN_CHAR(position) <= 32) {
            while (position < m_FileSize && TOKEN_CHAR(position) <= 32) {
                if (TOKEN_CHAR(position) == '\n') {
                    return 1;
                }
                ++position;
            }
            continue;
        }
        if (TOKEN_CHAR(position + 0) == '/' && TOKEN_CHAR(position + 1) == '/') {
            position += 2;
            while (TOKEN_CHAR(position) != '\n' && position < m_FileSize) {
                ++position;
            }
            if (TOKEN_CHAR(position) == '\n') {
                return 1;
            }
            continue;
        }
        if (TOKEN_CHAR(position + 0) == '/' && TOKEN_CHAR(position + 1) == '*') {
            position += 2;
            while (position <= m_FileSize - 1) {
                if (TOKEN_CHAR(position + 0) == '*' && TOKEN_CHAR(position + 1) == '/') {
                    position += 2;
                    break;
                }
                if (TOKEN_CHAR(position) == '\n') {
                    return 1;
                }
                ++position;
            }
            continue;
        }
        return 0;
    }
}

// Compatible later original helper APIs; reads use the established PC buffer-aware GetChar.
void token_stream::SkipToNextLine( void )
{
    while( 1 )
    {
        if( m_FilePos >= m_FileSize )
            return;

        // Move forward to end of line
        {
            while( (GetChar(m_FilePos)!='\n') && (m_FilePos<m_FileSize))
                m_FilePos++;

            if( GetChar(m_FilePos)=='\n' ) 
            {
                m_LineNumber++;
                m_FilePos++;
                return;
            }
        }
    }
}

f32 token_stream::ReadF32FromString( void )
{
    ReadString();

    // Transform string into a float
    m_Float     = x_atof( m_String );
    m_Int       = (s32)m_Float;
    m_Type      = TOKEN_NUMBER;
    m_IsFloat   = TRUE;

    return m_Float;
}

s32 token_stream::ReadS32FromString( void )
{
    ReadString();

    // Transform string into a s32
    m_Int       = x_atoi( m_String );
    m_Float     = (f32)m_Int;
    m_Type      = TOKEN_NUMBER;
    m_IsFloat   = FALSE;

    return m_Int;
}

xbool token_stream::ReadBoolFromString( void )
{
    ReadString();

    // Transform string into an xbool
    m_Int       = x_atoi( m_String );
    m_Float     = (f32)m_Int;
    m_Type      = TOKEN_NUMBER;
    m_IsFloat   = FALSE;

    return (m_Int) ? (TRUE):(FALSE);
}

char* token_stream::ReadLine( void )
{
    // Enf of file not reached or string buffer full?
    s32 i = 0 ;
    while((i < (TOKEN_STRING_SIZE-1)) && (m_FilePos<m_FileSize))
    {
        // Get char
        char C = GetChar(m_FilePos);
        m_FilePos++;
       
        // End of line reached?
        if ((C == '\n') || (C == 13))
           break ;

        // Add to string
        m_String[i++] = C ;
    }

    // Terminate the string
    m_String[i] = 0;
    m_Type      = TOKEN_SYMBOL;

    // Skip end of line feeds
    while(m_FilePos<m_FileSize)
    {
        // Get char
        char C = GetChar(m_FilePos);
        m_FilePos++;

        // End of line reached?
        if ((C != '\n') || (C != 13))
           break ;
    }

    return m_String;
}

char*   token_stream::ReadToSymbol ( char Sym )
{
    s32 i=0;
    while( GetChar(m_FilePos)!= Sym )
    {
        // Check for illegal ending of a string
        ASSERT((m_FilePos < m_FileSize) && "EOF in quote");
        ASSERT((i<TOKEN_STRING_SIZE-1) && "Quote too long");
    
        m_String[i] = GetChar(m_FilePos);
        i++;
        m_FilePos++;
    }

    m_FilePos++;
    m_String[i]  = 0;
    m_Type       = TOKEN_STRING;
    return m_String;
}

void token_stream::OpenText( const char* pTextString )
{
    // No file must be open
    ASSERT( m_pFile == NULL );
    ASSERT( pTextString );
    // Clear class
    m_FileSize   = 0;
    m_FilePos    = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;

    x_strcpy( m_Filename, "<internal string>" );

    m_bBuffered     = FALSE;
    m_FileBuffer    = (char*)pTextString;

    // Find how large the file is
    m_FileSize = x_strlen(pTextString)+1;

    Rewind();

    m_StartPosition = 0;
  
}

void token_stream::CloseText( void )
{
    ASSERT( m_bBuffered == FALSE );
    ASSERT( m_pFile == NULL );
    ASSERT( m_CurBuffer == NULL );

    m_FileBuffer = NULL;
}

#ifndef HOBBIT_XCORE_AUXILIARY_PARSING_TOKENIZER_HPP
#define HOBBIT_XCORE_AUXILIARY_PARSING_TOKENIZER_HPP

#include <rva.h>

#include <xCore/x_files/x_stdio.hpp>

#define TOKEN_STRING_SIZE 1024

class token_stream {
public:
    enum type {
        TOKEN_NONE,
        TOKEN_NUMBER,
        TOKEN_DELIMITER,
        TOKEN_SYMBOL,
        TOKEN_STRING,
        TOKEN_EOF = 0x7fffffff
    };
    token_stream();
    ~token_stream();
    int OpenFile(const char* name);
    int OpenFile(X_FILE* file, int buffered);
    void SkipToNextLine();
    float ReadF32FromString();
    int ReadS32FromString();
    int ReadBoolFromString();
    char* ReadLine();
    char* ReadToSymbol(char symbol);
    void OpenText(const char* text);
    void CloseText();
    int GetLineNumber() { return m_LineNumber; }
    char* GetFilename() { return m_Filename; }
    type Type() { return m_Type; }
    int IsEOF() const;
    int IsEOL();
    char* GetDelimeter();
    void SetDelimeter(char* delimiter);
    void CloseFile();
    void Rewind();
    void SetCursor(int position);
    int GetCursor();
    int Find(const char* token, int fromBeginning);
    type Read(int tokens = 1);
    int ReadHex();
    float ReadFloat();
    int ReadInt();
    char* ReadSymbol();
    char* ReadString();
    char Delimiter() {
        return m_Delimiter;
    }
    char* String() {
        return m_String;
    }
    int Int() {
        return m_Int;
    }
    float Float() {
        return m_Float;
    }
    int IsFloat() {
        return m_IsFloat;
    }

protected:
    char CHAR(int position);
    RVA(0x00244c10, 0x43)
    char GetChar(int position) {
        if (m_bBuffered) {
            if (position >= m_CurBufferStart && position <= m_CurBufferEnd) {
                return m_CurBuffer[position - m_CurBufferStart];
            }
            char ch = CHAR(position);
            return ch;
        }
        return m_FileBuffer[m_FilePos];
    }
    void SkipWhitespace();
    int m_FileSize;
    char* m_FileBuffer;
    int m_FilePos;
    int m_LineNumber;
    char m_Filename[64];
    char m_DelimiterStr[16];
    unsigned char m_IsCharNumber[256];
    type m_Type;
    char m_String[1024];
    float m_Float;
    int m_Int;
    int m_IsFloat;
    char m_Delimiter;
    int m_bBuffered;
    int m_CurBufferStart;
    int m_CurBufferEnd;
    char* m_CurBuffer;
    int m_StartPosition;
    X_FILE* m_pFile;
};
#endif

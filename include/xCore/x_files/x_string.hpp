#ifndef HOBBIT_X_STRING_HPP
#define HOBBIT_X_STRING_HPP

// Both temporary formatting classes contain one pointer (PC ctor/dtor use +0).
class xvfs
{
public:
    xvfs(const char* format, char* arguments);
    ~xvfs();
    operator const char*();
private:
    char* m_pString;
};

class xfs
{
public:
    xfs(const char* format, ...);
    ~xfs();
    operator const char*();
private:
    char* m_pString;
};

class xwstring;

// PC layout: pointer at 0, inline bytes at 4, length at 0x24, capacity at 0x28.
class xstring
{
public:
    xstring();
    xstring(int reserve, const char* string = 0);
    xstring(char character);
    xstring(const char* string);
    xstring(const xstring& string);
    xstring(const xwstring& string);
    ~xstring();
    operator const char*() const { return m_pData; }
    int GetLength() const { return m_Length; }
    char& operator[](int index) { return m_pData[index]; }
    char GetAt(int index) const { return m_pData[index]; }
    void SetAt(int index, char character) { m_pData[index] = character; }
    int IsEmpty() const { return m_Length == 0; }
    void SetLength(int length);
    void Clear();
    void FreeExtra();
    void IndexToRowCol(int index, int& row, int& column) const;
    xstring Mid(int index, int count) const;
    xstring Left(int count) const;
    xstring Right(int count) const;
    // Descriptive PC-only identity; no public Xbox name established.
    int Replace(const char* search, const char* replacement, int start = 0);
    int Format(const char* format, ...);
    int FormatV(const char* format, char* arguments);
    int AddFormat(const char* format, ...);
    int AddFormatV(const char* format, char* arguments);
    int LoadFile(const char* filename);
    int SaveFile(const char* filename) const;
    void Dump(int lineFeed = 0) const;
    void DumpHex() const;
    void MakeUpper();
    void MakeLower();
    void Insert(int index, char character);
    void Insert(int index, const char* string);
    void Insert(int index, const xstring& string);
    void Delete(int index, int count = 1);
    int Find(char character, int start = 0) const;
    int Find(const char* string, int start = 0) const;
    int Find(const xstring& string, int start = 0) const;
    const xstring& operator=(char character);
    const xstring& operator=(const char* string);
    const xstring& operator=(const xstring& string);
    const xstring& operator=(const xwstring& string);
    const xstring& operator+=(char character);
    const xstring& operator+=(const char* string);
    const xstring& operator+=(const xstring& string);

private:
    friend class xwstring;
    friend xstring operator+(const xstring& first, char second);
    friend xstring operator+(char first, const xstring& second);
    friend xstring operator+(const xstring& first, const char* second);
    friend xstring operator+(const char* first, const xstring& second);
    friend xstring operator+(const xstring& first, const xstring& second);
    friend int operator==(const xstring& first, const char* second);
    friend int operator==(const char* first, const xstring& second);
    friend int operator==(const xstring& first, const xstring& second);
    friend int operator!=(const xstring& first, const char* second);
    friend int operator!=(const char* first, const xstring& second);
    friend int operator!=(const xstring& first, const xstring& second);
    friend int operator<(const xstring& first, const char* second);
    friend int operator<(const char* first, const xstring& second);
    friend int operator<(const xstring& first, const xstring& second);
    friend int operator>(const xstring& first, const char* second);
    friend int operator>(const char* first, const xstring& second);
    friend int operator>(const xstring& first, const xstring& second);
    friend int operator<=(const xstring& first, const char* second);
    friend int operator<=(const char* first, const xstring& second);
    friend int operator<=(const xstring& first, const xstring& second);
    friend int operator>=(const xstring& first, const char* second);
    friend int operator>=(const char* first, const xstring& second);
    friend int operator>=(const xstring& first, const xstring& second);
    void EnsureCapacity(int capacity);
    // Descriptive recovered names: these three PC helper names are not in the
    // Xbox public map. Their separate call boundaries are retained in PC.
    void Init(int reserve);
    void Init(int length, const char* string);
    void Init(int length, const unsigned short* string);
    char* m_pData;
    char m_LocalData[32];
    int m_Length;
    int m_BufferSize;
};

// PC wide layout is 0x4c, independently observed; SDK's 0x2c estimate is wrong.
class xwstring
{
public:
    xwstring();
    xwstring(int reserve, const unsigned short* string = 0);
    xwstring(unsigned short character);
    xwstring(const unsigned short* string);
    xwstring(const char* string);
    xwstring(const xwstring& string);
    ~xwstring();
    operator const unsigned short*() const { return m_pData; }
    int GetLength() const { return m_Length; }
    unsigned short& operator[](int index) { return m_pData[index]; }
    unsigned short GetAt(int index) const { return m_pData[index]; }
    void SetAt(int index, unsigned short character) { m_pData[index] = character; }
    int IsEmpty() const { return m_Length == 0; }
    void Clear();
    void FreeExtra();
    const xwstring& operator=(unsigned short character);
    const xwstring& operator=(const unsigned short* string);
    const xwstring& operator=(const xwstring& string);
    const xwstring& operator=(const xstring& string);
    const xwstring& operator+=(unsigned short character);
    const xwstring& operator+=(const unsigned short* string);
    const xwstring& operator+=(const xwstring& string);
    void Insert(int index, unsigned short character);
    void Insert(int index, const unsigned short* string);
    void Insert(int index, const xwstring& string);
    void Delete(int index, int count = 1);
    int Find(unsigned short character, int start = 0) const;
    int Find(const unsigned short* string, int start = 0) const;
    int Find(const xwstring& string, int start = 0) const;
    int LoadFile(const char* filename);
    int SaveFile(const char* filename) const;
    void DumpHex() const;
    // Descriptive PC-only identity, established by prefix extraction/comparison.
    int StartsWith(const xwstring& prefix) const;
    int GetHashKey(int index, int count) const;
    xwstring Mid(int index, int count) const;
    xwstring Left(int count) const;
    xwstring Right(int count) const;

private:
    friend class xstring;
    friend xwstring operator+(const xwstring& first, unsigned short second);
    friend xwstring operator+(unsigned short first, const xwstring& second);
    friend xwstring operator+(const xwstring& first, const unsigned short* second);
    friend xwstring operator+(const unsigned short* first, const xwstring& second);
    friend xwstring operator+(const xwstring& first, const xwstring& second);
    friend int operator==(const xwstring& first, const unsigned short* second);
    friend int operator==(const unsigned short* first, const xwstring& second);
    friend int operator==(const xwstring& first, const xwstring& second);
    friend int operator!=(const xwstring& first, const unsigned short* second);
    friend int operator!=(const unsigned short* first, const xwstring& second);
    friend int operator!=(const xwstring& first, const xwstring& second);
    friend int operator<(const xwstring& first, const unsigned short* second);
    friend int operator<(const unsigned short* first, const xwstring& second);
    friend int operator<(const xwstring& first, const xwstring& second);
    friend int operator>(const xwstring& first, const unsigned short* second);
    friend int operator>(const unsigned short* first, const xwstring& second);
    friend int operator>(const xwstring& first, const xwstring& second);
    friend int operator<=(const xwstring& first, const unsigned short* second);
    friend int operator<=(const unsigned short* first, const xwstring& second);
    friend int operator<=(const xwstring& first, const xwstring& second);
    friend int operator>=(const xwstring& first, const unsigned short* second);
    friend int operator>=(const unsigned short* first, const xwstring& second);
    friend int operator>=(const xwstring& first, const xwstring& second);
    void EnsureCapacity(int capacity);
    void Init(int reserve);
    void Init(int length, const char* string);
    void Init(int length, const unsigned short* string);
    unsigned short* m_pData;
    unsigned short m_LocalData[32];
    int m_Length;
    int m_BufferSize;
};

#endif

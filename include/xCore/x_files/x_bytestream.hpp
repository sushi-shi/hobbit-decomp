#ifndef HOBBIT_X_BYTESTREAM_HPP
#define HOBBIT_X_BYTESTREAM_HPP

// PC methods load the sole pointer at +0. Allocation metadata is stored at
// m_pData-8 (allocated bytes) and m_pData-4 (stream length).
class xbytestream
{
public:
    xbytestream();
    xbytestream(const void* data, int count);
    xbytestream(const xbytestream& other);
    ~xbytestream();
    int GetLength() const;
    void Clear();
    void FreeExtra();
    unsigned char GetAt(int index) const;
    void SetAt(int index, unsigned char value);
    unsigned char* GetBuffer() const;
    void Insert(int index, unsigned char value);
    void Insert(int index, const void* data, int count);
    void Insert(int index, const xbytestream& other);
    void Delete(int index, int count = 1);
    void Append(unsigned char value);
    void Append(const void* data, int count);
    void Append(const xbytestream& other);
    int Find(unsigned char value, int start = 0) const;
    int Find(const xbytestream& pattern, int start = 0) const;
    const xbytestream& operator=(unsigned char value);
    const xbytestream& operator=(const xbytestream& other);
    const xbytestream& operator+=(const xbytestream& other);
    const xbytestream& operator+=(unsigned char value);
    const xbytestream& operator<<(const char* text);
    const xbytestream& operator<<(unsigned char value);
    const xbytestream& operator<<(signed char value);
    const xbytestream& operator<<(unsigned short value);
    const xbytestream& operator<<(short value);
    const xbytestream& operator<<(unsigned int value);
    const xbytestream& operator<<(int value);
    const xbytestream& operator<<(const xbytestream& other);
    int LoadFile(const char* name);
    int SaveFile(const char* name) const;

private:
    void EnsureCapacity(int capacity);
    unsigned char* m_pData;
};
#endif

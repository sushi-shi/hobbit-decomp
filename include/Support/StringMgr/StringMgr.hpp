#ifndef HOBBIT_STRINGMGR_HPP
#define HOBBIT_STRINGMGR_HPP

#include <xCore/x_files/x_array.hpp>

class string_table {
    friend class string_mgr;

public:
    string_table();
    ~string_table();
    int Load(const char* tableName, const char* fileName);
    int GetCount() const;
    const unsigned short* GetAt(int index) const;
    const unsigned short* GetAt(const char* lookupString) const;
    const unsigned short* FindString(const char* lookupString) const;
    const unsigned short* GetSubTitleSpeaker(const char* lookupString) const;
    // Descriptive PC-only name; the original method spelling is not established.
    int FindIndex(const char* lookupString) const;

protected:
    char* m_pTableName;
    unsigned char* m_pData;
    int m_nStrings;
    // Sibling name for the fourth word in the PC's 16-byte allocation.
    // Its Hobbit semantics are unresolved; the constructor leaves it untouched.
    int m_version;
};

class string_mgr {
public:
    string_mgr();
    ~string_mgr();
    int LoadTable(const char* tableName, const char* fileName);
    void UnloadTable(const char* tableName);
    int GetStringCount(const char* tableName);
    const unsigned short* operator()(const char* tableName, int index) const;
    const unsigned short* operator()(const char* tableName, const char* title) const;
    const unsigned short* GetSubTitleSpeaker(const char* tableName, const char* speaker) const;
    // Descriptive PC-only name, paired with string_table::FindIndex.
    int FindIndex(const char* tableName, const char* lookupString) const;
    const string_table* FindTable(const char* tableName) const;

protected:
    static int m_Initialized;
    xarray<string_table*> m_Tables;
};

extern string_mgr g_StringMgr;

#endif

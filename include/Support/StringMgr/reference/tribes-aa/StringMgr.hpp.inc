//==============================================================================
//
//  StringMgr.hpp
//
//==============================================================================

#ifndef STRINGMGR_HPP
#define STRINGMGR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_files.hpp>

//==============================================================================
//  string_table
//==============================================================================

class string_table
{
    friend class string_mgr;

public:
                        string_table    ( void );
                       ~string_table    ( void );

    xbool               Load            ( const char* pTableName, const char* pFileName );
    const xwchar*       GetAt           ( s32 Index ) const;

protected:
    const char*     m_pTableName;
    byte*           m_pData;
    s32             m_nStrings;
};

//==============================================================================
//  string_mgr
//==============================================================================

class string_mgr
{
public:
                        string_mgr      ( void );
                       ~string_mgr      ( void );

    xbool               LoadTable       ( const char* pTableName, const char* pFileName );
    void                UnloadTable     ( const char* pTableName );
    const string_table* FindTable       ( const char* pTableName ) const;
    const xwchar*       operator ()     ( const char* pTableName, s32 Index ) const;

protected:
    static xbool            m_Initialized;
    xarray<string_table*>   m_Tables;
};

//==============================================================================

extern string_mgr StringMgr;

//==============================================================================
#endif // STRINGMGR_HPP
//==============================================================================

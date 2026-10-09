//==============================================================================
//
//  StringMgr.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/StringMgr/StringMgr.hpp>

//==============================================================================
//  STORAGE
//==============================================================================

string_mgr StringMgr;               // Instantiate the one and only string manager

//------------------------------------------------------------------------------

xbool   string_mgr::m_Initialized = FALSE;

//==============================================================================
//  FUNCTIONS
//==============================================================================

string_table::string_table( void )
{
    m_pTableName    = NULL;
    m_pData         = NULL;
    m_nStrings      = 0;
}

//==============================================================================

string_table::~string_table( void )
{
    if( m_pData )
    {
        x_free( m_pData );
        m_pData     = NULL;
        m_nStrings  = 0;
    }
}

//==============================================================================

xbool string_table::Load( const char* pTableName, const char* pFileName )
{
    xbool Success = FALSE;

    X_FILE* pFile = x_fopen( pFileName, "rb" );
    ASSERT( pFile );
    if( pFile )
    {
        s32 Length = x_flength( pFile );

        m_pData = (byte*)x_malloc( Length );
        ASSERT( m_pData );
        VERIFY( x_fread( m_pData, 1, Length, pFile ) == Length );
        x_fclose( pFile );

        m_pTableName = pTableName;
        m_nStrings = *(s32*)m_pData;

        Success = TRUE;
    }

    return Success;
}

//==============================================================================

const xwchar* string_table::GetAt( s32 Index ) const
{
    const xwchar* pString = NULL;

    ASSERT( (Index >= 0) && (Index < m_nStrings) );
    s32* pIndex = (s32*)(m_pData+4);
    pString = (const xwchar*)(m_pData+4+4*m_nStrings+pIndex[Index]);

    return pString;
}

//==============================================================================

string_mgr::string_mgr( void )
{
    ASSERT( !m_Initialized );
    m_Initialized = TRUE;
}

//==============================================================================

string_mgr::~string_mgr( void )
{
    ASSERT( m_Initialized );
    m_Initialized = FALSE;
}

//==============================================================================

xbool string_mgr::LoadTable( const char* pTableName, const char* pFileName )
{
    // Assert that the table is not already loaded
    ASSERT( FindTable( pTableName ) == 0 );

    // Create a new table
    string_table* pTable = new string_table;

    // Load the table
    xbool Success = pTable->Load( pTableName, pFileName );

    // Keep it or heave it.
    if( Success )
    {
        m_Tables.Append( pTable );
    }
    else
    {
        delete pTable;
    }

    // Return success code
    return Success;
}

//==============================================================================

void string_mgr::UnloadTable( const char* pTableName )
{
    s32 iTable = -1;
    
    // Find Table
    for( s32 i=0 ; i<m_Tables.GetCount() ; i++ )
    {
        if( x_strcmp( m_Tables[i]->m_pTableName, pTableName ) == 0 )
        {
            iTable = i;
            break;
        }
    }

    // Found?
    if( iTable != -1 )
    {
        delete m_Tables[iTable];
        m_Tables.Delete( iTable );
    }
}

//==============================================================================

const string_table* string_mgr::FindTable( const char* pTableName ) const
{
    // Find Table
    for( s32 i=0 ; i<m_Tables.GetCount() ; i++ )
    {
        if( x_strcmp(m_Tables[i]->m_pTableName, pTableName) == 0 )
        {
            return m_Tables[i];
        }
    }

    return NULL;
}

//==============================================================================

const xwchar* string_mgr::operator () ( const char* pTableName, s32 Index ) const
{
    const xwchar*       pString = NULL;

    const string_table* pTable  = FindTable( pTableName );
    if( pTable )
    {
        pString = pTable->GetAt( Index );
    }
    return pString;
}

//==============================================================================


#ifndef DHARRY_HPP
#define DHARRY_HPP

///////////////////////////////////////////////////////////////////////////
// INCLUDES 
///////////////////////////////////////////////////////////////////////////

#include <xCore/x_files/x_files.hpp>

///////////////////////////////////////////////////////////////////////////
// Dynamic Handle Array
///////////////////////////////////////////////////////////////////////////

struct xhandle
{
    s32 Handle;

    xbool operator == ( const xhandle& H  );
    xbool operator != ( const xhandle& H  );
    xbool operator <  ( const xhandle& H  );
    xbool operator <= ( const xhandle& H  );
    xbool operator >  ( const xhandle& H  );
    xbool operator >= ( const xhandle& H  );

    xbool operator == ( const s32 D  );
    xbool operator != ( const s32 D  );
    xbool operator <  ( const s32 D  );
    xbool operator <= ( const s32 D  );
    xbool operator >  ( const s32 D  );
    xbool operator >= ( const s32 D  );

    s32   operator =  ( const s32 D  );
};

template< class T >
class dharray
{
public:
                dharray             ( void );
               ~dharray             ( void );
    s32         GetCount            ( void ) const;
    s32         GetCapacity         ( void ) const;
    void        DeleteAll           ( xbool bReorder = FALSE );
    T&          AddNode             ( void );
    T&          AddNode             ( xhandle&  hHandle  );
    void        GrowListBy          ( s32       nNodes   );
    T&          operator[]          ( s32       Index    );
    const T&    operator[]          ( s32       Index    ) const;
    T&          operator()          ( xhandle   hHandle  );
    const T&    operator()          ( xhandle   hHandle  ) const;
    void        DeleteNodeByIndex   ( s32       Index    );
    void        DeleteNodeByHandle  ( xhandle   hHandle  );
    xhandle     GetHandleFromIndex  ( s32       Index    );
    s32         GetIndexFromHandle  ( xhandle   hHandle  );
    xbool       Save                ( X_FILE*   Fp       );
    xbool       Load                ( X_FILE*   Fp       );

protected:

    s32         m_Capacity;
    s32         m_nNodes;
    xhandle*    m_pHandle;
    T*          m_pList;    
};

///////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////

//=========================================================================

template< class T >
dharray<T>::dharray( void )
{
    m_Capacity    = 0;
    m_nNodes      = 0;
    m_pHandle     = NULL;
    m_pList       = NULL;    
}

//=========================================================================

template< class T >
dharray<T>::~dharray( void )
{
    if( m_pHandle ) delete []m_pHandle;
    if( m_pList   ) delete []m_pList;
}

//=========================================================================

template< class T >
void dharray<T>::GrowListBy( s32 nNodes )
{
    xhandle*    pNewHandle;
    T*          pNewList;    
    s32         i;

    ASSERT( nNodes > 0 );

    //
    // Increase the capacity
    //
    m_Capacity += nNodes;

    //
    // Allocate the new arrays
    //
    pNewHandle = new xhandle[ m_Capacity ];
    ASSERT( pNewHandle );

    pNewList   = new T[ m_Capacity ];
    ASSERT( pNewList );

    //
    // Copy all the previous nodes to the new arrays
    //
    for( i=0; i<m_nNodes; i++ )
    {
        s32 Index         = m_pHandle[i].Handle;
        pNewHandle[ i ]   = Index;
        pNewList[ Index ] = m_pList[ Index ];
    }

    //
    // Fill in the rest of the hash entries
    //
    for( ;i<m_Capacity; i++ )
    {
        pNewHandle[ i ] = i;
    }

    //
    // Update the class with the new lists
    //
    if( m_pHandle ) delete []m_pHandle;
    if( m_pList ) delete []m_pList;

    m_pHandle = pNewHandle;
    m_pList = pNewList;
}

//=========================================================================

template< class T >
T& dharray<T>::operator[]( s32 Index )
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    return m_pList[ m_pHandle[ Index ].Handle ];
}

//=========================================================================

template< class T >
const T& dharray<T>::operator[]( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    return m_pList[ m_pHandle[ Index ].Handle ];
}

//=========================================================================

template< class T >
T& dharray<T>::operator()( xhandle hHandle )
{
    return m_pList[ hHandle.Handle ];
}

//=========================================================================

template< class T >
const T& dharray<T>::operator()( xhandle hHandle ) const
{
    return m_pList[ hHandle.Handle ];
}

//=========================================================================

template< class T > inline
s32 dharray<T>::GetCount( void ) const
{
    return m_nNodes;
}

//=========================================================================

template< class T > inline
s32 dharray<T>::GetCapacity( void ) const
{
    return m_Capacity;
}
    
//=========================================================================

template< class T > inline
xhandle dharray<T>::GetHandleFromIndex( s32 Index )
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    return m_pHandle[ Index ];
}

//=========================================================================

template< class T > inline
s32 dharray<T>::GetIndexFromHandle( xhandle hHandle )
{
    for( s32 i=0; i<m_nNodes; i++ )
    {
        if( m_pHandle[ i ] == hHandle ) return i;
    }

    ASSERT( 0 && "Not hash entry contain that ID" );
    return 0;
}

//=========================================================================

template< class T > inline
void dharray<T>::DeleteNodeByIndex( s32 Index )
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    xhandle  HandleID = m_pHandle[ Index ];

    m_nNodes--;

    // Call Destructor? m_pList[ HandleID ]

    x_memmove( &m_pHandle[ Index ], &m_pHandle[ Index+1 ], sizeof(s32)*(m_nNodes-Index) );
    m_pHandle[ m_nNodes ] = HandleID;    
}

//=========================================================================

template< class T > inline
void dharray<T>::DeleteNodeByHandle( xhandle hHandle )
{
    DeleteNodeByIndex( GetIndexFromHandle( hHandle ) );
}

//=========================================================================

template< class T > inline
T& dharray<T>::AddNode( xhandle& hHandle )
{
    //
    // Grow if need it
    //
    if( m_nNodes >= m_Capacity )
    {
        GrowListBy( MAX( m_Capacity/2, 100 ) );
    }

    hHandle = m_pHandle[ m_nNodes ];
    m_nNodes++;

    return m_pList[ hHandle.Handle ];
}

//=========================================================================

template< class T > inline
T& dharray<T>::AddNode( void )
{
    xhandle hHandle;
    return AddNode( hHandle );
}

//=========================================================================

template< class T > inline
void dharray<T>::DeleteAll( xbool bReorder )
{
    m_nNodes = 0;

    if( bReorder )
    {
        for( s32 i=0; i<m_Capacity; i++ )
            m_pHandle[i] = i;
    }
}

//=========================================================================

template< class T > inline
xbool dharray<T>::Save( X_FILE* Fp )
{
    x_fwrite( this,         sizeof(*this),          1,              Fp );
    x_fwrite( m_pHandle,    sizeof(*m_pHandle),     m_Capacity,     Fp );
    x_fwrite( m_pList,      sizeof(*m_pList),       m_Capacity,     Fp );

    return TRUE;
}

//=========================================================================

template< class T > inline
xbool dharray<T>::Load( X_FILE* Fp )
{
    x_fread( this,         sizeof(*this),          1,          Fp );

    m_pHandle = new xhandle[ m_Capacity ];
    ASSERT( m_pHandle );

    m_pList   = new T[ m_Capacity ];
    ASSERT( m_pList );

    x_fread( m_pHandle,    sizeof(*m_pHandle),     m_Capacity, Fp );
    x_fread( m_pList,      sizeof(*m_pList),       m_Capacity, Fp );

    return TRUE;
}

//=========================================================================

inline xbool xhandle::operator == ( const xhandle& H  ) { return Handle == H.Handle; } 
inline xbool xhandle::operator != ( const xhandle& H  ) { return Handle != H.Handle; } 
inline xbool xhandle::operator <  ( const xhandle& H  ) { return Handle <  H.Handle; } 
inline xbool xhandle::operator <= ( const xhandle& H  ) { return Handle <= H.Handle; } 
inline xbool xhandle::operator >  ( const xhandle& H  ) { return Handle >  H.Handle; } 
inline xbool xhandle::operator >= ( const xhandle& H  ) { return Handle >= H.Handle; } 

inline xbool xhandle::operator == ( const s32 D  ) { return Handle == D; } 
inline xbool xhandle::operator != ( const s32 D  ) { return Handle != D; } 
inline xbool xhandle::operator <  ( const s32 D  ) { return Handle <  D; } 
inline xbool xhandle::operator <= ( const s32 D  ) { return Handle <= D; } 
inline xbool xhandle::operator >  ( const s32 D  ) { return Handle >  D; } 
inline xbool xhandle::operator >= ( const s32 D  ) { return Handle >= D; } 

inline s32   xhandle::operator =  ( const s32 D )  { return Handle =  D; } 

///////////////////////////////////////////////////////////////////////////
// EMD 
///////////////////////////////////////////////////////////////////////////
#endif


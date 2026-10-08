#ifndef HOBBIT_X_ARRAY_HPP
#define HOBBIT_X_ARRAY_HPP

#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_plus.hpp>

// PC xarray is 28 bytes. Matching StringMgr and BinOut consumers establish
// the field offsets and growth behavior; it differs from both sibling layouts.
// Only template bodies supported by PC evidence are defined here. Unused
// sibling interfaces remain declarations until their behavior is recovered.
#define HOBBIT_XARRAY_MAX(a, b) (((a) > (b)) ? (a) : (b))
template<class T> class xarray {
public:
    xarray(void);
    xarray(const xarray<T>& Array);
    xarray(T* Array, int Capacity, int Count = 0);
    ~xarray(void);

    operator T*(void) const;
    T& operator[](int Index) const;

    T& GetAt(int Index) const;
    void SetAt(int Index, const T& Element);

    T& Insert(int Index);
    void Insert(int Index, const T& Element);
    void Insert(int Index, const xarray<T>& Array);
    void Delete(int Index, int Count = 1);

    T& Append(void);
    void Append(const T& Element);
    void Append(const xarray<T>& Array);

    int GetCount(void) const;
    void SetCount(int NewCount);
    int GetCapacity(void) const;
    void SetCapacity(int Capacity);

    void SetGrowAmount(int Amt);

    void Clear(void);
    void FreeExtra(void);

    int Find(const T& Element, int StartIndex = 0) const;

    void SetLocked(int Locked);
    int IsLocked(void) const;
    int IsStatic(void) const;

    int Save(X_FILE* pFile) const;
    int Load(X_FILE* pFile);

    T* GetPtr(void) const;

    const xarray<T>& operator=(const xarray<T>& Array);
    const xarray<T>& operator+=(const xarray<T>& Array);
    const xarray<T>& operator+=(const T& Element);
    const xarray<T> operator+(const xarray<T>& Array) const;

protected:
    int CalcGrowth(void);

    // Genuine initialized storage, not padding: constructors zero both words.
    // Their semantics are unresolved. Count/data begin at offsets 8/12.
    unsigned int m_UnresolvedWord0;
    unsigned int m_UnresolvedWord4;
    int m_Count;
    T* m_pData;
    int m_Capacity;
    int m_Status;
    int m_GrowAmount;
};

#define HOBBIT_XARRAY_STATUS_NORMAL 0
#define HOBBIT_XARRAY_STATUS_LOCKED 1
#define HOBBIT_XARRAY_STATUS_STATIC 2

template<class T> inline T& xarray<T>::operator[](int Index) const {

    return (m_pData[Index]);
}

template<class T> inline T& xarray<T>::GetAt(int Index) const {

    return (m_pData[Index]);
}

template<class T> inline int xarray<T>::GetCount(void) const {
    return (m_Count);
}

template<class T> inline int xarray<T>::GetCapacity(void) const {
    return (m_Capacity);
}

template<class T> xarray<T>::xarray(void) {
    m_pData = 0;
    m_Count = 0;
    m_Capacity = 0;
    m_GrowAmount = 0;
    m_Status = HOBBIT_XARRAY_STATUS_NORMAL;
    m_UnresolvedWord0 = 0;
    m_UnresolvedWord4 = 0;
}

template<class T> xarray<T>::~xarray(void) {
    if (m_Status != HOBBIT_XARRAY_STATUS_STATIC) {
        delete[] m_pData;
    }
}

template<class T> T& xarray<T>::Insert(int Index) {
    int i, iElement;

    if (m_Capacity < m_Count + 1) {

        int NewCapacity;
        if (m_GrowAmount == 0) {
            NewCapacity = HOBBIT_XARRAY_MAX(1, m_Count + HOBBIT_XARRAY_MAX(1, m_Capacity / 2));
        } else {
            NewCapacity = m_Count + m_GrowAmount;
        }
        T* pNewData = new T[NewCapacity];

        for (i = 0; i < Index; i++) {
            pNewData[i] = m_pData[i];
        }

        for (i = Index; i < m_Count; i++) {
            pNewData[i + 1] = m_pData[i];
        }

        iElement = Index;

        delete[] m_pData;
        m_pData = pNewData;
        m_Capacity = NewCapacity;

        m_Count += 1;
    } else {

        for (i = m_Count; i > Index; i--) {
            m_pData[i] = m_pData[i - 1];
        }

        iElement = Index;

        m_Count += 1;
    }

    return m_pData[iElement];
}

template<class T> inline void xarray<T>::Insert(int Index, const T& Element) {
    Insert(Index) = Element;
}

template<class T> void xarray<T>::Delete(int Index, int Count) {
    int Shift;

    Shift = m_Count - (Index + Count);
    while (Shift > 0) {
        m_pData[Index] = m_pData[Index + Count];
        Shift--;
        Index++;
    }

    m_Count -= Count;
}

template<class T> T& xarray<T>::Append(void) {
    if (m_Capacity < m_Count + 1) {

        int i;
        T* pNewData;

        int NewCapacity;
        if (m_GrowAmount == 0) {
            NewCapacity = HOBBIT_XARRAY_MAX(1, m_Count + HOBBIT_XARRAY_MAX(1, m_Capacity / 2));
        } else {
            NewCapacity = m_Count + m_GrowAmount;
        }
        pNewData = new T[NewCapacity];

        for (i = 0; i < m_Count; i++) {
            pNewData[i] = m_pData[i];
        }

        delete[] m_pData;
        m_pData = pNewData;
        m_Capacity = NewCapacity;
    }

    m_Count++;

    return (m_pData[m_Count - 1]);
}

template<class T> void xarray<T>::Append(const T& Element) {
    if (m_Capacity < m_Count + 1) {

        int i;
        T* pNewData;

        int NewCapacity;
        if (m_GrowAmount == 0) {
            NewCapacity = HOBBIT_XARRAY_MAX(1, m_Count + HOBBIT_XARRAY_MAX(1, m_Capacity / 2));
        } else {
            NewCapacity = m_Count + m_GrowAmount;
        }
        pNewData = new T[NewCapacity];

        for (i = 0; i < m_Count; i++) {
            pNewData[i] = m_pData[i];
        }

        delete[] m_pData;
        m_pData = pNewData;
        m_Capacity = NewCapacity;
    }

    m_pData[m_Count] = Element;

    m_Count++;
}

template<class T> void xarray<T>::SetCapacity(int Capacity) {

    if (Capacity != m_Capacity) {
        int i;
        T* pNewData = new T[Capacity];

        for (i = 0; i < m_Count; i++) {
            pNewData[i] = m_pData[i];
        }

        delete[] m_pData;
        m_pData = pNewData;
        m_Capacity = Capacity;
    }
}

template<class T> void xarray<T>::Clear(void) {
    if (m_Status == HOBBIT_XARRAY_STATUS_NORMAL) {
        delete[] m_pData;
        m_pData = 0;
        m_Count = 0;
        m_Capacity = 0;
    } else {
        m_Count = 0;
    }
}

template<class T> void xarray<T>::FreeExtra(void) {
    if (m_Capacity > m_Count) {
        int i;
        T* pNewData = new T[m_Count];
        for (i = 0; i < m_Count; i++) {
            pNewData[i] = m_pData[i];
        }
        delete[] m_pData;
        m_pData = pNewData;
        m_Capacity = m_Count;
    }
}

template<class T> const xarray<T>& xarray<T>::operator=(const xarray<T>& Array) {
    int i;

    if (this != &Array) {
        if (m_Capacity < Array.m_Count) {

            delete[] m_pData;

            m_pData = new T[Array.m_Count];

            m_Count = 0;
            m_Capacity = Array.m_Count;
        }

        for (i = 0; i < Array.m_Count; i++) {

            m_pData[i] = Array.m_pData[i];
        }

        m_Count = Array.m_Count;
    }

    return (*this);
}

template<class T> void xarray<T>::SetGrowAmount(int Amt) {

    m_GrowAmount = Amt;
}

template<class T> T* xarray<T>::GetPtr(void) const {
    return m_pData;
}

// Genuine remaining sibling template API bodies adapted to existing member names.
// The two unresolved Hobbit prefix words are not given invented semantics.
template< class T >
inline int xarray<T>::CalcGrowth( void )
{
    int GrowBy = 1;

#ifdef TARGET_PC
    if( m_GrowAmount == 0 )
        GrowBy = MAX( 128, (m_Capacity/2) );
    else
        GrowBy = m_GrowAmount;
#else
    if( m_GrowAmount == 0 )
        GrowBy = 1;
    else
        GrowBy = 1;
#endif

    return GrowBy;
}

template< class T >
inline void xarray<T>::SetAt( int Index, const T& Element )
{
    ASSERT( Index >= 0 );
    ASSERT( Index <  m_Count );

    m_pData[ Index ] = Element;
}

template< class T >
inline void xarray<T>::SetCount( int NewCount )
{
    SetCapacity( MAX( NewCount, m_Capacity ) );
    ASSERT(NewCount <= m_Capacity);
    m_Count = NewCount;
}

template< class T >
inline xarray<T>::operator T* ( void ) const
{
    return( m_pData );
}

template< class T >
inline void xarray<T>::SetLocked( int Locked )
{
    ASSERT( m_Status != HOBBIT_XARRAY_STATUS_STATIC );
    m_Status = Locked ? HOBBIT_XARRAY_STATUS_LOCKED : HOBBIT_XARRAY_STATUS_NORMAL;
}

template< class T >
inline int xarray<T>::IsLocked( void ) const
{
    return( (m_Status == HOBBIT_XARRAY_STATUS_LOCKED) || (m_Status == HOBBIT_XARRAY_STATUS_STATIC) );
}

template< class T >
inline int xarray<T>::IsStatic( void ) const
{
    return( m_Status == HOBBIT_XARRAY_STATUS_STATIC );
}

template< class T >
inline const xarray<T>& xarray<T>::operator += ( const xarray<T>& Array )
{
    Append( Array );
    return( *this );
}

template< class T >
inline const xarray<T>& xarray<T>::operator += ( const T& Element )
{
    Append( Element );
    return( *this );
}

template< class T >
xarray<T>::xarray( const xarray<T>& Array )
{
    int i;

    m_Status        = HOBBIT_XARRAY_STATUS_NORMAL;
    m_Capacity      = Array.GetCount();
    m_Count         = m_Capacity;
    m_GrowAmount    = 0;
    m_pData         = new T[ m_Count ];

    for( i = 0; i < m_Count; i++ )
    {
        m_pData[i] = Array.m_pData[i];
    }
}

template< class T >
xarray<T>::xarray( T* Array, int Capacity, int Count )
{
    ASSERT( Array );
    ASSERT( Count    >= 0 );
    ASSERT( Capacity >= Count );
    m_pData         = Array;
    m_Count         = Count;
    m_GrowAmount    = 0;
    m_Capacity      = Capacity;
    m_Status        = HOBBIT_XARRAY_STATUS_STATIC;
}

template< class T >
void xarray<T>::Insert( int Index, const xarray<T>& Array )
{
    int i;
    ASSERT( Index >= 0 );
    ASSERT( Index <= m_Count ); // Allow "insertion" at the end of the array.
    ASSERT( *this != Array );   // Can't insert self into self!  (Ouch!)

    if( m_Capacity < (m_Count + Array.m_Count) )
    {
        // Need to grow the buffer.
        ASSERT( m_Status != HOBBIT_XARRAY_STATUS_LOCKED );
        ASSERT( m_Status != HOBBIT_XARRAY_STATUS_STATIC );

        int NewCapacity = m_Count + Array.m_Count;
        T*  pNewData    = new T[ NewCapacity ];

        // Copy over elements below insertion point.
        for( i = 0; i < Index; i++ )
        {
            pNewData[i] = m_pData[i];
        }

        // Copy over elements above insertion point.
        for( i = Index; i < m_Count; i++ )
        {
            pNewData[ i + Array.m_Count ] = m_pData[i];
        }

        // Copy in the inserted elements.
        for( i = 0; i < Array.m_Count; i++ )
        {
            pNewData[ i + Index ] = Array.m_pData[i];
        }

        // Ditch the old allocation, and install the new one.
        delete [] m_pData;
        m_pData    = pNewData;
        m_Capacity = NewCapacity;

        // Update count.
        m_Count += Array.m_Count;
    }
    else
    {
        // We don't need to grow.

        // Shift elements over.
        for( i = m_Count + Array.m_Count - 1; i >= Index + Array.m_Count; i-- )
        {
            m_pData[i] = m_pData[ i - Array.m_Count ];
        }

        // Bring in new elements.
        for( i = 0; i < Array.m_Count; i++ )
        {
            m_pData[ i + Index ] = Array.m_pData[i];
        }

        // Update count.
        m_Count += Array.m_Count;
    }   
}

template< class T >
void xarray<T>::Append( const xarray<T>& Array )
{
    int i;
    ASSERT( *this != Array );   // Can't append self onto self!

    if( m_Capacity < (m_Count + Array.m_Count) )
    {
        // Need to grow the buffer.
        ASSERT( m_Status != HOBBIT_XARRAY_STATUS_LOCKED );
        ASSERT( m_Status != HOBBIT_XARRAY_STATUS_STATIC );

        T*  pNewData = new T[ m_Count + Array.m_Count ];

        // Copy over original elements.
        for( i = 0; i < m_Count; i++ )
        {
            pNewData[i] = m_pData[i];
        }

        // Ditch the old allocation, and install the new one.
        delete [] m_pData;
        m_pData    = pNewData;
        m_Capacity = m_Count + Array.m_Count;
    }

    // Bring in new elements.
    for( i = 0; i < Array.m_Count; i++ )
    {
        m_pData[ m_Count + i ] = Array.m_pData[i];
    }    

    // Update count.
    m_Count += Array.m_Count;
}

template< class T >
int xarray<T>::Find( const T& Element, int StartIndex ) const
{
    int i;

    ASSERT( StartIndex <= m_Count );

    // Linear search.
    for( i = StartIndex; i < m_Count; i++ )
        if( m_pData[i] == Element )
            return( i );

    // Not found!
    return( -1 );
}

template< class T >
const xarray<T> xarray<T>::operator + ( const xarray<T>& Array ) const
{
    xarray<T> Result;

    Result.SetCapacity( m_Count + Array.m_Count );
    Result.Append( *this );
    Result.Append( Array );

    return( Result );
}

template< class T >
int xarray<T>::Save( X_FILE* pFile ) const
{
    ASSERT( pFile );
    x_fwrite( this, sizeof(*this),1,pFile);
    x_fwrite( m_pData, sizeof(*m_pData), m_Count, pFile);

    return TRUE;
}

template< class T >
int xarray<T>::Load( X_FILE* pFile )
{
    ASSERT( pFile );
    x_fread( this, sizeof(*this),1,pFile);

    m_pData = new T[ m_Capacity ];

    x_fread( m_pData, sizeof(*m_pData), m_Count, pFile);

    return TRUE;
}

#undef HOBBIT_XARRAY_STATUS_NORMAL
#undef HOBBIT_XARRAY_STATUS_LOCKED
#undef HOBBIT_XARRAY_STATUS_STATIC

#undef HOBBIT_XARRAY_MAX

// Complete genuine sibling handle-array API; provisional, no PC layout claim.
template< class T >
class xharray
{
public:
                xharray             ( void );
               ~xharray             ( void );
    s32         GetCount            ( void ) const;

    s32         GetCapacity         ( void ) const;
    void        Clear               ( xbool bReorder = FALSE );
    T&          Add                 ( void );
    T&          Add                 ( xhandle&  hHandle  );

    T&          operator[]          ( s32       Index    );
    const T&    operator[]          ( s32       Index    ) const;
    T&          operator()          ( xhandle   hHandle  );
    const T&    operator()          ( xhandle   hHandle  ) const;

    void        DeleteByIndex       ( s32       Index    );
    void        DeleteByHandle      ( xhandle   hHandle  );
    xhandle     GetHandleByIndex    ( s32       Index    ) const;
    s32         GetIndexByHandle    ( xhandle   hHandle  ) const;

    s32         CalcGrowth          ( void );
    void        GrowListBy          ( s32       nNodes   );

    xbool       Save                ( X_FILE*   Fp       );
    xbool       Load                ( X_FILE*   Fp       );

    const   xharray<T>&  operator = ( const xharray<T>& Array   );

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
inline s32 xharray<T>::CalcGrowth( void )
{
    s32 GrowBy = 1;

#ifdef TARGET_PC
    GrowBy = MAX( 100, (m_Capacity/2) );
#else
    GrowBy = 1;
#endif

    return GrowBy;
}

//=========================================================================

template< class T > inline
xharray<T>::xharray( void )
{
    m_Capacity    = 0;
    m_nNodes      = 0;
    m_pHandle     = NULL;
    m_pList       = NULL;    
}

//=========================================================================

template< class T > inline
xharray<T>::~xharray( void )
{
    Clear( FALSE );

    delete[] m_pHandle;
    delete[] m_pList;
}

//=========================================================================

template< class T > inline
const xharray<T>& xharray<T>::operator = ( const xharray<T>& Array )
{
    //
    // delete all the nodes
    //
    Clear( FALSE );
    
    delete[] m_pHandle;
    delete[] m_pList;
    m_pHandle = NULL;
    m_pList = NULL;

    //
    // Reset all the variables
    //
    m_Capacity    = 0;
    m_nNodes      = 0;
    m_pHandle     = NULL;
    m_pList       = NULL;    

    //
    // Allocate the buffers
    //
    GrowListBy( Array.GetCount() );

    //
    // Copy all the nodes
    //
    for( s32 i=0; i<Array.GetCount(); i++ )
    {
        Add() = Array[i];
    }

    return *this;
}

//=========================================================================

template< class T >
void xharray<T>::GrowListBy( s32 nNodes )
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
    pNewList   = new T[m_Capacity];
    ASSERT( pNewList );

    pNewHandle = new xhandle[m_Capacity];
    ASSERT( pNewHandle );

    //
    // Copy all the previous nodes to the new arrays
    //
    for( i=0; i<m_nNodes; i++ )
    {
        pNewHandle[i] = m_pHandle[i];
        pNewList[i] = m_pList[i];
    }

    //
    // Fill in the rest of the hash entries
    //
    for( i = m_nNodes; i<m_Capacity; i++ )
    {
        pNewHandle[ i ].Handle = i;
    }

    //
    // Update the class with the new lists
    //
    delete[] m_pHandle;
    delete[] m_pList;

    m_pHandle = pNewHandle;
    m_pList   = pNewList;
}

//=========================================================================

template< class T > inline
T& xharray<T>::operator[]( s32 Index )
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    return m_pList[ m_pHandle[ Index ].Handle ];
}

//=========================================================================

template< class T > inline
const T& xharray<T>::operator[]( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    return m_pList[ m_pHandle[ Index ].Handle ];
}

//=========================================================================

template< class T > inline
T& xharray<T>::operator()( xhandle hHandle )
{
    ASSERT( hHandle >= 0 );
    ASSERT( hHandle < m_Capacity );

    return m_pList[ hHandle.Handle ];
}

//=========================================================================

template< class T > inline
const T& xharray<T>::operator()( xhandle hHandle ) const
{
    ASSERT( hHandle >= 0 );
    ASSERT( hHandle < m_Capacity );

    return m_pList[ hHandle.Handle ];
}

//=========================================================================

template< class T > inline
s32 xharray<T>::GetCount( void ) const
{
    return m_nNodes;
}

//=========================================================================

template< class T > inline
s32 xharray<T>::GetCapacity( void ) const
{
    return m_Capacity;
}
    
//=========================================================================

template< class T > inline
xhandle xharray<T>::GetHandleByIndex( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );
    ASSERT( m_pHandle[ Index ] != HNULL );

    return m_pHandle[ Index ];
}

//=========================================================================

template< class T > inline
s32 xharray<T>::GetIndexByHandle( xhandle hHandle ) const
{
    ASSERT( hHandle != HNULL );

    for( s32 i=0; i<m_nNodes; i++ )
    {
        if( m_pHandle[ i ] == hHandle ) return i;
    }

    ASSERT( 0 && "Not hash entry contain that ID" );
    return 0;
}

//=========================================================================

template< class T > inline
void xharray<T>::DeleteByIndex( s32 Index )
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nNodes );

    xhandle  HandleID = m_pHandle[ Index ];

    m_nNodes--;

    // Copy the last node into the deleted node. We don't care if it is the same.
    m_pHandle[ Index ]    = m_pHandle[ m_nNodes ];
    m_pHandle[ m_nNodes ] = HandleID;    
}

//=========================================================================

template< class T > inline
void xharray<T>::DeleteByHandle( xhandle hHandle )
{
    ASSERT( hHandle != HNULL );
    DeleteByIndex( GetIndexByHandle( hHandle ) );
}

//=========================================================================

template< class T > inline
T& xharray<T>::Add( xhandle& hHandle )
{
    //
    // Grow if need it
    //
    if( m_nNodes >= m_Capacity )
    {
        GrowListBy( CalcGrowth() );
    }

    hHandle = m_pHandle[ m_nNodes ];
    m_nNodes++;

    ASSERT( hHandle != HNULL );

    // return the node
    return m_pList[ hHandle.Handle ];
}

//=========================================================================

template< class T > inline
T& xharray<T>::Add( void )
{
    xhandle hHandle;
    return Add( hHandle );
}

//=========================================================================

template< class T > inline
void xharray<T>::Clear( xbool bReorder )
{
    s32 i;

    // reorder if the user request it
    if( bReorder )
    {
        for( i=0; i<m_Capacity; i++ )
            m_pHandle[i].Handle = i;
    }

    m_Capacity = 0;
    m_nNodes = 0;

    delete[] m_pHandle;
    delete[] m_pList;
    m_pHandle = NULL;
    m_pList = NULL;
}

//=========================================================================

template< class T > inline
xbool xharray<T>::Save( X_FILE* Fp )
{
    x_fwrite( this,         sizeof(*this),          1,              Fp );
    x_fwrite( m_pHandle,    sizeof(*m_pHandle),     m_Capacity,     Fp );
    x_fwrite( m_pList,      sizeof(*m_pList),       m_Capacity,     Fp );

    return TRUE;
}

//=========================================================================

template< class T > inline
xbool xharray<T>::Load( X_FILE* Fp )
{
    x_fread( this,         sizeof(*this),          1,          Fp );

    m_pList   = (T*)x_malloc( (sizeof(xhandle)+sizeof(T)) * m_Capacity );
    ASSERT( m_pList );

    m_pHandle = (xhandle*)(m_pList+m_Capacity);

    x_fread( m_pHandle,    sizeof(*m_pHandle),     m_Capacity, Fp );
    x_fread( m_pList,      sizeof(*m_pList),       m_Capacity, Fp );

    return TRUE;
}


#endif

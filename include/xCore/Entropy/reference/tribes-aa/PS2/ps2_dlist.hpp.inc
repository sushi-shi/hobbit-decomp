//=========================================================================
//
// ps2_dlist.hpp
//
//=========================================================================

#ifndef PS2_DLIST_H
#define PS2_DLIST_H

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_debug.hpp>

//=========================================================================
// DEFINES
//=========================================================================

// Comment in the DEBUG_PS2_DLIST define to fill dlist alloc's with 0xDEADBEEF
// (the dlist functions will be no longer inline - they are in ps2_dlist.cpp)
// For now - it is only turned on in debug mode
#ifdef X_DEBUG
#define DEBUG_PS2_DLIST
#endif


//=========================================================================
// CLASSES
//=========================================================================

class dlist
{
    byte* m_Addr;
    byte* m_StartAddr;
    byte* m_EndAddr;
    s32   m_Size;
  
public:

            dlist           (void);
            ~dlist          (void);
            dlist           ( byte* StartAddr, s32 Size );
    void    Setup           ( byte* StartAddr, s32 Size );
    void    Reset           ( void );
    byte*   GetStartAddr    ( void );

    // Inline functions (unless DEBUG_PS2_DLIST is defined)
    void*   Alloc           ( s32 Size );
    void    Dealloc         ( s32 Size );
    void    Align16         ( void );
    byte*   GetAddr         ( void );
	void    SetAddr         ( byte* Addr );
	s32     GetLength       ( void );
};


//=========================================================================
// INLINE FUNCTIONS
//=========================================================================

#ifndef DEBUG_PS2_DLIST

inline void dlist::Align16( void )
{
    while( ((u32)m_Addr) & 0xF )
    {
        *m_Addr = 0;
        m_Addr++;
    }

    ASSERT( m_Addr < m_EndAddr );
}

//=========================================================================

inline dlist::dlist( byte* StartAddr, s32 Size )
{
    Setup(StartAddr,Size);
}

//=========================================================================

inline void* dlist::Alloc( s32 Size )
{
    byte* AllocAddr;
    //printf("ALLOC: %8X %1d\n",m_Addr,Size);
    AllocAddr = m_Addr;
    m_Addr += Size;
    ASSERT( m_Addr < m_EndAddr );
    return AllocAddr;
}

//=========================================================================

inline void dlist::Dealloc( s32 Size )
{
    m_Addr -= Size;
    ASSERT( m_Addr >= m_StartAddr );
}

//=========================================================================

inline byte* dlist::GetAddr( void )
{
    return m_Addr;
}

//=========================================================================

inline 	void dlist::SetAddr ( byte* Addr )
{
	m_Addr = Addr;
}

//=========================================================================

inline s32 dlist::GetLength( void )
{
	return (s32)(m_Addr - m_StartAddr);
}

//=========================================================================

#endif  //#ifndef DEBUG_PS2_DLIST


//=========================================================================

#endif  //#ifndef PS2_DLIST_H

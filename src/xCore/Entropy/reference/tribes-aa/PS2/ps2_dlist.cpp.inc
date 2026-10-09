//=========================================================================
//
// PS2_DLIST.CPP
//
//=========================================================================

#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_string.hpp>
#include <xCore/Entropy/PS2/ps2_dlist.hpp>

//=========================================================================

dlist::dlist( void )
{
    m_StartAddr = 0;
    m_Size      = 0;
    m_EndAddr   = 0;
    m_Addr      = 0;
}

//=========================================================================

dlist::~dlist( void )
{
    m_StartAddr = 0;
    m_Size      = 0;
    m_EndAddr   = 0;
    m_Addr      = 0;
}

//=========================================================================

void dlist::Setup( byte* StartAddr, s32 aSize )
{
    m_StartAddr = StartAddr;
    m_Size      = aSize;
    m_EndAddr   = StartAddr + aSize;
    m_Addr      = StartAddr;
    //printf("SETUP: %8X %1d\n",m_Addr,aSize);
}
    
//=========================================================================

void dlist::Reset( void )
{
    m_Addr = m_StartAddr;
}

//=========================================================================

byte* dlist::GetStartAddr( void )
{
    return m_StartAddr;
}

//=========================================================================



//=========================================================================
// DEBUG IMPLEMENTATION
//=========================================================================

#ifdef DEBUG_PS2_DLIST

//=========================================================================

void dlist::Align16( void )
{
    while( ((u32)m_Addr) & 0xF )
    {
        *m_Addr = 0;
        m_Addr++;
    }

    ASSERTS( m_Addr < m_EndAddr,
             (const char*)xfs( "m_StartAddr:%08X  m_EndAddr:%08X  m_Addr:%08X", 
                               m_StartAddr, m_EndAddr, m_Addr ) );
}

//=========================================================================

dlist::dlist( byte* StartAddr, s32 Size )
{
    Setup(StartAddr,Size);
}

//=========================================================================

void* dlist::Alloc( s32 Size )
{
    ASSERT(Size) ;
    byte* AllocAddr;

    //printf("ALLOC: %8X %1d\n",m_Addr,Size);

    AllocAddr = m_Addr;
    m_Addr += Size;

    ASSERTS( m_Addr < m_EndAddr,
             (const char*)xfs( "m_StartAddr:%08X  m_EndAddr:%08X  m_Addr:%08X  Size:%d", 
                               m_StartAddr, m_EndAddr, m_Addr, Size ) );
    
    #if X_DEBUG
    // Fill with crap to try find vif/dma errors
    {
        byte* Dest = AllocAddr ;
        
        while(Size)
        {
            if (Size)
            {
                *Dest++ = 0xDE ;
                Size-- ;
            }

            if (Size)
            {
                *Dest++ = 0xAD ;
                Size-- ;
            }

            if (Size)
            {
                *Dest++ = 0xBE ;
                Size-- ;
            }

            if (Size)
            {
                *Dest++ = 0xEF ;
                Size-- ;
            }
        }

        ASSERT(Dest == m_Addr) ;
    }
    #endif

    return AllocAddr;
}

//=========================================================================

void dlist::Dealloc( s32 Size )
{
    m_Addr -= Size;
    ASSERTS( m_Addr >= m_EndAddr,
             (const char*)xfs( "m_StartAddr:%08X  m_EndAddr:%08X  m_Addr:%08X  Size:%d", 
                               m_StartAddr, m_EndAddr, m_Addr, Size ) );
}

//=========================================================================

byte* dlist::GetAddr( void )
{
    return m_Addr;
}

//=========================================================================

void dlist::SetAddr ( byte* Addr )
{
	m_Addr = Addr;
}

//=========================================================================

s32 dlist::GetLength( void )
{
	return (s32)(m_Addr - m_StartAddr);
}

//=========================================================================

#endif  //#ifdef DEBUG_PS2_DLIST





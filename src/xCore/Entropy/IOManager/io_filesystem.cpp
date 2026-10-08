#include <rva.h>
#include <xCore/Entropy/IOManager/io_filesystem.hpp>

// Genuine earlier PC class/storage owner. Remaining filesystem method bodies
// await revision recovery; full prior and later originals remain retained.
#if defined(TARGET_PC) && !defined(HOBBIT_IO_LATER_FILESYSTEM)

DATA(0x003f72c0)
io_fs g_IOFSMgr;

io_fs::io_fs( void )
{
    m_CurrentDFS      = -1;
    m_CurrentDFSIndex = 0;
    m_Retries         = 10;
    m_LogFlags        = 0;
}

//==============================================================================

io_fs::~io_fs( void )
{
}

//==============================================================================


#else
#include "reference/before-cohort19-iofs-PC/io_filesystem.cpp.inc"
#endif

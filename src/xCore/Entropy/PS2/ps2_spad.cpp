// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-alternate-platforms.md.
//=========================================================================
//
// PS2_SPAD.CPP
//
//=========================================================================

#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_string.hpp>
#include <xCore/Entropy/PS2/ps2_spad.hpp>

//=========================================================================
// GLOBALS
//=========================================================================

scratchpad SPAD;

//=========================================================================
// IMPLEMENTATION
//=========================================================================

scratchpad::scratchpad( void )
{
#if defined(X_ASSERT)
    m_bLocked = FALSE;
#endif
}
//=========================================================================

scratchpad::~scratchpad( void )
{
    ASSERT( !m_bLocked );
}

//=========================================================================

// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-alternate-platforms.md.
//=========================================================================
//
// ps2_gsreg.cpp
//
//=========================================================================

#include <xCore/Entropy/e_Engine.hpp>
#include <xCore/Entropy/PS2/ps2_misc.hpp>

//=========================================================================

byte* s_GsregData;

#ifdef X_ASSERT
gsreg_header* s_pGsregHeader = NULL;
#endif

//=========================================================================
// All of the functions are inlined...look in the header file
//=========================================================================

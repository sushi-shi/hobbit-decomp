// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-network-memcard.md.
#ifndef IOP_UTILITIES_HPP
#define IOP_UTILITIES_HPP

#include <xCore/x_files/x_types.hpp>

s32 iop_LoadModule   ( const char* pFilename, const char* pArg=NULL, s32 ArgLength = 0, xbool AllowFail = FALSE  );
void iop_UnloadModule( s32 Id );


#endif
// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-alternate-platforms.md.
#ifndef _XBOXMGR_HPP_
#define _XBOXMGR_HPP_

    #define kPOOL_GENERAL 0
    #define kPOOL_TILED   1
    #define kPOOL_TEMP    2
    #define kPOOL_RECORD  0
    #define kPOOL_PUSH    1

    #include <xCore/x_files/x_files.hpp>
    #include <xCore/Entropy/Xbox/QuikHeap.h>
    #include <xCore/Entropy/e_Singleton.hpp>
    #include <xCore/Entropy/Xbox/Xbox_private.hpp>

    #ifndef TARGET_XBOX
    #error not xbox target
    #endif

#endif

#ifndef HOBBIT_X_TARGET_HPP
#define HOBBIT_X_TARGET_HPP
// Original Area51 x_files/x_target.hpp enum; values corroborated by PC branches.
enum platform {
    PLATFORM_NONE = 0,
    PLATFORM_PC = (1 << 0),
    PLATFORM_GCN = (1 << 1),
    PLATFORM_PS2 = (1 << 2),
    PLATFORM_XBOX = (1 << 3),
    PLATFORM_ALL = 0xffffffff
};
#define PS2_ALIGNMENT(a)
#define GCN_ALIGNMENT(a)

#if defined(TARGET_PC)
#define TARGET_PLATFORM PLATFORM_PC
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#endif

#if defined( TARGET_PC ) || defined( TARGET_XBOX ) 
#define PC_ALIGNMENT(a) __declspec(align(a))
#else
#define PC_ALIGNMENT(a)
#endif

#endif

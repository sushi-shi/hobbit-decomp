#ifndef HOBBIT_X_TYPES_HPP
#define HOBBIT_X_TYPES_HPP

// Primitive aliases and generic helpers from the pinned Area51 x_types.hpp.
// Widths follow the existing Hobbit PC interfaces; guid/xhandle are provisional
// sibling API additions, not admitted PC layout or address claims.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned __int64 u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed __int64 s64;
typedef float f32;
typedef double f64;
typedef u8 byte;
typedef s32 xbool;
typedef u16 xwchar;
typedef u32 xalloctype;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NULL
#define NULL 0
#endif
#define U8_MAX 255
#define U8_MIN 0
#define U16_MAX 65535
#define U16_MIN 0
#define U32_MAX 4294967295U
#define U32_MIN 0
#define U64_MAX 18446744073709551615ui64
#define U64_MIN 0
#define S8_MAX 127
#define S8_MIN -128
#define S16_MAX 32767
#define S16_MIN -32768
#define S32_MAX ((s32)2147483647)
#define S32_MIN (-((s32)2147483647))
#define S64_MAX 9223372036854775807i64
#define S64_MIN (-9223372036854775807i64 - 1)
#define F32_MIN 1.175494351e-38f
#define F32_MAX 3.402823466e+38f
#define F64_MIN 2.2250738585072014e-308
#define F64_MAX 1.7976931348623158e+308
#define X_KILOBYTE(x) ((x) * 1024)
#define X_MEGABYTE(x) ((x) * 1048576)
#define X_SECTION(x)
#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MINMAX
#define MINMAX(a,v,b) MAX((a), MIN((v),(b)))
#endif
#ifndef IN_RANGE
#define IN_RANGE(a,v,b) ((a) <= (v) && (v) <= (b))
#endif
#ifndef BIT
#define BIT(x) (1 << (x))
#endif
#ifdef __cplusplus
inline s32 iMax(s32 a, s32 b) { return a > b ? a : b; }
inline f32 fMax(f32 a, f32 b) { return a > b ? a : b; }
inline s32 iMin(s32 a, s32 b) { return a < b ? a : b; }
inline f32 fMin(f32 a, f32 b) { return a < b ? a : b; }
template<class T> inline T x_abs(T a) { return a < 0 ? -a : a; }
template<class T> inline T x_min(T a, T b) { return a < b ? a : b; }
template<class T> inline T x_min(T a, T b, T c) { return x_min(x_min(a,b),c); }
template<class T> inline T x_max(T a, T b) { return a > b ? a : b; }
template<class T> inline T x_max(T a, T b, T c) { return x_max(x_max(a,b),c); }
template<class T> inline T x_sign(T a) { return a < 0 ? -1 : a > 0 ? 1 : 0; }
// Hobbit PC/Xbox Guid interfaces use the scalar unsigned 64-bit ABI.
// The later sibling class representation is retained as a version alternative.
#if defined(HOBBIT_LATER_GUID_CLASS)
struct guid {
    u64 Guid;
    guid() { Guid = 0; }
    guid(u64 value) { Guid = value; }
    operator const u64() const { return Guid; }
    u32 GetLow() { return (u32)Guid; }
    u32 GetHigh() { return (u32)(Guid >> 32); }
};
#else
typedef u64 guid;
#endif
#define NULL_GUID ((guid)0)
#define HNULL -1
struct xhandle {
    s32 Handle;
    xhandle() {}
    xhandle(s32 value) { Handle = value; }
    operator const s32() const { return Handle; }
    xbool IsNonNull() const { return Handle != HNULL; }
    xbool IsNull() const { return Handle == HNULL; }
};

// VC6's genuine <new> supplies the same placement-new/delete definitions as
// the sibling helper. Use one owner so real STL headers can include it too.
#include <new>
template<class T> inline void xConstruct(T* Ptr) {
    (void)new (Ptr) T;
}

template<class T, class S> inline T* xConstruct(T* Ptr, S& Ref) { return new (Ptr) T(Ref); }

#endif // __cplusplus

#endif

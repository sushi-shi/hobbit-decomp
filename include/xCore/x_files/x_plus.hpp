#ifndef HOBBIT_X_PLUS_HPP
#define HOBBIT_X_PLUS_HPP

#include <stdarg.h>
#include <xCore/x_files/x_target.hpp>
#include <xCore/x_files/x_color.hpp>
typedef va_list x_va_list;
#define x_va_start(list,prev) va_start(list,prev)
#define x_va_end(list) va_end(list)
#define x_va_arg(list,type) va_arg(list,type)
#include <xCore/x_files/x_math.hpp>

// Sibling x_plus.hpp endian operations; PC bitmap I/O preserves these exact
// integer promotions, including the repeated reads from the supplied lvalue.
#define ENDIAN_SWAP_16(A) ((static_cast<unsigned short>(A) >> 8) | (static_cast<unsigned short>(A) << 8))
#define ENDIAN_SWAP_32(A) ((static_cast<unsigned int>(A) >> 24) | (static_cast<unsigned int>(A) << 24) | ((static_cast<unsigned int>(A) & 0x00FF0000) >> 8) | ((static_cast<unsigned int>(A) & 0x0000FF00) << 8))

#ifdef LITTLE_ENDIAN
#define LITTLE_ENDIAN_16(A) A
#define LITTLE_ENDIAN_32(A) A
#define BIG_ENDIAN_16(A) ENDIAN_SWAP_16(A)
#define BIG_ENDIAN_32(A) ENDIAN_SWAP_32(A)
#endif
#ifdef BIG_ENDIAN
#define LITTLE_ENDIAN_16(A) ENDIAN_SWAP_16(A)
#define LITTLE_ENDIAN_32(A) ENDIAN_SWAP_32(A)
#define BIG_ENDIAN_16(A) A
#define BIG_ENDIAN_32(A) A
#endif

char* x_strsavecpy(char* pDest, const char* pSrc, s32 Count);
inline xbool x_ishex(s32 C) { return ((C >= 'A') && (C <= 'F')) || ((C >= 'a') && (C <= 'f')) || ((C >= '0') && (C <= '9')); }

#define _XBIN( A, L ) ( (u32)(((((u64)0##A)>>(3*L)) & 1)<<L) )
#define XBIN( N ) ((u32)( _XBIN( N,  0 ) | _XBIN( N,  1 ) | _XBIN( N,  2 ) | _XBIN( N,  3 ) | \
                          _XBIN( N,  4 ) | _XBIN( N,  5 ) | _XBIN( N,  6 ) | _XBIN( N,  7 ) | \
                          _XBIN( N,  8 ) | _XBIN( N,  9 ) | _XBIN( N, 10 ) | _XBIN( N, 11 ) | \
                          _XBIN( N, 12 ) | _XBIN( N, 13 ) | _XBIN( N, 14 ) | _XBIN( N, 15 ) ))
#define XBINH( N ) ( XBIN( N ) << 16 )



#define ALIGN_64(n) ((((s32)(n)) + 63) & -64)
#define ALIGN_32(n) ((((s32)(n)) + 31) & -32)
#define ALIGN_16(n) ((((s32)(n)) + 15) & -16)
#define ALIGN_8(n) ((((s32)(n)) + 7) & -8)
#define ALIGN_4(n) ((((s32)(n)) + 3) & -4)
#define ALIGN_2(n) ((((s32)(n)) + 1) & -2)
#define X_RAND_MAX 0x7fff

#define ENDIAN_SWAP_64(A)           ( (((u64)(A)) >> 56) |  \
                                      (((u64)(A)) << 56) |  \
                                      ((((u64)(A)) & 0x00FF000000000000) >> 40)   | \
                                      ((((u64)(A)) & 0x000000000000FF00) << 40)   | \
                                      ((((u64)(A)) & 0x0000FF0000000000) >> 24)   | \
                                      ((((u64)(A)) & 0x0000000000FF0000) << 24)   | \
                                      ((((u64)(A)) & 0x000000FF00000000) >> 8)    | \
                                      ((((u64)(A)) & 0x00000000FF000000) << 8) ) 

#define X_MAX_PATH 260
#define X_MAX_DRIVE 3
#define X_MAX_DIR 256
#define X_MAX_FNAME 256
#define X_MAX_EXT 256

// PC constructors initialize the sole state word at offset zero. The seed
// update and public names are supported by the sibling-engine random family.
class random
{
public:
    random();
    random(int seed);
    void srand(int seed);
    int rand();
    int irand(int minimum, int maximum);
    vector2 v2(float minX, float maxX, float minY, float maxY);
    vector3 v3(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
    xcolor color(unsigned char alpha = 255);
    float frand(float minimum, float maximum);

private:
    int m_Seed;
};

void x_srand(int seed);
int x_rand();
int x_irand(int minimum, int maximum);
float x_frand(float minimum, float maximum);

// Shared PC case-conversion tables. Names reconstructed from behavior.
extern unsigned char x_UpperCase[256];
extern unsigned char x_LowerCase[256];
extern unsigned char x_CharacterClass[256];

inline int x_isspace(int character)
{
    return x_CharacterClass[static_cast<unsigned char>(character)] & 1;
}

inline int x_isdigit(int character)
{
    return x_CharacterClass[static_cast<unsigned char>(character)] & 2;
}

inline int x_toupper(int character)
{
    int result;
    if (static_cast<unsigned int>(character) < 256)
        result = x_UpperCase[character];
    else
        result = character;
    return result;
}

inline int x_tolower(int character)
{
    int result;
    if (static_cast<unsigned int>(character) < 256)
        result = x_LowerCase[character];
    else
        result = character;
    return result;
}

// Recovered x_files interfaces. The integer search argument follows the Xbox
// x_strchr decorated symbol; the PC implementation narrows it to one byte.
char* x_strchr(const char* string, int character);
char* x_strrchr(const char* string, int character);
int x_strlen(const char* string);
int x_wstrlen(const unsigned short* string);
char* x_strcpy(char* destination, const char* source);
char* x_strncpy(char* destination, const char* source, int count);
char* x_strcat(char* front, const char* back);
char* x_strncat(char* front, const char* back, int count);
int x_strcmp(const char* first, const char* second);
int x_strncmp(const char* first, const char* second, int count);
int x_stricmp(const char* first, const char* second);
char* x_strstr(const char* string, const char* substring);
char* x_stristr(const char* string, const char* substring);
char* x_strtoupper(char* string);
char* x_strtolower(char* string);
unsigned short* x_wstrcpy(unsigned short* destination, const unsigned short* source);
unsigned short* x_wstrncpy(unsigned short* destination, const unsigned short* source, int count);
unsigned short* x_wstrcat(unsigned short* front, const unsigned short* back);
unsigned short* x_wstrncat(unsigned short* front, const unsigned short* back, int count);
int x_wstrcmp(const unsigned short* first, const unsigned short* second);
int x_wstricmp(const unsigned short* first, const unsigned short* second);
int x_wstrncmp(const unsigned short* first, const unsigned short* second, int count);
unsigned short* x_wstrstr(const unsigned short* string, const unsigned short* substring);
unsigned short* x_wstrchr(const unsigned short* string, int character);
unsigned short* x_wstrrchr(const unsigned short* string, int character);
void* x_memchr(void* buffer, int character, int count);
void* x_memcpy(void* destination, const void* source, int count);
void* x_memmove(void* destination, const void* source, int count);
void* x_memset(void* buffer, int character, int count);
int x_memcmp(const void* first, const void* second, int count);
unsigned int x_chksum(const void* buffer, int count);


typedef int compare_fn(const void*, const void*);
void x_qsort(const void* base, int count, int itemSize, compare_fn* compare);
void* x_bsearch(const void* key, const void* base, int count, int itemSize, compare_fn* compare);
void* x_bsearch(const void* key, const void* base, int count, int itemSize, compare_fn* compare, void*& location);
char* x_strdup(const char* string);
unsigned short* x_strdup(const unsigned short* string);
void x_memswap(void* first, void* second, int count);
int x_atoi(const char* string);
float x_atof(const char* string);
void x_strippath_dir(const char* path, char* directory);
void x_strippath_file(const char* path, char* filename);
void x_splitpath(const char* path, char* drive, char* directory, char* filename, char* extension);
void x_makepath(char* path, const char* drive, const char* directory, const char* filename, const char* extension);

// Genuine sibling generic sorting/encryption API, provisional PC instantiations.
void x_encrypt(void* data, s32 length, u32 key[4]);
void x_decrypt(void* data, s32 length, u32 key[4]);
template <class T>
class x_compare_functor
{
public:
    s32 operator()( T, T );
};

//------------------------------------------------------------------------------
//
//  What the hell is a PseudoQuickSort?  Simple!  Its "sorta quick sort".
//
//  Find a partition element and put it in the first position of the list.  The 
//  partition value is the median value of the first, middle, and last items.  
//  (Using the median value of these three items rather than just using the 
//  first item is a big win.)
//
//  Then, the usual partitioning and swapping, followed by swapping the 
//  partition element into place.
//
//  Then, of the two portions of the list which remain on either side of the 
//  partition, sort the smaller portion recursively, and sort the larger 
//  portion via another iteration of the same code.
//
//  Do not bother "quick sorting" lists (or sub lists) which have fewer than
//  RECURSION_THRESH items.  All final sorting is handled with an insertion sort
//  which is executed by the caller (x_qsort).  This is another huge win.  This 
//  means that this function does not actually completely sort the input list.  
//  It mostly sorts it.  That is, each item will be within RECURSION_THRESH 
//  positions of its correct position.  Thus, an insert sort will be able to 
//  finish off the sort process without a serious performance hit.
//
//  All data swaps are done in-line, which trades some code space for better
//  performance.  There are only three swap points, anyway.
//
//------------------------------------------------------------------------------

template <class T, class Cmp>
static
void PseudoQuickSort( T*      pBase,
                      T*      pMax,           
                      s32     RecursionThresh,
                      s32     PartitionThresh,
                      Cmp     Compare )
{
    T*  i;
    T*  j;
    T*  jj;
    T*  mid;
    T*  tmp;
    s32 lo;
    s32 hi;

    lo = (s32)(pMax - pBase);   // Total data to sort in objects.

    // Deep breath now...
    do
    {
        //
        // At this point, lo is the number of objects in the items in the current
        // partition.  We want to find the median value item of the first, 
        // middle, and last items.  This median item will become the middle 
        // item.  Set j to the "greater of first and middle".  If last is larger
        // than j, then j is the median.  Otherwise, compare the last item to 
        // "the lesser of the first and middle" and take the larger.  The code 
        // is biased to prefer the middle over the first in the event of a tie.
        //

        mid = i = pBase + ((u32)(lo) >> 1);

        if( lo >= PartitionThresh )
        {
            j = (Compare( *(jj = pBase), *i ) > 0)  ?  jj : i;

            if( Compare( *j, *(tmp = pMax - 1) ) > 0 )
            {
                // Use lesser of first and middle.  (First loser.)
                j = (j == jj ? i : jj);
                if( Compare( *j, *tmp ) < 0 )
                    j = tmp;
            }

            if( j != i )
            {
                // Swap!
                T c  = *i;
                *i++ = *j;
                *j++ = c;
            }
        }

        //
        // Semi-standard quicksort partitioning / swapping...
        //

        for( i = pBase, j = pMax - 1;  ;  )
        {
            while( (i < mid) && (Compare( *i, *mid ) <= 0) )
            {
                i += 1;
            }

            while( j > mid )
            {
                if( Compare( *mid, *j ) <= 0 )
                {
                    j -= 1;
                    continue;
                }

                tmp = i + 1;     // Value of i after swap.

                if( i == mid )
                {
                    // j <-> mid, new mid is j.
                    mid = jj = j;       
                }
                else
                {
                    // i <-> j
                    jj = j;
                    j -= 1;
                }

                goto SWAP;
            }

            if( i == mid )
            {
                break;
            }
            else
            {
                jj  = mid;
                tmp = mid = i;
                j  -= 1;
            }
SWAP:
            {
                T c   = *i;
                *i++  = *jj;
                *jj++ = c;
            }

            i = tmp;
        }

        //
        // Consider the sizes of the two partitions.  Process the smaller
        // partition first via recursion.  Then process the larger partition by
        // iterating through the above code again.  (Update variables as needed
        // to make it work.)
        //
        // NOTE:  Do not bother sorting a given partition, either recursively or
        // by another iteration, if the size of the partition is less than 
        // RECURSION_THRESH items.
        //

        j = mid;
        i = mid + 1;

        if( (lo = (j-pBase)) <= (hi = (pMax-i)) )
        {
            if( lo >= RecursionThresh )
                PseudoQuickSort( pBase, j, 
                                 RecursionThresh,
                                 PartitionThresh,
                                 Compare );
            pBase = i;
            lo    = hi;
        }
        else
        {
            if( hi >= RecursionThresh )
                PseudoQuickSort( i, pMax,
                                 RecursionThresh,
                                 PartitionThresh,
                                 Compare );
            pMax = j;
        }

    } while( lo >= RecursionThresh );
}

//==============================================================================

#define X_QSORT_RECURSION_THRESH 4
#define X_QSORT_PARTITION_THRESH 6

template <class T, class Cmp>
void x_qsort( T* apBase, s32 NItems, Cmp Compare )
{
    T*  i;
    T*  j;
    T*  lo;
    T*  hi;
    T*  min;
    T*  pMax;
    T*  pBase = apBase;
    s32 RecursionThresh;          // Recursion threshold in objects
    s32 PartitionThresh;          // Partition threshold in objects

    ASSERT( pBase    );
    ASSERT( NItems   >= 0 );

    // Easy out?
    if( NItems <= 1 )
        return;

    // Set up some useful values.
    RecursionThresh = X_QSORT_RECURSION_THRESH;
    PartitionThresh = X_QSORT_PARTITION_THRESH;
    pMax            = pBase + NItems;

    //
    // Set the 'hi' value.
    // Also, if there are enough values, call the PseudoQuickSort function.
    //
    if( NItems >= X_QSORT_RECURSION_THRESH )
    {
        PseudoQuickSort( pBase,
                         pMax, 
                         RecursionThresh, 
                         PartitionThresh, 
                         Compare );
        hi = pBase + RecursionThresh;
    }
    else
    {
        hi = pMax;
    }

    //
    // Find the smallest element in the first "MIN(RECURSION_THRESH,NItems)"
    // items.  At this point, the smallest element in the entire list is 
    // guaranteed to be present in this sublist.
    //
    for( j = lo = pBase; (lo += 1) < hi;  )
    {
        if( Compare( *j, *lo ) > 0 )
            j = lo;
    }

    // 
    // Now put the smallest item in the first position to prime the next 
    // loop.
    //
    if( j != pBase )
    {
        i  = pBase;
        hi = pBase + 1;

        T c  = *j;
        *j++ = *i;
        *i++ = c;
    }

    //
    // Smallest item is in place.  Now we run the following hyper-fast
    // insertion sort.  For each remaining element, min, from [1] to [n-1],
    // set hi to the index of the element AFTER which this one goes.  Then,
    // do the standard insertion sort shift on a byte at a time basis.
    //
    for( min = pBase; (hi = min += 1) < pMax;  )
    {
        while( Compare( *(hi -= 1), *min ) > 0 )
        {
            // No body in this loop.
        }

        if( (hi += 1) != min )
        {
            lo = min;
            T c = *lo;
            for( i = j = lo; (j -= 1) >= hi; i = j )
            {
                *i = *j;
            }
            *i = c;
        }
    }
} 

#endif

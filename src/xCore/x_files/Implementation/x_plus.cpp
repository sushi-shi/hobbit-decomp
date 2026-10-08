#include <rva.h>

#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>

// PC dynamic initializer 0x647ae0 constructs this state with seed 2.
DATA(0x003ccf18) static random s_Random(2);
RVA_DYNINIT(0x00247ad0, 0x5, s_Random)
RVA_DYNINIT(0x00247ae0, 0xd, s_Random)

// The mutable narrow null-string fallback is observed in PC .data. Its
// semantic name is reconstructed; no Xbox public name has been established.
DATA(0x0034c278) static char s_NullString[] = "(NULL)";
DATA(0x0034c280) static unsigned short s_NullWideString[] = { '(', 'N', 'U', 'L', 'L', ')', 0 };

// PC byte translation table; values recovered from the hash-pinned image.
DATA(0x0034c290) unsigned char x_UpperCase[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

// PC byte translation table; values recovered from the hash-pinned image.
DATA(0x0034c390) unsigned char x_LowerCase[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

// Character-class bits recovered from the PC lookup used by decimal parsers.
DATA(0x0034c490) unsigned char x_CharacterClass[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
    0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x2c, 0x2c, 0x2c, 0x2c, 0x2c, 0x2c, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24,
    0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void PseudoQuickSort(unsigned char*, unsigned char*, int, int, int, compare_fn*);

#define RECURSION_THRESH 4
#define PARTITION_THRESH 6
#define X_MAX_DRIVE 3
#define X_MAX_DIR 256
#define X_MAX_FNAME 256
#define X_MAX_EXT 256
#define MIN(a,b) ((a)<(b)?(a):(b))

RVA(0x00247700, 0x122)
void x_qsort( const void*  apBase,
              int          NItems,
              int          ItemSize,
              compare_fn*  pCompare )
{
    unsigned char  c;
    unsigned char* i;
    unsigned char* j;
    unsigned char* lo;
    unsigned char* hi;
    unsigned char* min;
    unsigned char* pMax;
    unsigned char* pBase = static_cast<unsigned char*>(const_cast<void*>(apBase));
    int   RecursionThresh;
    int   PartitionThresh;

    if (!NItems) return;
    if( NItems <= 1 )
        return;

    RecursionThresh = ItemSize * RECURSION_THRESH;
    PartitionThresh = ItemSize * PARTITION_THRESH;
    pMax            = pBase + (NItems * ItemSize);

    if( NItems >= RECURSION_THRESH )
    {
        PseudoQuickSort( pBase,
                         pMax,
                         ItemSize,
                         RecursionThresh,
                         PartitionThresh,
                         pCompare );
        hi = pBase + RecursionThresh;
    }
    else
    {
        hi = pMax;
    }

    for( j = lo = pBase; (lo += ItemSize) < hi;  )
    {
        if( pCompare( j, lo ) > 0 )
            j = lo;
    }

    if( j != pBase )
    {
        for( i = pBase, hi = pBase + ItemSize; i < hi;  )
        {
            c    = *j;
            *j++ = *i;
            *i++ = c;
        }
    }

    for( min = pBase; (hi = min += ItemSize) < pMax;  )
    {
        while( (hi > pBase) && (pCompare( hi -= ItemSize, min ) > 0) )
        {

        }

        if( (hi += ItemSize) != min )
        {
            for( lo = min + ItemSize; --lo >= min;  )
            {
                c = *lo;
                for( i = j = lo; (j -= ItemSize) >= hi; i = j )
                {
                    *i = *j;
                }
                *i = c;
            }
        }
    }
}

RVA(0x00247830, 0x185)
static
void PseudoQuickSort( unsigned char*        pBase,
                      unsigned char*        pMax,
                      int          ItemSize,
                      int          RecursionThresh,
                      int          PartitionThresh,
                      compare_fn*  pCompare )
{
    unsigned char* i;
    unsigned char* j;
    unsigned char* jj;
    unsigned char* mid;
    unsigned char* tmp;
    unsigned char  c;
    int   ii;
    int   lo;
    int   hi;

    lo = static_cast<int>(pMax - pBase);

    do
    {

        mid = i = pBase + ItemSize * (static_cast<unsigned int>(lo / ItemSize) >> 1);

        if( lo >= PartitionThresh )
        {
            j = (pCompare( (jj = pBase), i ) > 0)  ?  jj : i;

            if( pCompare( j, (tmp = pMax - ItemSize) ) > 0 )
            {

                j = (j == jj ? i : jj);
                if( pCompare( j, tmp ) < 0 )
                    j = tmp;
            }

            if( j != i )
            {

                ii = ItemSize;
                do
                {
                    c    = *i;
                    *i++ = *j;
                    *j++ = c;
                } while( --ii );
            }
        }

        for( i = pBase, j = pMax - ItemSize;  ;  )
        {
            while( (i < mid) && (pCompare( i, mid ) <= 0) )
            {
                i += ItemSize;
            }

            while( j > mid )
            {
                if( pCompare( mid, j ) <= 0 )
                {
                    j -= ItemSize;
                    continue;
                }

                tmp = i + ItemSize;

                if( i == mid )
                {

                    mid = jj = j;
                }
                else
                {

                    jj = j;
                    j -= ItemSize;
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
                j  -= ItemSize;
            }
SWAP:
            ii = ItemSize;
            do
            {
                c     = *i;
                *i++  = *jj;
                *jj++ = c;
            } while( --ii );

            i = tmp;
        }

        j = mid;
        i = mid + ItemSize;

        if( (lo = (j-pBase)) <= (hi = (pMax-i)) )
        {
            if( lo >= RecursionThresh )
                PseudoQuickSort( pBase, j,
                                 ItemSize,
                                 RecursionThresh,
                                 PartitionThresh,
                                 pCompare );
            pBase = i;
            lo    = hi;
        }
        else
        {
            if( hi >= RecursionThresh )
                PseudoQuickSort( i, pMax,
                                 ItemSize,
                                 RecursionThresh,
                                 PartitionThresh,
                                 pCompare );
            pMax = j;
        }

    } while( lo >= RecursionThresh );
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x002479c0, 0x27)
void* x_bsearch( const void*  pKey,
                 const void*  pBase,
                 int          NItems,
                 int          ItemSize,
                 compare_fn*  pCompare )
{
    void* pLocation;
    return( x_bsearch( pKey, pBase, NItems, ItemSize, pCompare, pLocation ) );
}

RVA(0x002479f0, 0xd5)
void* x_bsearch( const void*  pKey,
                 const void*  pBase,
                 int          NItems,
                 int          ItemSize,
                 compare_fn*  pCompare,
                 void*&       pLocation )
{
    unsigned char* pLo;
    unsigned char* pHi;
    unsigned char* pMid;
    int   Half1;
    int   Half2;
    int   Result;

    pLocation = 0;

    pLo = static_cast<unsigned char*>(const_cast<void*>(pBase));
    pHi = pLo + ((NItems-1) * ItemSize);

    while( pLo <= pHi )
    {

        if( NItems >= 2 )
        {
            Half1  = NItems / 2;
            Half2  = NItems - Half1 - 1;
            pMid   = pLo + (Half1 * ItemSize);

            Result = pCompare( pKey, pMid );

            if( Result < 0 )
            {

                pHi    = pMid - ItemSize;
                NItems = Half1;
            }

            else if( Result > 0 )
            {

                pLo    = pMid + ItemSize;
                NItems = Half2;
            }

            else
            {

                Result = pCompare( pKey, pMid-ItemSize );

                if( Result > 0 )
                {

                    return( pMid );
                }
                else
                {

                    pHi    = pMid - ItemSize;
                    NItems = Half1;
                }
            }
        }
        else
        {

            Result = pCompare( pKey, pLo );
            if( Result == 0 )
            {

                return( pLo );
            }

            if( Result > 0 )
            {
                pLocation = pLo + ItemSize;
            }
            else
            {
                pLocation = pLo;
            }

            return( 0 );
        }
    }

    return( 0 );
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00247af0, 0x10)
void x_srand(int seed)
{
    s_Random.srand(seed);
}

RVA(0x00247b00, 0xa)
int x_rand()
{
    return s_Random.rand();
}

RVA(0x00247b10, 0x15)
int x_irand(int minimum, int maximum)
{
    return s_Random.irand(minimum, maximum);
}

RVA(0x00247b30, 0x15)
float x_frand(float minimum, float maximum)
{
    return s_Random.frand(minimum, maximum);
}

RVA(0x00247b50, 0x9)
random::random()
    : m_Seed(2)
{
}

RVA(0x00247b60, 0xb)
random::random(int seed)
    : m_Seed(seed)
{
}

RVA(0x00247b70, 0x9)
void random::srand(int seed)
{
    m_Seed = seed;
}

RVA(0x00247b80, 0x1b)
int random::rand()
{
    m_Seed = m_Seed * 214013 + 2531011;
    return (m_Seed >> 16) & 32767;
}

RVA(0x00247ba0, 0x1c)
int random::irand(int minimum, int maximum)
{
    return rand() % (maximum - minimum + 1) + minimum;
}

RVA(0x00247bc0, 0x26)
float random::frand(float minimum, float maximum)
{
    float unit = static_cast<float>(rand()) * (1.0f / 32767.0f);
    return unit * (maximum - minimum) + minimum;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00247bf0, 0x38)
vector2 random::v2(float minX, float maxX, float minY, float maxY)
{
    return vector2(frand(minX, maxX), frand(minY, maxY));
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00247c30, 0x54)
vector3 random::v3(float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
    return vector3(frand(minX, maxX), frand(minY, maxY), frand(minZ, maxZ));
}

RVA(0x00247c90, 0x52)
xcolor random::color(unsigned char alpha)
{
    return xcolor(static_cast<unsigned char>(irand(0, 255)),
                  static_cast<unsigned char>(irand(0, 255)),
                  static_cast<unsigned char>(irand(0, 255)), alpha);
}

RVA(0x00247cf0, 0x1b)
int x_strlen(const char* string)
{
    if (!string)
        string = s_NullString;
    const char* end = string;
    while (*end++)
        ;
    return static_cast<int>(end - string - 1);
}

RVA(0x00247d10, 0x20)
char* x_strcpy(char* destination, const char* source)
{
    if (!source)
        source = s_NullString;
    char* cursor = destination;
    while ((*cursor++ = *source++))
        ;
    return destination;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00247d30, 0x3c)
char* x_strdup(const char* string)
{
    if (!string) string = s_NullString;
    char* result = static_cast<char*>(x_malloc_fn(x_strlen(string) + 1, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_plus.cpp", 685));
    char* cursor = result;
    while ((*cursor++ = *string++)) {}
    return result;
}

RVA(0x00247d70, 0x4e)
char* x_strncpy(char* destination, const char* source, int count)
{
    if (!source)
        source = s_NullString;
    char* start = destination;
    while (count && (*destination++ = *source++))
        --count;
    if (count)
        while (--count)
            *destination++ = 0;
    return start;
}

RVA(0x00247dc0, 0x34)
char* x_strcat(char* front, const char* back)
{
    if (!back)
        back = s_NullString;
    char* cursor = front;
    while (*cursor)
        ++cursor;
    while ((*cursor++ = *back++))
        ;
    return front;
}

RVA(0x00247e00, 0x3a)
char* x_strncat(char* front, const char* back, int count)
{
    if (!back)
        back = s_NullString;
    char* start = front;
    while (*front++)
        ;
    --front;
    while (count--)
        if (!(*front++ = *back++))
            return start;
    *front = 0;
    return start;
}

RVA(0x00247e40, 0x3f)
int x_strcmp(const char* first, const char* second)
{
    if (!first)
        first = s_NullString;
    if (!second)
        second = s_NullString;
    int result = 0;
    while (!(result = static_cast<int>(*first) - static_cast<int>(*second)) && *first)
    {
        ++first;
        ++second;
    }
    return result;
}

RVA(0x00247e80, 0x49)
int x_strncmp(const char* first, const char* second, int count)
{
    if (!first)
        first = s_NullString;
    if (!second)
        second = s_NullString;
    if (!count)
        return 0;
    while (--count && *first && *first == *second)
    {
        ++first;
        ++second;
    }
    return static_cast<int>(*first) - static_cast<int>(*second);
}

RVA(0x00247ed0, 0x4a)
int x_stricmp(const char* first, const char* second)
{
    if (!first)
        first = s_NullString;
    if (!second)
        second = s_NullString;
    char left;
    char right;
    do
    {
        left = *first++;
        right = *second++;
        left = static_cast<char>(x_UpperCase[static_cast<unsigned char>(left)]);
        right = static_cast<char>(x_UpperCase[static_cast<unsigned char>(right)]);
    } while (left == right && left);
    return static_cast<int>(left) - static_cast<int>(right);
}

RVA(0x00247f20, 0x64)
char* x_strstr(const char* string, const char* substring)
{
    if (!string)
        return 0;
    char* cursor = const_cast<char*>(string);
    if (!substring || !*substring)
        return const_cast<char*>(string);
    while (*cursor)
    {
        const char* first = cursor;
        const char* second = substring;
        while (*first && *second && !(*first - *second))
        {
            ++first;
            ++second;
        }
        if (!*second)
            return cursor;
        ++cursor;
    }
    return 0;
}

RVA(0x00247f90, 0x72)
char* x_stristr(const char* string, const char* substring)
{
    if (!string)
        return 0;
    char* cursor = const_cast<char*>(string);
    if (!substring || !*substring)
        return const_cast<char*>(string);
    while (*cursor)
    {
        const char* first = cursor;
        const char* second = substring;
        while (*first && *second &&
               !(static_cast<char>(x_LowerCase[static_cast<unsigned char>(*first)]) -
                 static_cast<char>(x_LowerCase[static_cast<unsigned char>(*second)])))
        {
            ++first;
            ++second;
        }
        if (!*second)
            return cursor;
        ++cursor;
    }
    return 0;
}

RVA(0x00248010, 0x25)
char* x_strchr(const char* string, int character)
{
    if (!string)
        return 0;

    while (*string && *string != static_cast<char>(character))
        ++string;

    if (*string == static_cast<char>(character))
        return const_cast<char*>(string);
    return 0;
}

RVA(0x00248040, 0x31)
char* x_strrchr(const char* string, int character)
{
    if (!string)
        return 0;

    const char* start = string;
    while (*string++)
        ;

    while (--string != start && *string != static_cast<char>(character))
        ;

    if (*string == static_cast<char>(character))
        return const_cast<char*>(string);
    return 0;
}

RVA(0x00248080, 0x29)
char* x_strtoupper(char* string)
{
    if (!string)
        string = s_NullString;
    char* cursor = string;
    while (*cursor)
    {
        *cursor = static_cast<char>(x_UpperCase[static_cast<unsigned char>(*cursor)]);
        ++cursor;
    }
    return string;
}

RVA(0x002480b0, 0x29)
char* x_strtolower(char* string)
{
    if (!string)
        string = s_NullString;
    char* cursor = string;
    while (*cursor)
    {
        *cursor = static_cast<char>(x_LowerCase[static_cast<unsigned char>(*cursor)]);
        ++cursor;
    }
    return string;
}

RVA(0x002480e0, 0x25)
int x_wstrlen(const unsigned short* string)
{
    if (!string)
        string = s_NullWideString;
    const unsigned short* end = string;
    if (!string)
        return 0;
    while (*end++)
        ;
    return static_cast<int>(end - string - 1);
}

RVA(0x00248110, 0x27)
unsigned short* x_wstrcpy(unsigned short* destination, const unsigned short* source)
{
    if (!source)
        source = s_NullWideString;
    unsigned short* cursor = destination;
    while ((*cursor++ = *source++))
        ;
    return destination;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00248140, 0x45)
unsigned short* x_strdup(const unsigned short* string)
{
    if (!string) string = s_NullWideString;
    unsigned short* cursor = static_cast<unsigned short*>(x_malloc_fn(2 * (x_wstrlen(string) + 1), "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_plus.cpp", 1033));
    unsigned short* result = cursor;
    while ((*cursor++ = *string++)) {}
    return result;
}

RVA(0x00248190, 0x50)
unsigned short* x_wstrncpy(unsigned short* destination, const unsigned short* source, int count)
{
    if (!source)
        source = s_NullWideString;
    unsigned short* start = destination;
    while (count && (*destination++ = *source++))
        --count;
    if (count)
        while (--count)
            *destination++ = 0;
    return start;
}

RVA(0x002481e0, 0x43)
unsigned short* x_wstrcat(unsigned short* front, const unsigned short* back)
{
    if (!back)
        back = s_NullWideString;
    unsigned short* cursor = front;
    while (*cursor)
        ++cursor;
    while ((*cursor++ = *back++))
        ;
    return front;
}

RVA(0x00248230, 0x4e)
unsigned short* x_wstrncat(unsigned short* front, const unsigned short* back, int count)
{
    if (!back)
        back = s_NullWideString;
    unsigned short* start = front;
    while (*front++)
        ;
    --front;
    while (count--)
        if (!(*front++ = *back++))
            return start;
    *front = 0;
    return start;
}

RVA(0x00248280, 0x42)
int x_wstrcmp(const unsigned short* first, const unsigned short* second)
{
    if (!first)
        first = s_NullWideString;
    if (!second)
        second = s_NullWideString;
    unsigned short left;
    unsigned short right;
    do
    {
        left = *first++;
        right = *second++;
    } while (left == right && left);
    return static_cast<int>(left) - static_cast<int>(right);
}

RVA(0x002482d0, 0x64)
int x_wstricmp(const unsigned short* first, const unsigned short* second)
{
    if (!first)
        first = s_NullWideString;
    if (!second)
        second = s_NullWideString;
    unsigned short left;
    unsigned short right;
    do
    {
        left = static_cast<unsigned short>(x_toupper(*first++));
        right = static_cast<unsigned short>(x_toupper(*second++));
    } while (left == right && left);
    return static_cast<int>(left) - static_cast<int>(right);
}

RVA(0x00248340, 0x50)
int x_wstrncmp(const unsigned short* first, const unsigned short* second, int count)
{
    if (!first)
        first = s_NullWideString;
    if (!second)
        second = s_NullWideString;
    if (!count)
        return 0;
    while (--count && *first && *first == *second)
    {
        ++first;
        ++second;
    }
    return static_cast<int>(*first) - static_cast<int>(*second);
}

RVA(0x00248390, 0x75)
unsigned short* x_wstrstr(const unsigned short* string, const unsigned short* substring)
{
    if (!string)
        return 0;
    unsigned short* cursor = const_cast<unsigned short*>(string);
    if (!substring || !*substring)
        return const_cast<unsigned short*>(string);
    while (*cursor)
    {
        const unsigned short* first = cursor;
        const unsigned short* second = substring;
        while (*first && *second && !(*first - *second))
        {
            ++first;
            ++second;
        }
        if (!*second)
            return cursor;
        ++cursor;
    }
    return 0;
}

RVA(0x00248410, 0x2d)
unsigned short* x_wstrchr(const unsigned short* string, int character)
{
    if (!string)
        return 0;

    while (*string && *string != static_cast<unsigned short>(character))
        ++string;

    if (*string == static_cast<unsigned short>(character))
        return const_cast<unsigned short*>(string);
    return 0;
}

RVA(0x00248440, 0x35)
unsigned short* x_wstrrchr(const unsigned short* string, int character)
{
    if (!string)
        return 0;

    const unsigned short* start = string;
    while (*string++)
        ;

    while (--string != start && *string != static_cast<unsigned short>(character))
        ;

    if (*string == static_cast<unsigned short>(character))
        return const_cast<unsigned short*>(string);
    return 0;
}

RVA(0x00248480, 0x26)
void* x_memcpy(void* destination, const void* source, int count)
{
    if (!count || source == destination)
        return destination;
    return x_memmove(destination, source, count);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x002484b0, 0x5c)
void x_memswap(void* first, void* second, int count)
{
    unsigned char* left = static_cast<unsigned char*>(first);
    unsigned char* right = static_cast<unsigned char*>(second);
    while ((count & ~3) > 0)
    {
        count -= 4;
        unsigned int value = *reinterpret_cast<unsigned int*>(left + count);
        *reinterpret_cast<unsigned int*>(left + count) = *reinterpret_cast<unsigned int*>(right + count);
        *reinterpret_cast<unsigned int*>(right + count) = value;
    }
    while (count > 0)
    {
        --count;
        unsigned char value = left[count];
        left[count] = right[count];
        right[count] = value;
    }
}

RVA(0x00248510, 0xe6)
void* x_memmove(void* destination, const void* source, int count)
{
    if (!count || source == destination)
        return destination;
    const unsigned char* from = static_cast<const unsigned char*>(source);
    unsigned char* to = static_cast<unsigned char*>(destination);
    int block;
    if (to < from)
    {
        block = reinterpret_cast<int>(from);
        if ((block | reinterpret_cast<int>(to)) & 3)
        {
            if (((block ^ reinterpret_cast<int>(to)) & 3) || count < 4)
                block = count;
            else
                block = 4 - (block & 3);
            count -= block;
            do
            {
                *to++ = *from++;
            } while (--block);
        }
        block = count >> 2;
        if (block)
        {
            do
            {
                *reinterpret_cast<unsigned int*>(to) = *reinterpret_cast<const unsigned int*>(from);
                from += 4;
                to += 4;
            } while (--block);
        }
        block = count & 3;
        if (block)
        {
            do
            {
                *to++ = *from++;
            } while (--block);
        }
    }
    else
    {
        from += count;
        to += count;
        block = reinterpret_cast<int>(from);
        if ((block | reinterpret_cast<int>(to)) & 3)
        {
            if (((block ^ reinterpret_cast<int>(to)) & 3) || count <= 4)
                block = count;
            else
                block &= 3;
            count -= block;
            do
            {
                *--to = *--from;
            } while (--block);
        }
        block = count >> 2;
        if (block)
        {
            do
            {
                from -= 4;
                to -= 4;
                *reinterpret_cast<unsigned int*>(to) = *reinterpret_cast<const unsigned int*>(from);
            } while (--block);
        }
        block = count & 3;
        if (block)
        {
            do
            {
                *--to = *--from;
            } while (--block);
        }
    }
    return destination;
}

RVA(0x00248600, 0x7c)
void* x_memset(void* buffer, int character, int count)
{
    if (!count)
        return buffer;
    unsigned char* cursor = static_cast<unsigned char*>(buffer);
    unsigned char* end = cursor + count;
    unsigned int value = (static_cast<unsigned int>(character) << 24) |
                         (static_cast<unsigned int>(character) << 16) |
                         (static_cast<unsigned int>(character) << 8) |
                          static_cast<unsigned int>(character);
    while (cursor < end && (reinterpret_cast<unsigned int>(cursor) & 3))
        *cursor++ = static_cast<unsigned char>(character);
    unsigned int* words = reinterpret_cast<unsigned int*>(cursor);
    while (words + 1 < reinterpret_cast<unsigned int*>(end))
        *words++ = value;
    cursor = reinterpret_cast<unsigned char*>(words);
    while (cursor < end)
        *cursor++ = static_cast<unsigned char>(character);
    return buffer;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00248680, 0x1f)
void* x_memchr(void* buffer, int character, int count)
{
    unsigned char* cursor = static_cast<unsigned char*>(buffer);
    unsigned char value = static_cast<unsigned char>(character);
    while (count && *cursor != value)
    {
        ++cursor;
        --count;
    }
    return count ? cursor : 0;
}

RVA(0x002486a0, 0x5d)
int x_memcmp(const void* first, const void* second, int count)
{
    const unsigned char* left = static_cast<const unsigned char*>(first);
    const unsigned char* right = static_cast<const unsigned char*>(second);
    const unsigned char* end = left + count;
    if (count >= 4 && !(reinterpret_cast<unsigned int>(left) & 3) &&
        !(reinterpret_cast<unsigned int>(right) & 3))
    {
        end -= 4;
        while (left <= end)
        {
            if (*reinterpret_cast<const unsigned int*>(left) !=
                *reinterpret_cast<const unsigned int*>(right))
                break;
            left += 4;
            right += 4;
        }
        end += 4;
    }
    while (left < end)
    {
        if (*left != *right)
            return *left < *right ? -1 : 1;
        ++left;
        ++right;
    }
    return 0;
}

RVA(0x00248700, 0x27)
unsigned int x_chksum(const void* buffer, int count)
{
    unsigned int result = 0xaaaaaaaa;
    const unsigned char* cursor = static_cast<const unsigned char*>(buffer);
    const unsigned char* end = cursor + count;
    while (cursor != end)
    {
        unsigned int high = result & 0x80000000;
        result ^= *cursor;
        result <<= 1;
        result |= high >> 31;
        ++cursor;
    }
    return result;
}

RVA(0x00248730, 0x6b)
int x_atoi( const char* pStr )
{
    char C;
    char Sign;
    int  Total;

    for( ; x_isspace(*pStr); ++pStr )
        ;

    C = *pStr++;
    Sign = C;

    if( (C == '-') || (C == '+') )
    {
        C = *pStr++;
    }

    Total = 0;

    while( x_isdigit(C) )
    {

        Total = (10 * Total) + (C - '0');

        C = *pStr++;
    }

    if( Sign == '-' )
        Total = -Total;

    return( Total );
}

RVA(0x002487a0, 0x202)
float x_atof( const char* pStr )
{
    int   ISign   = 1;
    int   IValue  = 0;
    int   DValue  = 0;
    int   DDenom  = 1;
    int Integer = 1;

    for( ; x_isspace(*pStr); ++pStr )
        ;

    if( *pStr == '-' )  { pStr++; ISign = -1; }
    if( *pStr == '+' )  { pStr++;             }

    while( x_isdigit(*pStr) )
    {
        IValue = (IValue<<3) + (IValue<<1) + (*pStr-'0');
        pStr++;
    }

    if( *pStr == '.' )
    {

        Integer = 0;

        pStr++;

        while( x_isdigit(*pStr) )
        {
            DValue = (DValue<<3) + (DValue<<1) + (*pStr-'0');
            DDenom = (DDenom<<3) + (DDenom<<1);
            pStr++;
        }
    }

    if( (*pStr == 'e') || (*pStr == 'E') || (*pStr == 'd') || (*pStr == 'D') )
    {
        float  EValue = 0;
        float  ESign  = 1;
        float  e;

        pStr++;

        if( *pStr == '-' )  { pStr++; ESign = -1; }
        if( *pStr == '+' )  { pStr++;             }

        while( x_isdigit(*pStr) )
        {
            EValue = (10.0f * EValue) + (*pStr - '0');
            pStr++;
        }

        if( EValue < 30 )
        {
            e = 1.0f;
            while( EValue )
            {
                e *= 10.0f;
                EValue--;
            }
        }
        else
        {
            e = x_pow( 10.0f, EValue );
        }

        {
            float Temp = ISign * (IValue + (static_cast<float>(DValue) / static_cast<float>(DDenom)));

            if( ESign < 0 )  return( Temp / e );
            else             return( Temp * e );
        }
    }

    if( Integer ) return( static_cast<float>(ISign * IValue) );
    else          return( ISign * (IValue + (static_cast<float>(DValue)/static_cast<float>(DDenom))) );
}

RVA(0x002489b0, 0x5c)
void x_strippath_dir(const char* path, char* directory)
{
    char drive[3];
    char dir[256];
    x_splitpath(path, drive, dir, 0, 0);
    x_sprintf(directory, "%s%s", drive, dir);
    int length = x_strlen(directory);
    if (length > 0 && directory[length - 1] == '\\')
        directory[length - 1] = 0;
}

RVA(0x00248a10, 0x47)
void x_strippath_file(const char* path, char* filename)
{
    char name[256];
    char extension[256];
    x_splitpath(path, 0, 0, name, extension);
    x_sprintf(filename, "%s%s", name, extension);
}

RVA(0x00248a60, 0x15d)
void x_splitpath( const char* pPath, char* pDrive,
                                     char* pDir,
                                     char* pFName,
                                     char* pExt )
{
    char* p;
    char* pLastSlash = 0;
    char* pLastDot   = 0;
    int   Len;

    if( (x_strlen(pPath) >= X_MAX_DRIVE-2) &&
        (pPath[X_MAX_DRIVE-2] == ':') )
    {
        if( pDrive != 0 )
        {
            x_strncpy( pDrive, pPath, X_MAX_DRIVE-1 );
            pDrive[X_MAX_DRIVE-1] = '\0';
        }
        pPath += X_MAX_DRIVE-1;
    }
    else
    if( pDrive )
    {

        pDrive[0] = '\0';
    }

    for( p = const_cast<char*>(pPath); *p; ++p )
    {
        if( (*p == '/') || (*p == '\\') )
            pLastSlash = p+1;
        else
        if( *p == '.' )
            pLastDot = p;
    }

    if( pLastSlash != 0 )
    {

        if( pDir != 0 )
        {
            Len = MIN( (pLastSlash - pPath), X_MAX_DIR-1 );
            x_strncpy( pDir, pPath, Len );
            pDir[Len] = '\0';
        }
        pPath = pLastSlash;
    }
    else
    if ( pDir != 0 )
    {

        pDir[0] = '\0';
    }

    if( (pLastDot != 0) && (pLastDot >= pPath) )
    {

        if( pFName != 0 )
        {
            Len = MIN( (pLastDot - pPath), X_MAX_FNAME-1 );
            x_strncpy( pFName, pPath, Len );
            pFName[Len] = '\0';
        }

        if( pExt != 0 )
        {
            Len = MIN( (p - pLastDot), X_MAX_EXT-1 );
            x_strncpy( pExt, pLastDot, Len );
            pExt[Len] = '\0';
        }
    }
    else
    {
        if( pFName != 0 )
        {
            Len = MIN( (p - pPath), X_MAX_FNAME );
            x_strncpy( pFName, pPath, Len );
            pFName[Len] = '\0';
        }
        if ( pExt != 0 )
        {
            pExt[0] = '\0';
        }
    }
}

RVA(0x00248bc0, 0xa0)
void x_makepath( char* pPath, const char* pDrive,
                              const char* pDir,
                              const char* pFName,
                              const char* pExt )
{
    const char* p;

    if( pDrive && *pDrive )
    {
        *pPath++ = *pDrive;
        *pPath++ = ':';
    }

    if( pDir && *pDir )
    {
        p = pDir;
        do
        {
            *pPath++ = *p++;
        } while( *p );

        p--;
        if( (*p != '/') && (*p != '\\') )
        {
            *pPath++ = '\\';
        }
    }

    if( pFName )
    {
        p = pFName;
        while( *p )
        {
            *pPath++ = *p++;
        }
    }

    if( pExt )
    {
        p = pExt;
        if( *p && (*p != '.') )
        {
            *pPath++ = '.';
        }
        while( *p )
        {
            *pPath++ = *p++;
        }
    }

    *pPath = '\0';
}

// Genuine sibling source API; PC span is not admitted.
char* x_strsavecpy( char* pDest, const char* pSrc, s32 Count )
{
    char* pStart = pDest;

    ASSERT( pDest );
    ASSERT( pSrc  );
    ASSERT( Count > 0 );

    while( Count && (*pDest = *pSrc) )
    {
        pDest++;
        pSrc++;
        Count--;
    }

    if( Count )
    {
#ifdef X_DEBUG
        while( --Count )
            *pDest++ = '\0';
#endif
    }
    else
    {
        *(pDest-1) = 0;
    }

    return( pStart );
}


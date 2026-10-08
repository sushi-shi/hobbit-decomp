// Sibling cmap algorithm, byte-verified for Hobbit PC.

#include <rva.h>

#include <xCore/x_files/Implementation/x_bitmap_private.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_memory.hpp>

DATA(0x003e87f0)
static int bcenter;
DATA(0x003e8834)
static int gcenter;
DATA(0x003e8800)
static int rcenter;
DATA(0x003e87c8)
static int gdist;
DATA(0x003e87b4)
static int rdist;
DATA(0x003e87b8)
static int cdist;
DATA(0x003e882c)
static int cbinc;
DATA(0x003e87d4)
static int cginc;
DATA(0x003e8794)
static int crinc;
DATA(0x003e87c4)
static unsigned int *gdp;
DATA(0x003e87dc)
static unsigned int *rdp;
DATA(0x003e87a8)
static unsigned int *cdp;
DATA(0x003e8830)
static unsigned char *grgbp;
DATA(0x003e8820)
static unsigned char *rrgbp;
DATA(0x003e8824)
static unsigned char *crgbp;
DATA(0x003e87e4)
static int gstride;
DATA(0x003e8798)
static int rstride;
DATA(0x003e87b0)
static int x;
DATA(0x003e87d0)
static int xsqr;
DATA(0x003e881c)
static int colormax;
DATA(0x003e87c0)
static int cindex;

#define NBITS       6
#define NENTRIES    (1<<(3*NBITS))
DATA(0x003e8838)
static unsigned char *   s_Index = 0;
DATA(0x003e883c)
static unsigned int *    s_Dist = 0;
DATA(0x003e87a0)
static int s_NColors;
DATA(0x003e87f4)
static xcolor* s_Color;
DATA(0x003e87bc)
static int s_UseAlpha;

void maxfill( unsigned int* buffer, int side );
int redloop( void );
int blueloop( int restart );
int greenloop( int restart );

RVA(0x00255b20, 0x1c9)
void
compute_cmap2( void )
{
    int nbits = 8 - NBITS;

    colormax = 1 << NBITS;
    x = 1 << nbits;
    xsqr = 1 << (2 * nbits);

    gstride = colormax;
    rstride = colormax * colormax;

    maxfill( s_Dist, colormax );

    for ( cindex = 0; cindex < s_NColors; cindex++ )
    {

    if (nbits == 3)
    {
      int r, g, b;

    x = 2;
    xsqr = 4;

      r = rcenter = s_Color[cindex].R;
    g = gcenter = s_Color[cindex].G;
      b = bcenter = s_Color[cindex].B;

    r >>= 2;
    g >>= 2;
    b >>= 2;
    rcenter >>= 3;
    gcenter >>= 3;
    bcenter >>= 3;

    rdist = r - ((rcenter << 1) + 1);
    gdist = g - ((gcenter << 1) + 1);
    cdist = b - ((bcenter << 1) + 1);
    cdist = rdist * rdist + gdist * gdist + cdist * cdist;

    crinc = 4 - (4 * r) + (8 * rcenter);
    cginc = 4 - (4 * g) + (8 * gcenter);
    cbinc = 4 - (4 * b) + (8 * bcenter);
    }
    else
    {
      rcenter = s_Color[cindex].R >> nbits;
    gcenter = s_Color[cindex].G >> nbits;
      bcenter = s_Color[cindex].B >> nbits;

    rdist = s_Color[cindex].R - (rcenter * x + x/2);
      gdist = s_Color[cindex].G - (gcenter * x + x/2);
    cdist = s_Color[cindex].B - (bcenter * x + x/2);
      cdist = rdist*rdist + gdist*gdist + cdist*cdist;

    crinc = 2 * ((rcenter + 1) * xsqr - (s_Color[cindex].R * x));
      cginc = 2 * ((gcenter + 1) * xsqr - (s_Color[cindex].G * x));
    cbinc = 2 * ((bcenter + 1) * xsqr - (s_Color[cindex].B * x));
  }

    cdp = s_Dist + rcenter * rstride + gcenter * gstride + bcenter;
    crgbp = s_Index + rcenter * rstride + gcenter * gstride + bcenter;

    (void)redloop();
    }

}

RVA(0x00255cf0, 0x1ac)
int
redloop()
{
    int detect;
    int r;
    int first;
    long txsqr = xsqr + xsqr;

    DATA(0x003e8804)
static long rxx;

    detect = 0;

    for ( r = rcenter, rdist = cdist, rxx = crinc,
      rdp = cdp, rrgbp = crgbp, first = 1;
      r < colormax;
      r++, rdp += rstride, rrgbp += rstride,
      rdist += rxx, rxx += txsqr, first = 0 )
    {
    if ( greenloop( first ) )
        detect = 1;
    else if ( detect )
        break;
    }

    for ( r = rcenter - 1, rxx = crinc - txsqr, rdist = cdist - rxx,
      rdp = cdp - rstride, rrgbp = crgbp - rstride, first = 1;
      r >= 0;
      r--, rdp -= rstride, rrgbp -= rstride,
      rxx -= txsqr, rdist -= rxx, first = 0 )
    {
    if ( greenloop( first ) )
        detect = 1;
    else if ( detect )
        break;
    }

    return detect;
}

RVA(0x00255ea0, 0x32e)
int greenloop( int restart )
{
    int detect;
    int g;
    int first;
    long txsqr = xsqr + xsqr;
    DATA(0x003e87e0)
static int here;
    DATA(0x003e87ac)
static int min;
    DATA(0x003e87e8)
static int max;

    DATA(0x003e879c)
static int prevmax;
    DATA(0x003e87ec)
static int prevmin;
    int thismax, thismin;

    DATA(0x003e8810)
static long ginc;
    DATA(0x003e87d8)
static long gxx;
    DATA(0x003e8814)
static long gcdist;
    DATA(0x003e87f8)
static unsigned int *gcdp;
    DATA(0x003e87fc)
static unsigned char *gcrgbp;

    if ( restart )
    {
    here = gcenter;
    min = 0;
    max = colormax - 1;
    ginc = cginc;

    prevmax = 0;
    prevmin = colormax;

    }

    thismin = min;
    thismax = max;

    detect = 0;

    for ( g = here, gcdist = gdist = rdist, gxx = ginc,
      gcdp = gdp = rdp, gcrgbp = grgbp = rrgbp, first = 1;
      g <= max;
      g++, gdp += gstride, gcdp += gstride, grgbp += gstride, gcrgbp += gstride,
      gdist += gxx, gcdist += gxx, gxx += txsqr, first = 0 )
    {
    if ( blueloop( first ) )
    {
        if ( !detect )
        {

        if ( g > here )
        {
            here = g;
            rdp = gcdp;
            rrgbp = gcrgbp;
            rdist = gcdist;
            ginc = gxx;

            thismin = here;

        }
        detect = 1;
        }
    }
    else if ( detect )
    {

        thismax = g - 1;

        break;
    }
    }

    for ( g = here - 1, gxx = ginc - txsqr, gcdist = gdist = rdist - gxx,
      gcdp = gdp = rdp - gstride, gcrgbp = grgbp = rrgbp - gstride,
      first = 1;
      g >= min;
      g--, gdp -= gstride, gcdp -= gstride, grgbp -= gstride, gcrgbp -= gstride,
      gxx -= txsqr, gdist -= gxx, gcdist -= gxx, first = 0 )
    {
    if ( blueloop( first ) )
    {
        if ( !detect )
        {

        here = g;
        rdp = gcdp;
        rrgbp = gcrgbp;
        rdist = gcdist;
        ginc = gxx;

        thismax = here;

        detect = 1;
        }
    }
    else if ( detect )
    {

        thismin = g + 1;

        break;
    }
    }

    if ( detect )
    {
    if ( thismax < prevmax )
        max = thismax;

    prevmax = thismax;

    if ( thismin > prevmin )
        min = thismin;

    prevmin = thismin;
    }

    return detect;
}

RVA(0x002561d0, 0x216)
int blueloop( int restart )
{
    int detect;
    register unsigned int *dp;
    register unsigned char *rgbp;
    register long bdist, bxx;
    register int b, i = cindex;
    register long txsqr = xsqr + xsqr;
    register int lim;
    DATA(0x003e8818)
static int here;
    DATA(0x003e880c)
static int min;
    DATA(0x003e87cc)
static int max;

    DATA(0x003e8808)
static int prevmin;
    DATA(0x003e8828)
static int prevmax;
    int thismin, thismax;

    DATA(0x003e87a4)
static long binc;

    if ( restart )
    {
    here = bcenter;
    min = 0;
    max = colormax - 1;
    binc = cbinc;

    prevmin = colormax;
    prevmax = 0;

    }

    detect = 0;

    thismin = min;
    thismax = max;

    for ( b = here, bdist = gdist, bxx = binc, dp = gdp, rgbp = grgbp, lim = max;
      b <= lim;
      b++, dp++, rgbp++,
      bdist += bxx, bxx += txsqr )
    {

    if ( *dp > static_cast<unsigned int>(bdist) )
    {

        if ( b > here )
        {
        here = b;
        gdp = dp;
        grgbp = rgbp;
        gdist = bdist;
        binc = bxx;

        thismin = here;

        }
        detect = 1;

        break;
    }
    }

    for ( ;
      b <= lim;
      b++, dp++, rgbp++,
      bdist += bxx, bxx += txsqr )
    {

    if ( *dp > static_cast<unsigned int>(bdist) )
    {

        *dp = bdist;
        *rgbp = i;
    }
    else
    {

        thismax = b - 1;

        break;
    }
    }

    lim = min;
    b = here - 1;
    bxx = binc - txsqr;
    bdist = gdist - bxx;
    dp = gdp - 1;
    rgbp = grgbp - 1;

    if ( !detect )
    for ( ;
          b >= lim;
          b--, dp--, rgbp--,
          bxx -= txsqr, bdist -= bxx )
    {

        if ( *dp > static_cast<unsigned int>(bdist) )
        {

        here = b;
        gdp = dp;
        grgbp = rgbp;
        gdist = bdist;
        binc = bxx;

        thismax = here;

        detect = 1;

        break;
        }
    }

    for ( ;
      b >= lim;
      b--, dp--, rgbp--,
      bxx -= txsqr, bdist -= bxx )
    {

    if ( *dp > static_cast<unsigned int>(bdist) )
    {

        *dp = bdist;
        *rgbp = i;
    }
    else
    {

        thismin = b + 1;

        break;
    }
    }

    if ( detect )
    {

    if ( thismax < prevmax )
        max = thismax;

    if ( thismin > prevmin )
        min = thismin;

    prevmax = thismax;
    prevmin = thismin;
    }

    return detect;
}

RVA(0x002563f0, 0x21)
void maxfill( unsigned int* buffer, int side )
{
    (void)side;
    register unsigned long maxv = ~0L;
    register long i;
    register unsigned int *bp;

    for ( i = colormax * colormax * colormax, bp = buffer;
      i > 0;
      i--, bp++ )
    *bp = maxv;
}

RVA(0x00256420, 0x121)
void compute_cmap( void )
{
    register unsigned int *dp;
    register unsigned char *rgbp;
    register long bdist, bxx;
    register int b, i;
    int nbits = 8 - NBITS;
    register int colormax = 1 << NBITS;
    register long xsqr = 1 << (2 * nbits);
    int x = 1 << nbits;
    int rinc, ginc, binc, r, g;
    long rdist, gdist, rxx, gxx;

    for ( i = 0; i < s_NColors; i++ )
    {

    rdist = s_Color[i].R - x/2;
    gdist = s_Color[i].G - x/2;
    bdist = s_Color[i].B - x/2;
    rdist = rdist*rdist + gdist*gdist + bdist*bdist;

    rinc = 2 * (xsqr - (s_Color[i].R << nbits));
    ginc = 2 * (xsqr - (s_Color[i].G << nbits));
    binc = 2 * (xsqr - (s_Color[i].B << nbits));
    dp = s_Dist;
    rgbp = s_Index;
    for ( r = 0, rxx = rinc;
          r < colormax;
          rdist += rxx, r++, rxx += xsqr + xsqr )
        for ( g = 0, gdist = rdist, gxx = ginc;
          g < colormax;
          gdist += gxx, g++, gxx += xsqr + xsqr )
        for ( b = 0, bdist = gdist, bxx = binc;
              b < colormax;
              bdist += bxx, b++, dp++, rgbp++,
              bxx += xsqr + xsqr )
        {

            if ( i == 0 || *dp > static_cast<unsigned int>(bdist) )
            {

            *dp = bdist;
            *rgbp = i;
            }
        }
    }

}

RVA(0x00256550, 0xa3)
void cmap_Begin( const xcolor* pColor, int NColors, int UseAlpha )
{

    int i = NColors-1;
    while( (i>0) && (pColor[i].R==0) && (pColor[i].G==255) && (pColor[i].B==0) )
        i--;

    s_UseAlpha = UseAlpha;
    s_NColors = i+1;
    s_Color   = const_cast<xcolor*>(pColor);

    if( !s_UseAlpha )
    {
        s_Dist = static_cast<unsigned int*>(x_malloc_fn(sizeof(unsigned int) * NENTRIES, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_cmap.cpp", 806));
        s_Index = static_cast<unsigned char*>(x_malloc_fn(NENTRIES, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_cmap.cpp", 807));

        compute_cmap2();

        x_free_fn(s_Dist, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_cmap.cpp", 813);
        s_Dist = 0;
    }
}

RVA(0x00256600, 0xf0)
int cmap_GetIndex( xcolor Color )
{
    if( !s_UseAlpha )
    {
        int Entry = ((Color.R>>(8-NBITS))<<(2*NBITS)) |
                    ((Color.G>>(8-NBITS))<<(1*NBITS)) |
                    ((Color.B>>(8-NBITS))<<(0*NBITS));

        return s_Index[ Entry ];
    }

    int BE=0x7FFFFFFF;
    int BI=0;
    int CV[4];
    int PV[4];
    CV[0] = Color.R;
    CV[1] = Color.G;
    CV[2] = Color.B;
    CV[3] = Color.A;

    for( int i=0; i<s_NColors; i++ )
    {
        PV[0] = s_Color[i].R;
        PV[1] = s_Color[i].G;
        PV[2] = s_Color[i].B;
        PV[3] = s_Color[i].A;

        int E = (PV[0]-CV[0])*(PV[0]-CV[0]) +
                (PV[1]-CV[1])*(PV[1]-CV[1]) +
                (PV[2]-CV[2])*(PV[2]-CV[2]) +
                ((PV[3]-CV[3])*(PV[3]-CV[3])*1);

        if( E < BE )
        {
            BE = E;
            BI = i;
        }
    }

    return BI;
}

RVA(0x002566f0, 0x2c)
void cmap_End( void )
{
    if( !s_UseAlpha )
    {

        x_free_fn(s_Index, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_cmap.cpp", 871);
        s_Index=0;
    }
}



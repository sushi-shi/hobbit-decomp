// Hobbit PC quantizer. Sibling algorithms checked against every PC byte and relocation.
// PC keeps the alpha work arrays on the stack; reference heap allocation is later behavior.

#include <rva.h>

#include <xCore/x_files/Implementation/x_bitmap_private.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_time.hpp>

RVA_COMPGEN(0x00001080, 0x30, ??_H@YGXPAXIHP6EX0@Z@Z)

#define ASSERT(x) ((void)0)
#define ABS(x) (((x)<0)?-(x):(x))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define FALSE 0
#define NULL 0
#define HIST_BIT   (6)
#define HIST_MAX   (1 << HIST_BIT)
#define R_STRIDE   (HIST_MAX * HIST_MAX)
#define G_STRIDE   (HIST_MAX)
#define HIST_SHIFT (8 - HIST_BIT)
#define MAX_BOXES   256

struct qbox
{
  int  variance;
  int  total_weight;
  int  tt_sum;
  int  t_ur;
  int  t_ug;
  int  t_ub;
  int  ir, ig, ib;
  int  jr, jg, jb;
};

DATA(0x003e8840)
static qbox s_QBox[MAX_BOXES];
DATA(0x003eb844)
static int s_NQBoxes;
DATA(0x003eb848)
static int* s_pQuantHist;
DATA(0x003eb84c)
static xcolor* s_Pixel;
DATA(0x003eb840)
static int s_NPixels;

RVA(0x00257080, 0x154)
static void ComputeSum( int ir, int ig, int ib,
                        int jr, int jg, int jb,
                        int& total_weight, int& tt_sum,
                        int& t_ur, int& t_ug, int& t_ub)
{
  int i, j, r, g, b;
  int rs, ts;
  int w, tr, tg, tb;
  int *rp, *gp, *bp;

  j = 0;

  tr = tg = tb = i = 0;

  rp = s_pQuantHist + ((ir * R_STRIDE) + (ig * G_STRIDE) + ib);

  for (r = ir; r <= jr; r++)
  {
    rs = r * r;
    gp = rp;

    for (g = ig; g <= jg; g++)
    {
      ts = rs + (g * g);
      bp = gp;

      for (b = ib; b <= jb; b++)
        if (*bp++)
        {
          w   = *(bp - 1);
          j  += w;
          tr += r * w;
          tg += g * w;
          tb += b * w;
          i  += (ts + b * b) * w;
        }

      gp += G_STRIDE;
    }

    rp += R_STRIDE;
  }

  total_weight = j;
  tt_sum       = i;
  t_ur         = tr;
  t_ug         = tg;
  t_ub         = tb;
}

RVA(0x002571e0, 0x335)
static void ShrinkBox( int  ir, int  ig, int  ib,
                       int  jr, int  jg, int  jb,
                       int& lr, int& lg, int& lb,
                       int& hr, int& hg, int& hb )
{
  int r, g, b;
  int *rp, *gp, *bp, *s;
  int newlr,newlg,newlb;
  int newhr,newhg,newhb;

  newlr=newlg=newlb=-1;
  newhr=newhg=newhb=-1;

  s = s_pQuantHist + (ir * R_STRIDE + ig * G_STRIDE + ib);

  rp = s;

  for (r = ir; r <= jr; r++)
  {
    gp = rp;

    for (g = ig; g <= jg; g++)
    {
      bp = gp;

      for (b = ib; b <= jb; b++)
        if (*bp++) { newlr = r; goto lr_done; }

      gp += G_STRIDE;
    }

    rp += R_STRIDE;
  }

lr_done:

  gp = s;

  for (g = ig; g <= jg; g++)
  {
    rp = gp;

    for (r = ir; r <= jr; r++)
    {
      bp = rp;

      for (b = ib; b <= jb; b++)
        if (*bp++) { newlg = g; goto lg_done; }

      rp += R_STRIDE;
    }

    gp += G_STRIDE;
  }

lg_done:

  bp = s;

  for (b = ib; b <= jb; b++)
  {
    rp = bp;

    for (r = ir; r <= jr; r++)
    {
      gp = rp;

      for (g = ig; g <= jg; g++, gp += G_STRIDE)
        if (*gp) { newlb = b; goto lb_done; }

      rp += R_STRIDE;
    }

    bp++;
  }

lb_done:

  s = s_pQuantHist + (jr * R_STRIDE + jg * G_STRIDE + jb);

  rp = s;

  for (r = jr; r >= ir; r--)
  {
    gp = rp;

    for (g = jg; g >= ig; g--)
    {
      bp = gp;

      for (b = jb; b >= ib; b--)
        if (*bp--) { newhr = r; goto hr_done; }

      gp -= G_STRIDE;
    }

    rp -= R_STRIDE;
  }

hr_done:

  gp = s;

  for (g = jg; g >= ig; g--)
  {
    rp = gp;

    for (r = jr; r >= ir; r--)
    {
      bp = rp;

      for (b = jb; b >= ib; b--)
        if (*bp--) { newhg = g; goto hg_done; }

      rp -= R_STRIDE;
    }

    gp -= G_STRIDE;
  }

hg_done:

  bp = s;

  for (b = jb; b >= ib; b--)
  {
    gp = bp;

    for (g = jg; g >= ig; g--)
    {
      rp = gp;

      for (r = jr; r >= ir; r--, rp -= R_STRIDE)
        if (*rp) { newhb = b; goto hb_done; }

      gp -= G_STRIDE;
    }

    bp--;
  }

hb_done:
  ;

  ASSERT( (newlr>=0) && (newlr<HIST_MAX) );
  ASSERT( (newlg>=0) && (newlg<HIST_MAX) );
  ASSERT( (newlb>=0) && (newlb<HIST_MAX) );
  ASSERT( (newhr>=0) && (newhr<HIST_MAX) );
  ASSERT( (newhg>=0) && (newhg<HIST_MAX) );
  ASSERT( (newhb>=0) && (newhb<HIST_MAX) );

    lr=newlr;
    lg=newlg;
    lb=newlb;
    hr=newhr;
    hg=newhg;
    hb=newhb;

}

RVA(0x00257cf0, 0x32)
static int ComputeVariance( int tw,   int tt_sum,
                            int t_ur, int t_ug, int t_ub )
{
  float temp;

  temp  = static_cast<float>(t_ur) * static_cast<float>(t_ur);
  temp += static_cast<float>(t_ug) * static_cast<float>(t_ug);
  temp += static_cast<float>(t_ub) * static_cast<float>(t_ub);
  temp /= static_cast<float>(tw);

  return( static_cast<int>(static_cast<float>(tt_sum) - temp) );
}

RVA(0x00257520, 0x7c1)
static void SplitBox( int ID )
{
  int   i;
  qbox* new_box;
  qbox* old_box = s_QBox+ID;

  ASSERT( old_box->variance > 0 );

  int total_weight;
  int tt_sum, t_ur, t_ug, t_ub;
  int ir, ig, ib, jr, jg, jb;

  int total_weight1;
  int tt_sum1, t_ur1, t_ug1, t_ub1;
  int ir1, ig1, ib1, jr1, jg1, jb1;

  int total_weight2;
  int tt_sum2, t_ur2, t_ug2, t_ub2;
  int ir2, ig2, ib2, jr2, jg2, jb2;

  int total_weight3;
  int tt_sum3, t_ur3, t_ug3, t_ub3;

  int lowest_variance, variance_r, variance_g, variance_b;
  int pick_r, pick_g, pick_b;

  new_box = s_QBox + s_NQBoxes;
  s_NQBoxes++;

  total_weight          = old_box->total_weight;
  tt_sum                = old_box->tt_sum;
  t_ur                  = old_box->t_ur;
  t_ug                  = old_box->t_ug;
  t_ub                  = old_box->t_ub;
  ir                    = old_box->ir;
  ig                    = old_box->ig;
  ib                    = old_box->ib;
  jr                    = old_box->jr;
  jg                    = old_box->jg;
  jb                    = old_box->jb;

  total_weight1         = 0;
  tt_sum1               = 0;
  t_ur1                 = 0;
  t_ug1                 = 0;
  t_ub1                 = 0;

  total_weight2         = total_weight;
  tt_sum2               = tt_sum;
  t_ur2                 = t_ur;
  t_ug2                 = t_ug;
  t_ub2                 = t_ub;

  variance_r = 0x7FFFFFFF;

  pick_r = ir;
  for (i = ir; i < jr; i++)
  {
    int total_variance;

    ComputeSum(i, ig, ib, i, jg, jb,
        total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);

    ASSERT(total_weight3 <= total_weight);

    total_weight1 += total_weight3;
    tt_sum1       += tt_sum3;
    t_ur1         += t_ur3;
    t_ug1         += t_ug3;
    t_ub1         += t_ub3;

    total_weight2 -= total_weight3;
    tt_sum2       -= tt_sum3;
    t_ur2         -= t_ur3;
    t_ug2         -= t_ug3;
    t_ub2         -= t_ub3;

    ASSERT((total_weight1 + total_weight2) == total_weight);

    total_variance = ComputeVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                     ComputeVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

    if (total_variance < variance_r)
    {
      variance_r = total_variance;
      pick_r = i;
    }
  }

  total_weight1         = 0;
  tt_sum1               = 0;
  t_ur1                 = 0;
  t_ug1                 = 0;
  t_ub1                 = 0;

  total_weight2         = total_weight;
  tt_sum2               = tt_sum;
  t_ur2                 = t_ur;
  t_ug2                 = t_ug;
  t_ub2                 = t_ub;

  variance_g = 0x7FFFFFFF;
  pick_g = ig;
  for (i = ig; i < jg; i++)
  {
    int total_variance;

    ComputeSum(ir, i, ib, jr, i, jb,
        total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);

    ASSERT(total_weight3 <= total_weight);

    total_weight1 += total_weight3;
    tt_sum1       += tt_sum3;
    t_ur1         += t_ur3;
    t_ug1         += t_ug3;
    t_ub1         += t_ub3;

    total_weight2 -= total_weight3;
    tt_sum2       -= tt_sum3;
    t_ur2         -= t_ur3;
    t_ug2         -= t_ug3;
    t_ub2         -= t_ub3;

    ASSERT((total_weight1 + total_weight2) == total_weight);

    total_variance = ComputeVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                     ComputeVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

    if (total_variance < variance_g)
    {
      variance_g = total_variance;
      pick_g = i;
    }
  }

  total_weight1         = 0;
  tt_sum1               = 0;
  t_ur1                 = 0;
  t_ug1                 = 0;
  t_ub1                 = 0;

  total_weight2         = total_weight;
  tt_sum2               = tt_sum;
  t_ur2                 = t_ur;
  t_ug2                 = t_ug;
  t_ub2                 = t_ub;

  variance_b = 0x7FFFFFFF;
    pick_b = ib;

  for (i = ib; i < jb; i++)
  {
    int total_variance;

    ComputeSum(ir, ig, i, jr, jg, i,
        total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);

    ASSERT(total_weight3 <= total_weight);

    total_weight1 += total_weight3;
    tt_sum1       += tt_sum3;
    t_ur1         += t_ur3;
    t_ug1         += t_ug3;
    t_ub1         += t_ub3;

    total_weight2 -= total_weight3;
    tt_sum2       -= tt_sum3;
    t_ur2         -= t_ur3;
    t_ug2         -= t_ug3;
    t_ub2         -= t_ub3;

    ASSERT((total_weight1 + total_weight2) == total_weight);

    total_variance = ComputeVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                     ComputeVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

    if (total_variance < variance_b)
    {
      variance_b = total_variance;
      pick_b = i;
    }
  }

  lowest_variance = variance_r;
  i = 0;

  if (variance_g < lowest_variance)
  {
    lowest_variance = variance_g;
    i = 1;
  }

  if (variance_b < lowest_variance)
  {
    lowest_variance = variance_b;
    i = 2;
  }

  ir1 = ir; ig1 = ig; ib1 = ib;
  jr2 = jr; jg2 = jg; jb2 = jb;

  switch (i)
  {
    case 0:
    {
      jr1 = pick_r + 0; jg1 = jg; jb1 = jb;
      ir2 = pick_r + 1; ig2 = ig; ib2 = ib;
      break;
    }
    case 1:
    {
      jr1 = jr; jg1 = pick_g + 0; jb1 = jb;
      ir2 = ir; ig2 = pick_g + 1; ib2 = ib;
      break;
    }
    case 2:
    {
      jr1 = jr; jg1 = jg; jb1 = pick_b + 0;
      ir2 = ir; ig2 = ig; ib2 = pick_b + 1;
      break;
    }
  }

  ShrinkBox(ir1, ig1, ib1, jr1, jg1, jb1,
            ir1, ig1, ib1, jr1, jg1, jb1);

  ShrinkBox(ir2, ig2, ib2, jr2, jg2, jb2,
            ir2, ig2, ib2, jr2, jg2, jb2);

  ComputeSum(ir1, ig1, ib1, jr1, jg1, jb1,
      total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1);

  total_weight2         = total_weight - total_weight1;
  tt_sum2               = tt_sum - tt_sum1;
  t_ur2                 = t_ur - t_ur1;
  t_ug2                 = t_ug - t_ug1;
  t_ub2                 = t_ub - t_ub1;

  old_box->variance     = ComputeVariance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1);
  old_box->total_weight = total_weight1;
  old_box->tt_sum       = tt_sum1;
  old_box->t_ur         = t_ur1;
  old_box->t_ug         = t_ug1;
  old_box->t_ub         = t_ub1;
  old_box->ir           = ir1;
  old_box->ig           = ig1;
  old_box->ib           = ib1;
  old_box->jr           = jr1;
  old_box->jg           = jg1;
  old_box->jb           = jb1;

  new_box->variance     = ComputeVariance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);
  new_box->total_weight = total_weight2;
  new_box->tt_sum       = tt_sum2;
  new_box->t_ur         = t_ur2;
  new_box->t_ug         = t_ug2;
  new_box->t_ub         = t_ub2;
  new_box->ir           = ir2;
  new_box->ig           = ig2;
  new_box->ib           = ib2;
  new_box->jr           = jr2;
  new_box->jg           = jg2;
  new_box->jb           = jb2;

}

RVA(0x00256720, 0x16)
void quant_ClearHistogram( void )
{
    ASSERT( s_pQuantHist );
    x_memset(s_pQuantHist,0,sizeof(int)*HIST_MAX*HIST_MAX*HIST_MAX);
}

RVA(0x00256740, 0x21)
void quant_Begin( void )
{
    s_pQuantHist = static_cast<int*>(x_malloc_fn(sizeof(int)*HIST_MAX*HIST_MAX*HIST_MAX, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_quant.cpp", 634));
    ASSERT(s_pQuantHist);
    quant_ClearHistogram();
}

RVA(0x00256770, 0x14)
void quant_SetPixels( const xcolor* pColor, int NColors )
{
    s_Pixel = const_cast<xcolor*>(pColor);
    s_NPixels = NColors;
}

RVA(0x00256e80, 0x1fc)
static
int quant_BuildPaletteFromHistogram( xcolor* pPalette, int NColors )
{
    int i;

    {
        int total_weight;
        int tt_sum, t_ur, t_ug, t_ub;
        int ir, ig, ib, jr, jg, jb;

        s_NQBoxes = 1;

        ShrinkBox( 0, 0, 0, HIST_MAX-1, HIST_MAX-1, HIST_MAX-1, ir, ig, ib, jr, jg, jb );

        ComputeSum( ir, ig, ib, jr, jg, jb, total_weight, tt_sum, t_ur, t_ug, t_ub );

        s_QBox[0].total_weight = total_weight;
        s_QBox[0].variance     = 1;
        s_QBox[0].tt_sum       = tt_sum;
        s_QBox[0].t_ur         = t_ur;
        s_QBox[0].t_ug         = t_ug;
        s_QBox[0].t_ub         = t_ub;
        s_QBox[0].ir           = ir;
        s_QBox[0].ig           = ig;
        s_QBox[0].ib           = ib;
        s_QBox[0].jr           = jr;
        s_QBox[0].jg           = jg;
        s_QBox[0].jb           = jb;
    }

    while( s_NQBoxes < NColors )
    {

        int WorstID    = 0;
        int WorstScore = 0;
        for( i=0; i<s_NQBoxes; i++ )
        if( s_QBox[i].variance > WorstScore )
        {
            WorstID = i;
            WorstScore = s_QBox[i].variance;
        }

        if( WorstScore==0 )
            break;

        if( (s_QBox[WorstID].ir == s_QBox[WorstID].jr) &&
            (s_QBox[WorstID].ig == s_QBox[WorstID].jg) &&
            (s_QBox[WorstID].ib == s_QBox[WorstID].jb) )
            break;

        SplitBox( WorstID );
    }

    int UsableColors=0;
    for( i=0; i<NColors; i++ )
    {
        xcolor C;
        int tw = s_QBox[i].total_weight;
        if( tw==0 )
        {
            C.R = 0;
            C.G = 255;
            C.B = 0;
            C.A = 255;
        }
        else
        {
            C.R = (((s_QBox[i].t_ur << HIST_SHIFT) + (tw>>1)) / tw);
            C.G = (((s_QBox[i].t_ug << HIST_SHIFT) + (tw>>1)) / tw);
            C.B = (((s_QBox[i].t_ub << HIST_SHIFT) + (tw>>1)) / tw);
            C.A = 255;
            UsableColors++;
        }
        pPalette[i] = C;
    }

    return UsableColors;
}

struct alpha_info
{
    int HistCount;
    int QAlpha;
};

struct alpha_box
{
    int MinA;
    int MaxA;
    int Error;
    int Alpha;
    int NSrcColors;
    int NPalColors;
    int TotalColors;
};

struct alpha_pal
{
    xcolor C[256];
    int    NColors;
};

RVA(0x00256790, 0x78)
void ComputeCenterAlpha( alpha_info* pAI, alpha_box* pAB )
{

    int WA=0;
    int C=0;
    int i;

    for( i=pAB->MinA; i<=pAB->MaxA; i++ )
    {
        WA += i*pAI[i].HistCount;
        C  += pAI[i].HistCount;
    }

    pAB->Alpha = (C)?(WA/C):(255);
    pAB->TotalColors = C;

    pAB->Error = 0;
    for( i=pAB->MinA; i<=pAB->MaxA; i++ )
    {
        int DE = ABS( i-pAB->Alpha );
        pAB->Error += DE*pAI[i].HistCount;
    }
}

RVA(0x00256810, 0xc2)
void FindAlphaSplit( alpha_info* pAI,
                     alpha_box* pAB,
                     alpha_box* pLB,
                     alpha_box* pRB )
{
    int LA = pAB->MinA;
    int RA = pAB->MaxA;
    int i;
    int BestE=0x7FFFFFFF;
    alpha_box BestLB;
    alpha_box BestRB;
    alpha_box LB;
    alpha_box RB;

    for( i=LA+1; i<=RA; i++ )
    {
        LB.MinA = LA;
        LB.MaxA = i-1;
        RB.MinA = i;
        RB.MaxA = RA;
        ComputeCenterAlpha( pAI, &LB );
        ComputeCenterAlpha( pAI, &RB );
        if( (LB.Error+RB.Error) < BestE )
        {
            BestE = LB.Error+RB.Error;
            BestLB = LB;
            BestRB = RB;
        }
    }

    *pLB = BestLB;
    *pRB = BestRB;
}

RVA(0x002568e0, 0x591)
void quant_End( xcolor* pPalette, int aNColors, int UseAlpha )
{
    xtimer T;
    T.Start();
    ASSERT( s_Pixel != NULL );

    if( UseAlpha == FALSE )
    {
        quant_ClearHistogram();

        int NColors = s_NPixels;
        xcolor* pColor = s_Pixel;
        while( NColors-- )
        {
            int R = pColor->R >> HIST_SHIFT;
            int G = pColor->G >> HIST_SHIFT;
            int B = pColor->B >> HIST_SHIFT;
            s_pQuantHist[ (R*R_STRIDE) + (G*G_STRIDE) + B ]++;
            pColor++;
        }

        quant_BuildPaletteFromHistogram( pPalette, aNColors );
    }
    else
    {
        alpha_info AI[256];
        alpha_box AB[256];
        alpha_pal AP[256];
        int NAlphaBoxes;
        int NColors;
        xcolor* pColor;
        int i;

        ASSERT(AI && AB && AP);
        x_memset(AB,0,sizeof(alpha_box)*256);
        x_memset(AI,0,sizeof(alpha_info)*256);
        NColors = s_NPixels;
        pColor = s_Pixel;
        while( NColors-- )
        {
            AI[ pColor->A ].HistCount++;
            pColor++;
        }

        NAlphaBoxes = 1;
        AB[0].MinA  = 0;
        AB[0].MaxA  = 255;
        ComputeCenterAlpha(AI,&AB[0]);

        while( NAlphaBoxes < MIN(aNColors/2,32) )
        {

            int WE=0;
            int WI=0;
            for( i=0; i<NAlphaBoxes; i++ )
            if( AB[i].Error > WE )
            {
                WE = AB[i].Error;
                WI = i;
            }

            if( WE==0 )
                break;

            alpha_box LB;
            alpha_box RB;
            FindAlphaSplit( AI, &AB[WI], &LB, &RB );
            AB[WI] = LB;
            AB[NAlphaBoxes] = RB;
            NAlphaBoxes++;
        }

        {

            for( i=0; i<NAlphaBoxes; i++ )
            {

                quant_ClearHistogram();
                int NColors = s_NPixels;
                xcolor* pColor = s_Pixel;
                while( NColors-- )
                {
                    if( (pColor->A >= AB[i].MinA) &&
                        (pColor->A <= AB[i].MaxA) )
                    {
                        int R = pColor->R >> HIST_SHIFT;
                        int G = pColor->G >> HIST_SHIFT;
                        int B = pColor->B >> HIST_SHIFT;
                        s_pQuantHist[ (R*R_STRIDE) + (G*G_STRIDE) + B ]++;
                    }
                    pColor++;
                }

                AP[i].NColors = quant_BuildPaletteFromHistogram( AP[i].C, aNColors );
            }

            for( i=0; i<NAlphaBoxes; i++ )
            {
                AB[i].NSrcColors = 0;

                for( int j=AB[i].MinA; j<=AB[i].MaxA; j++ )
                {
                    AB[i].NSrcColors += AI[j].HistCount;
                }

                AB[i].NPalColors = (aNColors*AB[i].NSrcColors)/(s_NPixels);

                if( AP[i].NColors < AB[i].NPalColors )
                    AB[i].NPalColors = AP[i].NColors;

                if( AB[i].NPalColors < 1 )
                    AB[i].NPalColors = 1;
            }

            {
                int TotalC=0;

                for( i=0; i<NAlphaBoxes; i++ )
                    TotalC += AB[i].NPalColors;

                while( TotalC > aNColors )
                {

                    int BestI = 0;
                    for( i=0; i<NAlphaBoxes; i++ )
                    if( AB[i].NPalColors > AB[BestI].NPalColors )
                        BestI = i;

                    ASSERT( AB[BestI].NPalColors > 1 );

                    AB[BestI].NPalColors--;
                    TotalC--;
                }
            }

            while(1)
            {

                int TotalC=0;
                int BC=0;
                int BI=-1;
                for( i=0; i<NAlphaBoxes; i++ )
                {
                    TotalC += AB[i].NPalColors;

                    if( AP[i].NColors > 0 )
                    {
                        int Diff = (1000*(AP[i].NColors - AB[i].NPalColors))/AP[i].NColors;
                        if( Diff >= BC )
                        {
                            BC = Diff;
                            BI = i;
                        }
                    }
                }

                if( TotalC == aNColors )
                    break;

                ASSERT( TotalC <= aNColors );

                if( BI==-1 )
                    break;

                AB[BI].NPalColors++;
            }
        }

        {
            int PalI=0;
            xcolor Pal[256];
            for( i=0; i<NAlphaBoxes; i++ )
            {
                int NUsedColors;

                if( AB[i].NPalColors == AP[i].NColors )
                {
                    x_memcpy(Pal,AP[i].C,sizeof(xcolor)*AP[i].NColors);
                    NUsedColors = AP[i].NColors;
                }
                else
                {

                    quant_ClearHistogram();
                    int NColors = s_NPixels;
                    xcolor* pColor = s_Pixel;
                    while( NColors-- )
                    {
                        if( (pColor->A >= AB[i].MinA) &&
                            (pColor->A <= AB[i].MaxA) )
                        {
                            int R = pColor->R >> HIST_SHIFT;
                            int G = pColor->G >> HIST_SHIFT;
                            int B = pColor->B >> HIST_SHIFT;
                            s_pQuantHist[ (R*R_STRIDE) + (G*G_STRIDE) + B ]++;
                        }
                        pColor++;
                    }

                    NUsedColors = quant_BuildPaletteFromHistogram(Pal,AB[i].NPalColors);
                }

                for( int j=0; j<NUsedColors; j++ )
                {
                    pPalette[PalI+j]    = Pal[j];
                    pPalette[PalI+j].A  = static_cast<unsigned char>(AB[i].Alpha);
                }
                PalI += NUsedColors;
            }

            while( PalI < aNColors )
            {
                pPalette[PalI].R = 0;
                pPalette[PalI].G = 255;
                pPalette[PalI].B = 0;
                pPalette[PalI].A = 255;
                PalI++;
            }

        }

    }

    x_free_fn(s_pQuantHist, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_quant.cpp", 1088);
    s_pQuantHist = NULL;
    T.Stop();

}



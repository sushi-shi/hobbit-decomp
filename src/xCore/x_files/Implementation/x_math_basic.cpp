#include <rva.h>

#include <xCore/x_files/x_math.hpp>

#pragma intrinsic(atan)

// IEEE-754 bit operations and polynomial trigonometry are reconstructed
// from the pinned PC region 0x24c330..0x24cb26. Xbox object membership
// identifies x_math_basic; PC bytes establish these bodies and constants.
union f32_forge
{
    unsigned int Bits;
    float F32;
    f32_forge() {}
    f32_forge(unsigned int value) : Bits(value) {}
    f32_forge(float value) : F32(value) {}
};

RVA(0x0024c520, 0xb3)
static void f32_UnloadExponent(float& value, int& exponent)
{
    exponent=0;
    if(value==0){value=0;return;}
    f32_forge bits(value);
    if(bits.Bits&0x7f800000)
    {
        exponent=int((bits.Bits&0x7f800000)>>23)-126;
        bits.Bits&=~0x7f800000;
        bits.Bits|=126<<23;
    }
    else
    {
        unsigned int sign=bits.Bits&0x80000000;
        exponent=-125;
        while(!(bits.Bits&0x00ff8000)){bits.Bits<<=8;exponent-=8;}
        while(!(bits.Bits&0x00800000)){bits.Bits<<=1;exponent-=1;}
        bits.Bits&=0x007fffff;
        bits.Bits|=sign;
        bits.Bits|=126<<23;
    }
    value=bits.F32;
}
RVA(0x0024c600, 0xa2)
static void f32_LoadExponent(float& value,int exponent)
{
    int stored;
    if(value==0){value=0;return;}
    f32_forge bits(value);
    stored=int((bits.Bits&0x7f800000)>>23);
    stored+=exponent;
    if(stored>0)
    {
        bits.Bits&=~0x7f800000;
        bits.Bits|=stored<<23;
    }
    else
    {
        unsigned int sign=bits.Bits&0x80000000;
        bits.Bits&=0x007fffff;
        bits.Bits|=0x00800000;
        while(stored < -6){bits.Bits>>=8;stored+=8;}
        while(stored < 1){bits.Bits>>=1;stored+=1;}
        bits.Bits|=sign;
    }
    value=bits.F32;
}
RVA(0x0024c330, 0x98)
float x_floor(float value)
{
    f32_forge bits;int exponent;unsigned int sign;
    if(value==0)return 0;
    bits.F32=value;
    exponent=int((bits.Bits&0x7f800000)>>23)-127;
    if(exponent>23)return value;
    sign=bits.Bits&0x80000000;
    if(exponent<0)return sign?-1.0f:0.0f;
    bits.Bits&=(-1<<(23-exponent));
    if(sign && value<bits.F32)return bits.F32-1.0f;
    return bits.F32;
}
RVA(0x0024c3d0, 0x98)
float x_ceil(float value)
{
    f32_forge bits;int exponent;unsigned int sign;
    if(value==0)return 0;
    bits.F32=value;
    exponent=int((bits.Bits&0x7f800000)>>23)-127;
    if(exponent>23)return value;
    sign=bits.Bits&0x80000000;
    if(exponent<0)return sign?0.0f:1.0f;
    bits.Bits&=(-1<<(23-exponent));
    if(!sign && value>bits.F32)return bits.F32+1.0f;
    return bits.F32;
}
RVA(0x0024c470, 0x8e)
float x_modf(float value,float* whole)
{
    f32_forge bits;int exponent;
    if(value==0){*whole=0;return 0;}
    bits.F32=value;
    exponent=int((bits.Bits&0x7f800000)>>23)-127;
    if(exponent>23){*whole=value;return 0;}
    if(exponent<0){*whole=0;return value;}
    bits.Bits&=(-1<<(23-exponent));
    *whole=bits.F32;
    return value-bits.F32;
}
RVA(0x0024c500, 0x17)
float x_frexp(float value,int* exponent)
{
    f32_UnloadExponent(value,*exponent);
    return value;
}
RVA(0x0024c5e0, 0x17)
float x_ldexp(float value,int exponent)
{
    f32_LoadExponent(value,exponent);
    return value;
}
RVA(0x0024c6b0, 0x14a)
float x_fmod(float value,float divisor)
{
    int negative;float multiple;int magnitude;int unused;float fraction;
    // The PC build adds this positive-range fast return.
    if(divisor>0 && value>=0 && value<divisor)return value;
    if(value==0)return 0;
    if(divisor<0)divisor=-divisor;
    if(value<0){value=-value;negative=1;}else negative=0;
    while(value>=divisor)
    {
        fraction=value;multiple=divisor;
        f32_UnloadExponent(fraction,magnitude);
        f32_UnloadExponent(multiple,unused);
        f32_LoadExponent(multiple,magnitude);
        while(magnitude>=0)
        {
            if(value>=multiple){value-=multiple;break;}
            else {f32_LoadExponent(multiple,-1);magnitude--;}
        }
    }
    return negative?-value:value;
}


RVA(0x0024c810, 0xb9)
static float SineAndCosine(float angle, int offset)
{
    float result;
    float squared;
    int quadrant;
    result = angle * (2.0f / 3.14159265358979323846f);
    quadrant = (int)(result > 0 ? result + 0.5f : result - 0.5f);
    angle = (angle - quadrant * 1.5707960f) - quadrant * 3.1391647e-7f;
    quadrant += offset;
    squared = angle * angle;
    if (quadrant & 1)
        result = 1.0f - squared * (0.5f - squared * (1 / 24.0f - squared * (1 / 720.0f - squared * (1 / 40320.0f))));
    else
        result = angle * (1.0f + squared * (-1 / 6.0f + squared * (1 / 120.0f + squared * (-1 / 5040.0f + squared * (1 / 362880.0f)))));
    return quadrant & 2 ? -result : result;
}

RVA(0x0024c800, 0x10)
float x_sin(float angle)
{
    return SineAndCosine(angle, 0);
}

RVA(0x0024c8d0, 0x10)
float x_cos(float angle)
{
    return SineAndCosine(angle, 1);
}

RVA(0x0024c8e0, 0x12b)
void x_sincos(float angle, float& sine, float& cosine)
{
    float first;
    float second;
    float squared;
    int quadrant;
    first = angle * (2.0f / 3.14159265358979323846f);
    quadrant = (int)(first > 0 ? first + 0.5f : first - 0.5f);
    angle = (angle - quadrant * 1.5707960f) - quadrant * 3.1391647e-7f;
    squared = angle * angle;
    first = 1.0f - squared * (0.5f - squared * (1 / 24.0f - squared * (1 / 720.0f - squared * (1 / 40320.0f))));
    second = angle * (1.0f + squared * (-1 / 6.0f + squared * (1 / 120.0f + squared * (-1 / 5040.0f + squared * (1 / 362880.0f)))));
    // The PC build collapses small polynomial results independently.
    if (first > -0.000001f && first < 0.000001f) first = 0.0f;
    if (second > -0.000001f && second < 0.000001f) second = 0.0f;
    if (quadrant & 1)
    {
        sine = (quadrant & 2) ? -first : first;
        cosine = ((quadrant + 1) & 2) ? -second : second;
    }
    else
    {
        sine = ((quadrant + 1) & 2) ? -second : second;
        cosine = (quadrant & 2) ? -first : first;
    }
}

RVA(0x0024ca10, 0x116)
float x_atan2(float y, float x)
{
    float ratio;
    float absoluteRatio;
    unsigned int sign;
    DATA(0x0034cc84) static float SignArray[] =
    {0.0f, 3.14159265358979323846f, 0.0f, -3.14159265358979323846f};
    int collapsed = 0;
    if (x > -0.000001f && x < 0.000001f) { x = 0.0f; ++collapsed; }
    if (y > -0.000001f && y < 0.000001f) { y = 0.0f; ++collapsed; }
    if (collapsed == 2)
        return 0.0f;
    if (x == 0.0f)
    {
        if (y >= 0.0f) return 1.57079632679489661923f;
        else return -1.57079632679489661923f;
    }
    if (y == 0.0f)
    {
        if (x >= 0.0f) return 0.0f;
        else return 3.14159265358979323846f;
    }
    if (x < 0) sign = 1;
    else sign = 0;
    if (y < 0) sign |= 2;
    ratio = y / x;
    if (sign && sign != 3) absoluteRatio = -ratio;
    else absoluteRatio = ratio;
    if (absoluteRatio < 0.000001f)
        return SignArray[sign];
    return (float)atan((double)ratio) + SignArray[sign];
}

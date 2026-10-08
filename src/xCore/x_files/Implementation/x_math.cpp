#include <rva.h>

#include <xCore/x_files/x_math.hpp>

RVA_COMPGEN(0x00001080, 0x30, ??_H@YGXPAXIHP6EX0@Z@Z)

#pragma intrinsic(sqrt)

// The integer exponent-mask test includes all finite values and excludes
// both infinities and NaNs; it performs no floating-point comparison.
RVA(0x0024cf30, 0x16)
int x_isvalid(float value)
{
    return (*(unsigned int*)&value & 0x7f800000) != 0x7f800000;
}

static inline float magnitude(float value)
{
    return value < 0.0f ? -value : value;
}

RVA(0x0024d480, 0x24d)
int matrix4::Invert()
{
    float augmented[4][8];
    float multiplier;
    int column, row, cell, selectedRow, pivot;
    int order[4];
    for (row = 0; row < 4; ++row)
    {
        for (cell = 0; cell < 4; ++cell)
        {
            augmented[row][cell] = m_Cell[row][cell];
            augmented[row][cell + 4] = 0.0f;
        }
        augmented[row][row + 4] = 1.0f;
        order[row] = row;
    }
    for (column = 0; column < 4; ++column)
    {
        cell = column;
        multiplier = magnitude(augmented[order[cell]][cell]);
        for (row = column + 1; row < 4; ++row)
        {
            selectedRow = order[row];
            if (multiplier < magnitude(augmented[selectedRow][column]))
            {
                cell = row;
                multiplier = magnitude(augmented[selectedRow][column]);
            }
        }
        pivot = order[cell];
        order[cell] = order[column];
        order[column] = pivot;
        multiplier = augmented[pivot][column];
        if (multiplier == 0.0f)
            return 0;
        augmented[pivot][column] = 1.0f;
        for (cell = column + 1; cell < 8; ++cell)
            augmented[pivot][cell] /= multiplier;
        for (row = column + 1; row < 4; ++row)
        {
            selectedRow = order[row];
            multiplier = -augmented[selectedRow][column];
            if (multiplier == 0.0f)
                continue;
            augmented[selectedRow][column] = 0.0f;
            for (cell = column + 1; cell < 8; ++cell)
                augmented[selectedRow][cell] += multiplier * augmented[pivot][cell];
        }
    }
    for (column = 3; column > 0; --column)
    {
        pivot = order[column];
        for (row = column - 1; row >= 0; --row)
        {
            selectedRow = order[row];
            multiplier = augmented[selectedRow][column];
            for (cell = column; cell < 8; ++cell)
                augmented[selectedRow][cell] -= multiplier * augmented[pivot][cell];
        }
    }
    for (row = 0; row < 4; ++row)
    {
        selectedRow = order[row];
        for (cell = 0; cell < 4; ++cell)
            m_Cell[row][cell] = augmented[selectedRow][cell + 4];
    }
    return 1;
}

RVA(0x0024d6d0, 0x210)
int matrix4::InvertSRT()
{
    matrix4 original = *this;
    float determinant;
    determinant = original.m_Cell[0][0] * (original.m_Cell[1][1] * original.m_Cell[2][2] - original.m_Cell[1][2] * original.m_Cell[2][1])
                - original.m_Cell[0][1] * (original.m_Cell[1][0] * original.m_Cell[2][2] - original.m_Cell[1][2] * original.m_Cell[2][0])
                + original.m_Cell[0][2] * (original.m_Cell[1][0] * original.m_Cell[2][1] - original.m_Cell[1][1] * original.m_Cell[2][0]);
    if (magnitude(determinant) < 0.00001f)
        return 0;
    determinant = 1.0f / determinant;
    m_Cell[0][0] = determinant * (original.m_Cell[1][1] * original.m_Cell[2][2] - original.m_Cell[1][2] * original.m_Cell[2][1]);
    m_Cell[0][1] = -determinant * (original.m_Cell[0][1] * original.m_Cell[2][2] - original.m_Cell[0][2] * original.m_Cell[2][1]);
    m_Cell[0][2] = determinant * (original.m_Cell[0][1] * original.m_Cell[1][2] - original.m_Cell[0][2] * original.m_Cell[1][1]);
    m_Cell[0][3] = 0.0f;
    m_Cell[1][0] = -determinant * (original.m_Cell[1][0] * original.m_Cell[2][2] - original.m_Cell[1][2] * original.m_Cell[2][0]);
    m_Cell[1][1] = determinant * (original.m_Cell[0][0] * original.m_Cell[2][2] - original.m_Cell[0][2] * original.m_Cell[2][0]);
    m_Cell[1][2] = -determinant * (original.m_Cell[0][0] * original.m_Cell[1][2] - original.m_Cell[0][2] * original.m_Cell[1][0]);
    m_Cell[1][3] = 0.0f;
    m_Cell[2][0] = determinant * (original.m_Cell[1][0] * original.m_Cell[2][1] - original.m_Cell[1][1] * original.m_Cell[2][0]);
    m_Cell[2][1] = -determinant * (original.m_Cell[0][0] * original.m_Cell[2][1] - original.m_Cell[0][1] * original.m_Cell[2][0]);
    m_Cell[2][2] = determinant * (original.m_Cell[0][0] * original.m_Cell[1][1] - original.m_Cell[0][1] * original.m_Cell[1][0]);
    m_Cell[2][3] = 0.0f;
    m_Cell[3][0] = -(original.m_Cell[3][0] * m_Cell[0][0] + original.m_Cell[3][1] * m_Cell[1][0] + original.m_Cell[3][2] * m_Cell[2][0]);
    m_Cell[3][1] = -(original.m_Cell[3][0] * m_Cell[0][1] + original.m_Cell[3][1] * m_Cell[1][1] + original.m_Cell[3][2] * m_Cell[2][1]);
    m_Cell[3][2] = -(original.m_Cell[3][0] * m_Cell[0][2] + original.m_Cell[3][1] * m_Cell[1][2] + original.m_Cell[3][2] * m_Cell[2][2]);
    m_Cell[3][3] = 1.0f;
    return 1;
}

// The PC loop advances one four-float column at a time. Each output component
// combines that right-hand column with one row from the left-hand matrix.
RVA(0x0024e040, 0x131)
matrix4 operator*(const matrix4& left, const matrix4& right)
{
    matrix4 result;
    for (int column = 0; column < 4; ++column)
    {
        result.m_Cell[column][0] = (left.m_Cell[0][0] * right.m_Cell[column][0]) + (left.m_Cell[1][0] * right.m_Cell[column][1]) + (left.m_Cell[2][0] * right.m_Cell[column][2]) + (left.m_Cell[3][0] * right.m_Cell[column][3]);
        result.m_Cell[column][1] = (left.m_Cell[0][1] * right.m_Cell[column][0]) + (left.m_Cell[1][1] * right.m_Cell[column][1]) + (left.m_Cell[2][1] * right.m_Cell[column][2]) + (left.m_Cell[3][1] * right.m_Cell[column][3]);
        result.m_Cell[column][2] = (left.m_Cell[0][2] * right.m_Cell[column][0]) + (left.m_Cell[1][2] * right.m_Cell[column][1]) + (left.m_Cell[2][2] * right.m_Cell[column][2]) + (left.m_Cell[3][2] * right.m_Cell[column][3]);
        result.m_Cell[column][3] = (left.m_Cell[0][3] * right.m_Cell[column][0]) + (left.m_Cell[1][3] * right.m_Cell[column][1]) + (left.m_Cell[2][3] * right.m_Cell[column][2]) + (left.m_Cell[3][3] * right.m_Cell[column][3]);
    }
    return result;
}

// PC interpolation retains the original spherical path and the later
// polynomial normalization path as separate functions.
RVA(0x0024cf50, 0x191)
quaternion BlendSlow(const quaternion& first, const quaternion& second, float factor)
{
    quaternion result;
    float cosine, sine;
    float angle, reciprocalSine, interpolatedAngle;
    float firstWeight, secondWeight;
    float x, y, z, w;
    cosine = first.X * second.X + first.Y * second.Y + first.Z * second.Z + first.W * second.W;
    if (cosine < 0.0f)
    {
        x = -first.X;
        y = -first.Y;
        z = -first.Z;
        w = -first.W;
        cosine = -cosine;
    }
    else
    {
        x = first.X;
        y = first.Y;
        z = first.Z;
        w = first.W;
    }
    sine = 1.0f - cosine * cosine;
    if (sine < 0.0f)
        sine = -sine;
    sine = (float)sqrt((double)sine);
    if (sine < 1e-3 && sine > -1e-3)
        return first;
    angle = x_atan2(sine, cosine);
    reciprocalSine = 1.0f / sine;
    interpolatedAngle = factor * angle;
    firstWeight = x_sin(angle - interpolatedAngle) * reciprocalSine;
    secondWeight = x_sin(interpolatedAngle) * reciprocalSine;
    result.X = firstWeight * x + secondWeight * second.X;
    result.Y = firstWeight * y + secondWeight * second.Y;
    result.Z = firstWeight * z + secondWeight * second.Z;
    result.W = firstWeight * w + secondWeight * second.W;
    return result;
}

RVA(0x0024d0f0, 0x160)
quaternion BlendToIdentitySlow(const quaternion& first, float factor)
{
    quaternion result;
    float cosine, sine;
    float angle, reciprocalSine, interpolatedAngle;
    float firstWeight, secondWeight;
    float x, y, z, w;
    cosine = first.W;
    if (cosine < 0.0f)
    {
        x = -first.X;
        y = -first.Y;
        z = -first.Z;
        w = -first.W;
        cosine = -cosine;
    }
    else
    {
        x = first.X;
        y = first.Y;
        z = first.Z;
        w = first.W;
    }
    sine = 1.0f - cosine * cosine;
    if (sine < 0.0f)
        sine = -sine;
    sine = (float)sqrt((double)sine);
    if (sine < 1e-3 && sine > -1e-3)
        return first;
    angle = x_atan2(sine, cosine);
    reciprocalSine = 1.0f / sine;
    interpolatedAngle = factor * angle;
    firstWeight = x_sin(angle - interpolatedAngle) * reciprocalSine;
    secondWeight = x_sin(interpolatedAngle) * reciprocalSine;
    result.X = firstWeight * x;
    result.Y = firstWeight * y;
    result.Z = firstWeight * z;
    result.W = firstWeight * w + secondWeight;
    return result;
}

RVA(0x0024d250, 0x11f)
quaternion Blend(const quaternion& first, const quaternion& second, float factor)
{
    float dot;
    float lengthSquared;
    float scale;
    float x, y, z, w;
    dot = first.X * second.X + first.Y * second.Y + first.Z * second.Z + first.W * second.W;
    if (dot < 0.0f)
    {
        x = -first.X;
        y = -first.Y;
        z = -first.Z;
        w = -first.W;
    }
    else
    {
        x = first.X;
        y = first.Y;
        z = first.Z;
        w = first.W;
    }
    x = x + factor * (second.X - x);
    y = y + factor * (second.Y - y);
    z = z + factor * (second.Z - z);
    w = w + factor * (second.W - w);
    lengthSquared = x * x + y * y + z * z + w * w;
    if (lengthSquared < 0.857f)
        scale = (0.699368f * lengthSquared - 1.819985f) * lengthSquared + 2.126369f;
    else
        scale = (0.454012f * lengthSquared - 1.403517f) * lengthSquared + 1.949542f;
    return quaternion(x * scale, y * scale, z * scale, w * scale);
}

RVA(0x0024d370, 0x107)
quaternion BlendToIdentity(const quaternion& first, float factor)
{
    float lengthSquared;
    float scale;
    float x, y, z, w;
    if (first.W < 0.0f)
    {
        x = -first.X;
        y = -first.Y;
        z = -first.Z;
        w = -first.W;
    }
    else
    {
        x = first.X;
        y = first.Y;
        z = first.Z;
        w = first.W;
    }
    x = x + factor * (-x);
    y = y + factor * (-y);
    z = z + factor * (-z);
    w = w + factor * (1.0f - w);
    lengthSquared = x * x + y * y + z * z + w * w;
    if (lengthSquared < 0.857f)
        scale = (0.699368f * lengthSquared - 1.819985f) * lengthSquared + 2.126369f;
    else
        scale = (0.454012f * lengthSquared - 1.403517f) * lengthSquared + 1.949542f;
    return quaternion(x * scale, y * scale, z * scale, w * scale);
}

// Conversion and composition follow the PC path through a normalized
// quaternion; scale and translation are discarded by the rotation constructor.
RVA(0x0024d8e0, 0x308)
quaternion matrix4::GetQuaternion() const
{
    float scale;
    float x2,y2,z2,w2;
    int largest;
    quaternion result;
    matrix4 normalized=*this;
    normalized.ClearScale();
    w2=0.25f*(normalized(0,0)+normalized(1,1)+normalized(2,2)+1.0f);
    x2=w2-0.5f*(normalized(1,1)+normalized(2,2));
    y2=w2-0.5f*(normalized(2,2)+normalized(0,0));
    z2=w2-0.5f*(normalized(0,0)+normalized(1,1));
    largest=w2>x2 ? (w2>y2 ? (w2>z2 ? 0:3) : (y2>z2 ? 2:3)) : (x2>y2 ? (x2>z2 ? 1:3) : (y2>z2 ? 2:3));
    switch(largest)
    {
    case 0:
        result.W=(float)sqrt(w2);scale=0.25f/result.W;
        result.X=(normalized(1,2)-normalized(2,1))*scale;
        result.Y=(normalized(2,0)-normalized(0,2))*scale;
        result.Z=(normalized(0,1)-normalized(1,0))*scale;
        break;
    case 1:
        result.X=(float)sqrt(x2);scale=0.25f/result.X;
        result.W=(normalized(1,2)-normalized(2,1))*scale;
        result.Y=(normalized(1,0)+normalized(0,1))*scale;
        result.Z=((normalized(2,0))+(normalized(0,2)))*scale;
        break;
    case 2:
        result.Y=(float)sqrt(y2);scale=0.25f/result.Y;
        result.W=(normalized(2,0)-normalized(0,2))*scale;
        result.Z=(normalized(2,1)+normalized(1,2))*scale;
        result.X=(normalized(0,1)+normalized(1,0))*scale;
        break;
    case 3:
        result.Z=(float)sqrt(z2);scale=0.25f/result.Z;
        result.W=(normalized(0,1)-normalized(1,0))*scale;
        result.X=((normalized(2,0))+(normalized(0,2)))*scale;
        result.Y=(normalized(1,2)+normalized(2,1))*scale;
        break;
    }
    result.Normalize();
    return result;
}

RVA(0x0024dc00, 0x216)
matrix4 operator*(const matrix4& matrix,const quaternion& rotation)
{
    return matrix4((matrix*matrix4(rotation)).GetQuaternion());
}
RVA(0x0024de20, 0x216)
matrix4 operator*(const quaternion& rotation,const matrix4& matrix)
{
    return matrix4((matrix4(rotation)*matrix).GetQuaternion());
}


RVA(0x0024e180, 0x445)
int bbox::Intersect(const vector3* vertices,int count)const
{
    plane sides[6];
    sides[0].Setup(Min,vector3(1,0,0));
    sides[1].Setup(Min,vector3(0,1,0));
    sides[2].Setup(Min,vector3(0,0,1));
    sides[3].Setup(Max,vector3(-1,0,0));
    sides[4].Setup(Max,vector3(0,-1,0));
    sides[5].Setup(Max,vector3(0,0,-1));
    bbox bounds;
    bounds.AddVerts(vertices,count);
    if(!bounds.Intersect(*this))return 0;
    vector3 first[64];
    vector3 second[64];
    int firstCount,secondCount;
    sides[0].ClipNGon(first,firstCount,vertices,count);
    sides[1].ClipNGon(second,secondCount,first,firstCount);
    sides[2].ClipNGon(first,firstCount,second,secondCount);
    sides[3].ClipNGon(second,secondCount,first,firstCount);
    sides[4].ClipNGon(first,firstCount,second,secondCount);
    sides[5].ClipNGon(second,secondCount,first,firstCount);
    return secondCount!=0;
}

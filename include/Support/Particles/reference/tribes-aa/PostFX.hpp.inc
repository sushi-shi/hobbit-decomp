//==============================================================================
//
//  PostFX.hpp
//
//==============================================================================

//==============================================================================
//  DEFINES
//==============================================================================
#ifndef __POSTFX_HPP__
#define __POSTFX_HPP__


#define MAX_POST_FX   50                // max. effects per frame

//==============================================================================
//  TYPES
//==============================================================================
class postfx_element
{
public:
    enum type
    {
        UNDEFINED = 0,
        SPOT,
        QUAD
    };

    postfx_element              ( )                 { m_Life = 0.0f; }

    vector3                     m_Pt[2];            // location for the effect
    vector2                     m_Size;             // size of the effect
    xcolor                      m_Color;            // color (of course)
    type                        m_Type;             // spot or quad, presently
    f32                         m_Age;
    f32                         m_Life;
};


class postfx
{
    postfx_element              m_FX    [ MAX_POST_FX ];
    s32                         m_Idx;

public:

    void    Reset               ( void )    { m_Idx = 0; }
    void    AddFX               ( const vector3& Pt, const vector2& Size, const xcolor& Color, f32 Life = 0.01f );
    void    AddFX               ( const vector3& Pt1, const vector3& Pt2, f32 Size, const xcolor& Color );
    void    Render              ( void );

private:
    vector2 CalcUV              ( u32 x, u32 y );
    void    SetTextureSource    ( s32 FrameIndex );
    void    RenderGlow          ( const vector3& Pt, f32 Size, xcolor Color );

};


#endif
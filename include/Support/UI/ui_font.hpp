// Earlier Hobbit font family reconstructed from pinned PC RTTI, complete
// constructor/caller/vtable/field/body proofs. Original field/helper spellings
// remain explicitly reconstructed where no original symbol exists.
#ifndef UI_FONT_HPP
#define UI_FONT_HPP
// PC RTTI/constructor and complete caller reconstruction. Descriptive member
// and backend-operation spelling unknown; const/mutable spelling inferred.
#include <xCore/Entropy/Entropy.hpp>
class ui_font
{
public:
    enum Flags { h_left=1,h_center=2,h_right=4,v_top=8,v_center=16,v_bottom=32,clip_character=64,clip_l_justify=128,clip_r_justify=256,clip_ellipsis=512 };
    ui_font();
    virtual ~ui_font();
    virtual void Kill() = 0;
    virtual s32 GetLineHeight() const = 0;
    virtual s32 GetToken(xwstring& Token) const;
    virtual void MeasureToken(irect& Rect, s32 Type, const xwstring& Token) const = 0;
    virtual void RenderToken(vector3& Position, s32 Type, const xwstring& Token, xcolor Color) const = 0;
    void TextSize(irect& Rect, const xwchar* String, s32 Count = -1) const;
    s32 TextHeight(const xwchar* String,s32 Count=-1) const;
    void RenderText(const irect& Rect,u32 Flags,const xcolor& Color,const xwchar* String,s32 Count=0) const;
    void RenderText(const irect& Rect,u32 Flags,const xcolor& Color,const char* String) const;
    const xwchar* ClipEllipsis(const xwchar* String,const irect& Rect) const;
    const xwchar* TextWrap(const xwchar* String,const irect& Rect) const;
protected:
    void SetString(const xwchar* String) const;
    void PrependString(const xwchar* String) const;
    void TransformToken(s32& Type,xwstring& Token) const;
    mutable xwstring m_String;
};

// Actual PC RTTI and complete allocator/constructor/field proofs. Names of
// backend fields/operations descriptive except corroborated sibling interfaces.
#include <xCore/Entropy/D3DEngine/d3deng_private.hpp>
class ui_bitmap_font : public ui_font
{
public:
    struct Character { s32 X,Y,W; };
    ui_bitmap_font();
    virtual ~ui_bitmap_font();
    xbool Load(const char* Path);
    virtual void Kill();
    virtual s32 GetLineHeight() const { return m_Height; }
    const Character& GetCharacter(s32 Index) const;
    virtual void MeasureToken(irect&,s32,const xwstring&) const;
    virtual void RenderToken(vector3&,s32,const xwstring&,xcolor) const;
protected:
    xbitmap m_Bitmap;
    s32 m_BmWidth,m_BmHeight,m_RowHeight,m_MaxWidth,m_AvgWidth,m_Height;
    Character m_Characters[256];
};
class ui_truetype_font : public ui_font
{
public:
    ui_truetype_font();
    virtual ~ui_truetype_font();
    xbool Load(const char* FaceName,s32 Height,f32 WidthScale,s32 CharSet);
    virtual void Kill();
    virtual s32 GetLineHeight() const { return m_Height; }
    virtual void MeasureToken(irect&,s32,const xwstring&) const;
    virtual void RenderToken(vector3&,s32,const xwstring&,xcolor) const;
    void ReleaseFont();
    void CreateFont();
    static void BeforeReset(void* Context);
    static void AfterReset(void* Context);
protected:
    HFONT m_HFont;
    ID3DXFont* m_pFont;
    s32 m_Height;
    s32 m_CallbackID;
};

#endif // UI_FONT_HPP

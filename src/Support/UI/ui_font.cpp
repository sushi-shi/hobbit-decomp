// Earlier full Hobbit font family: complete PC-supported source methods.
// Predecessor Tribes-AA/Area51 source variants retained in packet/reference
// sources. NO ControlMap definition or allocation extent is manufactured.
// ControlMap.hpp is an honest missing reconstructed game dependency; its
// original header name/path/type/global spelling remains unknown.
#include <Support/UI/ui_font.hpp>
#include "ControlMap.hpp"
#include <Support/UI/ui_font.hpp>
// Complete earlier PC base methods; derived source is in backend.cpp.
ui_font::ui_font() {}
ui_font::~ui_font() {}
static xbool IsLineBreak(xwchar C) { return C == 10 || C == 13; }
static xbool IsSpace(xwchar C)
{
    return C < 256 && x_isspace(C) && !IsLineBreak(C);
}
void ui_font::SetString(const xwchar* String) const
{
    s32 Length = x_wstrlen(String);
    if (Length == 0) m_String.Clear();
    else if (String[Length-1] == 10) m_String = xwstring(Length, String);
    else
    {
        m_String = xwstring(Length, String);
        m_String += (xwchar)10;
    }
}
s32 ui_font::GetToken(xwstring& Token) const
{
    x_mem_owner __owner__("ui_font::RenderText");
    s32 Length=m_String.GetLength();
    if (Length==0) return 4;
    s32 Count=0;
    if (IsSpace(m_String.GetAt(0)))
    {
        while (Count<Length && IsSpace(m_String.GetAt(Count))) ++Count;
    Token = m_String.Left(Count);
    m_String = m_String.Mid(Count, Length-Count);
        return 2;
    }
    if (IsLineBreak(m_String.GetAt(0)))
    {
        while (Count<Length && IsLineBreak(m_String.GetAt(Count))) ++Count;
    Token = m_String.Left(Count);
    m_String = m_String.Mid(Count, Length-Count);
        return 3;
    }
    if (m_String.GetAt(0)=='<')
    {
        while (Count<Length)
        {
            xwchar C=m_String.GetAt(Count++);
            if (C=='>') break;
        }
    Token = m_String.Left(Count);
    m_String = m_String.Mid(Count, Length-Count);
        return 1;
    }
    while (Count<Length)
    {
        xwchar C=m_String.GetAt(Count);
        if (IsSpace(C) || IsLineBreak(C) || C=='<')
        {
    Token = m_String.Left(Count);
    m_String = m_String.Mid(Count, Length-Count);
            return 0;
        }
        ++Count;
    }
    Token=m_String;
    m_String.Clear();
    return 0;
}
RVA(0x2a7dc0, 0x1dc)
void ui_font::TextSize(irect& Rect, const xwchar* String, s32 Count) const
{
    x_mem_owner __owner__("ui_font::TextSize");
    Rect.Set(0,0,0,0);
    if (Count == -1) SetString(String);
    else SetString((const xwchar*)xwstring(Count,String));
    xwstring Token;
    s32 Left = 0, Top = 0, Right = 0, Bottom = 0;
    for (s32 Type = GetToken(Token); Type != 4; Type = GetToken(Token))
    {
        if (Type == 3)
        {
            Top += Rect.b;
            Bottom += Rect.b;
            Rect.l = MIN(Rect.l,Left);
            Rect.t = MIN(Rect.t,Top);
            Rect.r = MAX(Rect.r,Right);
            Rect.b = MAX(Rect.b,Bottom);
            Left = Top = Right = Bottom = 0;
        }
        else
        {
            irect R;
            R.Set(0,0,0,0);
            MeasureToken(R,Type,Token);
            R.l += Right;
            R.r += Right;
            Left = MIN(Left,R.l);
            Top = MIN(Top,R.t);
            Right = MAX(Right,R.r);
            Bottom = MAX(Bottom,R.b);
        }
    }
}

void ui_font::TransformToken(s32& Type,xwstring& Token) const
{
    if (Type!=1) return;
    Type=0;
    switch(Token.GetAt(1))
    {
    case 'A': Token=g_BilboControlMap.GetControlName(4); break;
    case 'D': Token=g_BilboControlMap.GetControlName(1); break;
    case 'L': Token=g_BilboControlMap.GetControlName(13); break;
    case 'R': Token=g_BilboControlMap.GetControlName(22); break;
    case 'S': Token=g_BilboControlMap.GetInputName((input_gadget)185); break;
    case 'a': Token=g_BilboControlMap.GetControlName(1); break;
    case 'b': Token=g_BilboControlMap.GetControlName(7); break;
    case 'd': Token=g_BilboControlMap.GetControlName(36); break;
    case 'e': Token=g_BilboControlMap.GetControlName(44); break;
    case 'f': Token=g_BilboControlMap.GetControlName(45); break;
    case 'g': Token=g_BilboControlMap.GetControlName(51); break;
    case 'h': Token=g_BilboControlMap.GetControlName(74); break;
    case 'i': Token=g_BilboControlMap.GetControlName(68); break;
    case 'j': Token=g_BilboControlMap.GetControlName(48); break;
    case 'k': Token=g_BilboControlMap.GetControlName(49); break;
    case 'l': Token=g_BilboControlMap.GetControlName(30); break;
    case 'q': Token=g_BilboControlMap.GetControlName(70); break;
    case 'r': Token=g_BilboControlMap.GetControlName(27); break;
    case 's': Token=g_BilboControlMap.GetControlName(50); break;
    case 't': Token=g_BilboControlMap.GetControlName(76); break;
    case 'u': Token=g_BilboControlMap.GetControlName(33); break;
    case 'y': Token=g_BilboControlMap.GetControlName(16); break;
    case 'z': Token=g_BilboControlMap.GetControlName(19); break;
    case 'c': case 'x': break;
    default: Type=1; break;
    }
}

// Complete PC 2a86f0/153: counted two natural xwstring temporaries.
// Descriptive method spelling; original spelling unknown.
void ui_font::PrependString(const xwchar* String) const
{
    m_String = xwstring(String) + m_String;
}

// Complete PC TextHeight 2a7d90/35; Xbox corroborates operation spelling.
s32 ui_font::TextHeight(const xwchar* String,s32 Count) const
{
    irect R; TextSize(R,String,Count); return R.b-R.t;
}
// Complete PC narrow RenderText 2a7d10/113; natural one xwstring lifetime.
void ui_font::RenderText(const irect& Rect,u32 Flags,const xcolor& Color,const char* String) const
{
    xwstring Text(String); RenderText(Rect,Flags,Color,Text,0);
}
// Complete PC wide RenderText 2a8af0/990. Final count parameter is unused
// in the complete PC body; Xbox spelling corroborates the signature only.
void ui_font::RenderText(const irect& Rect,u32 Flags,const xcolor& Color,const xwchar* String,s32 Count) const
{
    x_mem_owner __owner__("ui_font::RenderText");
    s32 Left=Rect.l,Top=Rect.t;
    xcolor DrawColor(Color);
    if(Flags&clip_ellipsis) String=ClipEllipsis(String,Rect);
    s32 Height=TextHeight(String,-1);
    if(Flags&v_center) Top+=(Rect.b-Rect.t-Height+4)/2;
    else if(Flags&v_bottom) Top+=Rect.b-Rect.t-Height;
    SetString(String);
    irect Bounds(0,0,0,0);
    xwstring Token;
    s32 Type=GetToken(Token);
    irect LineBounds(0,0,0,0);
    while(Type!=4)
    {
        PrependString(Token);
        xwstring Line;
        Type=GetToken(Token);
        while(Type!=3)
        {
            Line+=Token;
            irect R(0,0,0,0);
            MeasureToken(R,Type,Token);
            R.l+=LineBounds.r; R.r+=LineBounds.r;
            LineBounds.l=MIN(LineBounds.l,R.l);
            LineBounds.t=MIN(LineBounds.t,R.t);
            LineBounds.r=MAX(LineBounds.r,R.r);
            LineBounds.b=MAX(LineBounds.b,R.b);
            Type=GetToken(Token);
        }
        PrependString(Line+L"\n");
        s32 X;
        if(Flags&h_center) X=Left+(Rect.r-Rect.l-LineBounds.r+LineBounds.l)/2;
        else if(Flags&h_right) X=Left+Rect.r-Rect.l-LineBounds.r+LineBounds.l;
        else X=Left;
        vector3 Position((f32)X,(f32)(Top+Bounds.b+2),0.0f);
        Type=GetToken(Token);
        while(Type!=3)
        {
            RenderToken(Position,Type,Token,DrawColor);
            Type=GetToken(Token);
        }
        LineBounds.t+=Bounds.b; LineBounds.b+=Bounds.b;
        Bounds.l=MIN(Bounds.l,LineBounds.l);
        Bounds.t=MIN(Bounds.t,LineBounds.t);
        Bounds.r=MAX(Bounds.r,LineBounds.r);
        Bounds.b=MAX(Bounds.b,LineBounds.b);
        LineBounds.Clear();
        Type=GetToken(Token);
    }
}
// Genuine complete PC callee 2a7c70/65: allocates from a mutable aligned
// UTF16 scratch cursor and copies the real xwstring payload. Original spelling
// inferred; shared ScratchMem storage/provider independently admitted.
static const xwchar* CopyFontString(const xwstring& String);
const xwchar* ui_font::TextWrap(const xwchar* String,const irect& Rect) const
{
    x_mem_owner __owner__("ui_font::TextWrap");
    SetString(String);
    irect Bounds(0,0,0,0);
    xwstring Result,Token;
    irect Line(0,0,0,0);
    s32 Type=GetToken(Token);
    while(Type!=4)
    {
        if(Type==3)
        {
            Line.t+=Bounds.b; Line.b+=Bounds.b;
            Bounds.l=MIN(Bounds.l,Line.l); Bounds.t=MIN(Bounds.t,Line.t);
            Bounds.r=MAX(Bounds.r,Line.r); Bounds.b=MAX(Bounds.b,Line.b);
            Line.Clear();
        }
        else
        {
            irect R(0,0,0,0);
            MeasureToken(R,Type,Token);
            if(Line.r+R.r>Rect.r-Rect.l && Line.l<Line.r && Line.t<Line.b)
            {
                PrependString(Token);
                PrependString(L"\n");
                Type=GetToken(Token);
                continue;
            }
            R.l+=Line.r; R.r+=Line.r;
            Line.l=MIN(Line.l,R.l); Line.t=MIN(Line.t,R.t);
            Line.r=MAX(Line.r,R.r); Line.b=MAX(Line.b,R.b);
        }
        Result+=Token;
        Type=GetToken(Token);
    }
    return CopyFontString(Result);
}
const xwchar* ui_font::ClipEllipsis(const xwchar* String,const irect& Rect) const
{
    x_mem_owner __owner__("ui_font::ClipEllipsis");
    xwstring Ellipsis("...");
    irect EllipsisBounds;
    MeasureToken(EllipsisBounds,0,Ellipsis);
    SetString(String);
    irect Bounds(0,0,0,0);
    xwstring Result,Token;
    irect Line(0,0,0,0);
    s32 Type=GetToken(Token);
    while(Type!=4)
    {
        if(Type==3)
        {
            Line.t+=Bounds.b; Line.b+=Bounds.b;
            Bounds.l=MIN(Bounds.l,Line.l); Bounds.t=MIN(Bounds.t,Line.t);
            Bounds.r=MAX(Bounds.r,Line.r); Bounds.b=MAX(Bounds.b,Line.b);
            Line.Clear();
            Result+=Token;
            if(Bounds.b>=Rect.b-Rect.t) return CopyFontString(Result);
        }
        else if(Type!=0 && Type!=2)
        {
            irect R(0,0,0,0);
            MeasureToken(R,Type,Token);
            if(Line.r+R.r<=Rect.r-Rect.l) Result+=Token;
            else { Result+=Ellipsis; R=EllipsisBounds; }
            R.l+=Line.r; R.r+=Line.r;
            Line.l=MIN(Line.l,R.l); Line.t=MIN(Line.t,R.t);
            Line.r=MAX(Line.r,R.r); Line.b=MAX(Line.b,R.b);
        }
        else
        {
            xwstring Modified(Token);
            irect R(0,0,0,0);
            while(Token.GetLength()!=0)
            {
                R.Clear();
                MeasureToken(R,Type,Modified);
                if(Line.r+R.r<=Rect.r-Rect.l) break;
                Token=Token.Left(Token.GetLength()-1);
                Modified=Token+Ellipsis;
            }
            R.l+=Line.r; R.r+=Line.r;
            Line.l=MIN(Line.l,R.l); Line.t=MIN(Line.t,R.t);
            Line.r=MAX(Line.r,R.r); Line.b=MAX(Line.b,R.b);
            Result+=Modified;
        }
        Type=GetToken(Token);
    }
    return CopyFontString(Result);
}

// Complete actual PC 2a7c70/65. Uses independently admitted ScratchMem storage.
// Descriptive helper spelling; no new storage/address/provider admission.
static const xwchar* CopyFontString(const xwstring& String)
{
    s32 Length=String.GetLength();
    xwchar* Result=(xwchar*)smem_BufferAlloc((Length+1)*sizeof(xwchar));
    x_memcpy(Result,(const xwchar*)String,Length*sizeof(xwchar));
    Result[Length]=0;
    return Result;
}

#include <Support/UI/ui_font.hpp>
// Genuine completed runtime APIs are declared in the shared D3DEngine header.
// Original spellings remain reconstructed; owning full source is in this packet.
// Real Windows Unicode API: PC custom Unicode loader slot resolves CreateFontW.
#include <Support/StringMgr/StringMgr.hpp>
ui_bitmap_font::ui_bitmap_font() {}
ui_bitmap_font::~ui_bitmap_font() {}
const ui_bitmap_font::Character& ui_bitmap_font::GetCharacter(s32 Index) const { return m_Characters[Index]; }
void ui_bitmap_font::Kill() { vram_Unregister(m_Bitmap); m_Bitmap.Kill(); }
xbool ui_bitmap_font::Load( const char* pPathName )
{
    // Load font image
    m_Bitmap.Load( pPathName );

    // Setup info
    m_Height = m_Bitmap.GetHeight()-1;
    x_memset( &m_Characters, 0, sizeof(Character)*256 );

    // Get info from bitmap size
    m_BmWidth   = m_Bitmap.GetWidth();
    m_BmHeight  = m_Bitmap.GetHeight();

    // Clear Data
    m_MaxWidth  = 0;
    m_AvgWidth  = 0;

    // Scan through font building character map
    s32 y = 0;
    xbool Done = FALSE;
    for( s32 Row=0 ; (Row<(7+8)) && !Done ; Row++ )
    {
        // Initialize for character row
        s32 x1 = 0;
        for( s32 Col=0 ; Col<16 ; Col++ )
        {
            // Scan registration marks for character
            s32 x2 = x1+1;
            while( (x2 < m_BmWidth) && (m_Bitmap.GetPixelColor( x2, y ).R < 247) )
                x2++;

            // Skip out if nothing on the row
            ASSERT( x2 < m_BmWidth );

            // Add character
            m_Characters[16+Row*16+Col].X = x1;
            m_Characters[16+Row*16+Col].Y = y+1;
            m_Characters[16+Row*16+Col].W = x2-x1;

            // Update MaxWidth
            if( (x2-x1) > m_MaxWidth )
                m_MaxWidth = (x2-x1);

            // Set start of next character
            x1 = x2+1;
        }

        // Scan down to next row
        if( Row < (6+8) )
        {
            s32 yStart = y;
            y++;
            while( (y < m_BmHeight) &&
                   (m_Bitmap.GetPixelColor( 0, y ).R != 255) )
                y++;

			// Skip out if not found
			if( (y >= m_BmHeight) || ((y-yStart) == 1) )
            {
                Done = TRUE;
				break;
            }

            m_RowHeight = y - yStart;
            m_Height    = m_RowHeight - 1;
        }
    }

    // Set AvgWidth
    m_AvgWidth = m_Characters['x'].W;

    // Register the bitmap
    vram_Register( m_Bitmap );

    // Return success
    return TRUE;
}


void ui_bitmap_font::MeasureToken(irect& R,s32 Type,const xwstring& Token) const
{
    R.Set(0,0,0,0);
    xwstring Text(Token);
    TransformToken(Type,Text);
    if(Type==1)
    {
        switch(Text.GetAt(1))
        {
        case 'D':
        case 'S':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'l':
        case 'r':
        case 's':
        case 'u':
        case 'x':
        case 'y':
            R.r=R.b=18; break;
        case 'L':
        case 'R':
        case 'z':
            R.r=R.b=34; break;
        case 'P':
        case 'Q':
        case 'X':
        case 'p':
            R.r=R.b=16; break;
        }
    }
    else
    {
        R.b=m_RowHeight;
        for(s32 i=0;i<Text.GetLength();++i) R.r+=GetCharacter(Text.GetAt(i)).W;
    }
}
ui_truetype_font::ui_truetype_font() : m_HFont(0),m_pFont(0),m_Height(0),m_CallbackID(0) {}
ui_truetype_font::~ui_truetype_font() { Kill(); }
xbool ui_truetype_font::Load(const char* FaceName,s32 Height,f32 WidthScale,s32 CharSet)
{
    m_Height=Height;
    m_HFont=CreateFontW(-(Height*4)/5,(s32)(Height*WidthScale*0.5f),0,0,700,0,0,0,CharSet,0,0,4,0,(LPCWSTR)Ansi2Wide(FaceName));
    m_CallbackID=d3deng_RegisterFontReset(this,BeforeReset,AfterReset,false);
    CreateFont();
    return m_pFont!=0;
}
void ui_truetype_font::Kill()
{
    if(m_CallbackID) d3deng_UnregisterFontReset(m_CallbackID);
    if(m_HFont) DeleteObject(m_HFont);
    m_HFont=0;
    m_CallbackID=0;
}
void ui_truetype_font::BeforeReset(void* Context) { ((ui_truetype_font*)Context)->ReleaseFont(); }
void ui_truetype_font::AfterReset(void* Context) { ((ui_truetype_font*)Context)->CreateFont(); }
void ui_truetype_font::ReleaseFont()
{
    if(m_pFont) m_pFont->Release();
    m_pFont=0;
}
void ui_truetype_font::CreateFont()
{
    if(m_HFont)
    {
        d3deng_DisableMultisampling(TRUE);
        D3DXCreateFont(g_pd3dDevice,m_HFont,&m_pFont);
    }
}
void ui_truetype_font::MeasureToken(irect& R,s32 Type,const xwstring& Token) const
{
    R.Set(0,0,0,0);
    xwstring Text(Token);
    TransformToken(Type,Text);
    if(Type==1)
    {
        switch(Text.GetAt(1))
        {
        case 'D':
        case 'S':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'l':
        case 'r':
        case 's':
        case 'u':
        case 'x':
        case 'y':
            R.r=R.b=18; break;
        case 'L':
        case 'R':
        case 'z':
            R.r=R.b=34; break;
        case 'P':
        case 'Q':
        case 'X':
        case 'p':
            R.r=R.b=16; break;
        }
    }
    else
    {
        m_pFont->DrawTextW((LPCWSTR)(const xwchar*)Text,Text.GetLength(),(RECT*)&R,DT_SINGLELINE|DT_CALCRECT,0);
        R.b=MAX(R.b,m_Height);
    }
}

#include <Support/UI/ui_manager.hpp>
void ui_bitmap_font::RenderToken(vector3& Position,s32 Type,const xwstring& Token,xcolor Color) const
{
    xwstring Text(Token);
    if(Type==1) Color.Set(0,255,0,255);
    TransformToken(Type,Text);
    if(Type==3) return;
    if(Type==1)
    {
        s32 Icon;
        switch(Text[1])
        {
        case 'D': Icon=7; break;
        case 'L': Icon=8; break;
        case 'P': Icon=17; break;
        case 'Q': Icon=16; break;
        case 'R': Icon=9; break;
        case 'S': Icon=10; break;
        case 'X': Icon=15; break;
        case 'a': Icon=0; break;
        case 'b': Icon=1; break;
        case 'c': Icon=2; break;
        case 'd': Icon=3; break;
        case 'l': Icon=4; break;
        case 'p': Icon=18; break;
        case 'r': Icon=5; break;
        case 's': Icon=11; break;
        case 'u': Icon=6; break;
        case 'x': Icon=12; break;
        case 'y': Icon=13; break;
        case 'z': Icon=14; break;
        default: return;
        }
        draw_Begin(DRAW_SPRITES,0x17);
        draw_SetTexture(g_UiMgr->m_ButtonBitmaps[Icon],0);
        draw_Sprite(Position,vector2(18.0f,18.0f),xcolor(255,255,255,255));
        draw_End();
        Position.X+=18.0f;
        return;
    }
    draw_Begin(DRAW_SPRITES,0x37);
    draw_SetTexture(m_Bitmap,0);
    for(s32 i=0;i<Text.GetLength();++i)
    {
        const Character& C=GetCharacter(Text[i]);
        vector2 UV0((C.X+0.1f)/m_BmWidth,(C.Y+0.1f)/m_BmHeight);
        vector2 UV1((C.X+C.W+0.1f)/m_BmWidth,(C.Y+m_Height+0.1f)/m_BmHeight);
        draw_SpriteUV(Position,vector2((f32)C.W,(f32)m_Height),UV0,UV1,Color);
        Position.X+=(f32)C.W;
    }
    draw_End();
}

void ui_truetype_font::RenderToken(vector3& Position,s32 Type,const xwstring& Token,xcolor Color) const
{
    xwstring Text(Token);
    if(Type==1) Color.Set(0,255,0,255);
    else if(Color.R==Color.G && Color.G==Color.B)
    { Color.G=(u8)(Color.G*0.85f); Color.B=(u8)(Color.B*0.25f); }
    TransformToken(Type,Text);
    if(Type==3) return;
    if(Type==1)
    {
        s32 Icon;
        switch(Text[1])
        {
        case 'D': Icon=7; break;
        case 'L': Icon=8; break;
        case 'P': Icon=17; break;
        case 'Q': Icon=16; break;
        case 'R': Icon=9; break;
        case 'S': Icon=10; break;
        case 'X': Icon=15; break;
        case 'a': Icon=0; break;
        case 'b': Icon=1; break;
        case 'c': Icon=2; break;
        case 'd': Icon=3; break;
        case 'l': Icon=4; break;
        case 'p': Icon=18; break;
        case 'r': Icon=5; break;
        case 's': Icon=11; break;
        case 'u': Icon=6; break;
        case 'x': Icon=12; break;
        case 'y': Icon=13; break;
        case 'z': Icon=14; break;
        default: return;
        }
        draw_Begin(DRAW_SPRITES,0x17);
        draw_SetTexture(g_UiMgr->m_ButtonBitmaps[Icon],0);
        draw_Sprite(Position,vector2(18.0f,18.0f),Color);
        draw_End();
        Position.X+=18.0f;
        return;
    }
    d3deng_DisableMultisampling(TRUE);
    irect Bounds(0,0,0,0);
    m_pFont->DrawTextW(Text,Text.GetLength(),(RECT*)&Bounds,0x420,0);
    xcolor Shadow(0,0,0,128);
    irect R;
    if(Color!=xcolor(0,0,0,255))
    {
        R=Bounds; R.Translate((s32)(Position.X-1.0f),(s32)(Position.Y));
        m_pFont->DrawTextW(Text,Text.GetLength(),(RECT*)&R,0x120,Shadow);
        R=Bounds; R.Translate((s32)(Position.X),(s32)(Position.Y-1.0f));
        m_pFont->DrawTextW(Text,Text.GetLength(),(RECT*)&R,0x120,Shadow);
    }
    R=Bounds; R.Translate((s32)(Position.X+1.0f),(s32)(Position.Y));
    m_pFont->DrawTextW(Text,Text.GetLength(),(RECT*)&R,0x120,Shadow);
    R=Bounds; R.Translate((s32)(Position.X),(s32)(Position.Y+1.0f));
    m_pFont->DrawTextW(Text,Text.GetLength(),(RECT*)&R,0x120,Shadow);
    R=Bounds; R.Translate((s32)(Position.X),(s32)(Position.Y));
    m_pFont->DrawTextW(Text,Text.GetLength(),(RECT*)&R,0x120,Color);
    Position.X+=(f32)Bounds.r;
}


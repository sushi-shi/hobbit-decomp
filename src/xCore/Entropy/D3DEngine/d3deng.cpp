#if defined(HOBBIT_D3DENG_TRIBES_STATE)
#include "d3deng_tribes.inc"
#else
#include <rva.h>
#include <xCore/Entropy/D3DEngine/d3deng_private.hpp>
#include <xCore/Entropy/D3DEngine/d3deng_state.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include <xCore/x_files/x_threads.hpp>
#include <stdarg.h>

// Actual complete natural engine storage; inferred live names are ledger-marked.
RVA_COMPGEN(0x00265500, 0xab, ??1eng_locals@@QAE@XZ)
RVA_COMPGEN(0x00265a60, 0x40, ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ)
DATA(0x003edad8)
static eng_locals s;
// Renderer enable datum: actual mutable four-byte word initialized 1, sole
// read by text_RenderStr; identifier inferred, original spelling not recovered.
DATA(0x0034e234)
xbool g_TextRenderEnabled = 1;
DATA(0x003f0480)
xtimer rst;
DATA(0x003f0314)
int rstct;

// Original platform global; identity corroborated by actual device callsites.
DATA(0x003f1000)
IDirect3DDevice8* g_pd3dDevice = NULL;

// Actual mutable SDK presentation-parameters global; descriptive spelling
// inferred. CreateDevice/Reset pointer contracts and the PC13-DWORD copy
// independently establish the complete52-byte object, not its TU BSS order.
DATA(0x003f0448)
D3DPRESENT_PARAMETERS g_CurrentPresentParameters;

#define ENG_FONT_SIZEX 7
#define ENG_FONT_SIZEY 12
#define SCRACH_MEM_SIZE (2*1024*1024)
#define MAX_VERTEX_SHADERS 32
#define MAX_PIXEL_SHADERS 32
#define FULL_SCREEN_STYLE WS_POPUP
#define WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

// Declared platform dependencies have actual source in the imported family.
xbool d3deng_InitInput(HWND);
void vram_Init();
void vram_Kill();
void draw_Init();
void draw_Kill();

xbool eng_Begin(const char* Context);
void eng_End();

RVA(0x0025c8b0, 0x20)
void text_BeginRender()
{
    if (s.bD3DBeginScene == FALSE)
    {
        if (eng_Begin("text_BeginRender"))
            eng_End();
    }
}

RVA(0x0025c8d0, 0xbe)
void text_RenderStr(char* pStr, int NChars, xcolor Color, int PixelX, int PixelY)
{
    dxerr Error;
    RECT Rect;
    if (!g_TextRenderEnabled || !s.bD3DBeginScene)
        return;
    Rect.left = PixelX;
    Rect.top = PixelY;
    Rect.bottom = PixelY + 12;
    Rect.right = Rect.left + NChars * 7;
    rst.Start();
    Error = s.pFont->DrawTextA(pStr,NChars,&Rect,DT_BOTTOM|DT_SINGLELINE|DT_NOCLIP|DT_END_ELLIPSIS|DT_CALCRECT,*((unsigned int*)&Color));
    Error = s.pFont->DrawTextA(pStr,NChars,&Rect,DT_BOTTOM|DT_SINGLELINE|DT_NOCLIP|DT_END_ELLIPSIS,*((unsigned int*)&Color));
    if(Error != D3D_OK) rstct = Error;
    rst.Stop();
}

RVA(0x0025c990, 0x1)
void text_EndRender() {}

#include <stdarg.h>
#include <xCore/x_files/x_string.hpp>
RVA(0x00260210, 0x5f)
void DebugMessage(const char* FormatStr, ...)
{
    va_list Args;
    va_start(Args, FormatStr);
    OutputDebugString(xvfs(FormatStr, Args));
    va_end(Args);
}


// PC-only descriptive identities: original spellings not established.
s32 d3deng_RegisterFontReset(void* Context, void (*BeforeReset)(void*), void (*AfterReset)(void*), bool OneShot)
{
    observed_reset_registry_element Element;
    Element.Context = Context;
    Element.BeforeReset = BeforeReset;
    Element.AfterReset = AfterReset;
    Element.OneShot = OneShot;
    Element.Active = true;
    if (s.ResetRegistryFreeIndices.GetCount() == 0)
    {
        s.ResetRegistry.Append(Element);
        return s.ResetRegistry.GetCount()-1;
    }
    s32 Index = s.ResetRegistryFreeIndices[s.ResetRegistryFreeIndices.GetCount()-1];
    s.ResetRegistryFreeIndices.Delete(s.ResetRegistryFreeIndices.GetCount()-1);
    s.ResetRegistry[Index] = Element;
    return Index;
}
void d3deng_UnregisterFontReset(s32 Index)
{
    if (s.ResetRegistry[Index].Active)
    {
        s.ResetRegistry[Index].Active = false;
        s.ResetRegistryFreeIndices.Append(Index);
    }
}
xbool d3deng_DisableMultisampling(xbool Disable)
{
    // Existing field spellings are descriptive; offsets 0x30/0x34 independently proved.
    xbool Previous = s.SoftwareVertexProcessing;
    if (Disable != Previous && s.VertexProcessingMode == 2)
    {
        s.SoftwareVertexProcessing = Disable;
        if (Disable == 1)
            g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
        else
            g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    }
    return Previous;
}

#include "d3deng_platform.inc"

#include <math.h>
// Actual SDK-typed globals; descriptive original spellings UNKNOWN.
// GetGammaRamp writes1536B at VA7f04a8; no fake class or padding.
static D3DGAMMARAMP g_OriginalGammaRamp_UNKNOWN;
// IDirect3D8::GetDeviceCaps writes actual D3DCAPS8 atVA7f0f10.
static D3DCAPS8 g_DeviceCaps_UNKNOWN;
// Actual complete PC260a10/23; standard SDK pointer contract.
static void d3deng_RestoreGamma_UNKNOWN()
{
    if(g_pd3dDevice)
        g_pd3dDevice->SetGammaRamp(1,&g_OriginalGammaRamp_UNKNOWN);
}
// Actual complete PC2608d0/293. Original function spelling UNKNOWN.
// Real GammaLookup table deliberately remains undeclared: full source array
// extent/type/owning declaration not proven by the256-byte loop alone.
static void d3deng_SetGamma_UNKNOWN(f32 Gamma)
{
    if(Gamma<=0.0f) return;
    if(Gamma==1.0f)
    {
        d3deng_RestoreGamma_UNKNOWN();
        s.GammaMode=0;
        s.Gamma=Gamma;
        return;
    }
    D3DGAMMARAMP Ramp;
    double Exponent=1.0/(double)Gamma;
    for(s32 i=0;i<256;++i)
    {
        double Value=pow((double)i*(1.0/255.0),Exponent);
        Ramp.red[i]=Ramp.green[i]=Ramp.blue[i]=(WORD)(s32)(65535.0*Value);
        g_GammaLookup_UNKNOWN[i]=(unsigned char)(s32)(255.0*Value);
    }
    if(!g_CurrentPresentParameters.Windowed &&
       (g_DeviceCaps_UNKNOWN.Caps2&D3DCAPS2_FULLSCREENGAMMA))
    {
        g_pd3dDevice->SetGammaRamp(0,&Ramp);
        s.GammaMode=1;
    }
    else s.GammaMode=3;
    s.Gamma=Gamma;
}

// Actual complete PC260da0/213 and260e80/158 owner bodies.
// Descriptive UNKNOWN spellings carry no provider/address claim.
// Unreconstructed real gamma/container/count dependencies remain undeclared;
// no bridge prototypes or duplicate storage supplied merely to compile.
static void d3deng_BeforeDeviceReset_UNKNOWN()
{
    if (s.Resetting)
    {
        ++g_ResetWaitCount_UNKNOWN; // Actual mutable global VA7f1424.
        Sleep(100);
        if ((g_ResetWaitCount_UNKNOWN & (g_ResetWaitCount_UNKNOWN-1)) == 0)
            x_DebugMsg("Lost D3D Device (duplicate %d)\n",g_ResetWaitCount_UNKNOWN);
        return;
    }
    x_DebugMsg("Lost D3D Device\n");
    g_ResetWaitCount_UNKNOWN = 0;
    d3deng_RestoreGamma_UNKNOWN(); // Actual complete23B callee260a10, unresolved real gammaRamp.
    draw_Kill();
    if (s.pFont) { s.pFont->Release(); s.pFont=0; }
    d3deng_ReleaseDeviceResources_UNKNOWN(); // Actual265B container lifecycle262790.
    for (s32 Index=0; Index<s.ResetRegistry.GetCount(); ++Index)
        if (s.ResetRegistry[Index].Active && s.ResetRegistry[Index].BeforeReset)
            s.ResetRegistry[Index].BeforeReset(s.ResetRegistry[Index].Context);
    g_pd3dDevice->ResourceManagerDiscardBytes(0xffffffff);
    s.Resetting=true;
}
static void d3deng_AfterDeviceReset_UNKNOWN()
{
    x_DebugMsg("Reset D3D Device\n");
    draw_Init();
    D3DXCreateFont(g_pd3dDevice,s.Font,&s.pFont);
    d3deng_SetGamma_UNKNOWN(s.Gamma); // Actual complete293B callee2608d0.
    for(s32 Index=0; Index<s.ResetRegistry.GetCount(); ++Index)
    {
        observed_reset_registry_element Element=s.ResetRegistry[Index];
        if(Element.Active)
        {
            if(Element.OneShot) d3deng_UnregisterFontReset(Index);
            if(Element.AfterReset) Element.AfterReset(Element.Context);
        }
    }
    s.Resetting=false;
}


#endif

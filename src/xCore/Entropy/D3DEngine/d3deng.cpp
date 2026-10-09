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

#include <map>
// PC-inferred cache contract: both vertex and index buffers enter the same
// pointer-keyed tree. Both SDK interfaces have IDirect3DResource8 as their
// single primary base, with no pointer adjustment. Original private names
// and the original nominal map template arguments have not been recovered.
// Original private getter spelling unknown; actual no-argument cdecl ABI.
RVA(0x0025bb00, 0x6)
s32 d3deng_GetVertexProcessingMode_UNKNOWN()
{
    return s.VertexProcessingMode;
}

DATA(0x003f0498)
static std::map<IDirect3DResource8*, IDirect3DResource8*> g_BufferCopies;

RVA(0x00262390, 0x141)
IDirect3DVertexBuffer8* d3deng_GetVertexBufferCopy(IDirect3DVertexBuffer8* Buffer)
{
    if(d3deng_GetVertexProcessingMode_UNKNOWN() == 3) return Buffer;
    if(!Buffer) return NULL;
    std::map<IDirect3DResource8*, IDirect3DResource8*>::iterator Found = g_BufferCopies.find(Buffer);
    if(Found != g_BufferCopies.end())
        return static_cast<IDirect3DVertexBuffer8*>(Found->second);
    g_pd3dDevice->ResourceManagerDiscardBytes(0);
    D3DVERTEXBUFFER_DESC Desc;
    Buffer->GetDesc(&Desc);
    IDirect3DVertexBuffer8* Copy = NULL;
    HRESULT Error = g_pd3dDevice->CreateVertexBuffer(Desc.Size, Desc.Usage|D3DUSAGE_WRITEONLY, Desc.FVF, D3DPOOL_DEFAULT, &Copy);
    if(Error != D3D_OK) return NULL;
    BYTE* Source;
    BYTE* Destination;
    Buffer->Lock(0,0,&Source,0);
    Copy->Lock(0,0,&Destination,0);
    memcpy(Destination,Source,Desc.Size);
    Buffer->Unlock();
    Copy->Unlock();
    g_BufferCopies[Buffer] = Copy;
    return Copy;
}
RVA(0x002624e0, 0x2f)
HRESULT d3deng_SetStreamSource(UINT Stream, IDirect3DVertexBuffer8* Buffer, UINT Stride)
{
    return g_pd3dDevice->SetStreamSource(Stream,d3deng_GetVertexBufferCopy(Buffer),Stride);
}
RVA(0x00262510, 0x7a)
void d3deng_ReleaseVertexBuffer(IDirect3DVertexBuffer8** Buffer)
{
    if(*Buffer)
    {
        std::map<IDirect3DResource8*, IDirect3DResource8*>::iterator Found = g_BufferCopies.find(*Buffer);
        if(Found != g_BufferCopies.end())
        {
            Found->second->Release();
            g_BufferCopies.erase(Found);
        }
        (*Buffer)->Release();
        *Buffer = NULL;
    }
}
RVA(0x00262590, 0x171)
HRESULT d3deng_SetIndices(IDirect3DIndexBuffer8* Buffer, UINT BaseVertexIndex)
{
    // PC submission counter is the existing field at state+0x25a8.
    ++s.RendererStat6;
    if(d3deng_GetVertexProcessingMode_UNKNOWN() == 3)
        return g_pd3dDevice->SetIndices(Buffer,BaseVertexIndex);
    IDirect3DIndexBuffer8* Copy = NULL;
    std::map<IDirect3DResource8*, IDirect3DResource8*>::iterator Found = g_BufferCopies.find(Buffer);
    if(Found != g_BufferCopies.end())
        Copy = static_cast<IDirect3DIndexBuffer8*>(Found->second);
    else
    {
        g_pd3dDevice->ResourceManagerDiscardBytes(0);
        D3DINDEXBUFFER_DESC Desc;
        Buffer->GetDesc(&Desc);
        HRESULT Error = g_pd3dDevice->CreateIndexBuffer(Desc.Size,Desc.Usage|D3DUSAGE_WRITEONLY,Desc.Format,D3DPOOL_DEFAULT,&Copy);
        if(Error != D3D_OK) return Error;
        BYTE* Source;
        BYTE* Destination;
        Buffer->Lock(0,0,&Source,0);
        Copy->Lock(0,0,&Destination,0);
        memcpy(Destination,Source,Desc.Size);
        Buffer->Unlock();
        Copy->Unlock();
        g_BufferCopies[Buffer] = Copy;
    }
    g_pd3dDevice->SetIndices(Copy,BaseVertexIndex);
    return D3D_OK;
}
RVA(0x00262710, 0x7a)
void d3deng_ReleaseIndexBuffer(IDirect3DIndexBuffer8** Buffer)
{
    if(*Buffer)
    {
        std::map<IDirect3DResource8*, IDirect3DResource8*>::iterator Found = g_BufferCopies.find(*Buffer);
        if(Found != g_BufferCopies.end())
        {
            Found->second->Release();
            g_BufferCopies.erase(Found);
        }
        (*Buffer)->Release();
        *Buffer = NULL;
    }
}
RVA(0x00262790, 0x109)
void d3deng_ReleaseDeviceResources_UNKNOWN()
{
    for(std::map<IDirect3DResource8*, IDirect3DResource8*>::iterator It=g_BufferCopies.begin();It!=g_BufferCopies.end();++It)
        It->second->Release();
    g_BufferCopies.clear();
}

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
// PC writes exactly256 unsigned bytes; independent tree storage begins at
// the following byte3f0428. Original private spelling is unknown.
DATA(0x003f0328)
static unsigned char g_GammaLookup_UNKNOWN[256];
DATA(0x003f1424)
static int g_ResetWaitCount_UNKNOWN;
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
// Gamma lookup, wait counter and mixed-buffer cache now have complete
// PC-supported storage and operations above.
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

#ifndef HOBBIT_D3DENG_STATE_HPP
#define HOBBIT_D3DENG_STATE_HPP
#include <rva.h>
#include <windows.h>
#include <string.h>
#include <xstring> // Genuine VC6 basic_string and wstring declarations; no stream APIs needed.
#include <stddef.h>
#include <d3d8.h>
// PC-inferred performance record; original type/field spellings unknown.
struct observed_performance_record {
    float Levels[8]; int Count; float Current;
    observed_performance_record() : Count(0), Current(-1.0f) { memset(Levels, 0, sizeof(Levels)); }
};
// Actual 16-byte reset registry record; fields and callbacks established by
// PC registration/reset walks. The class/field spellings are inferred.
struct observed_reset_registry_element {
    void* Context;
    void (*BeforeReset)(void*);
    void (*AfterReset)(void*);
    bool OneShot;
    bool Active;
};
#include <xCore/Entropy/e_View.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_time.hpp>
#include <xCore/x_files/x_array.hpp>
struct ID3DXFont;

// Complete natural state type. Live fields follow independently recorded PC
// accesses and types; unused original members follow the sibling source
// lineage. Original versus inferred spellings are recorded in the ledgers.
struct eng_locals {
    RVA(0x002653e0, 0x11c)
    eng_locals() : PendingResolution(0), PendingWidth(0), PendingHeight(0), PendingFormat(D3DFMT_A8R8G8B8), PendingWindowed(0) { memset(this, 0, sizeof(eng_locals)); }
    HINSTANCE hInst;
    HWND hWnd;
    ID3DXFont* pFont;
    HFONT Font;
    xcolor TextColor;
    unsigned int Mode;
    xcolor BackColor;
    int bBeginScene;
    int bD3DBeginScene;
    int Adapter;
    float Gamma;
    int GammaMode;
    int VertexProcessingMode;
    int SoftwareVertexProcessing;
    int HasStencil;
    int ClearRequested;
    bool ShowInfo;
    bool ReferenceRasterizer;
    bool SoftwareTransformAndLight;
    bool SoftwareLighting;
    bool SafeSettings;
    int FrameCounter;
    int DrawCount;
    std::wstring WindowTitle;
    bool ForceTwoTextures;
    bool ForceNV10;
    view View[8];
    view* ActiveView[8];
    int nActiveViews;
    int nViews; // Genuine unused declaration in every Tribes source; cross-build evidence.
    unsigned int ViewMask;
    bool bActive;
    float AspectRatio;
    int WindowMode;
    int bReady;
    RECT rcWindowRect;
    int Width, Height;
    int MaxXRes, MaxYRes;
    MSG Msg; // Genuine unused declaration in both original sibling engines.
    int LastPressedKey; // Same evidence; zero PC direct uses.
    xtick MouseEventTime;
    float ABSMouseX, ABSMouseY;
    int MouseX, MouseY;
    int MouseCapture;
    int bMouseLeftButton,bMouseRightButton,bMouseMiddleButton;
    float MouseWheelAbs,MouseWheelRel,MouseWheelScale;
    int bMouseDelta; // Original xbool unused declaration; xbool is int32 in primary donor.
    int MouseMode;
    xtick FPSFrameTime[8];
    xtick FPSLastTime;
    int FPSIndex;
    xtimer CPUTIMER;
    float CPUMS, IMS;
    int RendererFrames, EmptyRendererFrames, NonemptyRendererFrames;
    int RendererSubmissions, RendererSubmissionSum, RendererCurrentSubmissions;
    int RendererStat6;
    HWND CursorWindow;
    HWND Windows[8];
    int WindowCount, CurrentWindow, WindowState;
    xarray<observed_reset_registry_element> ResetRegistry;
    xarray<int> ResetRegistryFreeIndices;
    bool Resetting,ResetRequested;
    D3DPRESENT_PARAMETERS d3dpp;
    int SavedWindowX,SavedWindowY,WindowWidth,WindowHeight;
    DWORD WindowStyle,WindowExStyle,FullScreenStyle,FullScreenExStyle;
    int VideoBitDepth;
    observed_performance_record PerformanceSettings[10];
    int PerformanceMode;
    int PendingResolution,PendingWidth,PendingHeight;
    D3DFORMAT PendingFormat;
    int PendingWindowed;
    int InitializationState,CommandOption;
    int CapabilityValues[4];
    bool OptionsAvailable;

};


#endif

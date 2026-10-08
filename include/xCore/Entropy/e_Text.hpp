//
// E_TEXT.HPP
//

#ifndef HOBBIT_E_TEXT_HPP
#define HOBBIT_E_TEXT_HPP

#include <xCore/x_files/x_color.hpp>

// Public calls
void text_PtToCell(int PixelX, int PixelY, int& CellX, int& CellY);
void text_CellToPt(int CellX, int CellY, int& PixelX, int& PixelY);
void text_Print(const char* pStr);
void text_PrintXY(const char* pStr, int CellX, int CellY);
void text_PrintPixelXY(const char* pStr, int PixelX, int PixelY);
void text_On();
void text_Off();

void text_SetParams(
    int ScreenWidth,
    int ScreenHeight,
    int XBorderWidth,
    int YBorderWidth,
    int CharacterWidth,
    int CharacterHeight,
    int NScrollLines
);

void text_GetParams(
    int& ScreenWidth,
    int& ScreenHeight,
    int& XBorderWidth,
    int& YBorderWidth,
    int& CharacterWidth,
    int& CharacterHeight,
    int& NScrollLines
);

void text_PushColor(xcolor C);
void text_PopColor();
xcolor text_GetColor();

// Private platform independent functions
void text_Init();
void text_Kill();
void text_ClearBuffers();
void text_Render();

// Original D3DEngine/d3deng.cpp interfaces; provider bodies remain unreconstructed.
void text_BeginRender();
void text_RenderStr(char* pStr, int NChars, xcolor Color, int PixelX, int PixelY);
void text_EndRender();

#endif

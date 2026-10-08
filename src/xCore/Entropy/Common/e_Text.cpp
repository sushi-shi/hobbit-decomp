#include <rva.h>

#include <xCore/Entropy/e_ScratchMem.hpp>
#include <xCore/Entropy/e_Text.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>

#define ASSERT(x) static_cast<void>(0)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Naturally emitted by the original static xcolor array.
RVA_COMPGEN(0x00001080, 0x30, ??_H@YGXPAXIHP6EX0@Z@Z)

DATA(0x00408f28)
static int s_Initialized = 0;

DATA(0x00408ed8)
static char* s_BufferMemory;
DATA(0x00408ebc)
static char* s_Buffer;
DATA(0x00408ed4)
static char* s_ScrollBuffer;
DATA(0x00408ee8)
static char* s_ScrollLine;
DATA(0x00408eb8)
static short* s_LineTouched;
DATA(0x00408ec8)
static int s_ScrollCursor;

DATA(0x00408f24)
static int s_ScreenWidth;
DATA(0x00408ec4)
static int s_ScreenHeight;
DATA(0x00408ee4)
static int s_XBorderWidth;
DATA(0x00408f1c)
static int s_YBorderWidth;
DATA(0x00408f18)
static int s_CharWidth;
DATA(0x00408eec)
static int s_CharHeight;
DATA(0x00408f20)
static int s_NScrollLines;
DATA(0x00408ee0)
static int s_ScrollBufferSize;

DATA(0x00408ef0)
static int s_BufferWidth;
DATA(0x00408ec0)
static int s_BufferHeight;
DATA(0x00408ef4)
static int s_BufferSize;

DATA(0x00408ed0)
static int s_NewParameters;

DATA(0x00408edc)
static int s_TextOff;

DATA(0x00408ef8)
static xcolor s_ColorStack[8];
RVA_DYNINIT(0x0027d010, 0x5, s_ColorStack)
RVA_DYNINIT(0x0027d020, 0x1, s_ColorStack)
DATA(0x00408f2c)
static int s_ColorStackIndex = 0;

struct text_str {
    int X;
    int Y;
    int Len;
    xcolor Color;
    text_str* pNext;
};

DATA(0x00408ecc)
static text_str* s_TextStringFirst;

//=========================================================================

RVA(0x0027d030, 0x6d)
void text_Init() {
    ASSERT(!s_Initialized);
    s_Initialized = 1;

    // Setup some default params but don't allocate buffers

    text_SetParams(1024, 768, 0, 0, 8, 8, 8);

    s_Initialized = 1;
    s_TextOff = 0;
    s_BufferMemory = 0;
    s_TextStringFirst = 0;
    s_ColorStackIndex = -1;

    x_SetPrintHook(text_Print);
    x_SetPrintAtHook(text_PrintXY);
}

//=========================================================================

RVA(0x0027d0a0, 0x2a)
void text_Kill() {
    ASSERT(s_Initialized);
    s_Initialized = 0;
    x_free_fn(s_BufferMemory, "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_Text.cpp", 93);
    s_BufferMemory = 0;
}

//=========================================================================

RVA(0x0027d0d0, 0xb)
void text_Off() {
    s_TextOff = 1;
}

//=========================================================================

RVA(0x0027d0e0, 0xb)
void text_On() {
    s_TextOff = 0;
}

//=========================================================================

RVA(0x0027d0f0, 0x53)
void text_GetParams(
    int& ScreenWidth,
    int& ScreenHeight,
    int& XBorderWidth,
    int& YBorderWidth,
    int& CharacterWidth,
    int& CharacterHeight,
    int& NScrollLines
) {
    ScreenWidth = s_ScreenWidth;
    ScreenHeight = s_ScreenHeight;
    XBorderWidth = s_XBorderWidth;
    YBorderWidth = s_YBorderWidth;
    CharacterWidth = s_CharWidth;
    CharacterHeight = s_CharHeight;
    NScrollLines = s_NScrollLines;
}

//=========================================================================

RVA(0x0027d150, 0x89)
void text_SetParams(
    int ScreenWidth,
    int ScreenHeight,
    int XBorderWidth,
    int YBorderWidth,
    int CharacterWidth,
    int CharacterHeight,
    int NScrollLines
) {
    s_ScreenWidth = ScreenWidth;
    s_ScreenHeight = ScreenHeight;
    s_XBorderWidth = XBorderWidth;
    s_YBorderWidth = YBorderWidth;
    s_CharWidth = CharacterWidth;
    s_CharHeight = CharacterHeight;
    s_NScrollLines = NScrollLines;

    s_BufferWidth = (s_ScreenWidth - (2 * s_XBorderWidth)) / (s_CharWidth);
    s_BufferHeight = (s_ScreenHeight - (2 * s_YBorderWidth)) / (s_CharHeight);
    s_BufferSize = s_BufferWidth * s_BufferHeight;
    s_ScrollBufferSize = s_BufferWidth * s_NScrollLines;

    s_NewParameters = 1;
}

//=========================================================================

RVA(0x0027d1e0, 0x10a)
void text_ClearBuffers() {
    ASSERT(s_Initialized);

    // Clear buffers and return if text is off
    if (s_TextOff) {
        if (s_BufferMemory) {
            x_DebugMsg("TEXT: Freeing buffers\n");
            x_free_fn(
                s_BufferMemory,
                "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_Text.cpp",
                168
            );
            s_BufferMemory = 0;
        }
        return;
    }

    // Realloc buffers if they have changed
    if (s_NewParameters || (!s_BufferMemory)) {

        int Size = s_BufferSize + s_ScrollBufferSize + (2 * s_BufferHeight);
        s_BufferMemory = static_cast<char*>(x_realloc_fn(
            s_BufferMemory,
            Size,
            "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_Text.cpp",
            182
        ));
        ASSERT(s_BufferMemory);

        s_Buffer = s_BufferMemory;
        s_ScrollBuffer = s_Buffer + s_BufferSize;
        // Byte-evidenced view into the original contiguous allocation.
        s_LineTouched = reinterpret_cast<short*>(s_ScrollBuffer + s_ScrollBufferSize);
        s_ScrollLine = s_ScrollBuffer + s_ScrollBufferSize - s_BufferWidth;
        s_NewParameters = 0;
        x_memset(s_ScrollBuffer, 0, s_ScrollBufferSize);
    }

    // Clear the buffers
    x_memset(s_Buffer, 0, s_BufferSize);
    x_memset(s_LineTouched, 0xFF, 2 * s_BufferHeight);
    s_TextStringFirst = 0;
}

//=========================================================================

RVA(0x0027d2f0, 0x33)
void text_PtToCell(int SX, int SY, int& CX, int& CY) {
    ASSERT(s_Initialized);
    CX = (SX - s_XBorderWidth) / s_CharWidth;
    CY = (SY - s_YBorderWidth) / s_CharHeight;
}

//=========================================================================

RVA(0x0027d330, 0x2e)
void text_CellToPt(int CX, int CY, int& SX, int& SY) {
    ASSERT(s_Initialized);
    SX = (CX * s_CharWidth) + s_XBorderWidth;
    SY = (CY * s_CharHeight) + s_YBorderWidth;
}

//=========================================================================

RVA(0x0027d4f0, 0x43)
static void ScrollTextOneLineUp() {
    x_memmove(s_ScrollBuffer, s_ScrollBuffer + s_BufferWidth, s_BufferWidth * (s_NScrollLines - 1));

    x_memset(s_ScrollLine, 0, s_BufferWidth);

    s_ScrollCursor = 0;
}

//=========================================================================

RVA(0x0027d360, 0x90)
void text_PrintPixelXY(const char* pStr, int PixelX, int PixelY) {
    ASSERT(s_Initialized);
    if (s_TextOff) {
        return;
    }
    if (s_NewParameters || (!s_BufferMemory)) {
        return;
    }

    int Len = x_strlen(pStr);
    int Size = sizeof(text_str) + Len + 1;
    // Byte-evidenced view into the original contiguous allocation.
    text_str* pTS = reinterpret_cast<text_str*>(smem_BufferAlloc(Size));
    // PC caller uses the unchecked inline release scratch allocator.

    pTS->X = PixelX;
    pTS->Y = PixelY;
    pTS->Len = Len;
    pTS->Color = text_GetColor();
    pTS->pNext = s_TextStringFirst;
    s_TextStringFirst = pTS;

    // Byte-evidenced view into the original contiguous allocation.
    x_strcpy(reinterpret_cast<char*>(pTS + 1), pStr);
}

//=========================================================================

RVA(0x0027d3f0, 0xf6)
void text_Print(const char* pStr) {
    // Do asserts and confirm buffers are available
    ASSERT(s_Initialized);
    if (s_TextOff) {
        return;
    }
    if (s_NewParameters || (!s_BufferMemory)) {
        return;
    }

    if (pStr == 0) {
        return;
    }

    // Add string to scroll text area.
    while (*pStr) {
        if ((*pStr) == 0x08) {
            if (s_ScrollCursor == -1) {
                ScrollTextOneLineUp();
            }

            // Expand the tab here.
            // Space to next multiple of 8 column.
            do {
                if (s_ScrollCursor < s_BufferWidth) {
                    s_ScrollLine[s_ScrollCursor] = ' ';
                }
                s_ScrollCursor++;
            } while (s_ScrollCursor & 0x07);
        } else if ((*pStr) == 0x0A) {
            if (s_ScrollCursor == -1) {
                ScrollTextOneLineUp();
            }

            // Handle line feed
            s_ScrollCursor = -1;
        } else if ((*pStr) == 0x0D) {
            // Handle carriage return
            // (Do nothing)
        } else if ((*pStr) == 0x20) {
            if (s_ScrollCursor == -1) {
                ScrollTextOneLineUp();
            }

            // Handle space
            if (s_ScrollCursor < s_BufferWidth) {
                s_ScrollLine[s_ScrollCursor] = ' ';
            }
            s_ScrollCursor++;
        } else {
            // Handle all other characters
            if ((s_ScrollCursor == -1) || (s_ScrollCursor >= s_BufferWidth)) {
                ScrollTextOneLineUp();
            }

            // Stick the character in the data buffer.
            s_ScrollLine[s_ScrollCursor] = *pStr;

            s_ScrollCursor++;
        }

        pStr++;
    }
}

//=========================================================================

RVA(0x0027d540, 0x100)
void text_PrintXY(const char* pStr, int X, int Y) {
    // Do asserts and confirm buffers are available
    ASSERT(s_Initialized);
    if (s_TextOff) {
        return;
    }
    if (s_NewParameters || (!s_BufferMemory)) {
        return;
    }

    int OriginalX = X;

    if (pStr == 0) {
        return;
    }

    // Too far right?  Don't even bother.
    if (X >= s_BufferWidth) {
        return;
    }

    // While there's still some string left AND we are not off the bottom.
    while ((*pStr) && (Y < s_BufferHeight)) {
        if ((*pStr) == 0x08) {
            // Expand the tab here.
            // Space to next multiple of 8 column.
            do {
                if ((X < s_BufferWidth) && (X >= 0) && (Y >= 0)) {
                    s_Buffer[(Y * (s_BufferWidth)) + X] = ' ';
                    s_LineTouched[Y] = MAX(X, s_LineTouched[Y]);
                }
                X++;
            } while (X & 0x07);
        } else if (*pStr == 0x0D) {
            // Carriage return, do nothing.
        } else if (*pStr == 0x0A) {
            // Line feed.
            Y++;
            X = OriginalX;
        } else {
            // All other characters.
            if ((X < s_BufferWidth) && (X >= 0) && (Y >= 0)) {
                s_Buffer[(Y * (s_BufferWidth)) + X] = *pStr;
                s_LineTouched[Y] = MAX(X, s_LineTouched[Y]);
            }
            X++;
        }

        pStr++;
    }
}

//=========================================================================

RVA(0x0027d640, 0x20e)
void text_Render() {
    ASSERT(s_Initialized);
    if (s_TextOff) {
        return;
    }
    if (s_NewParameters || (!s_BufferMemory)) {
        return;
    }

    // "Stamp" the scroll text onto the small text buffer.
    {
        int DestOffset = s_BufferWidth * (s_BufferHeight - s_NScrollLines);
        int Characters = s_ScrollBufferSize;

        char* s = s_ScrollBuffer;
        char* d = s_Buffer + DestOffset;

        while (Characters) {
            if (*s) {
                *d = *s;
            }
            s++;
            d++;
            Characters--;
        }

        // Touch all lines that scroll region overlaps
        for (int Y = (s_BufferHeight - s_NScrollLines); Y < s_BufferHeight; Y++) {
            s_LineTouched[Y] = s_BufferWidth - 1;
        }
    }

    text_BeginRender();

    // Loop through buffer and start spitting out strings to hardware
    // renderer
    if (1) {
        xcolor Color = text_GetColor();

        for (int Y = 0; Y < s_BufferHeight; Y++) {
            if (s_LineTouched[Y] >= 0) {
                int X = 0;
                int StartX = -1;
                int ScreenX = 0;
                int ScreenY = 0;
                int Len = 0;
                char* S = s_Buffer + (Y * s_BufferWidth);

                while (X <= s_LineTouched[Y]) {
                    if (S[X] != 0) {
                        if (StartX == -1) {
                            StartX = X;
                            ScreenX = (X * s_CharWidth) + s_XBorderWidth;
                            ScreenY = (Y * s_CharHeight) + s_YBorderWidth;
                            Len = 1;
                        } else {
                            Len++;
                        }
                    } else if (StartX != -1) {
                        // Flush this string
                        text_RenderStr(&S[StartX], Len, Color, ScreenX, ScreenY);
                        StartX = -1;
                    }

                    X++;
                }

                // Flush if str leftover
                if (StartX != -1) {
                    text_RenderStr(&S[StartX], Len, Color, ScreenX, ScreenY);
                }
            }
        }
    }

    // Run through the linked list of pixel strings
    text_str* pTS = s_TextStringFirst;
    while (pTS != 0) {
        // Byte-evidenced view into the original contiguous allocation.
        text_RenderStr(reinterpret_cast<char*>(pTS + 1), pTS->Len, pTS->Color, pTS->X, pTS->Y);
        pTS = pTS->pNext;
    }

    text_EndRender();
}

//=========================================================================

RVA(0x0027d850, 0x31)
void text_PushColor(xcolor C) {
    ASSERT(s_ColorStackIndex < 7);
    s_ColorStackIndex++;
    s_ColorStack[s_ColorStackIndex] = C;
}

//=========================================================================

RVA(0x0027d890, 0x7)
void text_PopColor() {
    ASSERT(s_ColorStackIndex >= 0);
    s_ColorStackIndex--;
}

//=========================================================================

RVA(0x0027d8a0, 0x42)
xcolor text_GetColor() {
    if (s_ColorStackIndex == -1) {
        return xcolor(255, 255, 255);
    }
    return s_ColorStack[s_ColorStackIndex];
}

//=========================================================================

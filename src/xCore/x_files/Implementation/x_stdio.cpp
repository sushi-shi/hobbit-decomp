#include <rva.h>

#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_string.hpp>

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

// PC file-record stride is 0x10c; all three fields are independently used.
struct file_name_entry
{
    X_FILE* fp;
    char FileName[256];
    char Mode[8];
};
DATA(0x003cd428) file_name_entry s_FileNameEntry[16];
DATA(0x003ce4e8) static CRITICAL_SECTION s_IOLock;

// Descriptive name: the PC helper initializes this translation unit's lock.
#pragma auto_inline(off)
RVA(0x0024ec80, 0x11)
static int InitIOLock()
{
    InitializeCriticalSection(&s_IOLock);
    return 1;
}
#pragma auto_inline(on)
DATA(0x003ce500) static int s_IOLockInitialized = InitIOLock();
RVA_DYNINIT(0x0024eca0, 0x5, s_IOLockInitialized)
RVA_DYNINIT(0x0024ecb0, 0xb, s_IOLockInitialized)

class io_scope
{
public:
    io_scope()
    {
        if (!s_IOLockInitialized)
            s_IOLockInitialized = InitIOLock();
        EnterCriticalSection(&s_IOLock);
    }
    ~io_scope() { LeaveCriticalSection(&s_IOLock); }
};

DATA(0x003ce504) static open_fn* s_pOpen;
DATA(0x003ce508) static close_fn* s_pClose;
DATA(0x003ce50c) static read_fn* s_pRead;
DATA(0x003ce510) static write_fn* s_pWrite;
DATA(0x003ce514) static seek_fn* s_pSeek;
DATA(0x003ce518) static tell_fn* s_pTell;
DATA(0x003ce51c) static flush_fn* s_pFlush;
DATA(0x003ce520) static eof_fn* s_pEOF;
DATA(0x003ce524) static length_fn* s_pLength;
DATA(0x003ce528) static print_fn* s_pPrint;
DATA(0x003ce52c) static print_at_fn* s_pPrintAt;

RVA(0x0024ecc0, 0xbc)
void AddFileNameEntry(X_FILE* file, const char* name, const char* mode)
{
    io_scope scope;
    if (file)
    {
        int i;
        for (i = 0; i < 16; ++i)
            if (!s_FileNameEntry[i].fp)
                break;
        if (i < 16)
        {
            s_FileNameEntry[i].fp = file;
            x_strncpy(s_FileNameEntry[i].FileName, name, 256);
            x_strncpy(s_FileNameEntry[i].Mode, mode, 8);
        }
    }
}

RVA(0x0024ed80, 0x79)
void DelFileNameEntry(X_FILE* file)
{
    io_scope scope;
    if (file)
        for (int i = 0; i < 16; ++i)
            if (s_FileNameEntry[i].fp == file)
            {
                s_FileNameEntry[i].fp = 0;
                s_FileNameEntry[i].FileName[0] = 0;
                s_FileNameEntry[i].Mode[0] = 0;
                break;
            }
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024ee00, 0x6a)
const file_name_entry* FindFileNameEntry(X_FILE* file)
{
    io_scope scope;
    if (file)
        for (int i = 0; i < 16; ++i)
            if (s_FileNameEntry[i].fp == file)
                return &s_FileNameEntry[i];
    return 0;
}

RVA(0x0024ee70, 0x81)
void x_SetFileIOHooks(open_fn* Open, close_fn* Close, read_fn* Read, write_fn* Write, seek_fn* Seek, tell_fn* Tell, flush_fn* Flush, eof_fn* EndOfFile, length_fn* Length)
{
    io_scope scope;
    s_pOpen = Open;
    s_pClose = Close;
    s_pRead = Read;
    s_pWrite = Write;
    s_pSeek = Seek;
    s_pTell = Tell;
    s_pFlush = Flush;
    s_pEOF = EndOfFile;
    s_pLength = Length;
}

RVA(0x0024ef00, 0x93)
void x_GetFileIOHooks(open_fn*& Open, close_fn*& Close, read_fn*& Read, write_fn*& Write, seek_fn*& Seek, tell_fn*& Tell, flush_fn*& Flush, eof_fn*& EndOfFile, length_fn*& Length)
{
    io_scope scope;
    Open = s_pOpen;
    Close = s_pClose;
    Read = s_pRead;
    Write = s_pWrite;
    Seek = s_pSeek;
    Tell = s_pTell;
    Flush = s_pFlush;
    EndOfFile = s_pEOF;
    Length = s_pLength;
}

RVA(0x0024efa0, 0x33)
void x_SetPrintHook(print_fn* hook)
{
    io_scope scope;
    s_pPrint = hook;
}

RVA(0x0024efe0, 0x33)
void x_SetPrintAtHook(print_at_fn* hook)
{
    io_scope scope;
    s_pPrintAt = hook;
}

RVA(0x0024f020, 0xa8)
int x_printf(const char* format, ...)
{
    io_scope scope;
    va_list Args;
    va_start(Args, format);
    xvfs XVFS(format, Args);
    int NChars = ((const int*)(const char*)XVFS)[-1];
    if (s_pPrint)
        s_pPrint(XVFS);
    va_end(Args);
    return NChars;
}

RVA(0x0024f0d0, 0xb2)
int x_printfxy(int x, int y, const char* format, ...)
{
    io_scope scope;
    va_list Args;
    va_start(Args, format);
    xvfs XVFS(format, Args);
    int NChars = ((const int*)(const char*)XVFS)[-1];
    if (s_pPrintAt)
        s_pPrintAt(XVFS, x, y);
    va_end(Args);
    return NChars;
}

RVA(0x0024f190, 0x72)
int x_sprintf(char* buffer, const char* format, ...)
{
    io_scope scope;
    va_list Args;
    va_start(Args, format);
    return x_vsprintf(buffer, format, Args);
}

RVA(0x0024f210, 0x9e)
X_FILE* x_fopen(const char* name, const char* mode)
{
    io_scope scope;
    xcontext __profile__("x_fopen");
    X_FILE* file = s_pOpen(name, mode);
    AddFileNameEntry(file, name, mode);
    return file;
}

RVA(0x0024f2b0, 0x99)
void x_fclose(X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_fclose");
    if (file)
    {
        x_fflush(file);
        DelFileNameEntry(file);
        s_pClose(file);
    }
}

RVA(0x0024f350, 0xcf)
int x_fread(void* buffer, int size, int count, X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_fread");
    if (size * count == 0)
        return 0;
    int Bytes = s_pRead(file, (unsigned char*)buffer, size * count);
    return Bytes / size;
}

RVA(0x0024f420, 0xcf)
int x_fwrite(const void* buffer, int size, int count, X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_fwrite");
    if (size * count == 0)
        return 0;
    int Bytes = s_pWrite(file, (const unsigned char*)buffer, size * count);
    return Bytes / size;
}

RVA(0x0024f4f0, 0xc6)
int x_fprintf(X_FILE* file, const char* format, ...)
{
    io_scope scope;
    xcontext __profile__("x_fprintf");
    int NChars;
    int Bytes;
    va_list Args;
    va_start(Args, format);
    xstring String;
    String.FormatV(format, Args);
    va_end(Args);
    NChars = String.GetLength();
    Bytes = s_pWrite(file, (const unsigned char*)(const char*)String, NChars);
    return Bytes;
}

RVA(0x0024f5c0, 0x8d)
int x_fflush(X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_fflush");
    return s_pFlush(file);
}

RVA(0x0024f650, 0x97)
int x_fseek(X_FILE* file, int offset, int origin)
{
    io_scope scope;
    xcontext __profile__("x_fseek");
    return s_pSeek(file, offset, origin);
}

RVA(0x0024f6f0, 0x8d)
int x_ftell(X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_ftell");
    return s_pTell(file);
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024f780, 0x8d)
int x_feof(X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_feof");
    return s_pEOF(file);
}

RVA(0x0024f810, 0xa1)
int x_fgetc(X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_fgetc");
    char Char;
    int Result;
    int Bytes = x_fread(&Char, 1, 1, file);
    if (Bytes == 0)
        Result = -1;
    else
        Result = Char;
    return Result;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024f8c0, 0xa9)
int x_fputc(int character, X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_fputc");
    char Char = (char)character;
    int Result;
    int Bytes = x_fwrite(&Char, 1, 1, file);
    if (Bytes == 0)
        Result = -1;
    else
        Result = Char;
    return Result;
}

RVA(0x0024f970, 0x8d)
int x_flength(X_FILE* file)
{
    io_scope scope;
    xcontext __profile__("x_flength");
    return s_pLength(file);
}

RVA(0x0024fa00, 0x64)
static X_FILE* ansi_Open(const char* name, const char* mode)
{
    io_scope scope;
    char directory[256];
    GetCurrentDirectory(255, directory);
    return (X_FILE*)fopen(name, mode);
}

RVA(0x0024fa70, 0x37)
static void ansi_Close(X_FILE* file)
{
    io_scope scope;
    fclose((FILE*)file);
}

RVA(0x0024fab0, 0x49)
static int ansi_Read(X_FILE* file, unsigned char* buffer, int count)
{
    io_scope scope;
    return fread(buffer, 1, count, (FILE*)file);
}

RVA(0x0024fb00, 0x49)
static int ansi_Write(X_FILE* file, const unsigned char* buffer, int count)
{
    io_scope scope;
    return fwrite(buffer, 1, count, (FILE*)file);
}

RVA(0x0024fb50, 0x47)
static int ansi_Seek(X_FILE* file, int offset, int origin)
{
    io_scope scope;
    return fseek((FILE*)file, offset, origin);
}

RVA(0x0024fba0, 0x3d)
static int ansi_Tell(X_FILE* file)
{
    io_scope scope;
    return ftell((FILE*)file);
}

RVA(0x0024fbe0, 0x3d)
static int ansi_Flush(X_FILE* file)
{
    io_scope scope;
    return fflush((FILE*)file);
}

RVA(0x0024fc20, 0x3b)
static int ansi_EOF(X_FILE* file)
{
    io_scope scope;
    return !!feof((FILE*)file);
}

RVA(0x0024fc60, 0x5c)
static int ansi_Length(X_FILE* file)
{
    io_scope scope;
    int position = ftell((FILE*)file);
    fseek((FILE*)file, 0, SEEK_END);
    int NChars = ftell((FILE*)file);
    fseek((FILE*)file, position, SEEK_SET);
    return NChars;
}

RVA(0x0024fcc0, 0x3c)
static void ansi_Print(const char* text)
{
    io_scope scope;
    printf("%s", text);
}

RVA(0x0024fd00, 0x9b)
static void ansi_PrintAt(const char* text, int x, int y)
{
    io_scope scope;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE)
        return;
    COORD position;
    position.X = (short)x;
    position.Y = (short)y;
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(output, &info);
    SetConsoleCursorPosition(output, position);
    printf("%s", text);
    SetConsoleCursorPosition(output, info.dwCursorPosition);
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024fda0, 0x53)
int x_GetFileTime(const char* name, unsigned __int64& time)
{
    WIN32_FIND_DATA data;
    HANDLE search = FindFirstFile(name, &data);
    if (search == INVALID_HANDLE_VALUE)
        return 0;
    x_memcpy(&time, &data.ftLastWriteTime, 8);
    FindClose(search);
    return 1;
}

RVA(0x0024fe00, 0x62)
void x_IOInit()
{
    x_SetFileIOHooks(ansi_Open, ansi_Close, ansi_Read, ansi_Write, ansi_Seek,
                     ansi_Tell, ansi_Flush, ansi_EOF, ansi_Length);
    x_SetPrintHook(ansi_Print);
    x_SetPrintAtHook(ansi_PrintAt);
    for (int i = 0; i < 16; ++i)
    {
        s_FileNameEntry[i].fp = 0;
        s_FileNameEntry[i].FileName[0] = 0;
    }
}

RVA(0x0024fe70, 0x29)
void x_IOKill()
{
    x_SetFileIOHooks(0, 0, 0, 0, 0, 0, 0, 0, 0);
    x_SetPrintHook(0);
    x_SetPrintAtHook(0);
}

// Kept destructor emitted for exception cleanup of the inline lock scope.
RVA_COMPGEN(0x0024fea0, 0xc, ??1io_scope@@QAE@XZ)

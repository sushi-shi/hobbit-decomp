#include <rva.h>

#include <xCore/x_files/Implementation/x_files_private.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_log.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_string.hpp>

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

static int s_DefaultRTFHandler(const char* file, int line, const char* expression, const char* message);
static void s_DefaultLogHandler(const char* message);

DATA(0x0034ccb0) static rtf_fn* s_pRTFHandler = s_DefaultRTFHandler;
DATA(0x0034ccb4) static log_fn* s_pLogHandler = s_DefaultLogHandler;
// The 128-byte object stores the original PC build's date and time, with no
// inserted space between the two compiler-predefined strings.
DATA(0x0034ccb8) static char s_DebugVersion[128] = "Oct 10 200311:53:13";
// Retained initialized flag: the sibling debug implementation names this
// SendTimeStamp. The PC leaves its initial value unused in the message paths.
DATA(0x0034cd38) static int SendTimeStamp = 1;
DATA(0x003cd010) static char s_ErrorBuffer[1024];
DATA(0x003cd414) static int s_Asserting;
DATA(0x003cd418) static int s_DebugSuppressed;
DATA(0x003cd41c) static int s_iErrorBuffer;
DATA(0x003cd420) static int s_iErrorLast;

// Descriptive recovered name; the PC body sets the debug-output guard.
RVA(0x0024e5d0, 0xa)
void x_DebugSetSuppressed(int suppressed)
{
    s_DebugSuppressed = suppressed;
}

RVA(0x0024e5e0, 0x22)
int xExceptionThrowHandler(const char* file, int line, const char* message, int concatenate)
{
    return xExceptionThrowHandler(file, line, message, concatenate, 0x0badbeeb);
}

// PC catch/display helper takes no arguments; unlike the later sibling API
// it directly displays the accumulated buffer without a skip-dialog flag.
RVA(0x0024e610, 0x15)
void xExceptionCatchHandler()
{
    MessageBox(0, s_ErrorBuffer, "ERROR", MB_OK | MB_ICONWARNING);
}

RVA(0x0024e630, 0x1a0)
int xExceptionThrowHandler(const char* file, int line, const char* message, int concatenate, int code)
{
    if (!concatenate)
    {
        s_iErrorLast = 0;
        s_iErrorBuffer = 0;
        s_ErrorBuffer[0] = 0;
    }
    else
    {
        s_iErrorLast = s_iErrorBuffer;
    }
    if (concatenate)
    {
        DATA(0x0034cd44) static const char* pDivider =
            "=============================================================\n";
        int length = x_strlen(pDivider);
        if (s_iErrorBuffer + length + 8 < 1024)
        {
            x_strcpy(&s_ErrorBuffer[s_iErrorBuffer], pDivider);
            s_iErrorBuffer += length;
        }
    }
    if (file)
    {
        int length = x_strlen(file);
        if (s_iErrorBuffer + length + 8 < 1024)
            s_iErrorBuffer += x_sprintf(&s_ErrorBuffer[s_iErrorBuffer], "%s", file);
    }
    if (s_iErrorBuffer + 32 < 1024)
        s_iErrorBuffer += x_sprintf(&s_ErrorBuffer[s_iErrorBuffer], "(%d)", line);
    if (s_iErrorBuffer + 32 < 1024 && code != 0x0badbeeb)
        s_iErrorBuffer += x_sprintf(&s_ErrorBuffer[s_iErrorBuffer], ":\nerror C%d: ", code);
    else
        s_iErrorBuffer += x_sprintf(&s_ErrorBuffer[s_iErrorBuffer], ":\n");
    if (message)
    {
        int length = x_strlen(message);
        if (s_iErrorBuffer + length + 8 < 1024)
            s_iErrorBuffer += x_sprintf(&s_ErrorBuffer[s_iErrorBuffer], "%s\n", message);
    }
    x_DebugMsg("*** EXCEPTION ***\n%s\n", &s_ErrorBuffer[s_iErrorLast]);
    xExceptionCatchHandler();
    return 1;
}

RVA(0x0024e7d0, 0x9b)
void x_DebugMsg(const char* format, ...)
{
    if (s_DebugSuppressed)
        return;
    va_list args;
    va_start(args, format);
    xvfs message(format, args);
    xfs output("%s", (const char*)message);
    OutputDebugString(output);
    va_end(args);
}

RVA(0x0024e870, 0x81)
void x_DebugMsg(int channel, const char* format, ...)
{
    if (s_DebugSuppressed)
        return;
    va_list args;
    va_start(args, format);
    xvfs message(format, args);
    xfs output("%s", (const char*)message);
    va_end(args);
}

RVA(0x0024e900, 0x7a)
void x_DebugLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    xvfs message(format, args);
    if (s_pRTFHandler)
        s_pLogHandler(message);
    else
        s_DefaultLogHandler(message);
    va_end(args);
}

RVA(0x0024e980, 0x18)
void x_DebugSetVersionString(const char* version)
{
    x_strncpy(s_DebugVersion, version, sizeof(s_DebugVersion));
}

RVA(0x0024e9a0, 0x6)
const char* x_DebugGetVersionString()
{
    return s_DebugVersion;
}

RVA(0x0024e9b0, 0x63)
int RTFHandler(const char* file, int line, const char* expression, const char* message)
{
    DATA(0x003cd424) static int s_ReportingFailure = 0;
    if (!s_ReportingFailure)
    {
        s_ReportingFailure = 1;
        // This call reaches the shared retained no-op at PC RVA25890.
        // The source name is inferred; folding may share other empty helpers.
        log_NULL(message ? message : expression, file, line);
    }
    if (!s_pRTFHandler)
    {
        s_ReportingFailure = 0;
        return 1;
    }
    else
    {
        s_ReportingFailure = 0;
        return s_pRTFHandler(file, line, expression, message);
    }
}

RVA(0x0024ea20, 0x18)
void x_SetRTFHandler(rtf_fn* handler)
{
    if (!handler)
        s_pRTFHandler = s_DefaultRTFHandler;
    else
        s_pRTFHandler = handler;
}

RVA(0x0024ea40, 0x6)
rtf_fn* x_GetRTFHandler()
{
    return s_pRTFHandler;
}

RVA(0x0024ea50, 0x18)
void x_SetLogHandler(log_fn* handler)
{
    if (!handler)
        s_pLogHandler = s_DefaultLogHandler;
    else
        s_pLogHandler = handler;
}

RVA(0x0024ea70, 0x51)
static void s_DefaultLogHandler(const char* message)
{
    X_FILE* file = x_fopen("debuglog.txt", "wat");
    if (file)
    {
        x_fwrite(message, x_strlen(message), 1, file);
        x_fclose(file);
    }
    else
        x_DebugMsg("Cannot open Log: %s\n", message);
}

static inline void DebugPrintString(const char* message)
{
    OutputDebugString(message);
    printf(message);
}

RVA(0x0024ead0, 0x195)
static int s_DefaultRTFHandler(const char* file, int line, const char* expression, const char* message)
{
    if (!x_GetInitialized())
    {
        DebugPrintString("***\n*** ERROR: x_files not initialized.\n***\n");
        __asm int 3
    }
    if (s_Asserting)
    {
        __asm int 3
    }
    s_Asserting = 1;
    DebugPrintString("***\n*** RUNTIME FAILURE\n");
    if (file)
        DebugPrintString(xfs("*** File: %s on line %d\n", file, line));
    else
        DebugPrintString(xfs("*** File: <unknown> on line %d\n", line));
    if (expression)
        DebugPrintString(xfs("*** Expr: %s\n", expression));
    if (message)
        DebugPrintString(xfs("*** Msg : %s\n", message));
    DebugPrintString("***\n");
    s_Asserting = 0;
    return 1;
}

RVA(0x0024ec70, 0x6)
const char* x_DebugGetCallStackString()
{
    return "Not Implemented";
}

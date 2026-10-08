#include <rva.h>

#include <xCore/x_files/x_stdio.hpp>

#include <stdio.h>

// PC is a tail jump to the CRT formatter. The Xbox signature
// PADPBD0 confirms that the argument cursor has the same char* type as buffer.
RVA(0x002554e0, 0x5)
int x_vsprintf(char* buffer, const char* format, char* arguments)
{
    return vsprintf(buffer, format, arguments);
}

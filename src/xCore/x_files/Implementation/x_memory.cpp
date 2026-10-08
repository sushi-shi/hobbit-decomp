#include <rva.h>

#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_log.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_threads.hpp>

#include <stdlib.h>

// This provider defines the internal release entry point used by x_free_fn.
// Consumers retain the public filename/line macro from x_memory.hpp.
#undef x_free

// These internal diagnostic names are descriptive; their original public
// linkage/name is not established. See docs/x-memory-reconstruction.md.
void DumpMemoryFragments(X_FILE* file, int bytes);
void DumpMemoryFragments(int bytes);

// Owner-stack extent follows the sibling bound and the adjacent PC globals.
DATA(0x003d2530) static int OwnerStack[16384];
DATA(0x003e2534) static int OwnerStackSize;
// Descriptive data name: read by x_MemGetAllocated; original spelling unknown.
DATA(0x003e2548) static int s_AllocatedBytes;

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024fec0, 0x23)
void FillFreedMemory(void* memory, unsigned int bytes)
{
    unsigned int* cursor = static_cast<unsigned int*>(memory);
    unsigned int* end = cursor + (bytes >> 2);
    while (cursor < end)
        *cursor++ = 0xdeadbeef;
}

RVA(0x0024fef0, 0x1)
void x_MemInit()
{
}

RVA(0x0024ff00, 0x1)
void x_MemKill()
{
}

// PC's internal release operation accepts the -1 zero-size sentinel. Its
// name is inferred from the sibling allocation family; debug callers use
// x_free_fn, whose decorated name is published in the Xbox map.
RVA(0x0024ff10, 0x24)
void x_free(void* memory)
{
    if (memory && memory != reinterpret_cast<void*>(-1))
    {
        x_BeginAtomic();
        free(memory);
        x_EndAtomic();
    }
}

RVA(0x0024ff40, 0x27)
void x_free_fn(void* memory, const char* file, int line)
{
    x_free(memory);
    if (memory)
        log_NULL(memory, file, line);
}

RVA(0x0024ff70, 0x1e)
void* x_malloc_info(int bytes, const char* file, int line)
{
    x_BeginAtomic();
    void* memory = malloc(bytes);
    x_EndAtomic();
    return memory;
}

RVA(0x0024ff90, 0x5)
void* x_malloc_fn(int bytes, const char* file, int line)
{
    return x_malloc_info(bytes, file, line);
}

RVA(0x0024ffa0, 0x56)
void* x_realloc_info(void* memory, int bytes, const char* file, int line)
{
    if (!memory)
        return x_malloc_info(bytes, file, line);
    if (!bytes)
    {
        x_free(memory);
        return reinterpret_cast<void*>(-1);
    }
    x_BeginAtomic();
    void* result = realloc(memory, bytes);
    x_EndAtomic();
    return result;
}

RVA(0x00250000, 0x5)
void* x_realloc_fn(void* memory, int bytes, const char* file, int line)
{
    return x_realloc_info(memory, bytes, file, line);
}

RVA(0x00250010, 0x3)
int x_MemAddMark(const char* comment)
{
    return 0;
}

RVA(0x00250030, 0x6)
int x_MemGetAllocated()
{
    return s_AllocatedBytes;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00250060, 0x10)
void DumpRecentAllocations()
{
    x_DebugMsg(1, "\n*** Last 72 Allocs, in order (last entry in list is most recent request): ***\n");
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00250070, 0x4a)
void DumpAllocationFailureToFile(int bytes)
{
    X_FILE* file = x_fopen("MemoryFragments.txt", "wt");
    x_fprintf(file, "Failed to Allocate %d bytes\n", bytes);
    DumpMemoryFragments(file, (bytes / 32) * 32);
    x_fflush(file);
    x_fclose(file);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x002500c0, 0x2d)
void DumpAllocationFailure(int bytes)
{
    x_DebugMsg(1, "Failed to Allocate %d bytes\n", bytes);
    DumpMemoryFragments((bytes / 32) * 32);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x002500f0, 0xc)
void x_MemDump()
{
    x_MemDump("MemDump.txt");
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00250110, 0x1f)
const char* LimitMemoryFilename(const char* text)
{
    if (text)
    {
        int length = x_strlen(text);
        if (length > 24)
        {
            text += length;
            text -= 24;
        }
    }
    return text;
}

RVA(0x00250130, 0x1)
void x_MemDump(const char* filename)
{
}

RVA(0x00250150, 0x1)
void x_debug_MemSanity(const char* file, int line)
{
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00250170, 0x53)
void x_MemGrabOwnerStack(unsigned char* buffer, int bufferSize)
{
    int stop = bufferSize < OwnerStackSize ? bufferSize : OwnerStackSize;
    int i;
    for (i = 0; i < stop; ++i)
        buffer[i] = static_cast<unsigned char>(OwnerStack[OwnerStackSize - i - 1]);
    for (i = OwnerStackSize; i < bufferSize; ++i)
        buffer[i] = 0;
}

RVA(0x002501d0, 0x5)
x_mem_owner::x_mem_owner(const char* name)
{
}

RVA(0x002501e0, 0x1)
x_mem_owner::~x_mem_owner()
{
}

RVA(0x002501f0, 0x1f)
void x_MemGetFree(int& freeBytes, int& largest, int& fragments)
{
    freeBytes = -1;
    largest = -1;
    fragments = -1;
}

RVA(0x00250210, 0x1f)
int x_MemGetFree()
{
    int freeBytes, largest, fragments;
    x_MemGetFree(freeBytes, largest, fragments);
    return freeBytes;
}

// Surviving sibling PC source explicitly returns zero for this API. The complete
// non-PC original remains in area51-original/x_memory.cpp.inc. No Hobbit RVA
// or equivalence to x_MemGetAllocated is asserted by this unannotated import.
int x_MemGetUsed()
{
    return 0;
}

// Definitions follow the callers so VC6 preserves the observed call boundaries.
// These are real RET-only retail bodies, with argument lists visible at callers.
RVA(0x00250040, 0x1)
void DumpMemoryFragments(X_FILE* file, int bytes)
{
}

RVA(0x00250050, 0x1)
void DumpMemoryFragments(int bytes)
{
}

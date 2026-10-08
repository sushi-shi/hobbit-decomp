// Separate Entropy ownership; see docs/vm-reconstruction.md.

#include <rva.h>

#include <xCore/Entropy/e_Virtual.hpp>
#include <xCore/x_files/x_memory.hpp>

RVA(0x002747f0, 0x15)
void* vm_Alloc(int nBytes) {
    return x_malloc_fn(nBytes, "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_Virtual.cpp", 29);
}

RVA(0x00274810, 0x15)
void vm_Free(void* pAddress) {
    x_free_fn(pAddress, "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_Virtual.cpp", 35);
}

// Generic hooks from Area51 431f72b9 Common/e_Virtual.cpp. Original empty
// implementations, not replacements for unavailable platform VM code.
void vm_Init(int VirtualSize, int PoolSize)
{
    (void)PoolSize;
    (void)VirtualSize;
}
void vm_Kill(void) {}
void vm_DumpList(void) {}
void* vm_GetSwapSpace(void) { return 0; }

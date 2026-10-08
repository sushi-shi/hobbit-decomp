#ifndef HOBBIT_E_VIRTUAL_HPP
#define HOBBIT_E_VIRTUAL_HPP

void* vm_Alloc(int nBytes);
void vm_Free(void* address);

void vm_Init(int VirtualSize, int PoolSize);
void vm_Kill(void);
void vm_DumpList(void);
void* vm_GetSwapSpace(void);

#endif

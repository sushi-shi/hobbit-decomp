#ifndef HOBBIT_E_SCRATCHMEM_HPP
#define HOBBIT_E_SCRATCHMEM_HPP

void smem_Init(int bytes);
void smem_Kill();
void smem_Toggle();
void smem_ChangeSize(int bytes);
int smem_GetMaxUsed();
int smem_GetBufferSize(void);

// Shared names are attested in the Xbox map. PC users and the complete natural
// compiler BSS layout independently establish their PC addresses and extents.
extern unsigned char* s_pScratchMemBufferTop;
extern unsigned char* s_pScratchMemStackTop;
extern unsigned char* s_pScratchMemMarker[16];
extern int s_ScratchMemNextMarker;

// Source-backed unchecked release allocator, verified in text_PrintPixelXY.
// This inline belongs to ScratchMem; it has no separate PC body claim.
inline unsigned char* smem_rfunc_BufferAlloc(int bytes) {
    unsigned char* allocation = s_pScratchMemBufferTop;
    s_pScratchMemBufferTop += (bytes + 15) & ~15;
    return allocation;
}
#define smem_BufferAlloc(bytes) smem_rfunc_BufferAlloc(bytes)

// Genuine sibling release stack operations; currently unlabelled on PC.
unsigned char* smem_rfunc_StackAlloc(int bytes);
void smem_rfunc_StackPushMarker();
void smem_rfunc_StackPopToMarker();
#define smem_StackAlloc(bytes) smem_rfunc_StackAlloc(bytes)
#define smem_StackPushMarker() smem_rfunc_StackPushMarker()
#define smem_StackPopToMarker() smem_rfunc_StackPopToMarker()

#endif

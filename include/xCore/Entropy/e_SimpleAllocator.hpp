#ifndef HOBBIT_E_SIMPLEALLOCATOR_HPP
#define HOBBIT_E_SIMPLEALLOCATOR_HPP

// PC layout: 24-byte nodes, 116-byte allocator. Local-link and sentinel
// member names are reconstructed; the Xbox map attests the class and methods.
class simple_allocator {
public:
    simple_allocator();
    ~simple_allocator();

    void* Alloc(int size);
    int Free(void* base);
    void Validate();
    int IsValid(void* base);
    void Init(void* base, int size);
    void Kill();
    void DumpList();
    void SanityCheck();

private:
    void GrowCapacity();
    void BuildFreeLists();

    enum alloc_state {
        AF_FREE = 0,  // This header block has never been used
        AF_AVAILABLE, // This header *should* be in the memory chain
        AF_ALLOCATED, // This is in the memory chain but also in use
    };

public:
    struct alloc_node {
        alloc_node* m_LocalNext;
        alloc_node* m_LocalPrev;
        alloc_node* m_pNext;
        alloc_state m_Status;
        void* m_Address;
        int m_Length;
    };

private:
    int m_TotalSize;
    void* m_pBase;
    int m_MemFree;
    int m_MemUsed;
    int m_AllocCount;
    int m_FreeCount;
    alloc_node m_RootBlock;
    alloc_node* m_pMemoryBlock;
    alloc_node m_Available;
    alloc_node m_Free;
    int m_nMemoryBlocks;
    int m_nMemoryBlocksAllocated;
    int m_LastBlockUsed;
    int m_Initialized;
};

#endif

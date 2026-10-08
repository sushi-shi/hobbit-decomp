#include <rva.h>

#include <xCore/Entropy/e_SimpleAllocator.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>

// Assertions retain the original expressions but emit no release instructions.
#define ASSERT(x) static_cast<void>(0)
#define ASSERTS(x, y) static_cast<void>(0)

inline void RemoveNodeFromLocalList(simple_allocator::alloc_node* pNode) {
    pNode->m_LocalPrev->m_LocalNext = pNode->m_LocalNext;
    pNode->m_LocalNext->m_LocalPrev = pNode->m_LocalPrev;
}
inline void
AddNodeToLocalList(simple_allocator::alloc_node* pNode, simple_allocator::alloc_node* pRoot) {
    pNode->m_LocalPrev = pRoot->m_LocalPrev;
    pNode->m_LocalNext = pRoot;
    pNode->m_LocalNext->m_LocalPrev = pNode;
    pNode->m_LocalPrev->m_LocalNext = pNode;
}

#define CAPACITY_GROW_SIZE 1024

//==============================================================================
// Heap management control functions. This class is a generic heap management
// class. Each heap can be anywhere in any memory space. All this class deals with
// is a base and a length for the heap. Everything else is assumed to be dealt
// with by the owner application.
//==============================================================================
RVA(0x00280ef0, 0x3)
simple_allocator::simple_allocator() {}

//==============================================================================
RVA(0x00280f00, 0x1)
simple_allocator::~simple_allocator() {
    ASSERT(!m_Initialized);
}

//==============================================================================
RVA(0x00280f10, 0x54)
void simple_allocator::Init(void* base, int size) {
    m_TotalSize = size;
    m_pBase = base;

    m_pMemoryBlock = 0;
    m_nMemoryBlocks = 0;
    m_nMemoryBlocksAllocated = 0;

    m_MemFree = m_TotalSize;
    m_MemUsed = 0;
    m_AllocCount = 0;

    m_Initialized = 1;

    m_FreeCount = 1;
    m_RootBlock.m_Status = AF_AVAILABLE;
    m_RootBlock.m_Length = m_TotalSize;
    m_RootBlock.m_Address = m_pBase;
    m_RootBlock.m_pNext = 0;
    m_Available.m_LocalPrev = m_Available.m_LocalNext = &m_Available;
    m_Free.m_LocalPrev = m_Free.m_LocalNext = &m_Free;
}

//==============================================================================
RVA(0x00280f70, 0x28)
void simple_allocator::Kill() {
    ASSERT(m_Initialized);
    m_Initialized = 0;
    x_free_fn(
        m_pMemoryBlock,
        "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_SimpleAllocator.cpp",
        76
    );
    m_pMemoryBlock = 0;
    m_nMemoryBlocks = 0;
    m_nMemoryBlocksAllocated = 0;
}

//==============================================================================

RVA(0x00280fa0, 0x8f)
void simple_allocator::BuildFreeLists() {
    m_Available.m_LocalPrev = m_Available.m_LocalNext = &m_Available;
    m_Free.m_LocalPrev = m_Free.m_LocalNext = &m_Free;
    alloc_node* pNode = &m_RootBlock;
    switch (pNode->m_Status) {
        case AF_AVAILABLE:
            AddNodeToLocalList(pNode, &m_Available);
            break;
        case AF_FREE:
            AddNodeToLocalList(pNode, &m_Free);
            break;
    }
    for (int i = 0; i < m_nMemoryBlocks; i++) {
        alloc_node* pNode = &m_pMemoryBlock[i];
        if (pNode->m_Status == AF_FREE) {
            AddNodeToLocalList(pNode, &m_Free);
        } else if (pNode->m_Status == AF_AVAILABLE) {
            AddNodeToLocalList(pNode, &m_Available);
        }
    }
}

RVA(0x00281030, 0x102)
void simple_allocator::GrowCapacity() {
    int i;
    // Convert current next ptrs to indices
    for (i = 0; i < m_nMemoryBlocks; i++) {
        if (m_pMemoryBlock[i].m_pNext) {
            // Byte-evidenced 32-bit address/offset arithmetic.
            m_pMemoryBlock[i].m_pNext = reinterpret_cast<alloc_node*>(
                // Byte-evidenced 32-bit address/offset arithmetic.
                reinterpret_cast<unsigned int>(m_pMemoryBlock[i].m_pNext)
                // Byte-evidenced 32-bit address/offset arithmetic.
                - reinterpret_cast<unsigned int>(m_pMemoryBlock)
            );
        } else {
            // Byte-evidenced 32-bit sentinel or address representation.
            m_pMemoryBlock[i].m_pNext = reinterpret_cast<alloc_node*>(0xFFFFFFFF);
        }
    }
    m_RootBlock.m_pNext = (m_RootBlock.m_pNext)
                              // Byte-evidenced 32-bit address/offset arithmetic.
                              ? (reinterpret_cast<alloc_node*>(
                                    // Byte-evidenced 32-bit address/offset arithmetic.
                                    reinterpret_cast<unsigned int>(m_RootBlock.m_pNext)
                                    // Byte-evidenced 32-bit address/offset arithmetic.
                                    - reinterpret_cast<unsigned int>(m_pMemoryBlock)
                                ))
                              // Byte-evidenced 32-bit sentinel or address representation.
                              : (reinterpret_cast<alloc_node*>(0xFFFFFFFF));

    m_nMemoryBlocksAllocated += CAPACITY_GROW_SIZE;
    m_pMemoryBlock = static_cast<alloc_node*>(x_realloc_fn(
        m_pMemoryBlock,
        sizeof(alloc_node) * m_nMemoryBlocksAllocated,
        "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_SimpleAllocator.cpp",
        148
    ));
    ASSERT(m_pMemoryBlock);

    // Convert next ptrs back into ptrs
    for (i = 0; i < m_nMemoryBlocks; i++) {
        // Byte-evidenced 32-bit sentinel or address representation.
        if (m_pMemoryBlock[i].m_pNext == reinterpret_cast<alloc_node*>(0xFFFFFFFF)) {
            m_pMemoryBlock[i].m_pNext = 0;
        } else {
            // Byte-evidenced 32-bit address/offset arithmetic.
            m_pMemoryBlock[i].m_pNext = reinterpret_cast<alloc_node*>(
                // Byte-evidenced 32-bit address/offset arithmetic.
                reinterpret_cast<unsigned int>(m_pMemoryBlock)
                // Byte-evidenced 32-bit address/offset arithmetic.
                + reinterpret_cast<unsigned int>(m_pMemoryBlock[i].m_pNext)
            );
        }
    }
    // Byte-evidenced 32-bit sentinel or address representation.
    m_RootBlock.m_pNext = (reinterpret_cast<unsigned int>(m_RootBlock.m_pNext) == 0xFFFFFFFF)
                              ? (0)
                              // Byte-evidenced 32-bit address/offset arithmetic.
                              : (reinterpret_cast<alloc_node*>(
                                    // Byte-evidenced 32-bit address/offset arithmetic.
                                    reinterpret_cast<unsigned int>(m_pMemoryBlock)
                                    // Byte-evidenced 32-bit address/offset arithmetic.
                                    + reinterpret_cast<unsigned int>(m_RootBlock.m_pNext)
                                ));

    // Clear new blocks
    for (i = m_nMemoryBlocks; i < m_nMemoryBlocksAllocated; i++) {
        m_pMemoryBlock[i].m_Status = AF_FREE;
        m_pMemoryBlock[i].m_pNext = 0;
        m_pMemoryBlock[i].m_Address = 0;
        m_pMemoryBlock[i].m_Length = 0;
    }

    m_LastBlockUsed = m_nMemoryBlocks;
    m_nMemoryBlocks = m_nMemoryBlocksAllocated;
    BuildFreeLists();
}

//==============================================================================

RVA(0x00281140, 0xed)
void* simple_allocator::Alloc(int size) {
    // Check if doing a zero-size allocation
    if (size == 0) {
        // Byte-evidenced 32-bit sentinel or address representation.
        return reinterpret_cast<void*>(0xFFFFFFFF);
    }

    // Be sure enough headers have been allocated.  We are doing it here so systems
    // can initialize the simple allocator before malloc is available.
    if ((m_AllocCount + m_FreeCount + 1) >= m_nMemoryBlocksAllocated) {
        GrowCapacity();
    }

    // Go through the heap and find the smallest block that
    // will fit what we're looking for
    alloc_node* pHeader;
    alloc_node* pBlockToUse;
    int LastSize, i;

    //
    // This can get very slow
    //
    size = (size + 3) & ~3;
    ASSERT(m_Initialized);
    pHeader = m_Available.m_LocalNext;
    LastSize = 1 << 30;
    pBlockToUse = 0;
    while (pHeader != &m_Available) {
        if ((pHeader->m_Length >= size) && (pHeader->m_Length < LastSize)) {
            LastSize = pHeader->m_Length;
            pBlockToUse = pHeader;
        }
        pHeader = pHeader->m_LocalNext;
    }

    // No available blocks to fit the size we're looking for, just
    // fail!
    if (!pBlockToUse) {
        return 0;
    }

    ASSERT(
        (pBlockToUse == &m_RootBlock)
        || ((pBlockToUse >= m_pMemoryBlock)
            && (pBlockToUse < (m_pMemoryBlock + m_nMemoryBlocksAllocated)))
    );

    RemoveNodeFromLocalList(pBlockToUse);
    pBlockToUse->m_Status = AF_ALLOCATED;
    m_AllocCount++;

    // If we didn't consume the entire block, we create a new header
    // which has the remainder of what is available within it
    if (pBlockToUse->m_Length != size) {
        // Now we grab any available header out of the header pool
        pHeader = m_Free.m_LocalNext;
        RemoveNodeFromLocalList(pHeader);

        ASSERTS(pHeader, "Ran out of memory blocks for simple allocator");

        //
        // Allocate this block and add it in to the used list
        //

        m_FreeCount++;

        pHeader->m_Status = AF_AVAILABLE;
        pHeader->m_Address =
            // Byte-evidenced 32-bit address/offset arithmetic.
            reinterpret_cast<void*>(reinterpret_cast<unsigned int>(pBlockToUse->m_Address) + size);
        pHeader->m_Length = pBlockToUse->m_Length - size;
        pHeader->m_pNext = pBlockToUse->m_pNext;

        AddNodeToLocalList(pHeader, &m_Available);

        pBlockToUse->m_Length = size;
        pBlockToUse->m_pNext = pHeader;
    }
    m_MemFree -= pBlockToUse->m_Length;
    m_MemUsed += pBlockToUse->m_Length;

    return (pBlockToUse->m_Address);
}

//==============================================================================
RVA(0x00281230, 0x2f)
int simple_allocator::IsValid(void* base) {
    alloc_node* pHeader;
    pHeader = &m_RootBlock;
    while (pHeader) {
        if (pHeader->m_Address == base) {
            break;
        }
        pHeader = pHeader->m_pNext;
    }

    return (pHeader && (pHeader->m_Status == AF_ALLOCATED));
}

//==============================================================================
RVA(0x00281260, 0x1)
void simple_allocator::Validate() {
    // In debug mode, this function will walk the memory allocation list and make
    // sure that it's state is consistent. Things that will be checked:
    // 1. No memory block overruns (only if memory is physical and accessable)
    // 2. Memory blocks have been coalesced where they could
    // 3. There are no holes
    // 4. All memory headers point to valid locations of "memory"
}

//==============================================================================
// Complete source candidate; currently 253 bytes versus the 259-byte PC body.
// The register-allocation discrepancy remains unresolved; see reconstruction evidence.
RVA(0x00281270, 0x103)
int simple_allocator::Free(void* base) {
    // Check if freeing a zero-size allocation
    // Byte-evidenced 32-bit sentinel or address representation.
    if (reinterpret_cast<unsigned int>(base) == 0xFFFFFFFF) {
        return 0;
    }

    // Check if freeing a null pointer
    if (base == 0) {
        return 0;
    }

    alloc_node* pHeader;
    alloc_node* pPrev;
    alloc_node* pNext;
    int Length;

    ASSERT(m_Initialized);
    // First, find the block that this memory is in
    pPrev = 0;

    pHeader = &m_RootBlock;
    while (pHeader) {
        if (pHeader->m_Address == base) {
            break;
        }
        pPrev = pHeader;
        pHeader = pHeader->m_pNext;
    }
    ASSERTS(pHeader, "The memory block freed was not valid");
    ASSERTS(pHeader->m_Status == AF_ALLOCATED, "The memory block has already been freed");

    pHeader->m_Status = AF_AVAILABLE;
    pNext = pHeader->m_pNext;

    Length = pHeader->m_Length;

    if (pNext) {
        // Verify that we should, indeed, be coalescing properly
        ASSERT(pHeader->m_pNext == pNext);
        ASSERT(
            // Faithful original no-op assertion uses 32-bit address arithmetic.
            pHeader->m_Length + reinterpret_cast<int>(pHeader->m_Address)
            // Faithful original no-op assertion uses 32-bit address arithmetic.
            == reinterpret_cast<int>(pNext->m_Address)
        );

        // Can we coalesce this with the following memory block?
        if (pNext->m_Status == AF_AVAILABLE) {
            m_FreeCount--;

            pHeader->m_Length += pNext->m_Length;
            pHeader->m_pNext = pNext->m_pNext;
            pNext->m_Status = AF_FREE;
            RemoveNodeFromLocalList(pNext);
            AddNodeToLocalList(pNext, &m_Free);
        }
    }

    if (pPrev) {
        // Verify that we should, indeed, be coalescing properly
        ASSERT(pPrev->m_pNext == pHeader);
        ASSERT(
            // Faithful original no-op assertion uses 32-bit address arithmetic.
            pPrev->m_Length + reinterpret_cast<int>(pPrev->m_Address)
            // Faithful original no-op assertion uses 32-bit address arithmetic.
            == reinterpret_cast<int>(pHeader->m_Address)
        );
        // Can we coalesce the previous memory block with the
        // current?
        if (pPrev->m_Status == AF_AVAILABLE) {
            m_FreeCount--;
            pPrev->m_Length += pHeader->m_Length;
            pPrev->m_pNext = pHeader->m_pNext;
            pHeader->m_Status = AF_FREE;
            RemoveNodeFromLocalList(pPrev);
            AddNodeToLocalList(pHeader, &m_Free);
            pHeader = pPrev;
        }
    }

    AddNodeToLocalList(pHeader, &m_Available);

    m_AllocCount--;
    m_MemFree += Length;
    m_MemUsed -= Length;

    return Length;
}

//==============================================================================

RVA(0x00281380, 0x34)
void simple_allocator::DumpList() {
    alloc_node* pHeader;

    DATA(0x00353c10)
    static char* s_Status[] = {
        "FREE",
        "AVAILABLE",
        "ALLOCATED",
    };

    pHeader = &m_RootBlock;
    while (pHeader) {
        x_DebugMsg(
            "START: 0x%08x  END: 0x%08x  LENGTH: %8d  STATUS: %s\n",
            pHeader->m_Address,
            // Byte-evidenced 32-bit address/offset arithmetic.
            reinterpret_cast<int>(pHeader->m_Address) + pHeader->m_Length,
            pHeader->m_Length,
            s_Status[pHeader->m_Status]
        );
        pHeader = pHeader->m_pNext;
    }
}

//==============================================================================

RVA(0x002813c0, 0x35)
void simple_allocator::SanityCheck() {
    alloc_node* pSlow = &m_Free;
    alloc_node* pFast = pSlow->m_LocalNext;
    while (1) {
        pSlow = pSlow->m_LocalNext;
        if (pSlow == &m_Free) {
            break;
        }
        pFast = pFast->m_LocalNext;
        if (pFast == &m_Free) {
            break;
        }
        pFast = pFast->m_LocalNext;
        if (pFast == &m_Free) {
            break;
        }
        ASSERT(pSlow != pFast);
    }
    pSlow = &m_Available;
    pFast = pSlow->m_LocalNext;
    while (1) {
        pSlow = pSlow->m_LocalNext;
        if (pSlow == &m_Available) {
            break;
        }
        pFast = pFast->m_LocalNext;
        if (pFast == &m_Available) {
            break;
        }
        pFast = pFast->m_LocalNext;
        if (pFast == &m_Available) {
            break;
        }
        ASSERT(pSlow != pFast);
    }
}

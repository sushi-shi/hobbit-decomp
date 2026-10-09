//==============================================================================
//
//  FibHeapAC.hpp
//
//==============================================================================

#ifndef FIBHEAPAC_HPP
#define FIBHEAPAC_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <xCore/x_files/x_files.hpp>

//==============================================================================
//  CONSTANTS
//==============================================================================

#define MAX_DEGREE 10
#define MAX_FIB_NODES  64
#ifdef acheng
#define RECORD_STATS 0
#else
#define RECORD_STATS 0
#endif
//==============================================================================
//  TYPES
//==============================================================================

// Fibonacci Heap Node
class fnode
{
    friend class fheap;

    public:
        fnode() 
        { 
            data = degree = 0;
            left = right = parent = child = -1;
            pNode = NULL;
        }

        s32 GetData() {return data;}

        void Set(s32 x, s32 deg = 0, s32 p = -1, s32 c = -1)
        {
            data = x; degree = deg; parent = p; child = c;
        }

    protected:
        // Heuristic value to compare nodes with.
        s32 data;

        // Address of node
        void *pNode;

        s32 degree;
        s32 left;
        s32 right;
        s32 parent;
        s32 child;
};

class fheap
{

    public:
                fheap                   ( void );
               ~fheap                   ( void );

        // Reset the heap
        void    Reset                   ( void );

        // Add a node to the heap.
        xbool   Insert                  ( void *pNode, s32 Cost );

        // Retrieve the minimum node from the heap.
        void*   ExtractMinPtr           ( void );

        xbool   IsEmpty                 ( void );
#if RECORD_STATS
        s32     CurrMaxSize;
#endif

    protected:
        void    Insert                  ( s32 newNode );
        void    Union                   ( s32 iRoot );
        s32     Minimum                 ( void );
        s32     ExtractMin              ( void );
        s32     GetMinSib               ( s32 node );
        xbool   Delete                  ( s32 delNode );
        void    ApplyDegreeRule         ( void );
        s32     ResolveConflict         ( s32 A, s32 B );

        // Memory Pool Stack operations
        // Retrieve the next open spot from within the memory pool.
        s32     Pop                     ( void );

        // Add a new open spot to the open stack.
        void    Push                    ( s32 Slot );

        // Empty out the contents of a node.  (you already got your data, i hope)
        void    ClearNode               ( s32 iNode );

    protected:
        s32     iMinRoot;
        s32     DegArray[MAX_DEGREE];

        // Memory Pool
        fnode   m_FibNodeList[MAX_FIB_NODES];

        // Memory Pool Stack
        s32     m_iStackTop;
        s32     m_OpenStack[MAX_FIB_NODES];


};

//==============================================================================
            
#endif // FIBHEAPAC_HPP
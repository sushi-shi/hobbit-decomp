#ifndef _FIBHEAP_H
#define _FIBHEAP_H

#include "CellRef.hpp"

//***************************************************************************
// The Fibonacci heap implementation contained in FIBHEAP.H and FIBHEAP.CPP
// is Copyright (c) 1996 by John Boyer
//
// Obtained from:
// (article) http://www.ddj.com/articles/1997/9701/9701o/9701o.htm?topic=algorithms
// and
// (source) http://www.ddj.com/ftp/1997/1997.01/
//
// Once this Fibonacci heap implementation (the software) has been published
// by Dr. Dobb's Journal, permission to use and distribute the software is
// granted provided that this copyright notice remains in the source and
// and the author (John Boyer) is acknowledged in works that use this program.
//
// Every effort has been made to ensure that this implementation is free of
// errors.  Nonetheless, the author (John Boyer) assumes no liability regarding
// your use of this software.
//
// The author would also be very glad to hear from anyone who uses the
// software or has any feedback about it.
// Email: jboyer@gulf.csc.uvic.ca
//***************************************************************************

#define OK      0
#define NOTOK   -1
#define TESTING_FIB_HEAP 0

//======================================================
// Fibonacci Heap Node Class
//======================================================

class FibHeap;

class FibHeapNode
{
    friend class FibHeap;

    FibHeapNode *Left, *Right, *Parent, *Child;
    s16 Degree, Mark, NegInfinityFlag;

protected:

    inline s32  FHN_Cmp(FibHeapNode& RHS);
    inline void FHN_Assign(FibHeapNode& RHS);

public:
    //----------------------------------------------------------------------
    // added for PathFinder
    //----------------------------------------------------------------------
    cell_ref cr; // reference to the cell represented by this node
    f32 g;  // the cost of the currently best path to cr
    f32 h;  // the minimum cost estimate of the path form cr to the destination

    FibHeapNode();
    virtual ~FibHeapNode();

    virtual void operator =(FibHeapNode& RHS);
    virtual s32  operator ==(FibHeapNode& RHS);
    virtual s32  operator <(FibHeapNode& RHS);
    virtual void Print();
    virtual void Draw( s32 VertexSpacing ) const;
};

typedef FibHeapNode* PFibHeapNode;

//========================================================================
// Fibonacci Heap Class
//========================================================================

class FibHeap
{
    FibHeapNode *MinRoot;
    s32 NumNodes, NumTrees, NumMarkedNodes;

    s32 HeapOwnershipFlag;

public:

    FibHeap();
    virtual ~FibHeap();

    // The Standard Heap Operations

    void Insert(FibHeapNode *NewNode);
    void Union(FibHeap *OtherHeap);

    inline FibHeapNode *Minimum();
    FibHeapNode *ExtractMin();

    s32 DecreaseKey(FibHeapNode *theNode, FibHeapNode& NewKey);
    s32 Delete(FibHeapNode *theNode);

    // Extra utility functions

    s32  GetHeapOwnership() { return HeapOwnershipFlag; };
    void SetHeapOwnership() { HeapOwnershipFlag = 1; };
    void ClearHeapOwnership() { HeapOwnershipFlag = 0; };

    s32 GetNumNodes() { return NumNodes; };
    s32 GetNumTrees() { return NumTrees; };
    s32 GetNumMarkedNodes() { return NumMarkedNodes; };

#if TESTING_FIB_HEAP
    void Print(FibHeapNode *Tree = NULL, FibHeapNode *theParent=NULL);
#endif
    void Draw(FibHeapNode *Tree = NULL ) const;
    
private:

    // Internal functions that help to implement the Standard Operations

    inline void _Exchange(FibHeapNode*&, FibHeapNode*&);
    void _Consolidate();
    void _Link(FibHeapNode *, FibHeapNode *);
    void _AddToRootList(FibHeapNode *);
    void _Cut(FibHeapNode *, FibHeapNode *);
    void _CascadingCut(FibHeapNode *);
};

#endif

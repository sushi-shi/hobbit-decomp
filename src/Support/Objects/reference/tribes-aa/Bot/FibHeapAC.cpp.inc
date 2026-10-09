//==============================================================================
//
//  FibHeap.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "FibHeapAC.hpp"

//==============================================================================
// Global/Static Vars
//==============================================================================
#if RECORD_STATS
#include "Objects\Bot\BotLog.hpp"
s32     g_StatsMax = 0;
f32     g_StatsAvg = 0;
s32     g_StatsCtr = 0;
s32     g_StatsSum = 0;

// 1-10, 11-20, ... , 250-260
s32     g_StatsDist10[26];
// 0-3, 4-7, 8-15, 16-31, 32-63, 64-127, 128-255
s32     g_StatsDist2[6];
xbool   g_StatsInited = FALSE;
#endif

//==============================================================================
//  FUNCTIONS
//==============================================================================

fheap::fheap()
{
    iMinRoot = -1;
    x_memset(DegArray, -1, sizeof(DegArray));
    x_memset(m_OpenStack, -1, sizeof(m_OpenStack));
    m_iStackTop = 0;
    m_OpenStack[0] = 0;

#if RECORD_STATS
    CurrMaxSize = 0;
    if (!g_StatsInited)
    {
        g_StatsInited = TRUE;
        x_memset(g_StatsDist10, 0, 26*4);
        x_memset(g_StatsDist2, 0, 7*4);
    }
#endif
}

//==============================================================================

fheap::~fheap()
{
    Reset();
}

//==============================================================================
#if RECORD_STATS

xbool DumpStatsToBotLog ( void )
{
    bot_log::Clear();
    bot_log::Log("Average Max Nodes in PQ = %.2f\n", g_StatsAvg);
    bot_log::Log("Max Nodes in PQ = %d\n", g_StatsMax);
    bot_log::Log("Max Nodes Distribution, Base 10:\n");
    for (s32 i = 0; i < 26; i++)
    {
        bot_log::Log("%d - %d: %d\n", i*10, (i+1)*10 - 1, g_StatsDist10[i]);
    }

    bot_log::Log("\n\nMax Nodes Distribution, Base 2: \n");
    for (i = 0; i < 6; i++)
    {
        s32 Exp = i + 3;
        s32 Max = (1 << Exp) - 1;
        bot_log::Log("Interval to %d: %d\n", Max, g_StatsDist2[i]);
    }

    return TRUE;
}

void ResetStats( void )
{

}
#endif
void fheap::Reset()
{
#if RECORD_STATS
    if (CurrMaxSize > 0)
    {
        g_StatsCtr++;
        g_StatsSum += CurrMaxSize;
        g_StatsAvg = (f32)g_StatsSum / (f32)g_StatsCtr;

        if (CurrMaxSize > g_StatsMax)
            g_StatsMax = CurrMaxSize;

        ASSERT( CurrMaxSize < 256 );
        g_StatsDist10[CurrMaxSize/10]++;
        s32 Max;
        s32 Idx;
        for (s32 i = 3; i < 9; i++)
        {
            Max = 1<<i;
            Idx = i - 3;
            if (CurrMaxSize < Max)
            {
                g_StatsDist2[Idx]++;
                break;
            }
        }
        if (g_StatsSum > S16_MAX)
        {
            DumpStatsToBotLog();
            g_StatsAvg = 0;
            g_StatsCtr = 0;
            g_StatsSum = 0;
        }
    }
    CurrMaxSize = 0;
#endif
    while (Delete(iMinRoot));
    iMinRoot = -1;

    x_memset(DegArray, -1, sizeof(DegArray));
    x_memset(m_OpenStack, -1, sizeof(m_OpenStack));
    
    m_iStackTop = 0;
    m_OpenStack[0] = 0;

}

//==============================================================================

void fheap::Insert(s32 newNode)
{
    if (iMinRoot == -1)
    {
        iMinRoot  =  
        m_FibNodeList[newNode].left = 
        m_FibNodeList[newNode].right = newNode;
    }
    else
    {
        // assign the indices in the new node.
        m_FibNodeList[newNode].left = iMinRoot;
        m_FibNodeList[newNode].right = m_FibNodeList[iMinRoot].right;

        // reassign the pointers in the heap.
        m_FibNodeList[m_FibNodeList[newNode].left].right = newNode; 
        m_FibNodeList[m_FibNodeList[newNode].right].left = newNode; 
    }

    // Check the priorities and set the new iMinRoot.
    if (m_FibNodeList[newNode].data < m_FibNodeList[iMinRoot].data)
    {
        iMinRoot = newNode;
    }
}

//==============================================================================

xbool fheap::Insert(void *pNode, s32 Cost)
{
    // Acquire a free spot in the open stack.
    s32 NewSpot = Pop();

    // Each newly inserted node should be clean & zero-ed out.
    ASSERT (m_FibNodeList[NewSpot].data == 0 &&
            m_FibNodeList[NewSpot].pNode == NULL);

    m_FibNodeList[NewSpot].pNode = pNode;

    m_FibNodeList[NewSpot].Set(Cost);
    Insert(NewSpot);

    if (NewSpot >= MAX_FIB_NODES - 2)
    {
        // Can't add any more- exit now or die.
        return FALSE;
    }
    return TRUE;
}

//==============================================================================

void fheap::Union(s32 iRoot)
{
    if(iRoot == -1)
        return;

    s32 root1 = iMinRoot;
    s32 right1 = m_FibNodeList[iMinRoot].right;

    s32 root2 = iRoot;
    s32 right2 = m_FibNodeList[iRoot].right;

    // Break the two circles and join them at the breaks.
    m_FibNodeList[root1].right = right2;
    m_FibNodeList[right2].left = root1;

    m_FibNodeList[right1].left = root2;
    m_FibNodeList[root2].right = right1;

    // Set the new iMinRoot.
    if (m_FibNodeList[root2].data < m_FibNodeList[root1].data)
        iMinRoot = root2;
}

//==============================================================================

s32 fheap::ExtractMin()
{
    // Extracts the minimum node without deleting it.

    s32 min = iMinRoot;

    if (iMinRoot == -1)       return -1;

    // Got siblings?
    if (iMinRoot != m_FibNodeList[iMinRoot].right)
    {
        // Sew up the hole.
        m_FibNodeList[m_FibNodeList[iMinRoot].left].right = m_FibNodeList[iMinRoot].right;
        m_FibNodeList[m_FibNodeList[iMinRoot].right].left = m_FibNodeList[iMinRoot].left;

        // Combine the children nodes into the heap, if any.
        if (m_FibNodeList[iMinRoot].child != -1)
        {
            // Isolate the child nodes from the heap.
            int childRoot = m_FibNodeList[iMinRoot].child;

            // Void all the parent pointers
            m_FibNodeList[m_FibNodeList[iMinRoot].child].parent = -1;
            s32 i;
            for (i = m_FibNodeList[m_FibNodeList[iMinRoot].child].right; 
                    m_FibNodeList[i].parent != -1; i = m_FibNodeList[i].right)
                m_FibNodeList[i].parent = -1;

            // Assign the new iMinRoot and join it to the children.
            if (m_FibNodeList[iMinRoot].right != iMinRoot)
                iMinRoot = GetMinSib(m_FibNodeList[iMinRoot].right);
            Union(childRoot);
        }
        iMinRoot = GetMinSib(m_FibNodeList[iMinRoot].right);
        ASSERT (iMinRoot != -1);
    }
    else    // only child at this level
    {
        iMinRoot = m_FibNodeList[iMinRoot].child;
        if(iMinRoot != -1)
        {
            m_FibNodeList[iMinRoot].parent = -1;
            // Void all the parent pointers for this generation
            s32 i;
            for (i = m_FibNodeList[iMinRoot].right; 
                    m_FibNodeList[i].parent != -1; i = m_FibNodeList[i].right)
                m_FibNodeList[i].parent = -1;
        }
    }
    ApplyDegreeRule();
    return min;
}

//==============================================================================

    
void *fheap::ExtractMinPtr()
{   
    // Cleanly extract iMinRoot of the heap, delete it, and return the handle of
    // the node.
    s32 Temp = ExtractMin();

    if(Temp == -1) 
    {
        return NULL;
    }

    void *pExtracted;
    
    // Store the handle of the minimum node 
    pExtracted = m_FibNodeList[Temp].pNode;

    // Mark the extracted node as free for our Open Stack.
    ClearNode(Temp);
    Push(Temp);

    return pExtracted;
}

//==============================================================================

s32 fheap::GetMinSib(s32 node)
{
    // Find the minimum node out of a circular list of nodes.
    s32 min, curr;
    min = node;
    curr = m_FibNodeList[node].right;
    while (curr != node)
    {
        if (m_FibNodeList[curr].data < m_FibNodeList[min].data)
            min = curr;
        curr = m_FibNodeList[curr].right;
    }
    return min;
}

//==============================================================================

xbool fheap::Delete(s32 delNode)
{
    // Are we deleting the root?    
    if (iMinRoot == delNode)
    {
        if (ExtractMin() != -1)
        {
            ClearNode(delNode);
            Push(delNode);
            return true;
        }
        else 
            return false;

    }
    
    // case: siblings exist.
    if (m_FibNodeList[delNode].right != delNode)
    {
        // Reassign siblings.
        m_FibNodeList[m_FibNodeList[delNode].right].left = m_FibNodeList[delNode].left;
        m_FibNodeList[m_FibNodeList[delNode].left].right = m_FibNodeList[delNode].right;

        // Find the minimum sibling of the list.
        s32 sibMin = GetMinSib(m_FibNodeList[delNode].right);

        // Reassign any existing parent
        if (m_FibNodeList[delNode].parent != -1)
        {
            m_FibNodeList[m_FibNodeList[delNode].parent].child = sibMin;
            m_FibNodeList[m_FibNodeList[delNode].parent].degree--;
        }


        // Merge the children nodes
        if (m_FibNodeList[delNode].child != -1)
        {
            m_FibNodeList[m_FibNodeList[delNode].child].parent = -1;

            Union(m_FibNodeList[delNode].child);
        }


    }
    // case: no siblings.
    else    
    {   
        // Reassign any existing parent
        if (m_FibNodeList[delNode].parent != -1)
        {
            m_FibNodeList[m_FibNodeList[delNode].parent].child = m_FibNodeList[delNode].child;
            m_FibNodeList[m_FibNodeList[delNode].parent].degree = m_FibNodeList[delNode].degree;
            
            if (m_FibNodeList[delNode].child != -1)
            {
                m_FibNodeList[m_FibNodeList[delNode].child].parent = m_FibNodeList[delNode].parent;
            }
        }
        else
        {   // No siblings and no parents.
            iMinRoot = m_FibNodeList[delNode].child;

            // This should be taken care of by the 1st case,
            // so it should never make it here(?).
            ASSERT(0);
        }
    }
    ClearNode(delNode);
    Push(delNode);

    return true;
}

//==============================================================================

s32 fheap::Minimum()
{
    return iMinRoot;
}


//==============================================================================

void fheap::ApplyDegreeRule()
{
    s32 end = iMinRoot;
    s32 curr = iMinRoot;
    x_memset(DegArray, -1, sizeof(DegArray));

    if (iMinRoot == -1)   return;

    do
    {
        curr = m_FibNodeList[curr].left;
        ASSERT(m_FibNodeList[curr].degree < MAX_DEGREE);
        if (DegArray[m_FibNodeList[curr].degree] == -1)
            DegArray[m_FibNodeList[curr].degree] = curr;
        else
            curr = ResolveConflict(curr, DegArray[m_FibNodeList[curr].degree]);
    } while (curr != end);
}

//==============================================================================

s32 fheap::ResolveConflict(s32 A, s32 B)
{
    if (A == B) return A;
    s32 min, max;

    if (m_FibNodeList[A].data < m_FibNodeList[B].data || A == iMinRoot)  
    {
        if (!(m_FibNodeList[A].data < m_FibNodeList[B].data))
            ASSERT (m_FibNodeList[A].data == m_FibNodeList[B].data);
        min = A; max = B;
    }
    else                    {   min = B; max = A;}

    // Remove the max from the top level.
    m_FibNodeList[m_FibNodeList[max].left].right = m_FibNodeList[max].right;
    m_FibNodeList[m_FibNodeList[max].right].left = m_FibNodeList[max].left;

    // Add the min to the other's children.

    if (m_FibNodeList[min].child == -1)
    {   // Set the only child value.
        m_FibNodeList[min].child  =  
        m_FibNodeList[max].left = 
        m_FibNodeList[max].right = max;
    }
    else
    {
        // Insert the node between the child and the one next to it.
        m_FibNodeList[max].left = m_FibNodeList[min].child;
        m_FibNodeList[max].right = m_FibNodeList[m_FibNodeList[min].child].right;

        m_FibNodeList[m_FibNodeList[max].left].right = max; 
        m_FibNodeList[m_FibNodeList[max].right].left = max; 
    }

    // Set the generation links.
    m_FibNodeList[max].parent = min;

    // Determine which child should be pointed to.
    if (m_FibNodeList[max].data < m_FibNodeList[m_FibNodeList[min].child].data)
        m_FibNodeList[min].child = max;

    m_FibNodeList[min].degree++;

    // Unassign the previous degree node pointer.
    DegArray[m_FibNodeList[max].degree] = -1;
    ASSERT(m_FibNodeList[min].degree < MAX_DEGREE);

    // Reassign the new degree node pointer, checking in case it is
    // already filled.
    if (DegArray[m_FibNodeList[min].degree] != -1)
    {
        min = ResolveConflict(min, DegArray[m_FibNodeList[min].degree]);
    }
    else
        DegArray[m_FibNodeList[min].degree] = min;
    return min;
}

//==============================================================================

s32 fheap::Pop( void )
{
    s32 Index = m_OpenStack[m_iStackTop];
    ASSERT(Index != -1);

    m_OpenStack[m_iStackTop] = -1;
    
    if (m_iStackTop == 0)
    {
        m_OpenStack[m_iStackTop] = Index + 1;
#if RECORD_STATS
        if (m_OpenStack[m_iStackTop] > CurrMaxSize)
            CurrMaxSize = m_OpenStack[m_iStackTop];
#endif
        ASSERT (m_OpenStack[m_iStackTop] < MAX_FIB_NODES);
//#ifdef acheng
//        static s32 MaxVal = m_OpenStack[m_iStackTop];
//        if (m_OpenStack[m_iStackTop] > MaxVal)
//            MaxVal = m_OpenStack[m_iStackTop];
//        volatile static xbool CheckAnyValOverN = 0;
//        volatile static s32 N = 150;
//        if (CheckAnyValOverN && m_OpenStack[m_iStackTop] > N)
//        {
//            BREAK;
//        }
//        x_DebugMsg("Pop: Max = %d, Curr = %d\n", MaxVal, m_OpenStack[m_iStackTop]);
//#endif
    }
    else
    {
        m_iStackTop--;
    }

    return Index;
}

//==============================================================================

void fheap::Push( s32 Slot )
{
    // This should be called when any node is deleted from the memory pool
    // via ExtractMin or Delete.
    m_OpenStack[++m_iStackTop] = Slot;
}

//==============================================================================

void fheap::ClearNode( s32 iNode )
{
    m_FibNodeList[iNode].data = 0;
    m_FibNodeList[iNode].degree = 0;
    m_FibNodeList[iNode].left = -1;
    m_FibNodeList[iNode].right = -1;
    m_FibNodeList[iNode].parent = -1;
    m_FibNodeList[iNode].child = -1;
    m_FibNodeList[iNode].pNode = NULL;
}

//==============================================================================

xbool fheap::IsEmpty( void )
{
    if (iMinRoot != -1)
        return FALSE;

    s32 i;
    for (i = 0; i < MAX_FIB_NODES; i++)
    {
        if (m_FibNodeList[i].data)
        {
            x_DebugMsg("Invalid data in fheap!");
            return FALSE;
        }
        if (m_FibNodeList[i].degree)
        {
            x_DebugMsg("Invalid data in fheap!");
            return FALSE;
        }
        if (m_FibNodeList[i].pNode)
        {
            x_DebugMsg("Invalid data in fheap!");
            return FALSE;
        }
    }
    return TRUE;
}
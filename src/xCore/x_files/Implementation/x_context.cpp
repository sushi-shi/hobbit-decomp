#include <rva.h>

#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_string.hpp>

DATA(0x003ccf28) xcontext_debug g_Context;
DATA(0x003ccff0) static int s_Initialized = 0;
DATA(0x003ccff4) static xcontext_node* s_pCurrentNode = 0;
DATA(0x003ccfe0) static int s_nNodes;
DATA(0x003ccfe4) int s_DisplayFrameCount;
DATA(0x003ccfe8) static xtick s_InitTicks;
DATA(0x003ccfb0) static int s_bSpikeOccurred;
DATA(0x0034c628) static xcontext_node s_GlobalContextNode = { "GLOBAL", 0, 0, 0, 0, 0, 0, 0, 0 };

RVA(0x00248c60, 0xcb)
void x_ContextInit()
{
    s_InitTicks = x_GetTime();
    s_pCurrentNode = &s_GlobalContextNode;
    s_nNodes = 1;
    g_Context.bProfilingEnabled = 0;
    g_Context.bDisplayEnabled = 0;
    g_Context.bDisplayHierarchyEnabled = 1;
    g_Context.bDisplaySummaryEnabled = 1;
    g_Context.bShowAllOverrideEnabled = 0;
    g_Context.bShowSubstringFilteredEnabled = 0;
    g_Context.bHideSubstringFilteredEnabled = 1;
    g_Context.bSubstringParentFilterEnabled = 1;
    g_Context.bMSFilterEnabled = 1;
    g_Context.bCallsFilterEnabled = 1;
    g_Context.TotalMSThreshold = 0.0125f;
    g_Context.TotalCallsThreshold = 250;
    g_Context.DisplaySpikeMSThreshold = 3.402823466e+38F;
    g_Context.bCommandSetAllSubstringToOn = 0;
    g_Context.bCommandSetAllSubstringToOff = 0;
    g_Context.bCommandFilterSubstring = 0;
    x_strcpy(g_Context.SubstringBuffer, "");
    g_Context.nFramesBetweenDisplays = 20;
    s_DisplayFrameCount = g_Context.nFramesBetweenDisplays;
    g_Context.bBuildTree = 0;
    s_Initialized = 1;
    s_bSpikeOccurred = 0;
}

RVA(0x00248d30, 0xb)
void x_ContextKill()
{
    s_Initialized = 0;
}

RVA(0x00248d40, 0xb)
void x_ContextEnableProfiling()
{
    g_Context.bProfilingEnabled = 1;
}

RVA(0x00248d50, 0xb)
void x_ContextDisableProfiling()
{
    g_Context.bProfilingEnabled = 0;
}


// Field operations and traversal order are checked against PC instruction
// bodies. Sibling-engine identifiers are reference leads, not Xbox addresses.
RVA(0x00248d90, 0x33)
static void ResetNode(xcontext_node* node)
{
    node->Hits = 0;
    node->Ticks = 0;
    node = node->pChildren;
    while (node)
    {
        ResetNode(node);
        node = node->pNext;
    }
}

RVA(0x00248f90, 0x3a)
static void PushClearSubstringFilter(xcontext_node* node, int on)
{
    node->Flags &= ~4;
    node->Flags &= ~2;
    if (on)
    {
        node->Flags |= 4;
        node->Flags |= 2;
    }
    node = node->pChildren;
    while (node)
    {
        PushClearSubstringFilter(node, on);
        node = node->pNext;
    }
}

RVA(0x00248d60, 0x28)
void x_ContextResetProfile()
{
    ResetNode(&s_GlobalContextNode);
    s_InitTicks = x_GetTime();
    g_Context.bBuildTree = g_Context.bProfilingEnabled;
}

RVA(0x00248f70, 0x13)
void x_ContextClearSubstringFilter(int on)
{
    PushClearSubstringFilter(&s_GlobalContextNode, on);
}

RVA(0x002496d0, 0x4a)
static xcontext_node* PushFindDuplicate(xcontext_node* node, const char* name)
{
    if (!node->pDuplicate && x_stricmp(name, node->pName) == 0)
        return node;
    node = node->pChildren;
    while (node)
    {
        xcontext_node* found = PushFindDuplicate(node, name);
        if (found)
            return found;
        node = node->pNext;
    }
    return 0;
}

RVA(0x00249600, 0xc7)
xcontext::xcontext(const char* name)
{
    if (!g_Context.bBuildTree || !s_Initialized)
        return;
    xcontext_node* node = s_pCurrentNode->pChildren;
    while (node)
    {
        if (node->pName == name)
        {
            s_pCurrentNode = node;
            if (g_Context.bProfilingEnabled)
                m_StartTicks = x_GetTime();
            return;
        }
        node = node->pNext;
    }
    xcontext_node* duplicate = PushFindDuplicate(&s_GlobalContextNode, name);
    node = static_cast<xcontext_node*>(x_malloc_fn(sizeof(xcontext_node),
        "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_context.cpp", 661));
    node->pName = name;
    node->Hits = 0;
    node->Ticks = 0;
    node->Flags = 1;
    node->pParent = s_pCurrentNode;
    node->pNext = s_pCurrentNode->pChildren;
    node->pChildren = 0;
    s_pCurrentNode->pChildren = node;
    node->pDuplicate = duplicate;
    node->ChildrenTicks = 0;
    s_pCurrentNode = node;
    ++s_nNodes;
    if (g_Context.bProfilingEnabled)
        m_StartTicks = x_GetTime();
}

RVA(0x00249720, 0x50)
xcontext::~xcontext()
{
    if (!s_Initialized || !g_Context.bBuildTree)
        return;
    if (g_Context.bProfilingEnabled)
    {
        ++s_pCurrentNode->Hits;
        s_pCurrentNode->Ticks += x_GetTime() - m_StartTicks;
    }
    s_pCurrentNode = s_pCurrentNode->pParent;
}

RVA(0x00248ff0, 0x56)
static void PushSubstringFilter(xcontext_node* node, const char* substring, int parentFiltered)
{
    if (parentFiltered)
        node->Flags |= 4;
    if (x_stristr(node->pName, substring))
    {
        parentFiltered = 1;
        node->Flags |= 2;
    }
    node = node->pChildren;
    while (node)
    {
        PushSubstringFilter(node, substring, parentFiltered);
        node = node->pNext;
    }
}

RVA(0x00248fd0, 0x15)
void x_ContextSubstringFilter(const char* substring)
{
    PushSubstringFilter(&s_GlobalContextNode, substring, 0);
}

RVA(0x002495a0, 0x38)
void DeleteTree(xcontext_node* node)
{
    if (node->Hits > 0)
        return;
    xcontext_node* child = node->pChildren;
    while (child)
    {
        DeleteTree(child);
        child = child->pNext;
    }
    if (node != &s_GlobalContextNode)
        delete node;
}

RVA(0x002495e0, 0x12)
void x_ContextClear()
{
    DeleteTree(&s_GlobalContextNode);
    x_ContextResetProfile();
}

RVA(0x00249200, 0xe3)
static void SolveVisibleNodes(xcontext_node* node)
{
    node->Flags |= 1;
    if ((node->Flags & 2) || (g_Context.bSubstringParentFilterEnabled && (node->Flags & 4)))
    {
        if (g_Context.bShowSubstringFilteredEnabled) node->Flags |= 1;
        if (g_Context.bHideSubstringFilteredEnabled) node->Flags &= ~1;
    }
    else
    {
        if (g_Context.bShowSubstringFilteredEnabled) node->Flags &= ~1;
        if (g_Context.bHideSubstringFilteredEnabled) node->Flags |= 1;
    }
    if (g_Context.bMSFilterEnabled && x_TicksToMs(node->Ticks) < g_Context.bMSFilterEnabled)
        node->Flags &= ~1;
    if (g_Context.bCallsFilterEnabled && node->Hits >= g_Context.TotalCallsThreshold)
        node->Flags |= 1;
    if (g_Context.bShowAllOverrideEnabled) node->Flags |= 1;
    if (node->Hits == 0) node->Flags &= ~1;
    node = node->pChildren;
    while (node)
    {
        SolveVisibleNodes(node);
        node = node->pNext;
    }
}

RVA(0x002492f0, 0x52)
static void SolveSummaryNode(xcontext_node* node)
{
    if (node->pDuplicate)
    {
        node->pDuplicate->Hits += node->Hits;
        node->pDuplicate->Ticks += node->Ticks;
        node->pDuplicate->ChildrenTicks += node->ChildrenTicks;
    }
    node = node->pChildren;
    while (node)
    {
        SolveSummaryNode(node);
        node = node->pNext;
    }
}

RVA(0x00249350, 0x49)
static void SolveChildrenTicks(xcontext_node* node)
{
    node->ChildrenTicks = 0;
    xcontext_node* child = node->pChildren;
    while (child)
    {
        node->ChildrenTicks += child->Ticks;
        child = child->pNext;
    }
    node = node->pChildren;
    while (node)
    {
        SolveChildrenTicks(node);
        node = node->pNext;
    }
}

static void PushPrintNode(xcontext_node* node, int depth);
RVA(0x002493a0, 0x134)
static void PrintHierarchyNode(xcontext_node* node, int depth)
{
    DATA(0x0034c658) static char VertBars[] = "||||||||||||||||||||||||||||||||";
    DATA(0x0034c67c) static char HorizBar[] = "--------------------------------";
    DATA(0x0034c6a0) static char NameTab[] = "................................";
    int distance = 20 - depth - 2;
    distance = (distance < 20) ? distance : 20;
    distance = (distance > 0) ? distance : 0;
    char previous = VertBars[depth];
    VertBars[depth] = 0;
    if (node->pChildren)
    {
        if (node->Flags & 1)
            x_DebugMsg(7, "%s*-%s[%06d]-(%09.3f)-(%07.3f)-(%08.3f) [%1d%1d] %s\n",
                VertBars, HorizBar + 32 - distance, node->Hits,
                x_TicksToMs(node->Ticks), x_TicksToMs(node->Ticks) - x_TicksToMs(node->ChildrenTicks),
                x_TicksToMs(node->ChildrenTicks), (node->Flags & 2) ? 1 : 0,
                (node->Flags & 4) ? 1 : 0, node->pName);
        VertBars[depth] = previous;
        PushPrintNode(node->pChildren, depth + 1);
    }
    else
    {
        if (node->Flags & 1)
            x_DebugMsg(7, "%s..%s[%06d].(%09.3f)..................... [%1d%1d] %s\n",
                VertBars, NameTab + 32 - distance, node->Hits, x_TicksToMs(node->Ticks),
                (node->Flags & 2) ? 1 : 0, (node->Flags & 4) ? 1 : 0, node->pName);
    }
    VertBars[depth] = previous;
}

RVA(0x002494e0, 0x2c)
static void PushPrintNode(xcontext_node* node, int depth)
{
    if (node)
    {
        if (node->pNext) PushPrintNode(node->pNext, depth);
        PrintHierarchyNode(node, depth);
    }
}

RVA(0x00249510, 0x89)
static void PrintSummaryNode(xcontext_node* node)
{
    if (!node->pDuplicate && (node->Flags & 1))
        x_DebugMsg(7, "[%06d]..(%09.3f).(%07.3f).(%08.3f)..%s\n", node->Hits,
            x_TicksToMs(node->Ticks), x_TicksToMs(node->Ticks) - x_TicksToMs(node->ChildrenTicks),
            x_TicksToMs(node->ChildrenTicks), node->pName);
    node = node->pChildren;
    while (node)
    {
        PrintSummaryNode(node);
        node = node->pNext;
    }
}

RVA(0x00249050, 0x1a6)
void x_ContextPrintProfile()
{
    if (g_Context.bCommandSetAllSubstringToOn)
    {
        x_ContextClearSubstringFilter(1);
        g_Context.bCommandSetAllSubstringToOn = 0;
    }
    if (g_Context.bCommandSetAllSubstringToOff)
    {
        x_ContextClearSubstringFilter(0);
        g_Context.bCommandSetAllSubstringToOff = 0;
    }
    if (g_Context.bCommandFilterSubstring)
    {
        x_ContextSubstringFilter(g_Context.SubstringBuffer);
        g_Context.bCommandFilterSubstring = 0;
    }
    int display = g_Context.bDisplayEnabled;
    s_GlobalContextNode.Hits = 1;
    s_GlobalContextNode.Ticks = x_GetTime() - s_InitTicks;
    if (!s_bSpikeOccurred && x_TicksToMs(s_GlobalContextNode.Ticks) > g_Context.DisplaySpikeMSThreshold)
    {
        s_DisplayFrameCount = 0;
        s_bSpikeOccurred = 1;
        display = 1;
    }
    else
    {
        --s_DisplayFrameCount;
        if (s_DisplayFrameCount >= 0) return;
        s_DisplayFrameCount = g_Context.nFramesBetweenDisplays;
        s_bSpikeOccurred = 0;
    }
    if (!display)
    {
        s_DisplayFrameCount = g_Context.nFramesBetweenDisplays;
        return;
    }
    SolveChildrenTicks(&s_GlobalContextNode);
    if (g_Context.bDisplayHierarchyEnabled)
    {
        SolveVisibleNodes(&s_GlobalContextNode);
        x_DebugMsg(7, "\n");
        x_DebugMsg(7, "*-----------nCalls-(TotalMs)-(InFunct)-(InChild)---------------*\n");
        PrintHierarchyNode(&s_GlobalContextNode, 0);
    }
    if (g_Context.bDisplaySummaryEnabled)
    {
        x_DebugMsg(7, "\n");
        x_DebugMsg(7, "*-------(TotalMs)-(InFunct)-(InChild)--------------------------*\n");
        SolveSummaryNode(&s_GlobalContextNode);
        SolveVisibleNodes(&s_GlobalContextNode);
        PrintSummaryNode(&s_GlobalContextNode);
        x_DebugMsg(7, "*--------------------------------------------------------------*\n");
        x_DebugMsg(7, "nNodes   : %d\n", s_nNodes);
        x_DebugMsg(7, "MemUsed  : %d\n", s_nNodes * (sizeof(xcontext_node) + 32));
        x_DebugMsg(7, "*--------------------------------------------------------------*\n");
    }
}

static void SaveProfileNodes(X_FILE* file, xcontext_node* node, int depth);

// PC-specific saved-tree printer: unlike the sibling version it owns a reusable
// local string and emits every node. The helper name describes its PC behavior.
RVA(0x00248e90, 0x8b)
static void SaveProfileNode(X_FILE* file, xcontext_node* node, int depth)
{
    DATA(0x003ccfb4) static xstring name;
    name = node->pName;
    x_fprintf(file, "%*s %6d %3.3f %s\n", depth * 2, "", node->Hits,
              x_TicksToMs(node->Ticks), (const char*)name);
    SaveProfileNodes(file, node->pChildren, depth + 1);
}

RVA(0x00248f20, 0x34)
static void SaveProfileNodes(X_FILE* file, xcontext_node* node, int depth)
{
    if (node)
    {
        if (node->pNext) SaveProfileNodes(file, node->pNext, depth);
        SaveProfileNode(file, node, depth);
    }
}

RVA(0x00248dd0, 0xb4)
void x_ContextSaveProfile(const char* path)
{
    X_FILE* file = x_fopen(path, "w");
    s_GlobalContextNode.Hits = 1;
    s_GlobalContextNode.Ticks = x_GetTime() - s_InitTicks;
    if (s_pCurrentNode)
    {
        x_fprintf(file, "CURRENT CONTEXT STACK:\n");
        xcontext_node* node = s_pCurrentNode;
        while (node)
        {
            x_fprintf(file, "%s\n", node->pName);
            node = node->pParent;
        }
    }
    else
        x_fprintf(file, "NO CURRENT CONTEXT STACK\n");
    x_fprintf(file, "CONTEXT DUMP: %d nodes\n", s_nNodes);
    SaveProfileNode(file, &s_GlobalContextNode, 0);
    x_fclose(file);
}

RVA_DYNINIT(0x00248f60, 0xa, SaveProfileNode)

RVA(0x00249770, 0xe3)
void x_ContextDisplayStack(int screen, int tty)
{
    const xcontext_node* node = s_pCurrentNode;
    int y = 1;
    int x = 1;
    if (tty)
    {
        x_DebugMsg("CONTEXT STACK DUMP\n");
        x_DebugMsg("------------------\n");
    }
    char name[25] = {0};
    while (node)
    {
        if (node->pName)
            x_strncpy(name, node->pName, 24);
        else
            x_strcpy(name, "-UNNAMED-");
        if (screen) x_printfxy(x, y, name);
        if (tty) x_DebugMsg("%s\n", name);
        ++y;
        if (y >= 22)
        {
            y = 1;
            x += 25;
        }
        node = node->pParent;
    }
    if (tty)
    {
        x_DebugMsg("-------------------------\n");
        x_DebugMsg("END OF CONTEXT STACK DUMP\n");
    }
}

#ifndef HOBBIT_X_CONTEXT_HPP
#define HOBBIT_X_CONTEXT_HPP

#include <xCore/x_files/x_time.hpp>

// Recovered from the PC context constructor allocation (0x30 bytes), member
// accesses, and recursive profile walkers. Natural i386 alignment supplies
// the final four bytes; no synthetic fields stand in for unexamined storage.
struct xcontext_node
{
    const char* pName;
    xcontext_node* pParent;
    xcontext_node* pNext;
    xcontext_node* pChildren;
    xtick Ticks;
    int Hits;
    xcontext_node* pDuplicate;
    xtick ChildrenTicks;
    unsigned int Flags;
};

struct xcontext_debug
{
    int bProfilingEnabled;
    int bDisplayEnabled;
    int bDisplaySummaryEnabled;
    int bDisplayHierarchyEnabled;
    int bShowAllOverrideEnabled;
    int bShowSubstringFilteredEnabled;
    int bHideSubstringFilteredEnabled;
    int bSubstringParentFilterEnabled;
    int bMSFilterEnabled;
    int bCallsFilterEnabled;
    int bCommandSetAllSubstringToOn;
    int bCommandSetAllSubstringToOff;
    int bCommandFilterSubstring;
    char SubstringBuffer[64];
    float TotalMSThreshold;
    float DisplaySpikeMSThreshold;
    int TotalCallsThreshold;
    int nFramesBetweenDisplays;
    int bBuildTree;
};

extern xcontext_debug g_Context;

class xcontext
{
public:
    xcontext(const char* name);
    ~xcontext();
protected:
    xtick m_StartTicks;
};

void x_ContextInit();
void x_ContextKill();
void x_ContextEnableProfiling();
void x_ContextDisableProfiling();
void x_ContextResetProfile();
void x_ContextClearSubstringFilter(int on);
void x_ContextSubstringFilter(const char* substring);

void x_ContextPrintProfile();
void x_ContextSaveProfile(const char* path);
void x_ContextDisplayStack(int screen = 1, int tty = 1);
void x_ContextClear();

// Original lexical context helper; Hobbit retains xcontext constructor calls.
#ifndef CONTEXT
#define CONTEXT(name) xcontext __profile__(name)
#endif

#endif

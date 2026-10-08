#ifndef HOBBIT_X_FILES_PRIVATE_HPP
#define HOBBIT_X_FILES_PRIVATE_HPP

// Both sibling trees declare this interface; Hobbit's formatter and thread
// initialization independently use these four fields in this order.
struct x_thread_globals
{
    int ThreadID;
    int NextOffset;
    int BufferSize;
    char* StringBuffer;
};

extern "C" void x_Init();
extern "C" void x_Kill();

x_thread_globals* x_GetThreadGlobals();
int x_GetInitialized();
int x_GetThreadID();

#endif

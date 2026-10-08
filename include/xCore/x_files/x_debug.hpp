#ifndef HOBBIT_X_DEBUG_HPP
#define HOBBIT_X_DEBUG_HPP

// PC overloads have distinct stack layouts: format first at RVA24e7d0,
// channel then format at RVA24e870. Context callers pass channel 7.
void x_DebugMsg(const char* format, ...);
void x_DebugMsg(int channel, const char* format, ...);

void x_DebugLog(const char* format, ...);
void x_DebugSetVersionString(const char* version);
const char* x_DebugGetVersionString();
const char* x_DebugGetCallStackString();
int x_DebugGetCallStack(int& depth, unsigned int*& stack);
typedef int rtf_fn(const char* file, int line, const char* expression, const char* message);
typedef void log_fn(const char* message);
int RTFHandler(const char* file, int line, const char* expression, const char* message);
void x_SetRTFHandler(rtf_fn* handler);
rtf_fn* x_GetRTFHandler();
void x_SetLogHandler(log_fn* handler);
int xExceptionThrowHandler(const char* file, int line, const char* message, int concatenate);
int xExceptionThrowHandler(const char* file, int line, const char* message, int concatenate, int code);

// Original release/debug assert contract. Platform breakpoint code is handled
// by the caller's debug profile; imported APIs do not manufacture error stubs.
#ifndef ASSERT
#if defined(X_ASSERT)
#define ASSERT(expr) ((expr) || RTFHandler(__FILE__, __LINE__, #expr, 0))
#define ASSERTS(expr,msg) ((expr) || RTFHandler(__FILE__, __LINE__, #expr, (msg)))
#elif defined(_MSC_VER)
#define ASSERT(expr) __assume(expr)
#define ASSERTS(expr,msg) __assume(expr)
#else
#define ASSERT(expr) ((void)0)
#define ASSERTS(expr,msg) ((void)0)
#endif
#endif
#ifndef VERIFY
#define VERIFY(expr) ((void)(expr))
#define VERIFYS(expr,msg) ((void)(expr))
#endif
#ifndef DEMAND
#define DEMAND(expr) ASSERT(expr)
#define DEMANDS(expr,msg) ASSERTS(expr,msg)
#endif

// Genuine sibling exception-profile macros. Disabled profile preserves its
// original lexical conditional blocks; enabled profile uses actual C++ EH.
#ifndef BREAK
#define BREAK { __asm int 3 }
#endif
#ifdef X_EXCEPTIONS
int xExceptionCatchHandler(const char* file, int line, const char* message, int& skip);
#define x_try try { ((void)0)
#define x_catch_begin } catch(...) { ((void)0)
#define x_catch_end } ((void)0)
#define x_catch_display } catch(...) { static int skip = 0; if (xExceptionCatchHandler(__FILE__, __LINE__, 0, skip)) { BREAK; } } ((void)0)
#define x_catch_display_msg(msg) } catch(...) { static int skip = 0; if (xExceptionCatchHandler(__FILE__, __LINE__, (msg), skip)) { BREAK; } } ((void)0)
#define x_throw(msg) do { xExceptionThrowHandler(__FILE__, __LINE__, (msg), 0); throw(0); } while(0)
#define x_append_throw(msg) do { xExceptionThrowHandler(__FILE__, __LINE__, (msg), 1); throw(0); } while(0)
#else
#define x_try if(1) {
#define x_catch_display }
#define x_catch_display_msg(msg) }
#define x_catch_begin } if(0) {
#define x_catch_end }
inline void x_throw(const char* message) { (void)message; ASSERT(0); }
inline void x_append_throw(const char* message) { (void)message; ASSERT(0); }
#endif
#define x_catch_append(msg) x_catch_begin; x_append_throw(msg); x_catch_end
#define x_catch_end_ret x_append_throw(0); x_catch_end

#endif

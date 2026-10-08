#ifndef HOBBIT_X_FILES_HPP
#define HOBBIT_X_FILES_HPP

// Bulk engine API umbrella. Existing Hobbit definitions retain ownership;
// provisional sibling additions are recorded in docs/engine-source-port.md.
#include <xCore/x_files/x_target.hpp>
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_string.hpp>
#include <xCore/x_files/x_array.hpp>
#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/x_files/x_time.hpp>
#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_log.hpp>
#include <xCore/x_files/x_threads.hpp>
#include <xCore/x_files/x_mutex.hpp>
#include <xCore/x_files/x_mqueue.hpp>
#include <xCore/x_files/x_locale.hpp>

// Hobbit's admitted implementation has no argc/argv parameters.
extern "C" void x_Init();
extern "C" void x_Kill();
int x_GetInitialized();
int x_GetThreadID();

#endif

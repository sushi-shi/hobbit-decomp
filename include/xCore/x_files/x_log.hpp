#ifndef HOBBIT_X_LOG_HPP
#define HOBBIT_X_LOG_HPP

#include <rva.h>
#include <xCore/x_files/x_types.hpp>

// Source analogue: Area 51 x_log_private.hpp's disabled logging branch.
// Hobbit retains a variadic empty call for free notifications. This name is
// inferred; the shared one-byte body may also represent other folded no-ops.
RVA(0x00025890, 0x1)
inline void log_NULL(...)
{
}

#include <xCore/x_files/Implementation/x_log_private.hpp>

#endif

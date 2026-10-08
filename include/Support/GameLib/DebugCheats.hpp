// Complete original Area51 431f72b9 source dependency; provisional sibling variant, no Hobbit PC identity.
// See docs/imports/object-generic-closure.json.
#ifndef DEBUG_CHEATS_HPP
#define DEBUG_CHEATS_HPP

#include <xCore/x_files/x_types.hpp>

//=============================================================================

#ifndef CONFIG_RETAIL

extern xbool DEBUG_INFINITE_AMMO;
extern xbool DEBUG_INVULNERABLE;
extern xbool DEBUG_EXPERT_JUMPINGBEAN;

#else

#define DEBUG_INFINITE_AMMO 0
#define DEBUG_INVULNERABLE  0
#define DEBUG_EXPERT_JUMPINGBEAN 0
#endif

//=============================================================================

#endif // DEBUG_CHEATS_HPP

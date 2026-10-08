//=========================================================================
// Tribes2 description file
//=========================================================================

#ifndef __TRIBES2_TYPES_HPP__
#define __TRIBES2_TYPES_HPP__



//=========================================================================
// Macros to compile label set files
//=========================================================================

// Begins enum set
#define BEGIN_LABEL_SET(__name__)                       \
enum __name__                                           \
{

// Adds a label to enum set
#define LABEL(__name__, __desc__)                       \
    __name__,                         

// Adds a label to enum set with a given value
#define LABEL_VALUE(__name__, __value__, __desc__)      \
    __name__ = __value__,                         

// Ends enum set
#define END_LABEL_SET(__name__)                         \
} ;


//=========================================================================
// Label set type files
//=========================================================================
#include "ShapeTypes.hpp"
#include "ModelTypes.hpp"
#include "NodeTypes.hpp"
#include "AnimTypes.hpp"
#include "HotPointTypes.hpp"
#include "ParticleTypes.hpp"
#include "SoundTypes.hpp"
#include "musictypes.hpp"
#include "voicetypes.hpp"
#include "trainingtypes.hpp"
#include "GUITypes.hpp"



#endif  //#ifndef __TRIBES2_TYPES_HPP__

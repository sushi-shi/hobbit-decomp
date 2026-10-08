//=========================================================================
// Tribes2 description file
//=========================================================================

// Available script file commands:

// #include ...file...

// LABEL_FILTER("Name")

// BEGIN_ATTRIBUTE_SET          (UserDataName)

// ATTRIBUTE_FLOAT      (VariableName, DefaultValue, MinValue, MaxValue, UIStep)
// ATTRIBUTE_INT        (VariableName, DefaultValue, MinValue, MaxValue, UIStep)
// ATTRIBUTE_STRING     (VariableName, DefaultValue)
// ATTRIBUTE_BOOL       (VariableName, DefaultValue - TRUE or FALSE)
// ATTRIBUTE_RADIUS     (VariableName)
// ATTRIBUTE_FRAMES     (VariableName)
// ATTRIBUTE_LABEL_SET  (VariableName, SetName)

// END_ATTRIBUTE_SET        


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
#include "GUITypes.hpp"

//=========================================================================
// Label filters to aid in selection
//=========================================================================

LABEL_FILTER("Character")
LABEL_FILTER("Weapon")
LABEL_FILTER("Turret")
LABEL_FILTER("GUI")
LABEL_FILTER("Particle")
LABEL_FILTER("Pickup")
LABEL_FILTER("Projectile")
LABEL_FILTER("LOD")
LABEL_FILTER("Damage")
LABEL_FILTER("Misc")
LABEL_FILTER("Inventory")
LABEL_FILTER("Mine")
LABEL_FILTER("Armor Pack")
LABEL_FILTER("Deployable Pack")
LABEL_FILTER("Scenery")
LABEL_FILTER("Asset")



//=========================================================================
// Description for types
//=========================================================================

// Assign one per max file
BEGIN_ATTRIBUTE_SET(ShapeInfo)
    ATTRIBUTE_LABEL_SET(Type, ShapeTypes)
END_ATTRIBUTE_SET(ShapeInfo)

// Assign one per max file
BEGIN_ATTRIBUTE_SET(ModelInfo)
    ATTRIBUTE_LABEL_SET (Type, ModelTypes)
    ATTRIBUTE_FLOAT     (Weight, 1.0f, 0.0f, 1.0f, 0.1f)             // name, default, min, max, step
END_ATTRIBUTE_SET(ModelInfo)

// Assign one per node (bone or mesh)
BEGIN_ATTRIBUTE_SET(NodeInfo)
    ATTRIBUTE_LABEL_SET (Type, NodeTypes)
    ATTRIBUTE_BOOL      (NeverAnimate, 0)					  // name, default
    ATTRIBUTE_FLOAT     (AnimWeight, 1.0f, 0.0f, 1.0f, 0.1f)  // name, default, min, max, step
    ATTRIBUTE_BOOL      (DoNotDelete, 0)					  // name, default
END_ATTRIBUTE_SET(NodeInfo)

// Assign to objects linked to the hierarchy
BEGIN_ATTRIBUTE_SET(HotPointInfo)
    ATTRIBUTE_LABEL_SET (Type, HotPointTypes)
END_ATTRIBUTE_SET(HotPointInfo)

// Assign one per animation
BEGIN_ATTRIBUTE_SET(AnimInfo)
    ATTRIBUTE_LABEL_SET (Type,        AnimTypes)                     // name, set
    ATTRIBUTE_FLOAT     (Weight,      1.0f,  0.0f, 1.0f,  0.1f)      // name, default, min, max, step
    ATTRIBUTE_FLOAT     (Fps,         30.0f, 0.0f, 60.0f, 10.0f)     // name, default, min, max, step
    ATTRIBUTE_INT       (StartFrame,  0, 0, 10000, 1)                // name, default, min, max, step
    ATTRIBUTE_INT       (EndFrame,    0, 0, 10000, 1)                // name, default, min, max, step
    ATTRIBUTE_INT       (LoopToFrame, 0, 0, 10000, 1)                // name, default, min, max, step
    ATTRIBUTE_BOOL      (RemoveRootTranslation, 0)                   // name, default
    ATTRIBUTE_BOOL      (Additive, 0)                                // name, default
    ATTRIBUTE_BOOL      (Compress, 1)                                // name, default
    ATTRIBUTE_LABEL_SET (AdditiveRefType, AnimTypes)                 // name, set
END_ATTRIBUTE_SET(AnimInfo)




//=========================================================================
// Description for special case data
//=========================================================================

// Assign one per node (bone or mesh) for front end files
BEGIN_ATTRIBUTE_SET(GUIInfo)
    ATTRIBUTE_LABEL_SET (Type,      GUITypes)
    ATTRIBUTE_LABEL_SET (HitLeft,   NodeTypes)
    ATTRIBUTE_LABEL_SET (HitRight,  NodeTypes)
    ATTRIBUTE_LABEL_SET (HitUp,     NodeTypes)
    ATTRIBUTE_LABEL_SET (HitDown,   NodeTypes)
END_ATTRIBUTE_SET(GUIInfo)



//=========================================================================
// Description for events
//=========================================================================
BEGIN_ATTRIBUTE_SET(ParticleEvent)
    ATTRIBUTE_LABEL_SET (Type, ParticleTypes)
    ATTRIBUTE_FRAMES    (Frames)
END_ATTRIBUTE_SET(ParticleEvent)

BEGIN_ATTRIBUTE_SET(SoundEvent)
    ATTRIBUTE_LABEL_SET (Type, SoundTypes)
    ATTRIBUTE_FRAMES    (Frames)
    ATTRIBUTE_FLOAT     (Volume, 1.0f, 0.0f, 1.0f, 0.1f)      // name, default, min, max, step
END_ATTRIBUTE_SET(SoundEvent)

BEGIN_ATTRIBUTE_SET(DamageEvent)
    ATTRIBUTE_FRAMES    (Frames)
    ATTRIBUTE_BOOL      (IsMulti, 1)
    ATTRIBUTE_RADIUS    (Radius)
    ATTRIBUTE_FLOAT     (Damage, 10.0f, 0.0f, 100.0f, 1.0f)   // name, default, min, max, step
END_ATTRIBUTE_SET(DamageEvent)

BEGIN_ATTRIBUTE_SET(ScreenFlashEvent)
    ATTRIBUTE_FLOAT     (Red, 1.0f, 0.0f, 1.0f, 0.1)   // name, default, min, max, step
    ATTRIBUTE_FLOAT     (Green, 1.0f, 0.0f, 1.0f, 0.1)   // name, default, min, max, step
    ATTRIBUTE_FLOAT     (Blue, 1.0f, 0.0f, 1.0f, 0.1)   // name, default, min, max, step
    ATTRIBUTE_FLOAT     (Time, 1.0f, 0.0f, 1.0f, 0.1)   // name, default, min, max, step
END_ATTRIBUTE_SET(ScreenFlashEvent)




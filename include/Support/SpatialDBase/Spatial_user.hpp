// Imported genuine area51 pristine431f72b9 engine reference: Support/SpatialDBase/Spatial_user.hpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.

#ifndef SPATIAL_USER_HPP
#define SPATIAL_USER_HPP

#define NUM_SPATIAL_CHANNELS 2
#define SPATIAL_CHANNEL_GENERIC 0
#define SPATIAL_CHANNEL_LIGHTS 1

struct spacial_user {
    spacial_user(void) {
        for (s32 j = 0; j < NUM_SPATIAL_CHANNELS; j++) {
            FirstObjectLink[j] = -1; // LINK_NULL
        }
    }

    // This is the link_id of the object
    s32 FirstObjectLink[NUM_SPATIAL_CHANNELS];
};

#endif
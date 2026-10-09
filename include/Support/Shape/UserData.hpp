#ifndef __USER_DATA_HPP__
#define __USER_DATA_HPP__


#include <xCore/x_files/x_types.hpp>


//==============================================================================
// User data structures that can be attached to a node etc.
//==============================================================================

// Structure used for front end gui nodes
struct user_data_gui_info
{
    s32 Type ;        // GUI type (button, listbox, edit etc) - see GUITypes.hpp
    s32 HitLeft ;     // Node type when hit cursor left      
    s32 HitRight ;    // Node type when hit cursor right
    s32 HitUp ;       // Node type when hit cursor up
    s32 HitDown ;     // Node type when hit cursor down
} ;


// Structure used to store info about animation
struct user_data_anim_info
{
    s32     Type ;              // Animation type - see AnimTypes.hpp
    f32     Weight ;            // Weight used when selecting anim
    f32     Fps ;               // Playback rate in frames per second
    s32     StartFrame ;        // Start frame of animation
    s32     EndFrame ;          // End frame of animation
    s32     LoopToFrame ;       // Frame to jump to after playing
    xbool   RemoveRootTrans ;   // Remove root translation flag
    xbool   Additive ;          // Additive anim flag
    xbool   Compress ;          // Compression flag
    s32     AdditiveRefType ;   // Reference animation type for additive anims
} ;

//==============================================================================




#endif  //#ifndef __USER_DATA_HPP__

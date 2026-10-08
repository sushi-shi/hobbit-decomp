#ifndef __NODE_TYPES_H__
#define __NODE_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************



// Node type - assigned one per node for use in identifying model parts, gui etc
BEGIN_LABEL_SET(NodeTypes)
    LABEL(NODE_TYPE_NONE,                           "<un-assigned>"                     )
                                                    
    // Character body parts                         
    LABEL(NODE_TYPE_CHARACTER_HEAD,                 "Character - Head"                  )
    LABEL(NODE_TYPE_CHARACTER_NECK,                 "Character - Neck"                  )
                                                    
    LABEL(NODE_TYPE_CHARACTER_UPPER_TORSO,          "Character - Upper torso"           )
    LABEL(NODE_TYPE_CHARACTER_LOWER_TORSO,          "Character - Lower torso"           )
    LABEL(NODE_TYPE_CHARACTER_PELVIS,               "Character - Pelvis"                )
                                                    
    LABEL(NODE_TYPE_CHARACTER_LEFT_SHOULDER,        "Character - Left shoulder"         )
    LABEL(NODE_TYPE_CHARACTER_LEFT_UPPER_ARM,       "Character - Left upper arm"        )
    LABEL(NODE_TYPE_CHARACTER_LEFT_LOWER_ARM,       "Character - Left lower arm"        )
    LABEL(NODE_TYPE_CHARACTER_LEFT_HAND,            "Character - Left hand"             )
                                                    
    LABEL(NODE_TYPE_CHARACTER_LEFT_UPPER_LEG,       "Character - Left upper leg"        )
    LABEL(NODE_TYPE_CHARACTER_LEFT_LOWER_LEG,       "Character - Left lower leg"        )
    LABEL(NODE_TYPE_CHARACTER_LEFT_FOOT,            "Character - Left foot"             )
                                                                                        
    LABEL(NODE_TYPE_CHARACTER_RIGHT_SHOULDER,       "Character - Right shoulder"        )
    LABEL(NODE_TYPE_CHARACTER_RIGHT_UPPER_ARM,      "Character - Right upper arm"       )
    LABEL(NODE_TYPE_CHARACTER_RIGHT_LOWER_ARM,      "Character - Right lower arm"       )
    LABEL(NODE_TYPE_CHARACTER_RIGHT_HAND,           "Character - Right hand"            )
                                                                                        
    LABEL(NODE_TYPE_CHARACTER_RIGHT_UPPER_LEG,      "Character - Right upper leg"       )
    LABEL(NODE_TYPE_CHARACTER_RIGHT_LOWER_LEG,      "Character - Right lower leg"       )
    LABEL(NODE_TYPE_CHARACTER_RIGHT_FOOT,           "Character - Right foot"            )

    // GUI identifiers

    // Shared id's
    LABEL(NODE_TYPE_GUI_EXIT,                       "GUI - Exit"                        )
    LABEL(NODE_TYPE_GUI_OPTIONS,                    "GUI - Options"                     )
                                                                                        
    // Main screen id's                                                                 
    LABEL(NODE_TYPE_GUI_START_GAME,                 "GUI - Start game"                  )
    LABEL(NODE_TYPE_GUI_JOIN_GAME,                  "GUI - Join game"                   )
    LABEL(NODE_TYPE_GUI_HELP,                       "GUI - Help"                        )

    LABEL(NODE_TYPE_CHAINGUN_MUZZLE_FLASH,          "Weapon - Chaingun Muzzle Flash"    )

END_LABEL_SET(NodeTypes)


#endif	// __NODE_TYPES_H__

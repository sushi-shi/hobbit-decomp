#ifndef __MODEL_TYPES_H__
#define __MODEL_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************



// Model types - assigned one per model
BEGIN_LABEL_SET(ModelTypes)
    
    LABEL(MODEL_TYPE_NONE,              "<un-assigned>"     )
                                        
    // Normal LOD's                     
    LABEL(MODEL_TYPE_LOD1,              "LOD1"              )
    LABEL(MODEL_TYPE_LOD2,              "LOD2"              )
                                        
    // Damaged LOD's                    
    LABEL(MODEL_TYPE_DAMAGE1_LOD1,      "Damage 1 - LOD1"   )
    LABEL(MODEL_TYPE_DAMAGE1_LOD2,      "Damage 1 - LOD2"   )
    LABEL(MODEL_TYPE_DAMAGE2_LOD1,      "Damage 2 - LOD1"   )
    LABEL(MODEL_TYPE_DAMAGE2_LOD2,      "Damage 2 - LOD2"   )
                                        
    // Destroyed LOD's                  
    LABEL(MODEL_TYPE_DESTROYED_LOD1,    "Destroyed - LOD1"  )
    LABEL(MODEL_TYPE_DESTROYED_LOD2,    "Destroyed - LOD2"  )
                                        
    // Collision models                 
    LABEL(MODEL_TYPE_COLLISION1,        "Collision 1"       )
    LABEL(MODEL_TYPE_COLLISION2,        "Collision 2"       )

    // Misc models
    LABEL(MODEL_TYPE_CLIPPING,          "Clipping"          )
    LABEL(MODEL_TYPE_SHADOW,            "Shadow"            )


END_LABEL_SET(ModelTypes)


#endif	// __MODEL_TYPES_H__

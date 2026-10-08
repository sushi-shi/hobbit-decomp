#ifndef __GUI_TYPES_H__
#define __GUI_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************



// GUI type
BEGIN_LABEL_SET(GUITypes)
    LABEL(GUI_TYPE_NONE,        "<un-assigned>"	        )
    LABEL(GUI_TYPE_BUTTON,      "GUI_Button"            )        
    LABEL(GUI_TYPE_EDIT_BOX,    "GUI_Edit box"          )
    LABEL(GUI_TYPE_LIST_BOX,    "GUI_List box"          )
END_LABEL_SET(GUITypes)


#endif	// __GUI_TYPES_H__

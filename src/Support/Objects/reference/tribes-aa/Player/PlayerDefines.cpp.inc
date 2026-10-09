//==============================================================================
//
//  PlayerDefines.cpp
//
//==============================================================================



//==============================================================================
//  INCLUDES
//==============================================================================

#include "PlayerObject.hpp"
#include "Tokenizer/Tokenizer.hpp"


//==============================================================================
// gmae_key functions
//==============================================================================

// Functions
xbool player_object::game_key::IsPressed(s32 ControllerID) const
{
	// Allow null keys
    if (MainGadget == INPUT_UNDEFINED)
		return FALSE ;

    // Main gadget must be pressed
    if (!input_IsPressed(MainGadget, ControllerID))
        return FALSE ;

    // Is shift key required?
    if ( (ShiftGadget != INPUT_UNDEFINED) && (input_IsPressed(ShiftGadget,ControllerID) != ShiftState) )
        return FALSE ;

    // Success!
    return TRUE ;
}



//==============================================================================
// SET DEFAULTS MACROS
//==============================================================================

#define DEFINE_ATTRIBUTE_F32(__name__, __value__)                           \
        __name__ = (f32)(__value__) ;                                              

#define DEFINE_ATTRIBUTE_S32(__name__, __value__)                           \
        __name__ = (s32)(__value__) ;                                              
                                                                            
#define DEFINE_ATTRIBUTE_R32(__name__, __value__)                           \
        __name__ = (radian)((__value__) * R_1) ;                                        
                                                                            
#define DEFINE_ATTRIBUTE_GAME_KEY(__name__, __main_gadget__, __shift_gadget__, __shift_state__) \
        __name__.MainGadget  = __main_gadget__ ;                                                \
        __name__.ShiftGadget = __shift_gadget__ ;                                               \
        __name__.ShiftState  = __shift_state__ ;

#define DEFINE_ATTRIBUTE_BBOX(__name__, __width__, __height__, __length__)  \
        __name__.Min = vector3((f32)-(__width__)/2, 0, (f32)-(__length__)/2) ;            \
        __name__.Max = vector3((f32)(__width__)/2, (f32)(__height__), (f32)(__length__)/2) ;

#define DEFINE_ATTRIBUTE_V3(__name__, __x__, __y__, __z__)                  \
        __name__ = vector3((f32)__x__, (f32)__y__, (f32)__z__) ;

#define DEFINE_ATTRIBUTE_ARRAY_S32(__name__, __total__)

#define DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( __name__, __value__)              \
        __name__ = (__value__) ;

#define BEGIN_ENUMS( __name__ )
#define DEFINE_ENUM( __name__ )
#define END_ENUMS( __name__ )



//==============================================================================
// SET DEFAULTS FUNCTIONS
//==============================================================================

// Sets up defaults
void player_object::common_info::SetDefaults()
{
    // Setup all members
    #include "PlayerCommonInfo.hpp"
}

//==============================================================================

// Sets up defaults
void player_object::control_info::SetDefaults()
{
    // Setup all members
    #include "PlayerControlInfo.hpp"
}

//==============================================================================

// Sets up defaults
void player_object::move_info::SetDefaults()
{
    // Setup all members
    #include "PlayerMoveInfo.hpp"
}

//==============================================================================
// REMOVE MACROS
//==============================================================================

#undef DEFINE_ATTRIBUTE_F32
#undef DEFINE_ATTRIBUTE_S32
#undef DEFINE_ATTRIBUTE_R32
#undef DEFINE_ATTRIBUTE_GAME_KEY
#undef DEFINE_ATTRIBUTE_BBOX
#undef DEFINE_ATTRIBUTE_V3
#undef DEFINE_ATTRIBUTE_ARRAY_S32
#undef DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY
#undef BEGIN_ENUMS 
#undef DEFINE_ENUM
#undef END_ENUMS



//==============================================================================
// LOAD FROM FILE MACROS
//==============================================================================

#define DEFINE_ATTRIBUTE_F32(__name__, __value__)           \
    if (x_strcmp(Tok.String(), #__name__) == 0)             \
    {                                                       \
        Tok.Read() ;                                        \
        __name__ = Tok.ReadFloat() ;                        \
    }

#define DEFINE_ATTRIBUTE_S32(__name__, __value__)           \
    if (x_strcmp(Tok.String(), #__name__) == 0)             \
    {                                                       \
        Tok.Read() ;                                        \
        __name__ = Tok.ReadInt() ;                          \
    }

#define DEFINE_ATTRIBUTE_R32(__name__, __value__)           \
    if (x_strcmp(Tok.String(), #__name__) == 0)             \
    {                                                       \
        Tok.Read() ;                                        \
        __name__ = Tok.ReadFloat() * R_1 ;                  \
    }    

#define DEFINE_ATTRIBUTE_GAME_KEY(__name__, __main_gadget__, __shift_gadget__, __shift_state__) \
    if (x_strcmp(Tok.String(), #__name__) == 0)                                                 \
    {                                                                                           \
        Tok.Read() ;                                                                            \
        __name__.MainGadget = input_LookupGadget(Tok.ReadSymbol()) ;                            \
        Tok.Read() ;                                                                            \
        __name__.ShiftGadget = input_LookupGadget(Tok.ReadSymbol()) ;                           \
        Tok.Read() ;                                                                            \
        __name__.ShiftState  = Tok.ReadInt() ;                                                  \
    }

#define DEFINE_ATTRIBUTE_BBOX(__name__, __width__, __height__, __length__)  \
    if (x_strcmp(Tok.String(), #__name__) == 0)                             \
    {                                                                       \
        f32 Width, Height, Length ;                                         \
        Tok.Read() ;                                                        \
        Width  = Tok.ReadFloat() ; Tok.Read() ;                             \
        Height = Tok.ReadFloat() ; Tok.Read() ;                             \
        Length = Tok.ReadFloat() ; Tok.Read() ;                             \
        __name__.Min = vector3(-Width/2, 0, -Length/2) ;                    \
        __name__.Max = vector3(Width/2, Height, Length/2) ;                 \
    }

#define DEFINE_ATTRIBUTE_V3(__name__, __x__, __y__, __z__)      \
    if (x_strcmp(Tok.String(), #__name__) == 0)                 \
    {                                                           \
        Tok.Read() ;                                            \
        __name__.X = Tok.ReadFloat() ;  Tok.Read() ;            \
        __name__.Y = Tok.ReadFloat() ;  Tok.Read() ;            \
        __name__.Z = Tok.ReadFloat() ;  Tok.Read() ;            \
    }

#define DEFINE_ATTRIBUTE_ARRAY_S32(__name__, __total__)

#define DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( __name__, __value__)  \
    if (x_strcmp(Tok.String(), #__name__) == 0)                 \
    {                                                           \
        Tok.Read() ;                                            \
        __name__ = Tok.ReadInt() ;                              \
    }

#define BEGIN_ENUMS( __name__ )
#define DEFINE_ENUM( __name__ )
#define END_ENUMS( __name__ )



//==============================================================================
// LOAD FROM FILE FUNCTIONS
//==============================================================================

// Initializes structure from text file
xbool player_object::common_info::LoadFromFile(const char *Filename)
{
	token_stream	Tok ; 

    // Set delimiter string to leave [] intact!
    Tok.SetDelimiterString(",{}()<>;=") ;

    // Show info
    //x_DebugMsg("Loading player_common_info defines from %s\n", Filename);

	// Try open file
	if (!Tok.OpenFile(Filename))
	{
        // Fail
		ASSERT(0) ;
		return FALSE ;
	}

    // Read all tokens
	do
	{
		// Read next token
		Tok.Read() ;

        // Found a symbol?
        if (Tok.Type() == token_stream::TOKEN_SYMBOL)
        {
            // Check all struct members
            #include "PlayerCommonInfo.hpp"
        }
    }
	while(Tok.Type() != token_stream::TOKEN_EOF) ;

	// Cleanup
	Tok.CloseFile() ;

    // Success
    return TRUE ;
}

//==============================================================================

// Initializes structure from text file
xbool player_object::move_info::LoadFromFile(const char *Filename)
{
	token_stream	Tok ; 

    // Set delimiter string to leave [] intact!
    Tok.SetDelimiterString(",{}()<>;=") ;

    // Show info
    //x_DebugMsg("Loading player_move_info defines from %s\n", Filename);

	// Try open file
	if (!Tok.OpenFile(Filename))
	{
        // Fail
		ASSERT(0) ;
		return FALSE ;
	}

    // Read all tokens
	do
	{
		// Read next token
		Tok.Read() ;

        // Found a symbol?
        if (Tok.Type() == token_stream::TOKEN_SYMBOL)
        {
            // Check for initializing struct members
            #include "PlayerMoveInfo.hpp"
        }
    }
	while(Tok.Type() != token_stream::TOKEN_EOF) ;

	// Cleanup
	Tok.CloseFile() ;

    // Success
    return TRUE ;
}

//==============================================================================

// Initializes structure from text file
xbool player_object::control_info::LoadFromFile(const char *Filename)
{
	token_stream	Tok ; 

    // Set delimiter string to leave [] intact!
    Tok.SetDelimiterString(",{}()<>;=") ;

    // Show info
    //x_DebugMsg("Loading player_control_info defines from %s\n", Filename);

	// Try open file
	if (!Tok.OpenFile(Filename))
	{
        // Fail
		ASSERT(0) ;
		return FALSE ;
	}

    // Read all tokens
	do
	{
		// Read next token
		Tok.Read() ;

        // Found a symbol?
        if (Tok.Type() == token_stream::TOKEN_SYMBOL)
        {
            // Check for initializing struct members
            #include "PlayerControlInfo.hpp"
        }
    }
	while(Tok.Type() != token_stream::TOKEN_EOF) ;

	// Cleanup
	Tok.CloseFile() ;

    // Success
    return TRUE ;
}



//==============================================================================
// REMOVE MACROS
//==============================================================================

#undef DEFINE_ATTRIBUTE_F32
#undef DEFINE_ATTRIBUTE_S32
#undef DEFINE_ATTRIBUTE_R32
#undef DEFINE_ATTRIBUTE_GAME_KEY
#undef DEFINE_ATTRIBUTE_BBOX
#undef DEFINE_ATTRIBUTE_V3
#undef DEFINE_ATTRIBUTE_ARRAY_S32
#undef DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY
#undef BEGIN_ENUMS 
#undef DEFINE_ENUM
#undef END_ENUMS



//==============================================================================


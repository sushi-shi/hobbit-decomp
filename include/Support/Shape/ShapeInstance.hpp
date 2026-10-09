#ifndef __SHAPE_INSTANCE_HPP__
#define __SHAPE_INSTANCE_HPP__


#include <Support/Shape/Shape.hpp>

//==============================================================================
// Defines
//==============================================================================


class collider;


//==============================================================================
// Shape Instance - Instance of a shape
//==============================================================================
class shape_instance
{
// Defines
public:

    // List of flags
    enum flags
    {
        // Dirty flags (flag that something needs re-calculating)
        FLAG_L2W_DIRTY                  = (1<<0),   // Means local->world matrix is dirty
        FLAG_WORLD_BOUNDS_DIRTY         = (1<<1),   // Means world bounds are dirty
        FLAG_RENDER_NODES_DIRTY         = (1<<2),   // Means recalculate render nodes
        FLAG_ALWAYS_DIRTY               = (1<<3),   // Draw always calculates render nodes

        // Remove root node positions from anims
        FLAG_REMOVE_ROOT_NODE_POS_X     = (1<<4),
        FLAG_REMOVE_ROOT_NODE_POS_Y     = (1<<5),
        FLAG_REMOVE_ROOT_NODE_POS_Z     = (1<<6),
                                        
        // Anim control flags           
        FLAG_ENABLE_ANIM_BLENDING       = (1<<7),   // Enables playback blending
        FLAG_ENABLE_ADDITIVE_ANIMS      = (1<<8),   // Enables additive anims in playback
        FLAG_ENABLE_MASKED_ANIMS        = (1<<9),   // Enables masked anims in playback
                                        
        // Property flags               
        FLAG_IS_STATIC                  = (1<<10),  // Instance model has 1 node and no anims
    } ;

    // Number of anims allowed
    enum anim_defines
    {
        MAX_ADDITIVE_ANIMS = 4,
        MAX_MASKED_ANIMS   = 1,
        MAX_ANIMS          = (1 + 1 + MAX_ADDITIVE_ANIMS + MAX_MASKED_ANIMS) // 1+1 for current+blend
    } ;

// Structures
public:

    // Use to store/restore anim status
    struct anim_status
    {
	    anim_state	    CurrentAnim ;	      // Current anim playing
	    s32             CurrentAnimType ;     // Current anim type playing
        s32             RequestedAnimType ;   // Next anim to be played
        f32             RequestedBlendTime ;  // Next anim blend time
        f32             RequestedStartFrame ; // Next anim frame (0->1) to start at

        anim_state	    BlendAnim ;	          // Last anim that was playeed
        f32             Blend ;               // Blend value (0.0f = CurrentAnim, 1.0f = BlendAnim)
        f32             BlendInc ;            // Blend speed (1.0f / TotalBlendTime)
    } ;


// Members
private:
	
    // Orientation vars
    render_node     m_WorldRenderNode ;     // NOTE - Must be 16 byte aligned for PS2!
    bbox            m_WorldBounds ;         // Current world bounds

    // Current orientation and pos
    u32             m_Flags ;               // Misc flags
	vector3	        m_vPos,                 // World position
                    m_vScale ;              // World scale
    radian3         m_rRot ;                // Local rotation

	// Shape vars
	shape*          m_pShape ;             // Shape to display
    model*          m_pModel ;             // Model within shape to display
    model*          m_pCollisionModel ;    // Model to use for collision
                                        
    // Animation vars				    
	anim_state	    m_CurrentAnim ;	        // Current anim playing
	s32             m_CurrentAnimType ;     // Current anim type playing
    s32             m_RequestedAnimType ;   // Next anim to be played
    f32             m_RequestedBlendTime ;  // Next anim blend time
    f32             m_RequestedStartFrame ; // Next anim frame (0->1) to start at

    anim_state	    m_BlendAnim ;	        // Last anim that was playeed
    f32             m_Blend ;               // Blend value (0.0f = CurrentAnim, 1.0f = BlendAnim)
    f32             m_BlendInc ;            // Blend speed (1.0f / TotalBlendTime)

    anim_state      m_AdditiveAnims[MAX_ADDITIVE_ANIMS] ;	// Array of layered anims
    anim_state      m_MaskedAnims[MAX_MASKED_ANIMS] ;		// Array of masked anims

	// Scratch allocation vars
	render_node*    m_RenderNodes ;				// Contains nodes L2W matrix, light info etc. ready for rendering
    s32             m_RenderNodesAllocSize ;	// Contains size of data allocated
    s32             m_RenderNodesAllocID ;	    // Contains ID of scratch memory that was allocated

    // User functions
    void*           m_NodeFuncParam ;           // Passed to node and render node functions
    node_fn*        m_NodeFunc ;                // Called when creating node orientation
    render_node_fn* m_RenderNodeFunc ;          // Called when creating render node info

    // Parent instance vars (for connecting instance to another)
    shape_instance* m_ParentInstance ;          // Parent instance
    s32             m_ParentHotPointType ;      // Parent's hot point type attached to
    hot_point*      m_ParentHotPoint ;          // Parent's hot point attached to
    model*          m_ParentModel ;             // Parent's model attached to

	// Draw vars
	vector4			m_Color ;				    // Draw color multiplier
	vector4			m_SelfIllumColor ;		    // Self illum draw color multiplier
    xcolor          m_FogColor ;                // Fog color to use
    u32             m_NodeVisibleFlags ;        // 1 bit per node
    u32             m_MaterialVisibleFlags ;    // 1 bit per material

    // Animated texture vars
    f32                     m_TextureFrame ;            // Current texture frame
    f32                     m_TextureAnimFPS ;          // Texture anim speed playback (frames per sec)
    material::playback_type m_TexturePlaybackType ;     // Playback type


// Functions
public:

    // Default constructor
    shape_instance() ;

    // Construct from shape
	shape_instance(shape *pShape ) ;

    // Init shape to valid state
    void Init( void ) ;

    // Set new shape
    void SetShape(shape *pShape ) ;

    // Set new model
    void SetModel					(model* Model ) ;
    void SetModelByIndex			( s32    Index ) ;
    void SetModelByType				( s32    Type ) ;

    // Set new collision model
    void SetCollisionModel          (model* Model ) ;
    void SetCollisionModelByIndex   ( s32    Index ) ;
    void SetCollisionModelByType    ( s32    Type ) ;

    // Debug collision functions
    s32  DrawCollisionModel         ( void  ) ;

    // Collision functions
    void ApplyCollision             ( collider& Collider );
    void ApplyNodeBBoxCollision     ( collider& Collider );

    // Use to store/restore current animation playback
    void GetAnimStatus				(       anim_status& AnimStatus ) ;
    void SetAnimStatus				( const anim_status& AnimStatus ) ;

    // Set new anim
    //  Type       = anim type
    //  BlendTime  = time to blend to new anim
    //  StartFrame = start frame (0=1st frame of anim, 1=last frame of anim)
    //  ForceBlend = blends to new anim even if the same type is already playing
    void SetAnimByType  ( s32      Type, 
                          f32      BlendTime  = 0, 
                          f32      StartFrame = 0,
                          xbool    ForceBlend = FALSE ) ;
    void SetAnimByIndex( s32 Index, f32 BlendTime=0.0f ) ;
    s32  GetAnimByType ( void ) { return m_CurrentAnimType ; }

    // Returns TRUE if instance is blending between anims
    xbool IsBlending    ( void )  { return m_Blend != 0.0f ; }

    // Returns blending amount (0=all current, 1=all blend)
    f32  GetBlend       ( void )  { return m_Blend ; }

private:
    // Clears masked/additive anims list
    void        ClearSpecialAnims	( void ) ;

    // Advances all active masked/additive anims
    void        AdvanceSpecialAnims( f32 DeltaTime ) ;

    // Quickly builds linked list of active masked/additive anims
    // (returns head of list, or NULL if none)
    anim_state* BuildSpecialAnimList( anim_state* List, s32 Max ) ;

public:
    // Set additive animation by type
    void		SetAdditiveAnimByType	( s32 ID, s32 Type ) ;

    // Creates and adds a new additive anim state
    anim_state*	AddAdditiveAnim			( s32 ID ) ;

    // Deletes additive animtion 
    void		DeleteAdditiveAnim		( s32 ID ) ;

    // Removes all additive anims
    void        DeleteAllAdditiveAnims  ( void ) ;

    // Returns state of additive animation (if active)
    anim_state*	GetAdditiveAnimState	( s32 ID ) ;


    // Set masked animation by type
    void		SetMaskedAnimByType		( s32 ID, s32 Type ) ;

    // Creates and adds a new masked anim state
    anim_state*	AddMaskedAnim			( s32 ID ) ;

    // Deletes masked animtion 
    void		DeleteMaskedAnim		( s32 ID ) ;

    // Removes all masked anims
    void        DeleteAllMaskedAnims    ( void ) ;

    // Returns state of masked animation (if active)
    anim_state*	GetMaskedAnimState		( s32 ID ) ;

    // Flag setting functions
    inline void SetFlag(u32 Flag, xbool Value)
    {
        // Set?
        if (Value)
            m_Flags |= Flag ;
        else
            m_Flags &= ~Flag ;
    }

    void SetRemoveRootNodePosX  (xbool Value) { SetFlag(FLAG_REMOVE_ROOT_NODE_POS_X, Value) ; }
    void SetRemoveRootNodePosY  (xbool Value) { SetFlag(FLAG_REMOVE_ROOT_NODE_POS_Y, Value) ; }
    void SetRemoveRootNodePosZ  (xbool Value) { SetFlag(FLAG_REMOVE_ROOT_NODE_POS_Z, Value) ; }
    void SetEnableAnimBlending  (xbool Value) { SetFlag(FLAG_ENABLE_ANIM_BLENDING,   Value) ; }
    void SetEnableAdditiveAnims (xbool Value) { SetFlag(FLAG_ENABLE_ADDITIVE_ANIMS,  Value) ; }
    void SetEnableMaskedAnims   (xbool Value) { SetFlag(FLAG_ENABLE_MASKED_ANIMS,    Value) ; }

    // Returns current local->world matrix
    matrix4			GetL2W					( void ) ;

    // Returns world bounds shape_instance
    bbox			GetWorldBounds			( void ) ;

    // Returns tight bounds of instance (use on static instances eg. scenic)
    // You should only call this once when game loads
    // since it pushes all the verts through the node matrices ie. it's slow!
    bbox            GetTightWorldBounds    ( void ) ;

    // Should be called before using any data in the world render node
    void			UpdateWorldRenderNode	( void ) ;

	// Calculates render nodes (which contain matrices and lights for each node)
    // ready for drawing shape_instance
    // Returns pointer to list if successful, otherwise NULL
	render_node*	CalculateRenderNodes	( void ) ;

    // Calculates simple linear fog. Thickess is amount of view volume (0->1) to fog
    f32				CalculateLinearFog		(f32 Thickness ) ;

    // Attach to another instances hot point
    void SetParentInstance					(shape_instance *Instance, s32 HotPointType ) ;
    shape_instance *GetParentInstance( void ) { return m_ParentInstance ; }

	// Draw functions

    // Color functions
    void        SetColor                        ( const vector4& Col ) ;
	void        SetColor                        ( const xcolor&  Col ) ;
    xcolor      GetColor                        ( void ) const;

    // SelfIllumColor functions
    void        SetSelfIllumColor               ( const vector4& Col ) ;
	void        SetSelfIllumColor               ( const xcolor&  Col ) ;
    xcolor      GetSelfIllumColor               ( void ) const;

    // Fog functions                            
	void        SetFogColor                     ( const xcolor& Col ) ;
    xcolor      GetFogColor                     ( void ) const;
                                                
    // Light functions                          
    void        SetLightDirection               ( const vector3& Dir, xbool Normalize=FALSE ) ;
    vector3     GetLightDirection               ( void ) const;
                                                
    void        SetLightColor                   ( const xcolor&  Col ) ;
    void        SetLightColor                   ( const vector4& Col ) ;
    vector4     GetLightColor                   ( void ) const;
                                                
    void        SetLightAmbientColor            ( const xcolor&  Col ) ;
    void        SetLightAmbientColor            ( const vector4& Col ) ;
    vector4     GetLightAmbientColor            ( void ) const;

    // Node visible functions
    void        SetNodeVisibleFlagByIndex       ( s32 NodeIndex, xbool Value ) ;
    xbool       GetNodeVisibleFlagByIndex       ( s32 NodeIndex ) ;
                                                
    void        SetNodeVisibleFlagByType        ( s32 NodeType,  xbool Value ) ;
    xbool       GetNodeVisibleFlagByType        ( s32 NodeType ) ;

    // Material visible functions
    void        SetMaterialVisibleFlagByIndex   ( s32 MaterialIndex, xbool Value ) ;
    xbool       GetMaterialVisibleFlagByIndex   ( s32 MaterialIndex ) ;
                
    void        SetMaterialVisibleFlagByName    ( const char *Name,  xbool Value ) ;
    xbool       GetMaterialVisibleFlagByName    ( const char *Name ) ;

    // Returns pointer to give hot point of the current displayed model if found, or NULL 
    hot_point*  GetHotPointByType               ( s32 Type ) ;
    hot_point*  GetHotPointByIndex              ( s32 Index ) ;

    // Returns local to world matrix of requested hot point
    matrix4     GetHotPointL2W                  ( hot_point *HotPoint ) ;
    matrix4     GetHotPointL2W                  ( s32 Type ) ;
    matrix4     GetHotPointByIndexL2W           ( s32 Index ) ;

    // Returns world position of requested hot point
    vector3     GetHotPointWorldPos             ( hot_point *HotPoint ) ;
    vector3     GetHotPointWorldPos             ( s32 Type ) ;
    vector3     GetHotPointByIndexWorldPos      ( s32 Index ) ;

    // Draw bounds, hot points, events etc
    void DrawFeatures( void ) ;

	// Draws shape_instance (assumes matrices and verts are already in world space)
    //
    // Flags:   shape::DRAW_FLAG_CLIP            = Use clipping vu code (slower)
    //          shape::DRAW_FLAG_REF_WORLD_SPACE = Reflection maps in world space (instead of view space)
    //          shape::DRAW_FLAG_FOG             = Draw with fog
    //
    // Returns TRUE if successful, otherwise FALSE
    //

    // Draw (with optional detail texture. eg. use for damage, and special fx material render function)
    xbool       Draw    ( u32                 Flags, 
                          texture*            DetailTexture       = NULL, 
                          s32                 DetailClutHandle    = -1,
                          render_material_fn* RenderMaterialFunc  = NULL,
                          void*               RenderMaterialParam = NULL ) ;

	// Moves and animates shape_instance ready for next frame
	void Advance(float DeltaTime, vector3* DeltaPos = NULL, quaternion* DeltaRot = NULL ) ;

    // Inline functions
    shape       *GetShape()                         { return m_pShape ;     }
    model       *GetModel()                         { return m_pModel ;     }
                                                    
    // Animation functions                          
    anim_state  &GetCurrentAnimState()              { return m_CurrentAnim ;}
    anim_state  &GetBlendAnimState()                { return m_BlendAnim ;  }

    // Orientation functions
    vector3     GetPos      ( void ) const          { return m_vPos ;       }
    vector3     GetScale    ( void ) const          { return m_vScale ;     }
    radian3     GetRot      ( void ) const          { return m_rRot ;       }

    void        SetPos		( const vector3& vPos   ) ;
    void        SetScale	( const vector3& vScale ) ;
    void        SetRot		( const radian3& rRot   ) ;

    // Node user functions
    void        SetNodeFuncParam   ( void *P)               { m_NodeFuncParam   = P ; }
    void        SetNodeFunc        ( node_fn *F)            { m_NodeFunc        = F ; }
    void        SetRenderNodeFunc  ( render_node_fn *F)     { m_RenderNodeFunc  = F ; }

    // Material draw functions
    f32         GetTextureFrame( void )                     { return m_TextureFrame ;  }
    void        SetTextureFrame(f32 Frame)                  { m_TextureFrame = Frame ; }
                                                            
    f32         GetTextureAnimFPS( void )                   { return m_TextureAnimFPS ;      }
    void        SetTextureAnimFPS(f32 AnimSpeed)            { m_TextureAnimFPS = AnimSpeed ; }

    material::playback_type GetTexturePlaybackType()                                { return m_TexturePlaybackType ; }
    void                    SetTexturePlaybackType(material::playback_type Type)    { m_TexturePlaybackType = Type ; }

} PS2_ALIGNMENT(16);

//==============================================================================


#endif	//#define __MODEL_H__

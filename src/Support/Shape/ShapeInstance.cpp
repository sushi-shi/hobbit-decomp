// ShapeInstance.cpp
//

#include <xCore/Entropy/Entropy.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include <Support/ObjectMgr/Collider.hpp>



//==============================================================================
// Debug defines
//==============================================================================

//#define DEBUG_PLAYER
#ifdef DEBUG_PLAYER

    #include "../../AADS/globals.hpp"
    #include "LabelSets/Tribes2Types.hpp"
    #undef BEGIN_LABEL_SET
    #undef LABEL
    #undef END_LABEL_SET
    // Begins enum set
    #define BEGIN_LABEL_SET(__name__)   const char* __name__##Strings[] = {

    // Adds a label to enum set
    #define LABEL(__name__, __desc__)   __desc__,

    // Ends enum set
    #define END_LABEL_SET(__name__) } ;

    #undef __SHAPE_TYPES_H__
    #include "LabelSets/ShapeTypes.hpp"

#endif



//==============================================================================
// Defines
//==============================================================================

#define PS2_USE_VU0




//==============================================================================
// Instance
//==============================================================================

// Construct with NULL shape
shape_instance::shape_instance()
{
    // Init to valid state
    Init() ;
}

// Construct from shape
shape_instance::shape_instance(shape *pShape)
{
    // Init to valid state
    Init() ;

    // Use shape
    SetShape(pShape) ;
}

//==============================================================================

// Init to valid state (sets shape and model to null)
void shape_instance::Init()
{
    s32 i ;

	// Shape vars
	m_pShape          = NULL ;              // Shape to display
    m_pModel          = NULL ;              // Model within shape to display
    m_pCollisionModel = NULL ;              // Model to use for collision
                                                 
    // Animation vars	
    m_CurrentAnim.SetAnim(NULL) ;           // Clear animation
    m_CurrentAnimType = -1 ;                // Current anim type playing
    
    m_RequestedAnimType = -1 ;              // Next anim to be played
    m_RequestedBlendTime = 0 ;              // Next anim blend time
    m_RequestedStartFrame = 0 ;             // Next anim start frame to use

    m_Blend    = 0.0f ;                     // Blend value (0.0f = CurrentAnim, 1.0f = BlendAnim)
    m_BlendInc = 0.0f ;                     // Blend speed (1.0f / TotalBlendTime)
   
    // Clear additive anims
    for (i = 0 ; i < MAX_ADDITIVE_ANIMS ; i++)
        m_AdditiveAnims[i].SetID(-1) ;

    // Clear masked anims
    for (i = 0 ; i < MAX_MASKED_ANIMS ; i++)
        m_MaskedAnims[i].SetID(-1) ;

    // Setup default flags
    m_Flags =   FLAG_L2W_DIRTY              |
                FLAG_WORLD_BOUNDS_DIRTY     |
                FLAG_RENDER_NODES_DIRTY     |
                FLAG_ENABLE_ANIM_BLENDING   |
                FLAG_ENABLE_ADDITIVE_ANIMS  |
                FLAG_ENABLE_MASKED_ANIMS    ;

	// Orientation vars				    
    m_vPos   = vector3(0.0f, 0.0f, 0.0f) ;  // World position
    m_vScale = vector3(1.0f, 1.0f, 1.0f) ;  // World scale
    m_rRot   = radian3(0.0f, 0.0f, 0.0f) ;  // Local rotation

    // Setup world render node and bounds
    m_WorldRenderNode.L2W.Identity() ;
    SetLightDirection   (shape::GetLightDirection()) ;
    SetLightColor       (shape::GetLightColor()) ;
    SetLightAmbientColor(shape::GetLightAmbientColor()) ;
    m_WorldBounds.Clear() ;

	// Scratch allocation vars
	m_RenderNodes           = NULL ;		        // Contains nodes L2W matrix, light info etc. ready for rendering
    m_RenderNodesAllocSize  = 0 ;	                // Contains size of data allocated
    m_RenderNodesAllocID    = 0 ;	                // Contains ID of scratch memory that was allocated

    // User functions
    m_NodeFuncParam         = NULL ;               // Passed to node and render node functions
    m_NodeFunc              = NULL ;               // Called when creating node orientation
    m_RenderNodeFunc        = NULL ;               // Called when creating render node info

    // Parent instance vars (for connecting instance to another)
    m_ParentInstance        = NULL ;
    m_ParentHotPointType    = -1 ;
    m_ParentHotPoint        = NULL ;
    m_ParentModel           = NULL ;

	// Draw vars
	m_Color                 = vector4(1,1,1,1) ;			    // Draw color multiplier
	m_SelfIllumColor        = vector4(1,1,1,1) ;			    // Self illum draw color multiplier
    m_FogColor              = shape::GetFogColor() ;            // Default to no fog        

    // Default to all materials and nodes visible
    m_NodeVisibleFlags      = 0xFFFFFFFF ;  // All nodes visible
    m_MaterialVisibleFlags  = 0xFFFFFFFF ;  // All materials visible

    // Animated texture vars
    m_TextureFrame          = 0 ;                               // Current texture frame
    m_TextureAnimFPS        = 30 ;                              // Texture anim speed playback
    m_TexturePlaybackType   = material::PLAYBACK_TYPE_LOOP ;    // Playback type
}

//==============================================================================

// Set new shape
void shape_instance::SetShape(shape *pShape)
{
    // Update shape?
    if (m_pShape != pShape)
    {
        // Keep shape (if any)
        m_pShape  = pShape ;

        // World bounds may change
        m_Flags |= FLAG_WORLD_BOUNDS_DIRTY ;

        // Setup default model and anim if shape is there
        if (pShape)
        {
            // Use first model in shape
            SetModelByIndex(0) ;
            SetCollisionModelByIndex(0) ;

	        // Use first anim in shape
	        if ( pShape->GetAnimCount() )
		        m_CurrentAnim.SetAnim(pShape->GetAnimByIndex(0)) ;
            else
                m_CurrentAnim.SetAnim(NULL) ;

            // Can't blend between different shapes!
            m_Blend = 0.0f ;
        }
        else
        {
            // Clear model
            m_pModel          = NULL ;
            m_pCollisionModel = NULL ;

            // Clear anim
            m_CurrentAnim.SetAnim(NULL) ;
            m_Blend = 0.0f ;
        }

        // Clear additive and masked anims
        ClearSpecialAnims() ;
        
        // Make sure node matrices are re-calculated!
        m_RenderNodes = NULL ;
    }
}

//==============================================================================

// Set new model
void shape_instance::SetModel(model* Model)
{
    // Keep pointer
    ASSERT(m_pShape) ;
    ASSERT(Model) ;

    // Change to new model?
    if (m_pModel != Model)
    {
        // Keep new model
        m_pModel = Model ;

        // Make sure node matrices are re-calculated!
        m_RenderNodes = NULL ;

        // World bounds may change
        m_Flags |= FLAG_WORLD_BOUNDS_DIRTY ;
    }
}

//==============================================================================

// Set new model by index
void shape_instance::SetModelByIndex(s32 Index)
{
    ASSERT(m_pShape) ;
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_pShape->GetModelCount()) ;

    SetModel(m_pShape->GetModelByIndex(Index)) ;
}

//==============================================================================

// Set new model by type
void shape_instance::SetModelByType(s32 Type)
{
    ASSERT(m_pShape) ;

    // Search for new model
    model *Model = m_pShape->GetModelByType(Type) ;
    ASSERT(Model) ;

    // Set it
    SetModel(Model) ;
}

//==============================================================================

// Set new collision model
void shape_instance::SetCollisionModel(model* Model)
{
    // Keep pointer
    ASSERT(Model) ;
    m_pCollisionModel = Model ;
}

//==============================================================================

// Set new collision model by index
void shape_instance::SetCollisionModelByIndex(s32 Index)
{
    ASSERT(m_pShape) ;
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_pShape->GetModelCount()) ;

    SetCollisionModel(m_pShape->GetModelByIndex(Index)) ;
}

//==============================================================================

// Set new collision model by type
void shape_instance::SetCollisionModelByType(s32 Type)
{
    ASSERT(m_pShape) ;

    // Search for new model
    model *Model = m_pShape->GetModelByType(Type) ;
    ASSERT(Model) ;

    // Set it
    SetCollisionModel(Model) ;
}

//==============================================================================

#define SHOW_NORMALS

// Draws the currently associated collision model
s32 shape_instance::DrawCollisionModel()
{
    random Random;
    s32 Polys=0;

    // Any collision model there?
    model* Model = m_pCollisionModel ;
    if (!Model)
        return 0 ;

#ifdef SHOW_NORMALS
    #define MAX_SHAPE_NORMALS   4096
    #define MAX_BBOX_PARTS      128

    bbox BBoxParts[MAX_BBOX_PARTS];
    static vector3 DrawNormal[MAX_SHAPE_NORMALS];
    s32 NNormals=0;
    s32  NBBoxes=0;
#endif

    #ifdef TARGET_PC
        // No culling (strips don't care)
		g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) ;
    #endif

    // Get world bounds that contains all geometry - could be used for culling
    bbox WorldBBox = GetWorldBounds() ;
    (void)WorldBBox ;

    // Calc node matrices
    render_node* RenderNodes = CalculateRenderNodes() ;

    // Since verts are scaled up on the PS2, we must scale them back down
    f32 InvScale = 1.0f / Model->GetPrecisionScale() ;

    // Loop through all materials in model
    draw_Begin(DRAW_TRIANGLES,DRAW_USE_ALPHA) ;
    //draw_Begin(DRAW_TRIANGLES) ;

    for (s32 cMat = 0 ; cMat < Model->GetMaterialCount() ; cMat++)
    {
        material& Mat = Model->GetMaterial(cMat) ;

        // Skip punch through materials
        if (!(Mat.GetPunchThroughFlag()))
        {
            // Loop through all display lists in material
            for (s32 cDList = 0 ; cDList < Mat.GetDListCount() ; cDList++)
            {
                material_dlist& MatDList = Mat.GetDList(cDList) ;

                // Get local to world matrix for this dlist
                matrix4 DListL2W = RenderNodes[MatDList.GetNode()].L2W ;

                // Calc world bounds of this dlist - could be used for culling
                bbox    DListWorldBBox = Model->GetNode(MatDList.GetNode()).GetBounds() ;
                DListWorldBBox.Transform(DListL2W) ;

                // Add bbox to draw list
                #ifdef SHOW_NORMALS
                if( NBBoxes < MAX_BBOX_PARTS )
                {
                    BBoxParts[NBBoxes] = DListWorldBBox;
                    NBBoxes++;
                }
                #endif

                // Scale verts down
                DListL2W.PreScale(InvScale) ;

                // Loop through all strips in dlist
                for (s32 cStrip = 0 ; cStrip < MatDList.GetStripCount() ; cStrip++)
                {
                    dlist_strip& DListStrip = MatDList.GetStrip(cStrip) ;

                    // Loop through all polys in strip
                    for (s32 cPoly = 0 ; cPoly < DListStrip.GetPolyCount() ; cPoly++)
                    {
                        vector3 Verts[3] ;
            
                        // Grab poly and draw poly if it kicks a draw
                        if (DListStrip.GetPoly(cPoly, Verts))
                        {
                            Polys++ ;

                            xcolor C = Random.color();
                            C.A = 128;
                            draw_Color( C ) ;

                            // Draw poly
                            for (s32 cVert = 0 ; cVert < 3 ; cVert++)
                            {
                                // Put vert into world space
                                Verts[cVert] = DListL2W * Verts[cVert] ;

                                // Draw!
                                draw_Vertex( Verts[cVert] ) ;
                            }

                            #ifdef SHOW_NORMALS
                            // Add normal to draw list
                            if( NNormals < MAX_SHAPE_NORMALS )
                            {
                                plane P(Verts[0],Verts[1],Verts[2]);
                                DrawNormal[NNormals*2+0] = (Verts[0]+Verts[1]+Verts[2])/3.0f;
                                DrawNormal[NNormals*2+1] = DrawNormal[NNormals*2+0] + P.Normal*0.25f;
                                NNormals++;
                            }
                            #endif
                        }
                    }
                }
            }
        }
    }
    draw_End() ;

#ifdef SHOW_NORMALS
    // Draw bboxes
    for( s32 i=0; i<NBBoxes; i++ )
        draw_BBox( BBoxParts[i], Random.color() );

    // Draw normals
    {
        draw_ClearL2W();
        draw_Begin(DRAW_LINES) ;
        draw_Color(XCOLOR_WHITE);
        for( s32 i=0; i<NNormals; i++ )
        {
            draw_Vertex( DrawNormal[i*2+0] );
            draw_Vertex( DrawNormal[i*2+1] );
        }
        draw_End();
        //x_printf("%1d\n",NNormals);
    }
#endif

    #ifdef TARGET_PC
	    // Turn culling back on
		g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
    #endif

    return Polys;
}

//==============================================================================

void shape_instance::ApplyCollision( collider& Collider )
{
    //xtimer Time;
    //Time.Start();

    // Early out?
    if (Collider.GetEarlyOut())
        return ;

    // Clear stats
    s32 BBoxesConsidered=0;
    s32 BBoxesSent=0;
    s32 TrisConsidered=0;
    s32 TrisSent=0;
    xbool HasScale;

    // Any collision model there?
    model* pModel = m_pCollisionModel ;
    if (!pModel)
        return ;

    // Calc node matrices etc
    render_node* RenderNodes = CalculateRenderNodes() ;
    if (!RenderNodes)
        return ;

    // Get movement bounds
    const bbox& CBBox = Collider.GetVolumeBBox();

    // Decide if model is using scale at all
    HasScale = (m_vScale.X!=1.0f) || (m_vScale.Y!=1.0f) || (m_vScale.Z!=1.0f);

    // Since verts are scaled up on the PS2, we must scale them back down
    f32 Scale    = pModel->GetPrecisionScale() ;
    f32 InvScale = 1.0f /  Scale;

    // Loop through all materials in model
    for (s32 cMat = 0 ; cMat < pModel->GetMaterialCount() ; cMat++)
    {
        material& Mat = pModel->GetMaterial(cMat) ;

        // Ignore punch through materials
        if (!Mat.GetPunchThroughFlag())
        {
            // Loop through all display lists in material
            for (s32 cDList = 0 ; cDList < Mat.GetDListCount() ; cDList++)
            {
                // Get material dlist
                material_dlist& MatDList = Mat.GetDList(cDList) ;

                // Get node info
                s32     NodeIndex = MatDList.GetNode() ;
                node&   Node      = pModel->GetNode(NodeIndex) ;
                matrix4 DListL2W  = RenderNodes[NodeIndex].L2W ;
                BBoxesConsidered++;

                // Optimization:
                // Only check for skipping node if there's more than one,
                // since if there's just one node - the bounds of it will be the
                // same as the instance bounds - and we wouldn't have even got
                // in here if the instance bounds didn't overlap the movement bounds!
                if (pModel->GetNodeCount() > 1)
                {
                    // Calc world bounds of this dlist
                    bbox    DListWorldBBox = Node.GetBounds() ;
                    DListWorldBBox.Transform(DListL2W) ;

                    // Skip node if bounds doesn't intersect movement
                    if (!CBBox.Intersect( DListWorldBBox ))
                        continue ;
                }

                // Okey dokey, lets check the strip polys
                BBoxesSent++;

                // Tell collider the current node
                Collider.SetNodeIndex(NodeIndex) ;
                Collider.SetNodeType (Node.GetType()) ;

                // Compute a bbox for collider in local space of node
                matrix4 M = DListL2W;
                if( HasScale ) M.InvertSRT();
                else           M.InvertRT();
                bbox LocalCBBox = CBBox;
                LocalCBBox.Transform( M );
                LocalCBBox.Min *= Scale;
                LocalCBBox.Max *= Scale;

                #ifdef TARGET_PS2
                    // Expand float bbox to next integer since verts are integer on PS2
                    dlist_strip::ibbox ILocalCBBox ;
                    ILocalCBBox.Min.X = (s32)x_floor(LocalCBBox.Min.X) ;
                    ILocalCBBox.Min.Y = (s32)x_floor(LocalCBBox.Min.Y) ;
                    ILocalCBBox.Min.Z = (s32)x_floor(LocalCBBox.Min.Z) ;
                    ILocalCBBox.Max.X = (s32)x_ceil(LocalCBBox.Max.X) ;
                    ILocalCBBox.Max.Y = (s32)x_ceil(LocalCBBox.Max.Y) ;
                    ILocalCBBox.Max.Z = (s32)x_ceil(LocalCBBox.Max.Z) ;
                #endif

                // Scale verts down
                DListL2W.PreScale(InvScale) ;

                // Loop through all strips in dlist
                for (s32 cStrip = 0 ; cStrip < MatDList.GetStripCount() ; cStrip++)
                {
                    dlist_strip& DListStrip = MatDList.GetStrip(cStrip) ;

                    // Update stats
                    TrisConsidered += DListStrip.GetPolyCount();

                    // Integer verts version
                    #ifdef TARGET_PS2
                        TrisSent += DListStrip.ApplyCollision( Collider, DListL2W, ILocalCBBox ) ;
                    #endif

                    // Floating point verts version
                    #if defined(TARGET_PC)||defined(TARGET_LINUX)
                        TrisSent += DListStrip.ApplyCollision( Collider, DListL2W, LocalCBBox ) ;
                    #endif

                    // Early out?
                    if (Collider.GetEarlyOut())
                        return ;
                }
            }
        }
    }

    //Time.Stop();
/*
    x_DebugMsg("ShapeColl BB(%3d %3d) TRI(%3d %3d) %1.4f\n",
        BBoxesConsidered,
        BBoxesSent,
        TrisConsidered,
        TrisSent,
        Time.ReadMs());
*/
}

//==============================================================================

void shape_instance::ApplyNodeBBoxCollision( collider& Collider )
{
    // Indices used to convert min + max of bbox into 8 corners
    static s32 CornerIndices[8*3]   = { 0,1,2, 
                                        3,1,2,
                                        0,4,2,
                                        3,4,2,
                                        0,1,5, 
                                        3,1,5,
                                        0,4,5,
                                        3,4,5 } ;

    // Indices used to convert 8 corners into a side NGon
    static s32 SideIndices[6*4]     = { 0,2,3,1,
                                        1,3,7,5,
                                        5,7,6,4,
                                        4,6,2,0,
                                        2,6,7,3,
                                        4,0,1,5 } ;

    // Locals
    s32             i, j ;
    model*          Model ;
    render_node*    RenderNodes ;   
    bbox            MovementBBox, NodeLocalBBox, NodeWorldBBox ;
    vector3         Corners[8] ;
    s32*            Indices ;
    f32*            BBoxF ;
    vector3         Side[4] ;

    // Early out?
    if (Collider.GetEarlyOut())
        return ;

    // Any model there?
    Model = m_pModel ;
    if (!Model)
        return ;

    // Overlapping instance?
    MovementBBox = Collider.GetVolumeBBox() ;
    if (!MovementBBox.Intersect(GetWorldBounds()))
        return ;

    // Calc render nodes
    RenderNodes = CalculateRenderNodes() ;
    if (!RenderNodes)
        return ;

    // Loop through all nodes
    for (i = 0 ; i < Model->GetNodeCount() ; i++)
    {
        // Get node
        node& Node = Model->GetNode(i) ;

        // Skip if node is not visible
        if ((i < 32) && (!GetNodeVisibleFlagByIndex(i)))
            continue ;

        // Calc world bounds of node
        NodeLocalBBox = Node.GetBounds() ;
        NodeWorldBBox = NodeLocalBBox ;
        NodeWorldBBox.Transform(RenderNodes[i].L2W) ;

        // Overlap movement?
        if (MovementBBox.Intersect(NodeWorldBBox))
        {
            // Tell collider the current node
            Collider.SetNodeIndex(i) ;
            Collider.SetNodeType (Node.GetType()) ;

            // Transform all corners of the local AA bbox into world space
            Indices = CornerIndices ;
            BBoxF   = &NodeLocalBBox.Min.X ;
            for (j = 0 ; j < 8 ; j++)
            {
                // Setup corner in local space
                Corners[j].X = BBoxF[*Indices++] ;
                Corners[j].Y = BBoxF[*Indices++] ;
                Corners[j].Z = BBoxF[*Indices++] ;

                // Transform into world space
                Corners[j] = RenderNodes[i].L2W * Corners[j] ;
            }

            // Check 6 sides of bbox
            Indices = SideIndices ;
            for (j = 0 ; j < 6 ; j++)
            {
                // Setup side of bbox
                Side[0] = Corners[*Indices++] ;
                Side[1] = Corners[*Indices++] ;
                Side[2] = Corners[*Indices++] ;
                Side[3] = Corners[*Indices++] ;

                // Check for collision with this side
                Collider.ApplyNGon(Side, 4) ;

                // Early out?
                if (Collider.GetEarlyOut())
                    return ;
            }
        }
    }
}

//==============================================================================

// Use to store/restore current animation playback
void shape_instance::GetAnimStatus( anim_status& AnimStatus )
{
    // Copy all important anims vars
	AnimStatus.CurrentAnim          = m_CurrentAnim ;
	AnimStatus.CurrentAnimType      = m_CurrentAnimType ;
    AnimStatus.RequestedAnimType    = m_RequestedAnimType ;
    AnimStatus.RequestedBlendTime   = m_RequestedBlendTime ;
    AnimStatus.RequestedStartFrame  = m_RequestedStartFrame ;
    AnimStatus.BlendAnim            = m_BlendAnim ;
    AnimStatus.Blend                = m_Blend ;
    AnimStatus.BlendInc             = m_BlendInc ;
}

//==============================================================================

void shape_instance::SetAnimStatus   ( const anim_status& AnimStatus )
{
   // Copy all important anims vars
	m_CurrentAnim                   = AnimStatus.CurrentAnim ;
	m_CurrentAnimType               = AnimStatus.CurrentAnimType ;
    m_RequestedAnimType             = AnimStatus.RequestedAnimType ;
    m_RequestedBlendTime            = AnimStatus.RequestedBlendTime ;
    m_RequestedStartFrame           = AnimStatus.RequestedStartFrame ;
    m_BlendAnim                     = AnimStatus.BlendAnim ;
    m_Blend                         = AnimStatus.Blend ;
    m_BlendInc                      = AnimStatus.BlendInc ;
}

//==============================================================================


// Set new anim
void shape_instance::SetAnimByType(s32      Type, 
                                   f32      BlendTime  /* =0 */, 
                                   f32      StartFrame /* =0 */,
                                   xbool    ForceBlend /* =FALSE */)
{
    ASSERT(m_pShape) ;

    // Snap straight to anim?
    if ( (BlendTime == 0) || (!(m_Flags & FLAG_ENABLE_ANIM_BLENDING)) )
    {
        // Flag requested type is playing
        m_CurrentAnimType = m_RequestedAnimType = Type ;

        // Only set if found, otherwise leave the old one playing
        anim *Anim = m_pShape->GetAnimByType(m_RequestedAnimType) ;
        if (Anim)
        {
            // No blend
            m_Blend = 0.0f ;

            // Set new current anim
            m_CurrentAnim.SetAnim(Anim) ;
            m_CurrentAnim.SetFrame((Anim->GetFrameCount()-1) * StartFrame) ;
        }
    }
    else
    {
        // Put into the requested vars, the advance will update it
        m_RequestedAnimType   = Type ;
        m_RequestedBlendTime  = BlendTime ;
        m_RequestedStartFrame = StartFrame ;

        // Force blend?
        if (ForceBlend)
            m_CurrentAnimType = -1 ;    // Will force switching
    }

    // Reset incase logic comes after this call...
    m_CurrentAnim.SetAnimPlayed(0) ;
}
    
//==============================================================================

// Set new anim by index
void shape_instance::SetAnimByIndex(s32 Index, f32 BlendTime)
{
    // Get new anim
    anim *Anim = m_pShape->GetAnimByIndex(Index) ;
    ASSERT(Anim) ;

    // Skip if already playing this anim?
    if (Anim == m_CurrentAnim.GetAnim())
        return ;

    // Stop type request vars from changing the anim
    m_CurrentAnimType   = Anim->GetType() ;
    m_RequestedAnimType = Anim->GetType() ;

    // Blend to new anim?
    if (BlendTime != 0.0f)
    {
        // Copy current anim to blend anim
        m_BlendAnim = m_CurrentAnim ;

        // Start the blend
        m_Blend    = 1.0f ;
        m_BlendInc = 1.0f / BlendTime ;
    }
    else
    {
        // No blend
        m_Blend = 0.0f ;
    }

    // Set new current anim
    m_CurrentAnim.SetAnim(Anim) ;
}

//==============================================================================

// Turns off all special anims
void shape_instance::ClearSpecialAnims( void )
{
    s32 i ;

    // Clear additive anims
    for (i = 0 ; i < MAX_ADDITIVE_ANIMS ; i++)
    {
        m_AdditiveAnims[i].SetAnim(NULL) ;
        m_AdditiveAnims[i].SetID(-1) ;
    }

    // Clear masked anims
    for (i = 0 ; i < MAX_MASKED_ANIMS ; i++)
    {
        m_MaskedAnims[i].SetAnim(NULL) ;
        m_MaskedAnims[i].SetID(-1) ;
    }
}

//==============================================================================

// Advances all active special anims
void shape_instance::AdvanceSpecialAnims( f32 DeltaTime )
{
    s32 i ;

    // Loop through all additive anims
    for (i = 0 ; i < MAX_ADDITIVE_ANIMS ; i++)
    {
        anim_state *AnimState = &m_AdditiveAnims[i] ;

        // Is this anim active?
        if (AnimState->GetID() != -1)
            AnimState->Advance( DeltaTime ) ;
    }

    // Loop through all masked anims
    for (i = 0 ; i < MAX_MASKED_ANIMS ; i++)
    {
        anim_state *AnimState = &m_MaskedAnims[i] ;

        // Is this anim active?
        if (AnimState->GetID() != -1)
            AnimState->Advance( DeltaTime ) ;
    }
}

//==============================================================================

// Walks through special active anims and builds a linked list
anim_state* shape_instance::BuildSpecialAnimList( anim_state* List, s32 Max )
{
    // Clear list
    anim_state* AnimsHead = NULL ;

    // Loop through all anims
    anim_state *LastAnimState = NULL ;
    for (s32 i = 0 ; i < Max ; i++)
    {
        anim_state *AnimState = &List[i] ;

        // Is this anim active?
        if ( (AnimState->GetID() != -1) && (AnimState->GetAnim()) && (AnimState->GetWeight() > 0) )
        {
            // Set as head or append to list?
            if (!LastAnimState)
                AnimsHead = AnimState ;
            else
                LastAnimState->SetNext(AnimState) ;

            // Update last state
            LastAnimState = AnimState ;
        }
    }

    // Terminate list if there are any items
    if (LastAnimState)
        LastAnimState->SetNext(NULL) ;

    return AnimsHead ;
}

//==============================================================================

// Set additive animation by type
void shape_instance::SetAdditiveAnimByType(s32 ID, s32 Type)
{
    ASSERT(m_pShape) ;

    // Search for animation
    anim *Anim = m_pShape->GetAnimByType(Type) ;
    if (Anim)
    {
        ASSERT(Anim->GetAdditiveFlag()) ;
    }

    // Is anim already there?
    anim_state *AnimState = GetAdditiveAnimState(ID) ;

    // Allocate anim if it's not there
    if (!AnimState)
        AnimState = AddAdditiveAnim(ID) ;

    ASSERT(AnimState) ;

    // Start the anim
    AnimState->SetAnim(Anim) ;
}

//==============================================================================

// Creates and adds a new anim state
anim_state *shape_instance::AddAdditiveAnim(s32 ID)
{
    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_ADDITIVE_ANIMS) ;

    // Get anim from list
    anim_state *AnimState = &m_AdditiveAnims[ID] ;

    // Signal anim state is in use
    AnimState->SetID(ID) ;

    // Return anims
    return AnimState ;
}

//==============================================================================

// Deletes additive animtion 
void shape_instance::DeleteAdditiveAnim(s32 ID)
{
    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_ADDITIVE_ANIMS) ;

    // Flag anim state is no londer used
    anim_state *AnimState = &m_AdditiveAnims[ID] ;
    AnimState->SetID(-1) ;
}

//==============================================================================

// Removes all additive anims
void shape_instance::DeleteAllAdditiveAnims( void )
{
    // Flag anim states that they are not used
    for (s32 i = 0 ; i < MAX_ADDITIVE_ANIMS ; i++)
        m_AdditiveAnims[i].SetID(-1) ;
}

//==============================================================================

// Returns animation state of additive animation (if active)
anim_state  *shape_instance::GetAdditiveAnimState(s32 ID)
{
    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_ADDITIVE_ANIMS) ;

    // Return state if it's active
    anim_state *AnimState = &m_AdditiveAnims[ID] ;
    if (AnimState->GetID() != -1)
        return AnimState ;
    else
        return NULL ;
}

//==============================================================================

// Set masked animation by type
void shape_instance::SetMaskedAnimByType(s32 ID, s32 Type)
{
    ASSERT(m_pShape) ;

    // Search for animation
    anim *Anim = m_pShape->GetAnimByType(Type) ;

    // Is anim already there?
    anim_state *AnimState = GetMaskedAnimState(ID) ;

    // Allocate anim if it's not there
    if (!AnimState)
        AnimState = AddMaskedAnim(ID) ;

    ASSERT(AnimState) ;

    // Start the anim
    AnimState->SetAnim(Anim) ;
}

//==============================================================================

// Creates and adds a new anim state
anim_state *shape_instance::AddMaskedAnim(s32 ID)
{
    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_MASKED_ANIMS) ;

    // Get anim from list
    anim_state *AnimState = &m_MaskedAnims[ID] ;

    // Signal anim state is in use
    AnimState->SetID(ID) ;

    // Return anims
    return AnimState ;
}

//==============================================================================

// Deletes masked animtion 
void shape_instance::DeleteMaskedAnim(s32 ID)
{
    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_MASKED_ANIMS) ;

    // Flag anim state is no londer used
    anim_state *AnimState = &m_MaskedAnims[ID] ;
    AnimState->SetID(-1) ;
}

//==============================================================================

// Removes all masked anims
void shape_instance::DeleteAllMaskedAnims( void )
{
    // Flag anim states that they are not used
    for (s32 i = 0 ; i < MAX_MASKED_ANIMS ; i++)
        m_MaskedAnims[i].SetID(-1) ;
}

//==============================================================================

// Returns animation state of masked animation (if active)
anim_state  *shape_instance::GetMaskedAnimState(s32 ID)
{
    ASSERT(ID >= 0) ;
    ASSERT(ID < MAX_MASKED_ANIMS) ;

    // Return state if it's active
    anim_state *AnimState = &m_MaskedAnims[ID] ;
    if (AnimState->GetID() != -1)
        return AnimState ;
    else
        return NULL ;
}

//==============================================================================

matrix4 shape_instance::GetL2W()
{
    // Must be using a geometry!
    ASSERT(m_pShape) ;
    ASSERT(m_pModel) ;

    // Make sure we can use the world render node data
    UpdateWorldRenderNode() ;

    // Grab local to world
    return m_WorldRenderNode.L2W ;
}

//==============================================================================

// Returns bounds of instance
bbox shape_instance::GetWorldBounds()
{
    ASSERT(m_pModel) ;

    // Re-calculate?
    if (    (m_Flags & FLAG_WORLD_BOUNDS_DIRTY) ||
            ((m_ParentInstance) && (m_ParentInstance->m_Flags & FLAG_WORLD_BOUNDS_DIRTY)) )
    {
        // Make sure we can use the world render node data
        UpdateWorldRenderNode() ;
    
        // Start with model bounds
        m_WorldBounds = m_pModel->GetBounds() ;

        // Put into world space
        m_WorldBounds.Transform(m_WorldRenderNode.L2W) ;

        // Flag clean
        m_Flags &= ~FLAG_WORLD_BOUNDS_DIRTY ;
    }

    return m_WorldBounds ;
}

//==============================================================================

// Returns tight bounds of static instance - you should only call this once when game loads
// since it pushes all the verts through the L2W ie. it's slow!
bbox shape_instance::GetTightWorldBounds()
{
    ASSERT(m_pModel) ;

    // Make sure we can use the world render node data
    UpdateWorldRenderNode() ;
    
    // Make sure instance is a static instance
    ASSERT(m_pModel) ;
    ASSERT(m_pShape) ;

    // Get model
    model* pModel = m_pModel ;

    // Try calculate render nodes
    render_node* RenderNodes = CalculateRenderNodes() ;
    if (!RenderNodes)
        return GetWorldBounds() ;

    // Clear the bounds
    m_WorldBounds.Clear() ;

    // Since verts are scaled up on the PS2, we must scale them back down
    f32 Scale    = pModel->GetPrecisionScale() ;
    f32 InvScale = 1.0f /  Scale;

    // Loop through all materials in model
    for (s32 cMat = 0 ; cMat < pModel->GetMaterialCount() ; cMat++)
    {
        material& Mat = pModel->GetMaterial(cMat) ;

        // Loop through all display lists in material
        for (s32 cDList = 0 ; cDList < Mat.GetDListCount() ; cDList++)
        {
            // Get material dlist
            material_dlist& MatDList = Mat.GetDList(cDList) ;

            // Get display list local to world
            matrix4 DListL2W = RenderNodes[MatDList.GetNode()].L2W ;
        
            // Scale verts down (for PS2)
            DListL2W.PreScale(InvScale) ;

            // Loop through all strips in dlist
            for (s32 cStrip = 0 ; cStrip < MatDList.GetStripCount() ; cStrip++)
            {
                dlist_strip& DListStrip = MatDList.GetStrip(cStrip) ;

                // Loop through all verts in strip
                for (s32 cVert = 0 ; cVert < DListStrip.GetVertCount() ; cVert++)
                {
                    // Get vert world space
                    vector3 Pos, Norm, WorldPos ;
                    vector2 UV ;
                    DListStrip.GetVert(cVert, Pos, Norm, UV) ;
                    WorldPos = DListL2W * Pos ;

                    // Update bounds
                    m_WorldBounds += WorldPos ;
                }
            }
        }
    }

    // Flag clean
    m_Flags &= ~FLAG_WORLD_BOUNDS_DIRTY ;

    return m_WorldBounds ;
}

//==============================================================================

void shape_instance::SetPos(const vector3& vPos)
{
    // Changed?
    if (m_vPos != vPos)
    {
        // Set new pos
	    m_vPos    = vPos ;
	    
        // Invalidate L2W matrix, world bounds, and render nodes
        m_Flags |= FLAG_L2W_DIRTY | FLAG_WORLD_BOUNDS_DIRTY | FLAG_RENDER_NODES_DIRTY ;
    }
}

void shape_instance::SetScale(const vector3& vScale)
{
    // Changed?
    if (m_vScale != vScale)
    {
        // Set new scale
	    m_vScale  = vScale ;

        // Invalidate L2W matrix, world bounds, and render nodes
        m_Flags |= FLAG_L2W_DIRTY | FLAG_WORLD_BOUNDS_DIRTY | FLAG_RENDER_NODES_DIRTY ;
    }
}

void shape_instance::SetRot(const radian3& rRot)
{
    // Changed?
    if (m_rRot != rRot)
    {
        // Changed?
	    m_rRot    = rRot ;

        // Invalidate L2W matrix, world bounds, and render nodes
        m_Flags |= FLAG_L2W_DIRTY | FLAG_WORLD_BOUNDS_DIRTY | FLAG_RENDER_NODES_DIRTY ;
    }
}

//==============================================================================

void shape_instance::UpdateWorldRenderNode()
{
    // Must be using a geometry!
    ASSERT(m_pShape) ;
    ASSERT(m_pModel) ;

    // Re-calculate world orientation?
    if (    (m_Flags & FLAG_L2W_DIRTY) || 
            ((m_ParentInstance) && (m_ParentInstance->m_Flags & FLAG_L2W_DIRTY)) )
    {
        // Setup parent orientation matrix
        m_WorldRenderNode.L2W.Identity() ;
        m_WorldRenderNode.L2W.SetScale(m_vScale) ;
        m_WorldRenderNode.L2W.Rotate(m_rRot) ;
        m_WorldRenderNode.L2W.SetTranslation(m_vPos) ;

        // If attached to a parent instance, take it into account
        if (m_ParentInstance)
        {
            ASSERT(m_ParentInstance->GetModel()) ;

            // Re-lookup hot point if model has changed
            if (m_ParentModel != m_ParentInstance->GetModel())
                SetParentInstance(m_ParentInstance, m_ParentHotPointType) ;

            // Apply parent orientation
            m_WorldRenderNode.L2W = m_ParentInstance->GetHotPointL2W(m_ParentHotPoint) * m_WorldRenderNode.L2W ;
        }

        // Flag clean
        m_Flags &= ~FLAG_L2W_DIRTY ;
    }

    // Light is updated whenever light functions are called...
}

//==============================================================================


// Calculates node matrices etc ready for drawing shape_instance
render_node* shape_instance::CalculateRenderNodes()
{
    int i ;

	// Must be a shape and model!
    ASSERT(m_pShape) ;
	ASSERT(m_pModel) ;

    // Needs to be aligned so it can be shipped to vu1 and used by vu0
    #ifdef TARGET_PS2
        ASSERT(ALIGN_16(&m_WorldRenderNode)) ;
    #endif

    // Static instance optimization:
    // Is this a static instance that can use the internal world render node?
    if ((m_Flags & FLAG_IS_STATIC) && (m_pShape->GetAnimCount() == 0) && (m_pModel->GetNodeCount() == 1))
    {
        // Make sure world render node is updated
        UpdateWorldRenderNode() ;

        // Use internal render node
        m_RenderNodes          = &m_WorldRenderNode ;
        m_RenderNodesAllocSize = sizeof(render_node) ;
        m_RenderNodesAllocID   = -1 ;

        // Render nodes are good
        m_Flags &= ~FLAG_RENDER_NODES_DIRTY ;

        return m_RenderNodes ;
    }

    // Calculate amount of scratch mem needed
    s32   ScratchMemNeeded = m_pModel->GetNodeCount() * sizeof(render_node) ;
    ASSERT(ScratchMemNeeded) ;

    // Re-allocate render nodes if needed
	if (        (m_Flags & FLAG_ALWAYS_DIRTY)
            ||  (m_RenderNodes == NULL)
            ||  (m_RenderNodesAllocID != smem_GetActiveID()) )
	{
        // Make sure they get re-calculated
        m_Flags |= FLAG_RENDER_NODES_DIRTY ;

		// Allocate matrices
		m_RenderNodes           = (render_node*)smem_BufferAlloc(ScratchMemNeeded) ;
        m_RenderNodesAllocSize  = ScratchMemNeeded ;
        m_RenderNodesAllocID    = smem_GetActiveID() ;

        // Fail?
        if (!m_RenderNodes)
        {
            x_DebugMsg("Could not allocate m_RenderNodes!!!\n");
			return NULL ;
        }

        // Make sure render nodes are valid
        ASSERT(m_RenderNodesAllocSize == ScratchMemNeeded) ;
        ASSERT(m_RenderNodesAllocID == smem_GetActiveID()) ;
        ASSERT((byte*)m_RenderNodes >= smem_GetActiveStartAddr()) ;
        ASSERT((byte*)m_RenderNodes  < smem_GetActiveEndAddr()) ;
        ASSERT((byte*)m_RenderNodes + ScratchMemNeeded <= smem_GetActiveEndAddr()) ;
	}
    else
    {
        // Make sure render nodes are valid
        ASSERT(m_RenderNodesAllocSize == ScratchMemNeeded) ;
        ASSERT(m_RenderNodesAllocID == smem_GetActiveID()) ;
        ASSERT((byte*)m_RenderNodes >= smem_GetActiveStartAddr()) ;
        ASSERT((byte*)m_RenderNodes  < smem_GetActiveEndAddr()) ;
        ASSERT((byte*)m_RenderNodes + ScratchMemNeeded <= smem_GetActiveEndAddr()) ;
    }
    
    // If nodes are not dirty, then we are done
    if (!(m_Flags & FLAG_RENDER_NODES_DIRTY))
        return m_RenderNodes ;

    // Render nodes are good after this function has finished
    m_Flags &= ~FLAG_RENDER_NODES_DIRTY ;

    // Make sure scratch isn't trashed
    smem_Validate() ;

    // Make sure render node addr is valid
    ASSERT(m_RenderNodesAllocSize == ScratchMemNeeded) ;
    ASSERT((byte*)m_RenderNodes >= smem_GetActiveStartAddr()) ;
    ASSERT((byte*)m_RenderNodes  < smem_GetActiveEndAddr()) ;
    ASSERT((byte*)m_RenderNodes + ScratchMemNeeded <= smem_GetActiveEndAddr()) ;

    #ifdef DEBUG_PLAYER
        //x_DebugMsg("Shape:%s Frame:%d\n", ShapeTypesStrings[m_pShape->GetType()], tgl.NRenders) ;
        if (m_pShape->GetType() == SHAPE_TYPE_CHARACTER_LIGHT_MALE)
        {
            static s32 Frame=-1 ;

            // doing twice???
            if (Frame == tgl.NRenders)
            {
                Frame++ ;
            }

            Frame = tgl.NRenders ;
        }
    #endif

	// Matrices aligned for ps2 so we can use vu0
	static matrix4 mNode   PS2_ALIGNMENT(16) ;

    // Need to be aligned to use sony's library
    #ifdef TARGET_PS2
        ASSERT(ALIGN_16(&mNode)) ;
    #endif

    // Make sure world render node is ready to use
    UpdateWorldRenderNode() ;

    // Use animation?
    if (m_pShape->GetAnimCount())
    {
		// Keyframe vars (PS2 uses vu0 so they need to be aligned)
		static vector4      vAnimPos        PS2_ALIGNMENT(16) ;
		static quaternion   qAnimRot        PS2_ALIGNMENT(16) ;
		static f32          AnimVis         PS2_ALIGNMENT(16) ;

		static vector4      vPosMasked      PS2_ALIGNMENT(16) ;
		static quaternion   qRotMasked      PS2_ALIGNMENT(16) ;
        
        static vector4      vPosCurrent     PS2_ALIGNMENT(16) ;
        static vector4      vPosBlend       PS2_ALIGNMENT(16) ;
        
        static quaternion   qRotCurrent     PS2_ALIGNMENT(16) ;
        static quaternion   qRotBlend       PS2_ALIGNMENT(16) ;

        static f32          VisCurrent      PS2_ALIGNMENT(16) ;
        static f32          VisBlend        PS2_ALIGNMENT(16) ;
        
        static vector4      vAdditivePos    PS2_ALIGNMENT(16) ;
        static quaternion   qAdditiveRot    PS2_ALIGNMENT(16) ;

        // Anim vars
        anim_state* AnimState ;
		f32			AnimWeight ;
        anim_info   AnimInfoList[MAX_ANIMS] ;
        anim_info*  AnimInfo ;
        anim_state* MaskedAnimsHead ;
        anim_state* AdditiveAnimsHead ;

        // Build masked anims list
        if (m_Flags & FLAG_ENABLE_MASKED_ANIMS)
            MaskedAnimsHead = BuildSpecialAnimList( m_MaskedAnims, MAX_MASKED_ANIMS ) ;
        else
            MaskedAnimsHead = NULL ;

        // Build additive anims list
        if (m_Flags & FLAG_ENABLE_ADDITIVE_ANIMS)
            AdditiveAnimsHead = BuildSpecialAnimList( m_AdditiveAnims, MAX_ADDITIVE_ANIMS ) ;
        else
            AdditiveAnimsHead = NULL ;

        // Begin at start of anim info list
        AnimInfo = AnimInfoList ;

        // Get anim info ready for looking up keys
        m_CurrentAnim.SetupAnimInfo(*AnimInfo++) ;

        // Get blend anim info ready for looking up keys
        if (m_Blend != 0.0f)
            m_BlendAnim.SetupAnimInfo(*AnimInfo++) ;

        // Setup anim info for masked animations
        AnimState = MaskedAnimsHead ;
        while(AnimState)
        {
            // Setup info and goto next
            AnimState->SetupAnimInfo(*AnimInfo++) ;    
            AnimState = AnimState->GetNext() ;
        }

        // Setup anim info for additive animations
        AnimState = AdditiveAnimsHead ;
        while(AnimState)
        {
            // Setup info and goto next
            AnimState->SetupAnimInfo(*AnimInfo++) ;    
            AnimState = AnimState->GetNext() ;
        }

        // Make sure we didn't overrun anim info list
        ASSERT(AnimInfo <= &AnimInfoList[MAX_ANIMS]) ;

        // Build node matrices
	    // (NOTE: Nodes are in parent -> children order, so there's no need to use recursion)
	    for (i = 0 ; i < m_pModel->GetNodeCount() ; i++)
	    {
            // Get node
		    node &Node = m_pModel->GetNode(i) ;

            // Begin at start of anim info list
            AnimInfo = AnimInfoList ;

		    // Single animation?
            if (m_Blend == 0.0f)
            {
		        m_CurrentAnim.GetKeys(*AnimInfo++, vAnimPos, qAnimRot, AnimVis) ;
            }
            else
            {
    	        // Blend animations
                m_CurrentAnim.GetKeys(*AnimInfo++, vPosCurrent, qRotCurrent, VisCurrent) ;
		        m_BlendAnim.GetKeys  (*AnimInfo++, vPosBlend, qRotBlend, VisBlend) ;
		        BlendLinear(vPosCurrent, vPosBlend, m_Blend, vAnimPos) ;
		        BlendFast  (qRotCurrent, qRotBlend, m_Blend, qAnimRot) ;
                AnimVis = VisCurrent + (m_Blend * (VisBlend - VisCurrent)) ;
            }

			// Apply masked animations
            AnimState = MaskedAnimsHead ;
            while(AnimState)
            {
                // Get key set and increment to next for next node
                key_set &KeySet = *AnimInfo->KeySet ;
				AnimWeight = KeySet.GetAnimWeight() * AnimState->GetWeight() ; 

                // Mask?
                if (AnimWeight > 0.0f)
				{
					// 100%?
					if (AnimWeight == 1.0f)
						AnimState->GetKeys(*AnimInfo, vAnimPos, qAnimRot) ;
					else
					{
						// Get keys
						AnimState->GetKeys(*AnimInfo, vPosMasked, qRotMasked) ;

						// Blend with main anim
						BlendLinear(vAnimPos, vPosMasked, AnimWeight, vAnimPos) ;
						BlendFast  (qAnimRot, qRotMasked, AnimWeight, qAnimRot) ;
					}
				}

                // Check next
                AnimInfo++ ;
                AnimState = AnimState->GetNext() ;
			}

            // Apply additive animations
            AnimState = AdditiveAnimsHead ;
            while(AnimState)
            {
                // Get key set and increment to next for next node
                key_set &KeySet = *AnimInfo->KeySet++ ;

                // Full additive?
                if (AnimState->GetWeight() == 1.0f)
                {
                    // Apply pos key?
                    if (KeySet.GetPosKeyCount())
                    {
					    // Get pos key
                        AnimState->GetPosKey(*AnimInfo, KeySet, vAdditivePos) ;
                    
					    // Blend on top of main anim
					    vAnimPos += vAdditivePos ;
                    }

                    // Apply rot key?
                    if (KeySet.GetRotKeyCount())
                    {
					    // Get rot key
                        AnimState->GetRotKey(*AnimInfo, KeySet, qAdditiveRot) ;

					    // Blend on top of main anim
                        qAnimRot = qAdditiveRot * qAnimRot ;
                    }
                }
                else
                {
                    // Slower weighted version...

                    // Apply pos key?
                    if (KeySet.GetPosKeyCount())
                    {
					    // Get pos key
                        AnimState->GetPosKey(*AnimInfo, KeySet, vAdditivePos) ;
                    
					    // Blend on top of main anim
					    vAnimPos = (vAdditivePos * AnimState->GetWeight()) + vAnimPos ;
                    }

                    // Apply rot key?
                    if (KeySet.GetRotKeyCount())
                    {
					    // Get rot key
                        AnimState->GetRotKey(*AnimInfo, KeySet, qAdditiveRot) ;
                        BlendIdentityFast(qAdditiveRot, 1.0f - AnimState->GetWeight(), qAdditiveRot) ;

					    // Blend on top of main anim
                        qAnimRot = qAdditiveRot * qAnimRot ;
                    }
                }

                // Check next
                AnimInfo++ ;
                AnimState = AnimState->GetNext() ;
            }

            // Make sure we didn't overrun anim info list
            ASSERT(AnimInfo <= &AnimInfoList[MAX_ANIMS]) ;

            // Root node special cases
            if (Node.GetParent() == -1)
            {
                // Remove root node X anim?
                if (m_Flags & FLAG_REMOVE_ROOT_NODE_POS_X)
                    vAnimPos.X = 0 ;
                
                // Remove root node Y anim?
                if (m_Flags & FLAG_REMOVE_ROOT_NODE_POS_Y)
                    vAnimPos.Y = 0 ;

                // Remove root node X anim?
                if (m_Flags & FLAG_REMOVE_ROOT_NODE_POS_Z)
                    vAnimPos.Z = 0 ;
            }

		    // Build node matrix from animation keys
		    
            // Set node local rotation
            mNode.Setup(qAnimRot) ;

            // Set node local translation
            mNode(3,0) = vAnimPos.X;
            mNode(3,1) = vAnimPos.Y;
            mNode(3,2) = vAnimPos.Z;

            // Make sure parent is valid
		    ASSERT(Node.GetParent() < i) ;
            ASSERT(Node.GetParent() >= -1) ;
            ASSERT(Node.GetParent() < m_pModel->GetNodeCount()) ;

            // Call user node function?
            if (m_NodeFunc)
                m_NodeFunc(m_NodeFuncParam, Node, mNode) ;

            // Setup light
            m_RenderNodes[i].Light = m_WorldRenderNode.Light ;

            // Take into account visiblity
            m_RenderNodes[i].Light.Amb.W = AnimVis ;

            // Calc final node orientation, taking parent into consideration
			#if defined(TARGET_PS2) && defined(PS2_USE_VU0)
                if (Node.GetParent() == -1)
    				sceVu0MulMatrix((sceVu0FMATRIX)(&m_RenderNodes[i].L2W), (sceVu0FMATRIX)(&m_WorldRenderNode.L2W), (sceVu0FMATRIX)(&mNode)) ;
                else
				    sceVu0MulMatrix((sceVu0FMATRIX)(&m_RenderNodes[i].L2W), (sceVu0FMATRIX)(&m_RenderNodes[Node.GetParent()].L2W), (sceVu0FMATRIX)(&mNode)) ;
			#else
                if (Node.GetParent() == -1)
    				m_RenderNodes[i].L2W = m_WorldRenderNode.L2W * mNode ;
                else
    				m_RenderNodes[i].L2W = m_RenderNodes[Node.GetParent()].L2W * mNode ;
			#endif
        }

	    // Put inverse bind world matrix infront of node matrices
	    //for (i = 0 ; i < m_pModel->GetNodeCount() ; i++)
            //mNodeOrient[i] = mNodeOrient[i] * m_pModel->GetNode(i).GetInvBindOrient() ;
            //sceVu0MulMatrix((vu0matrix)&(mNodeOrient[i](0,0)), (vu0matrix)&(mNodeOrient[i](0,0)), (vu0matrix)&(m_pModel->GetNode(i).GetInvBindOrient()(0,0))) ;
    }
    else
    {
        // Model has no animation and since verts are in world space, we can just use the same matrix for each node...
	    for (i = 0 ; i < m_pModel->GetNodeCount() ; i++)
        {
			#if defined(TARGET_PS2) && defined(PS2_USE_VU0)
                sceVu0CopyMatrix((sceVu0FMATRIX)(&m_RenderNodes[i].L2W), (sceVu0FMATRIX)(&m_WorldRenderNode.L2W)) ;
            #else
                m_RenderNodes[i].L2W = m_WorldRenderNode.L2W ;
            #endif

            // Setup light
            m_RenderNodes[i].Light = m_WorldRenderNode.Light ;
        }
    }

    // Make sure scratch mem wasn't trashed
    smem_Validate() ;

    // Success
    return m_RenderNodes ;
}

//==============================================================================

// Returns fog value for simple linear fogging
f32 shape_instance::CalculateLinearFog(f32 Thickness)
{
    // Get object position in camera space
    const view  *View   = eng_GetView() ;
    vector3     vCamPos = View->ConvertW2V(m_vPos) ;

    // Get view limits
    f32 NearZ, FarZ, ViewDist ;
    View->GetZLimits(NearZ, FarZ) ;

    // Keep z buffer values within range
    FarZ -= GetWorldBounds().GetRadius() ;
    ViewDist = FarZ - NearZ ;
    
    // Calculate fog range
    f32 FogStart  = FarZ - (ViewDist * Thickness) ;
    f32 FogDist   = FarZ - FogStart ;

    // Setup fog
    f32 Fog ;
    if (vCamPos.Z >= FarZ)
        Fog = 1.0f ;
    else
    if (vCamPos.Z > FogStart)
        Fog = (vCamPos.Z - FogStart) / FogDist ;
    else
        Fog = 0.0f ;

    return Fog ;
}

//==============================================================================

// Attach to another instances hot point
void shape_instance::SetParentInstance(shape_instance *Instance, s32 HotPointType)
{
    // Keep parent info
    m_ParentInstance     = Instance ;
    m_ParentHotPointType = HotPointType ;
    
    // Lookup parent info?
    if (Instance)
    {
        m_ParentModel = Instance->GetModel() ;
        ASSERT(m_ParentModel) ;

        m_ParentHotPoint = m_ParentModel->GetHotPointByType(HotPointType) ;
    }

    // Flag it's dirty
    m_Flags |= FLAG_L2W_DIRTY | FLAG_WORLD_BOUNDS_DIRTY ;
}

//==============================================================================

// Sets instance color
void shape_instance::SetColor(const vector4 &Col)
{
	m_Color = Col ;
}

//==============================================================================

// Sets instance color
void shape_instance::SetColor(const xcolor &Col)
{
	SetColor(vector4((f32)Col.R / 255.0f, 
                     (f32)Col.G / 255.0f, 
                     (f32)Col.B / 255.0f, 
                     (f32)Col.A / 255.0f)) ;
}

//==============================================================================

xcolor shape_instance::GetColor() const
{
    return xcolor((u8)(MIN(1, m_Color[0]) * 255), 
                  (u8)(MIN(1, m_Color[1]) * 255), 
                  (u8)(MIN(1, m_Color[2]) * 255), 
                  (u8)(MIN(1, m_Color[3]) * 255)) ;
}


//==============================================================================

// Sets instance self illum color
void shape_instance::SetSelfIllumColor(const vector4 &Col)
{
	m_SelfIllumColor = Col ;
}

//==============================================================================

// Sets instance self illum color
void shape_instance::SetSelfIllumColor(const xcolor &Col)
{
	SetSelfIllumColor(vector4((f32)Col.R / 255.0f, 
                     (f32)Col.G / 255.0f, 
                     (f32)Col.B / 255.0f, 
                     (f32)Col.A / 255.0f)) ;
}

//==============================================================================

// Returns self illum color
xcolor shape_instance::GetSelfIllumColor() const
{
    return xcolor((u8)(MIN(1, m_SelfIllumColor[0]) * 255), 
                  (u8)(MIN(1, m_SelfIllumColor[1]) * 255), 
                  (u8)(MIN(1, m_SelfIllumColor[2]) * 255), 
                  (u8)(MIN(1, m_SelfIllumColor[3]) * 255)) ;
}

//==============================================================================

void    shape_instance::SetFogColor (const xcolor& Col)
{
    m_FogColor = Col ;
}

//==============================================================================

xcolor  shape_instance::GetFogColor () const
{
    return m_FogColor ;
}

//==============================================================================

void    shape_instance::SetLightDirection(const vector3& Dir, xbool Normalize/*=FALSE*/)
{
    // Setup direction and normalize if needed
    vector3 LightDir = Dir ;
    if (Normalize)
        LightDir.Normalize() ;

    // Put into world render node
    m_WorldRenderNode.Light.Dir.X = LightDir.X ;
    m_WorldRenderNode.Light.Dir.Y = LightDir.Y ;
    m_WorldRenderNode.Light.Dir.Z = LightDir.Z ;
    m_WorldRenderNode.Light.Dir.W = 0 ;
}

//==============================================================================

vector3 shape_instance::GetLightDirection() const
{
    vector3 LightDir ;
    LightDir.X = m_WorldRenderNode.Light.Dir.X ;
    LightDir.Y = m_WorldRenderNode.Light.Dir.Y ;
    LightDir.Z = m_WorldRenderNode.Light.Dir.Z ;

    return LightDir ;
}

//==============================================================================

void    shape_instance::SetLightColor(const xcolor&  Col)
{
	SetLightColor(vector4((f32)Col.R / 255.0f, 
                          (f32)Col.G / 255.0f, 
                          (f32)Col.B / 255.0f, 
                          (f32)Col.A / 255.0f)) ;
}

//==============================================================================

void    shape_instance::SetLightColor(const vector4& Col)
{
    m_WorldRenderNode.Light.Col = Col ;
}

//==============================================================================

vector4  shape_instance::GetLightColor() const
{
    return m_WorldRenderNode.Light.Col ;
}

//==============================================================================

void    shape_instance::SetLightAmbientColor(const xcolor&  Col)
{
	SetLightAmbientColor(vector4((f32)Col.R / 255.0f, 
                                 (f32)Col.G / 255.0f, 
                                 (f32)Col.B / 255.0f, 
                                 (f32)Col.A / 255.0f)) ;
}

//==============================================================================

void    shape_instance::SetLightAmbientColor(const vector4& Col)
{
    m_WorldRenderNode.Light.Amb = Col ;
    m_WorldRenderNode.Light.Amb.W = 1.0f ;  // Controls alpha!
}

//==============================================================================

vector4  shape_instance::GetLightAmbientColor() const
{
    return m_WorldRenderNode.Light.Amb ;
}

//==============================================================================

void shape_instance::SetNodeVisibleFlagByIndex(s32 NodeIndex, xbool Value)
{
    ASSERT(m_pModel) ;
    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < 32) ;    // Only 32 bits currently supported
    ASSERT(NodeIndex < m_pModel->GetNodeCount()) ;

    if (Value)
        m_NodeVisibleFlags |= 1 << NodeIndex ;
    else
        m_NodeVisibleFlags &= ~(1 << NodeIndex) ;
}

//==============================================================================

xbool shape_instance::GetNodeVisibleFlagByIndex(s32 NodeIndex)
{
    ASSERT(m_pModel) ;
    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < 32) ;    // Only 32 bits currently supported
    ASSERT(NodeIndex < m_pModel->GetNodeCount()) ;

    return ((m_NodeVisibleFlags & (1 << NodeIndex)) != 0) ;
}

//==============================================================================

void shape_instance::SetNodeVisibleFlagByType(s32 NodeType, xbool Value)
{
    s32 i ;

    ASSERT(m_pModel) ;

    // Check all nodes since there may be one or more of the same type
    for (i = 0 ; i < m_pModel->GetNodeCount() ; i++)
    {
        node& Node = m_pModel->GetNode(i) ;

        // Set visiblity for this node if type matches
        if (Node.GetType() == NodeType)
            SetNodeVisibleFlagByIndex(i, Value) ;
    }
}

//==============================================================================

xbool shape_instance::GetNodeVisibleFlagByType(s32 NodeType)
{
    ASSERT(m_pModel) ;

    // Just get first node
    s32 Index = m_pModel->GetNodeIndexByType(NodeType) ;
    if (Index != -1)
        return GetNodeVisibleFlagByIndex(Index) ;
    else
    {
        ASSERTS(0, "Model does not contain requested node type - check max file") ;
        return TRUE ;
    }
}

//==============================================================================

void shape_instance::SetMaterialVisibleFlagByIndex(s32 MaterialIndex, xbool Value)
{
    ASSERT(m_pModel) ;
    ASSERT(MaterialIndex >= 0) ;
    ASSERT(MaterialIndex < 32) ;    // Only 32 bits currently supported
    ASSERT(MaterialIndex < m_pModel->GetMaterialCount()) ;

    if (Value)
        m_MaterialVisibleFlags |= 1 << MaterialIndex ;
    else
        m_MaterialVisibleFlags &= ~(1 << MaterialIndex) ;
}

//==============================================================================

xbool shape_instance::GetMaterialVisibleFlagByIndex(s32 MaterialIndex)
{
    ASSERT(m_pModel) ;
    ASSERT(MaterialIndex >= 0) ;
    ASSERT(MaterialIndex < 32) ;    // Only 32 bits currently supported
    ASSERT(MaterialIndex < m_pModel->GetMaterialCount()) ;

    return ((m_MaterialVisibleFlags & (1 << MaterialIndex)) != 0) ;
}

//==============================================================================

void shape_instance::SetMaterialVisibleFlagByName(const char *Name,  xbool Value)
{
    ASSERT(m_pModel) ;
    
    s32 Index = m_pModel->GetMaterialIndexByName(Name) ;
    if (Index != -1)
        SetMaterialVisibleFlagByIndex(Index, Value) ;
    else
    {
        ASSERTS(0, "Model does not contain requested material name - check max file") ;
    }
}

//==============================================================================

xbool shape_instance::GetMaterialVisibleFlagByName(const char *Name)
{
    ASSERT(m_pModel) ;

    s32 Index = m_pModel->GetMaterialIndexByName(Name) ;
    if (Index != -1)
        return GetMaterialVisibleFlagByIndex(Index) ;
    else
    {
        ASSERTS(0, "Model does not contain requested material name - check max file") ;
        return TRUE ;
    }
}

//==============================================================================

// Returns pointer to give hot point if found, or NULL 
hot_point *shape_instance::GetHotPointByType(s32 Type)
{
    ASSERT(m_pShape) ;
    ASSERT(m_pModel) ;

    return m_pModel->GetHotPointByType(Type) ;
}

//==============================================================================

// Returns pointer to give hot point
hot_point *shape_instance::GetHotPointByIndex(s32 Index)
{
    ASSERT(m_pShape) ;
    ASSERT(m_pModel) ;

    return m_pModel->GetHotPointByIndex(Index) ;
}

//==============================================================================

// Returns local to world matrix of given hot point
matrix4 shape_instance::GetHotPointL2W(hot_point *HotPoint)
{
    matrix4 Orient ;

    // So we can use render node L2W
    UpdateWorldRenderNode() ;

    // Hot point not there?
    if (!HotPoint)
    {
        // Failed - so just use instance L2W
        return m_WorldRenderNode.L2W ;
    }

    // Attached to a node?
    if (HotPoint->GetNode() != -1)
    {
        // Try calculate render nodes
        render_node* RenderNodes = CalculateRenderNodes() ;
        if (RenderNodes)
        {
            // Get local hot point orientation
            Orient.Setup(HotPoint->GetDir()) ;
            Orient.SetTranslation(HotPoint->GetPos()) ;

            // Mult by the attached node orientation
            return m_RenderNodes[HotPoint->GetNode()].L2W * Orient ;
        }
        else
        {
            // Failed - so just use instance L2W
            return m_WorldRenderNode.L2W ;
        }
    }
    else
    {
        // Get local hot point orientation
        Orient.Setup(HotPoint->GetDir()) ;
        Orient.SetTranslation(HotPoint->GetPos()) ;

        // Not attached to a node so just use L2W
        return m_WorldRenderNode.L2W * Orient ;
    }
}

//==============================================================================

// Returns orientation of requested hot point
matrix4 shape_instance::GetHotPointL2W(s32 Type)
{
    // Search for hot point
    hot_point *HotPoint = GetHotPointByType(Type) ;

    // Get orientation
    return GetHotPointL2W(HotPoint) ;
}

//==============================================================================

// Returns orientation of requested hot point
matrix4 shape_instance::GetHotPointByIndexL2W(s32 Index)
{
    // Lookup hot point
    hot_point *HotPoint = GetHotPointByIndex(Index) ;

    // Get orientation
    return GetHotPointL2W(HotPoint) ;
}

//==============================================================================

// Returns world position of given hot point
vector3 shape_instance::GetHotPointWorldPos(hot_point *HotPoint)
{
    // So we can use render node L2W
    UpdateWorldRenderNode() ;

    // Hot point not there?
    if (!HotPoint)
    {
        // Failed - so just use instance L2W position
        return m_WorldRenderNode.L2W.GetTranslation() ;
    }

    // Attached to a node?
    if (HotPoint->GetNode() != -1)
    {
        // Try calculate render nodes
        render_node* RenderNodes = CalculateRenderNodes() ;
        if (RenderNodes)
        {
            // Mult by the attached node orientation
            return m_RenderNodes[HotPoint->GetNode()].L2W * HotPoint->GetPos() ;
        }
        else
        {
            // Failed - so just use instance L2W
            return m_WorldRenderNode.L2W.GetTranslation() ;
        }
    }
    else
    {
        // Not attached to a node so just use L2W
        return m_WorldRenderNode.L2W * HotPoint->GetPos() ;
    }
}

//==============================================================================

// Returns world position of requested hot point
vector3 shape_instance::GetHotPointWorldPos(s32 Type)
{
    // Search for hot point
    hot_point *HotPoint = GetHotPointByType(Type) ;

    // Get orientation
    return GetHotPointWorldPos(HotPoint) ;
}

//==============================================================================

// Returns world position of requested hot point
vector3 shape_instance::GetHotPointByIndexWorldPos(s32 Index)
{
    // Lookup hot point
    hot_point *HotPoint = GetHotPointByIndex(Index) ;

    // Get orientation
    return GetHotPointWorldPos(HotPoint) ;
}

//==============================================================================

// Draw bounds, hot points, events etc
void shape_instance::DrawFeatures()
{
    s32 i ;

    // Calc node matrices
    render_node* RenderNodes = CalculateRenderNodes() ;
    if (!RenderNodes)
        return ;

    //======================================================================
    // Setup render states
    //======================================================================
    #ifdef TARGET_PC
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE) ;
        g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE) ;
        g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) ;
	    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) ;
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE) ; 
		g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) ;
    #endif

    //======================================================================
    // Draw node bounding boxes
    //======================================================================
    for (i = 0 ; i < m_pModel->GetNodeCount() ; i++)
    {
        draw_SetL2W(m_RenderNodes[i].L2W) ;
        draw_BBox(m_pModel->GetNode(i).GetBounds(), xcolor(255,0,0,128)) ;
    }

    //======================================================================
    // Draw hot points on this model
    //======================================================================
    ASSERT(m_pModel) ;
    for (i = 0 ; i < m_pModel->GetHotPointCount() ; i ++)
    {
        hot_point &HotPoint = m_pModel->GetHotPoint(i) ;
		//if (HotPoint.GetType() == 10)
		{
            // Get L2W
            matrix4 L2W ;
            if (HotPoint.GetNode() != -1)
                L2W = m_RenderNodes[HotPoint.GetNode()].L2W ;
            else
                L2W = m_WorldRenderNode.L2W ;

			// Draw hot point
            draw_SetL2W(L2W) ;
			draw_Sphere(HotPoint.GetPos(), 0.2f, xcolor(255,255,255,128)) ;
			draw_Label(L2W * HotPoint.GetPos(), XCOLOR_WHITE, "%d", HotPoint.GetType()) ;
		}
	}

    //======================================================================
    // Draw world bounds
    //======================================================================
    draw_SetL2W(GetL2W()) ;
    draw_BBox(m_pModel->GetBounds(), xcolor(0, 255, 0, 128)) ;

    //======================================================================
    // Draw axis aligned world bounds
    //======================================================================
    draw_ClearL2W() ;
    draw_BBox(GetWorldBounds(), xcolor(255,255,255, 224)) ;

    //======================================================================
    // Draw normals
    //======================================================================

    // Since verts are scaled up on the PS2, we must scale them back down
    f32 InvScale = 1.0f / m_pModel->GetPrecisionScale() ;

    // Loop through all materials in model
    for (s32 cMat = 0 ; cMat < m_pModel->GetMaterialCount() ; cMat++)
    {
        material& Mat = m_pModel->GetMaterial(cMat) ;

        // Loop through all display lists in material
        for (s32 cDList = 0 ; cDList < Mat.GetDListCount() ; cDList++)
        {
            material_dlist& MatDList = Mat.GetDList(cDList) ;

            // Get local to world matrix for this dlist
            matrix4 DListL2W = RenderNodes[MatDList.GetNode()].L2W ;
            
            // Is this node visible?
            if (RenderNodes[MatDList.GetNode()].Light.Amb.W > 0)
            {
                draw_SetL2W(DListL2W) ;
                draw_Begin(DRAW_LINES, DRAW_USE_ALPHA) ;

                // Loop through all strips in dlist
                for (s32 cStrip = 0 ; cStrip < MatDList.GetStripCount() ; cStrip++)
                {
                    dlist_strip& DListStrip = MatDList.GetStrip(cStrip) ;

                    // Loop through all verts strip
                    for (s32 cVert = 0 ; cVert < DListStrip.GetVertCount() ; cVert++)
                    {
                        // Get vert info
                        vector3 Pos, Norm ;
                        vector2 TexCoord ;
                        DListStrip.GetVert(cVert, Pos, Norm, TexCoord) ;

                        // Scale pos into correct local pos (verts are scaled up s16's on the ps2!)
                        Pos *= InvScale ;

                        // Draw normal line
                        draw_Color(XCOLOR_BLUE) ;
                        draw_Vertex(Pos) ;
                        draw_Color(XCOLOR_WHITE) ;
                        draw_Vertex(Pos + (Norm * 0.1f)) ;
                    }
                }

                draw_End() ;
                draw_ClearL2W() ;
            }
        }
    }

    //======================================================================
    // Draw lights
    //======================================================================

    draw_Begin(DRAW_LINES, DRAW_USE_ALPHA) ;

    vector3 LPos = GetWorldBounds().GetCenter() ;
    vector3 LDir = vector3(m_WorldRenderNode.Light.Dir.X, m_WorldRenderNode.Light.Dir.Y, m_WorldRenderNode.Light.Dir.Z) ;
       
    draw_Color(XCOLOR_RED) ;
    draw_Vertex(LPos) ;
    draw_Color(XCOLOR_WHITE) ;
    draw_Vertex(LPos + (LDir * 0.5f)) ;
    draw_End() ;
}


//==============================================================================

// Draws shape_instance (assumes matrices and verts are already in world space)
xbool shape_instance::Draw( u32                 Flags, 
                            texture*            DetailTexture       /*=NULL*/,
                            s32                 DetailClutHandle    /*=-1*/,
                            render_material_fn* RenderMaterialFunc  /*= NULL*/,
                            void*               RenderMaterialParam /*= NULL*/ )
{
    // You must have these!
    ASSERT(m_pModel) ;
    ASSERT(m_pShape) ;

    // If you assert here, it means you have not called the global "shape::BeginDraw()"
    // and "shape::EndDraw()" functoins around your shape render code... so do it!
    ASSERT(shape::InBeginDraw()) ;

    // Totally transparent?
    if (m_Color.W == 0)
        return FALSE ;

    #if 0

        // Call this when the view volume and clipping volume are NOT the same

        // Perform full view volume and clipping test (if needed)

        // Is shape inside view volume?
        bbox WorldBBox = GetWorldBounds() ;
        s32  Visible   = shape::BBoxInViewVolume( WorldBBox ) ;
        switch(Visible)
        {
            // Skip draw?
            case view::VISIBLE_NONE:
                return FALSE ;

            // Turn off clipping and draw?
            case view::VISIBLE_FULL:
                Flags &= ~shape::DRAW_FLAG_CLIP ;
                break ;

            // Check against clipping volume?
            case view::VISIBLE_PARTIAL:
                Visible = shape::BBoxInClipVolume( WorldBBox ) ;
                switch(Visible)
                {
                    // Skip draw?
                    case view::VISIBLE_NONE:
                        //ASSERT(0) ;
                        return FALSE ;

                    // Turn off clipping and draw?
                    case view::VISIBLE_FULL:
                        Flags &= ~shape::DRAW_FLAG_CLIP ;
                        break ;

                    // Turn on clipping and draw?
                    case view::VISIBLE_PARTIAL:
                        Flags |= shape::DRAW_FLAG_CLIP ;
                        break ;
                }
        }

    #else

        // Call this when the view volume and clipping volume are the same

        // Just perform clip check
        bbox WorldBBox = GetWorldBounds() ;
        s32  Visible   = shape::BBoxInClipVolume( WorldBBox ) ;
        switch(Visible)
        {
            // Totally out of clip volume?
            case view::VISIBLE_NONE:
                return FALSE ;  // skip draw

            // Turn off clipping, if we are fully in the clip volume
            case view::VISIBLE_FULL:
                Flags &= ~shape::DRAW_FLAG_CLIP ;       // Turn off clipping
                break ;

            // Turn on clipping?
            case view::VISIBLE_PARTIAL:
                Flags |= shape::DRAW_FLAG_CLIP ;    // Turn on clipping
                break ;
        }

    #endif

    // Allocate draw info
    material_draw_info* DrawInfo = (material_draw_info*)smem_BufferAlloc(sizeof(material_draw_info)) ;
    if (!DrawInfo)
        return FALSE ;

    // Model?
	if (!m_pModel)
		return FALSE ;

	// Setup draw matrices etc
    DrawInfo->RenderNodes = CalculateRenderNodes() ;

    // Fail?
    if (!DrawInfo->RenderNodes)
        return FALSE ;

    // Turn off alpha pass?
    if (!shape::s_DrawSettings.AlphaPass)
        Flags |= shape::DRAW_FLAG_TURN_OFF_ALPHA_PASS ;

    // Turn off diffuse pass?
    if (!shape::s_DrawSettings.DiffusePass)
        Flags |= shape::DRAW_FLAG_TURN_OFF_DIFFUSE_PASS ;

    // Turn off luminance pass?
    if (!shape::s_DrawSettings.LuminancePass)
        Flags |= shape::DRAW_FLAG_TURN_OFF_LUMINANCE_PASS ;
    
    // Turn off reflect pass?
    if (!shape::s_DrawSettings.ReflectPass)
        Flags |= shape::DRAW_FLAG_TURN_OFF_REFLECT_PASS ;
    
    // Turn off bump pass?
    if (!shape::s_DrawSettings.BumpPass)
        Flags |= shape::DRAW_FLAG_TURN_OFF_BUMP_PASS ;

    // Turn off detail pass?
    if (!shape::s_DrawSettings.DetailPass)
        Flags |= shape::DRAW_FLAG_TURN_OFF_DETAIL_PASS ;

    // Turn fog off?
    if ( (!(Flags & shape::DRAW_FLAG_FOG)) || (!shape::s_DrawSettings.Fog) )
        DrawInfo->FogColor = xcolor(0,0,0,0) ;   // Turn off fog
    else
        DrawInfo->FogColor = m_FogColor ;

#ifdef TARGET_PS2

    // Get current view
    const view *View = eng_GetActiveView(0) ;
	ASSERT(View) ;

    // Get world texture pixel size of model
    f32 WorldPixelSize = m_pModel->GetWorldPixelSize() ;
    
    // Take instance scale into account
    WorldPixelSize *= MAX(m_vScale.Z, MAX(m_vScale.X, m_vScale.Y)) ;

    // Setup MipK
    DrawInfo->MipK = vram_ComputeMipK(WorldPixelSize, *View) ;

    // Calculate bounds in world space
    bbox WorldBounds = GetWorldBounds() ;

    // Use mips?
    if (shape::s_DrawSettings.Mips == -1)
    {
        // Calculate the min and max lod mips to use
        vram_ComputeMipRange(WorldBounds, *View, WorldPixelSize, DrawInfo->MinLOD, DrawInfo->MaxLOD) ;
    }
    else
    {
        // Use all mips
        DrawInfo->MinLOD = -100.0f ;
        DrawInfo->MaxLOD = 100.0f ;
    }

    //x_printf("wps:%f min:%f max:%f\n", WorldPixelSize, DrawInfo->MinLOD, DrawInfo->MaxLOD) ;

#endif  // #ifdef TARGET_PS2

#ifdef TARGET_PC
    // Use all mips
    DrawInfo->MipK   = 0.0f ;
    DrawInfo->MinLOD = -100.0f ;
    DrawInfo->MaxLOD = 100.0f ;
#endif

    // Setup rest of draw info
    DrawInfo->TextureManager        = &(m_pShape->GetTextureManager()) ;
    DrawInfo->Flags                 = Flags ;
    DrawInfo->NodeVisibleFlags      = m_NodeVisibleFlags ;
    DrawInfo->MaterialVisibleFlags  = m_MaterialVisibleFlags ;
    DrawInfo->Color                 = m_Color ;
    DrawInfo->SelfIllumColor        = m_SelfIllumColor ;
    DrawInfo->Frame                 = (s32)m_TextureFrame ;
    DrawInfo->PlaybackType          = m_TexturePlaybackType ;
    DrawInfo->DetailTexture         = DetailTexture ;
    DrawInfo->DetailClutHandle      = DetailClutHandle ;
    DrawInfo->VertsPreScale         = 1.0f / m_pModel->GetPrecisionScale() ;
    DrawInfo->RenderMaterialFunc    = RenderMaterialFunc ;
    DrawInfo->RenderMaterialParam   = RenderMaterialParam ;

    // Temp to show which objects are being clipped
    //if (Flags & shape::DRAW_FLAG_CLIP)
    //{
        //DrawInfo->Color.X *= 2 ;
        //DrawInfo->Color.Y *= 2 ;
        //DrawInfo->Color.Z *= 2 ;
    //}


    // We only need to do 2 visibility passes, if the object is opaque
    if (DrawInfo->Color.W == 1)
    {
        // Is current anim using node visiblity animation?
        if ( (m_CurrentAnim.GetAnim()) && (m_CurrentAnim.GetAnim()->GetHasAnimatedVisFlag()) )
            DrawInfo->Flags |= shape::DRAW_FLAG_VISIBILITY_ANIMATED ;

        // Is blending anim using node visiblity animation?
       if ( (m_BlendAnim.GetAnim()) && (m_BlendAnim.GetAnim()->GetHasAnimatedVisFlag()) )
            DrawInfo->Flags |= shape::DRAW_FLAG_VISIBILITY_ANIMATED ;
    }

    // Incase we are doing visiblity passes
    if (DrawInfo->Flags & shape::DRAW_FLAG_VISIBILITY_ANIMATED)
        DrawInfo->Flags |= shape::DRAW_FLAG_VISIBILITY_PASS_OPAQUE ;

    // Draw now or add to draw list?
    if ((shape::s_DrawSettings.UseDrawList) && (Flags & shape::DRAW_FLAG_SORT))
    {
        // Add to the draw list for later...
        shape::AddInstanceToDrawList(*this, *DrawInfo) ;
    }
    else
    {
        // Draw model    
        m_pModel->Draw(*DrawInfo) ;
    }

    // Do we need to do another pass?
    if (DrawInfo->Flags & shape::DRAW_FLAG_VISIBILITY_ANIMATED)
    {
        // Allocate another draw info
        material_draw_info* TransDrawInfo = (material_draw_info*)smem_BufferAlloc(sizeof(material_draw_info)) ;
        if (!TransDrawInfo)
            return FALSE ;

        // Start with exactly the same info as before
        *TransDrawInfo = *DrawInfo ;

        // Now setup the draw info for transparent visiblity draw
        TransDrawInfo->Flags &= ~(shape::DRAW_FLAG_VISIBILITY_PASS_OPAQUE) ;
        //TransDrawInfo->Flags |= shape::DRAW_FLAG_VISIBILITY_PASS_TRANSPARENT | shape::DRAW_FLAG_PRIME_Z_BUFFER ;
        TransDrawInfo->Flags |= shape::DRAW_FLAG_VISIBILITY_PASS_TRANSPARENT ;
        TransDrawInfo->Color.W = 0.99f ;    // Trick material loader into being transparent!
        // Draw now or add to draw list?
        if ((shape::s_DrawSettings.UseDrawList) && (Flags & shape::DRAW_FLAG_SORT))
        {
            // Add to the draw list for later...
            shape::AddInstanceToDrawList(*this, *TransDrawInfo) ;
        }
        else
        {
            // Draw model    
            m_pModel->Draw(*TransDrawInfo) ;
        }
    }
    
    // Success!
    return TRUE ;
}

//==============================================================================

// Moves and animates shape_instance ready for next frame
void shape_instance::Advance(f32         DeltaTime,
                             vector3*    DeltaPos,
                             quaternion* DeltaRot)
{
    f32         PrevFrame,  NextFrame ;
    vector3     PrevPos,    NextPos ;
    quaternion  PrevRot,    NextRot ;
    anim*       Anim ;
    s32         tA, tB ;
    f32         Frac ;

    PrevFrame = 0;

    // Clear delta movement from animation
    if (DeltaPos)
        DeltaPos->Zero() ;
    if (DeltaRot)
        DeltaRot->Identity() ;

    // If no shape, exit
    if (!m_pShape)
        return ;
        
    // Advance the texture animation frame
    m_TextureFrame += m_TextureAnimFPS * DeltaTime ;

    // If this is a child instance, set the dirty bits incase parent processing happens first!
    if (m_ParentInstance)
        m_Flags |= FLAG_L2W_DIRTY | FLAG_WORLD_BOUNDS_DIRTY ;

    // If no animations, exit
    if (!m_pShape->GetAnimCount())
        return ;

    // Advance all animations...

    // Update L2W, bounds, and render nodes
    m_Flags |= FLAG_L2W_DIRTY | FLAG_WORLD_BOUNDS_DIRTY | FLAG_RENDER_NODES_DIRTY ;

    // Lookup previous pos or rot?
    if ((DeltaPos) || (DeltaRot))
    {
        // Get current anim and frame
        Anim      = m_CurrentAnim.GetAnim() ;
        PrevFrame = m_CurrentAnim.GetFrame() ;
        ASSERT(Anim) ;

        // Get current anim time info
        Anim->SetupKeyAccessVars(PrevFrame, tA, tB, Frac) ;

        // Lookup keys
        Anim->GetKeySet(0).GetPosKey(PrevPos, tA, tB, Frac) ;
        Anim->GetKeySet(0).GetRotKey(PrevRot, tA, tB, Frac) ;
    }

	// Advance the anim
    m_CurrentAnim.Advance(DeltaTime) ;

    // Blend anim running?
    if (m_Blend > 0.0f)
    {
        // Blend towards current anim
        m_Blend -= m_BlendInc * DeltaTime ;

        // End of blend?
        if (m_Blend < 0.0f)
        {
            m_Blend = 0.0f ;
            m_BlendAnim.SetAnim(NULL) ; // Turn off anim so code can use this anim pointer
        }
        else
            m_BlendAnim.Advance(DeltaTime) ; // advance blend anim

    }

    // Start the currently requested anim?
    if (m_CurrentAnimType != m_RequestedAnimType)
    {
        // Get rid of snapping when blending - only allow instance to go
        // straight back to the old current anim!
        if (IsBlending())
        {
            // Trying to go back to the original anim? 
            if ((m_CurrentAnimType != -1) && (m_RequestedAnimType == m_BlendAnim.GetAnim()->GetType()))
            {
                // Trying to blend to the new anim?
                if (m_RequestedBlendTime != 0.0f)
                {
                    // Blend back to the original anim
                    anim_state CurrentAnim  = m_CurrentAnim ;
                    m_CurrentAnim           = m_BlendAnim ;
                    m_BlendAnim             = CurrentAnim ;
                    m_Blend                 = 1.0f - m_Blend ;    // Flip blend time
                    m_BlendInc              = 1.0f / m_RequestedBlendTime ;
                }
                else
                {
                    // Jump straight back to the anim
                    m_CurrentAnim   = m_BlendAnim ;
                    m_Blend         = 0.0f ;
                }

                // Flag requested type is playing
                m_CurrentAnimType = m_RequestedAnimType ;
            }
        }
        else
        {
            // Flag requested type is playing
            m_CurrentAnimType = m_RequestedAnimType ;

            // Search for new animation
            Anim = m_pShape->GetAnimByType(m_RequestedAnimType) ;

            // Only set if found, otherwise leave the old one playing
            if (Anim)
            {
                // Blend to new anim?
                if (m_RequestedBlendTime != 0.0f)
                {
                    // Copy current anim to blend anim
                    m_BlendAnim = m_CurrentAnim ;
            
                    // Start the blend
                    m_Blend    = 1.0f ;
                    m_BlendInc = 1.0f / m_RequestedBlendTime ;
                }
                else
                {
                    // No blend
                    m_Blend = 0.0f ;
                }

                // Set new current anim
                m_CurrentAnim.SetAnim(Anim) ;
                m_CurrentAnim.SetFrame((Anim->GetFrameCount()-1) * m_RequestedStartFrame) ;
            }
        }

        // Keep logic working for the new anim
        m_CurrentAnim.SetAnimPlayed(0) ;
    }

    // Advance the active masked/additive animations
    AdvanceSpecialAnims( DeltaTime ) ;

    // Lookup next pos or rot?
    if ((DeltaPos) || (DeltaRot))
    {
        // Get current anim and frame
        Anim        = m_CurrentAnim.GetAnim() ;
        NextFrame   = m_CurrentAnim.GetFrame() ;
        ASSERT(Anim) ;

        // Looping not currently supported...
        if (NextFrame <= PrevFrame)
        {
            // Clear deltas
            if (DeltaPos)
                DeltaPos->Zero() ;

            if (DeltaRot)
                DeltaRot->Identity() ;
        }
        else
        {
            // Get current anim time info
            Anim->SetupKeyAccessVars(NextFrame, tA, tB, Frac) ;
       
            // Lookup keys
            Anim->GetKeySet(0).GetPosKey(NextPos, tA, tB, Frac) ;
            Anim->GetKeySet(0).GetRotKey(NextRot, tA, tB, Frac) ;

            // Calculate delta's
            if (DeltaPos)
                *DeltaPos = NextPos - PrevPos ;
        
            PrevRot.Invert() ;
            if (DeltaRot)
                *DeltaRot = PrevRot * NextRot ;  // Right->Left
        }
    }
}

//==============================================================================

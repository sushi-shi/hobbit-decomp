//==============================================================================
//
//  fx_Effect.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Auxiliary/fx_RunTime/fx_Mgr.hpp>
#include <xCore/Entropy/Entropy.hpp>

//==============================================================================
//  FX_EFFECT_BASE FUNCTIONS
//==============================================================================

RVA(0x2ba0b0, 0x88)
void fx_effect_base::Initialize( const fx_def* pEffectDef )
{
    m_Scale      ( 1.0f, 1.0f, 1.0f );
    m_Rotation   (  R_0,  R_0,  R_0 );
    m_Translation( 0.0f, 0.0f, 0.0f );
    m_Color.Set  (  255,  255,  255,  (u8)255 );

    m_pEffectDef  = pEffectDef;
    m_Flags       = pEffectDef->Flags;
    m_NReferences = 0;
    m_L2WDirty    = TRUE;
    m_EL2WDirty   = TRUE;
    m_ColorDirty  = TRUE;
    m_BBoxDirty   = TRUE;
    m_BBox.Clear();
}

//==============================================================================

RVA(0x2ba140, 0x128)
const matrix4& fx_effect_base::GetL2W( void ) const
{
    if( m_L2WDirty )
    {
        m_L2W.Setup( m_Scale, m_Rotation, m_Translation );
        m_L2WDirty = FALSE;
    }
    return( m_L2W );
}

//==============================================================================

RVA(0x2ba270, 0x10)
f32 fx_effect_base::GetUniformScale( void ) const
{
    return( (m_Scale.GetX() + m_Scale.GetY() + m_Scale.GetZ()) / 3.0f );
}

//==============================================================================

const vector3& fx_effect_base::GetScale( void ) const
{
    return( m_Scale );
}

//==============================================================================

const radian3& fx_effect_base::GetRotation( void ) const
{
    return( m_Rotation );
}

//==============================================================================

RVA(0x2ba280, 0x4)
const vector3& fx_effect_base::GetTranslation( void ) const
{
    return( m_Translation );
}

//==============================================================================

xcolor fx_effect_base::GetColor( void ) const
{
    return( m_Color );
}

//==============================================================================

RVA(0x2ba290, 0x23)
void fx_effect_base::SetScale( const vector3& Scale )
{
    ASSERT( Scale.IsValid() );
    m_Scale     = Scale;
    m_L2WDirty  = TRUE;
    m_EL2WDirty = TRUE;
}

//==============================================================================

RVA(0x2ba2c0, 0x23)
void fx_effect_base::SetRotation( const radian3& Rotation )
{
    ASSERT( Rotation.IsValid() );
    m_Rotation  = Rotation;
    m_L2WDirty  = TRUE;
    m_EL2WDirty = TRUE;
}

//==============================================================================

RVA(0x2ba2f0, 0x23)
void fx_effect_base::SetTranslation( const vector3& Translation )
{
    ASSERT( Translation.IsValid() );
    m_Translation = Translation;
    m_L2WDirty    = TRUE;
    m_EL2WDirty   = TRUE;
}

//==============================================================================

RVA(0x2ba320, 0x1f)
void fx_effect_base::SetColor( const xcolor Color )
{
    m_Color      = Color;
    m_ColorDirty = TRUE;
}

//==============================================================================

RVA(0x2ba370, 0x7)
void fx_effect_base::AddReference( void )
{
    m_NReferences++;
}

//==============================================================================

RVA(0x2ba380, 0x7)
void fx_effect_base::RemoveReference( void )
{
    m_NReferences--;
}

//==============================================================================

RVA(0x2ba390, 0x7)
s32 fx_effect_base::GetReferences( void )
{
    return( m_NReferences );
}

//==============================================================================


RVA(0x2ba340, 0x2d)
void fx_effect_base::GetBitmaps( s32 Index, const xbitmap*& pDiffuseMap, 
                                            const xbitmap*& pAlphaMap ) const
{
    ASSERT( Index < m_pEffectDef->NBitmaps );

    pDiffuseMap = m_pEffectDef->pDiffuseMap[Index];
    pAlphaMap   = m_pEffectDef->pAlphaMap[Index];
}

//==============================================================================

RVA(0x2ba3a0, 0x7)
fx_element** fx_effect_base::GetElementList( void )
{
    return( m_pElement );
}

//==============================================================================

RVA(0x2ba3b0, 0x195)
void fx_effect_base::Render( void )
{
    // Render each element.
    for( s32 i = 0; i < m_pEffectDef->NElements; i++ )
    {
              fx_element*      pElement    = m_pElement[i];
        const fx_element_def*  pElementDef = m_pEffectDef->pElementDef[i];

        if( m_ColorDirty )
            pElement->BaseColor( m_Color );

        if( (GetAge() >= pElementDef->TimeStart) && 
            (!pElement->IsFinished( this )) )
        {          
            pElement->BaseColor();
            pElement->Render( this );
        }        
    }

    m_ColorDirty = FALSE;

    //
    // Debug rendering.
    //

#ifdef DEBUG_FX
    if( !FXDebug.EffectReserved )
        return;

    if( FXDebug.EffectCenter )
        draw_Marker( m_Translation, XCOLOR_RED );

    if( FXDebug.EffectBounds )
        draw_BBox( GetBounds(), XCOLOR_RED );

    if( FXDebug.EffectAxis || FXDebug.EffectVolume )
    {
        draw_SetL2W( m_L2W );
        if( FXDebug.EffectAxis )
            draw_Axis( 100.0f );
        if( FXDebug.EffectVolume )
            draw_BBox( bbox( vector3(-0.5f,-0.5f,-0.5f),
                             vector3( 0.5f, 0.5f, 0.5f) ), XCOLOR_BLUE );
        draw_ClearL2W();
    }
#endif // DEBUG_FX
}

//==============================================================================
//  FX_EFFECT_CLONE FUNCTIONS
//==============================================================================

RVA(0x2ba550, 0xf8)
const bbox& fx_effect_clone::GetBounds( void ) const
{
    if( m_BBoxDirty )
    {
        m_BBox = m_pEffect->GetBounds();
        m_BBox.Transform( GetL2W() );

        m_BBoxDirty = FALSE;
    }

    return( m_BBox );
}

//==============================================================================

RVA(0x2ba650, 0xb)
f32 fx_effect_clone::GetAge( void ) const
{
    return( m_pEffect->GetAge() );
}

//==============================================================================

RVA(0x2ba660, 0xb)
xbool fx_effect_clone::IsSuspended( void ) const
{
    return( m_pEffect->IsSuspended() );
}

//==============================================================================

RVA(0x2ba670, 0xb)
xbool fx_effect_clone::IsFinished( void ) const
{
    return( m_pEffect->IsFinished() );
}

//==============================================================================



//==============================================================================

RVA(0x2ba680, 0xb)
void fx_effect_clone::SetSuspended( xbool Suspended )
{
    m_pEffect->SetSuspended( Suspended );
}

//==============================================================================

RVA(0x2ba690, 0x3d)
void fx_effect_clone::AdvanceLogic( f32 DeltaTime )
{
    m_BBoxDirty = TRUE;

    if( !(m_pEffect->m_Flags & FX_MASTER_LOGIC) )
    {
        m_pEffect->m_EL2WDirty = TRUE;
        m_pEffect->AdvanceLogic( DeltaTime );
        m_pEffect->m_Flags |= FX_MASTER_LOGIC;
    }
}

//==============================================================================

RVA(0x2ba6d0, 0x42)
void fx_effect_clone::Restart( void )
{
    m_pEffect->Restart();
    m_BBoxDirty = TRUE;
    m_BBox.Clear();
}

//==============================================================================

RVA(0x2ba720, 0x2b)
void fx_effect_clone::Initialize(       fx_effect_base* pMasterEffect, 
                                  const fx_def*         pEffectDef )
{
    // Initialize the base class.
    fx_effect_base::Initialize( pEffectDef );

    // Attach to the master copy.
    m_pElement = pMasterEffect->GetElementList();
    m_pEffect  = pMasterEffect;
    pMasterEffect->AddReference();
}

//==============================================================================
//  FX_EFFECT FUNCTIONS
//==============================================================================

RVA(0x2ba750, 0x1e0)
const bbox& fx_effect::GetBounds( void ) const
{
    if( m_BBoxDirty )
    {
        s32 i;

        m_BBox.Clear();

        for( i = 0; i < m_pEffectDef->NElements; i++ )
        {
            m_BBox += m_pElement[i]->GetBBox();
        }

        // Check the bounding box.  If it is empty, then add the effect anchor.
        if( (m_BBox.Min.GetX() > m_BBox.Max.GetX()) && 
            (m_BBox.Min.GetY() > m_BBox.Max.GetY()) && 
            (m_BBox.Min.GetZ() > m_BBox.Max.GetZ()) )
        {
            m_BBox += m_Translation;
        }

        m_BBoxDirty = FALSE;
    }

    return( m_BBox );
}

//==============================================================================

RVA(0x2ba930, 0x7)
f32 fx_effect::GetAge( void ) const
{
    return( m_Age );
}

//==============================================================================

xbool fx_effect::IsSuspended( void ) const
{
    return( m_Suspended );
}

//==============================================================================

RVA(0x2ba940, 0x7)
xbool fx_effect::IsFinished( void ) const
{
    return( m_Done );
}

//==============================================================================



//==============================================================================

RVA(0x2ba950, 0x49)
void fx_effect::SetSuspended( xbool Suspended )
{
    if( !m_Suspended && Suspended )
        m_Suspended = TRUE;

    if( m_Suspended && !Suspended )
    {
        if( m_Done )    Restart();
        else            m_Suspended = FALSE;
    }
}

//==============================================================================

RVA(0x2ba9a0, 0x115)
void fx_effect::Initialize( const fx_def* pEffectDef )
{
    s32 i;
    byte* pPointer;

    // Initialize the base class.
    fx_effect_base::Initialize( pEffectDef );

    //
    // Initialize the local data members.
    //

    m_Age        = 0.0f;
    m_Done       = FALSE;
    m_Suspended  = FALSE;

    // 
    // Initialize the staging area, controllers, and elements.
    //
    
    pPointer = (byte*)(this+1);                     // Point to addr AFTER this object.

    // Set the staging area pointer.
    m_pStagingArea = (f32*)pPointer;                // Set staging area pointer.
    pPointer += (pEffectDef->NSAValues * 4);        // Step over memory for staging area.

    // Setup the controllers.

    m_pCtrl = (fx_ctrl**)(pPointer);                // Set controller array pointer.
    pPointer += (pEffectDef->NControllers * 4);     // Step over memory for ctrl ptr array.
    fx_ctrl* pCtrl = (fx_ctrl*)pPointer;            // Pointer to first controller instance.

    for( i = 0; i < pEffectDef->NControllers; i++ )
    {
        // Locals to assist.
        s32 TypeIndex = pEffectDef->pCtrlDef[i]->TypeIndex;

        // Construct the appropriate type.
        fx_mgr::m_CtrlType[ TypeIndex ].pFactoryFn( pCtrl );

        // Enter address in controller array.
        m_pCtrl[i] = pCtrl;

        // Initialize and get values from initial evaluation.
        pCtrl->Initialize( pEffectDef->pCtrlDef[i], m_pStagingArea );
        pCtrl->Evaluate( pCtrl->ComputeLogicalTime() );

        // Advance walking pointer.
        pCtrl++;                                    
    }

    pPointer = (byte*)pCtrl;

    // Setup the elements.

    m_pElement = (fx_element**)pPointer;
    pPointer  += (pEffectDef->NElements * 4);

    
    for( i = 0; i < pEffectDef->NElements; i++ )
    {
        // Locals to assist.
        s32         TypeIndex = pEffectDef->pElementDef[i]->TypeIndex;
        fx_element* pElement  = (fx_element*)pPointer;

        // Construct the appropriate type.
        fx_mgr::m_ElementType[ TypeIndex ].pFactoryFn( pElement );

        // Enter address in element array.
        m_pElement[i] = pElement;

        // Initialize the element.
        pElement->Initialize( pEffectDef->pElementDef[i], m_pStagingArea );

        // Advance the walking pointer.
        pPointer += fx_mgr::m_ElementType[ TypeIndex ].pMemoryFn( *pEffectDef->pElementDef[i] );

    }
}

//==============================================================================

RVA(0x2baac0, 0xd9)
void fx_effect::Restart( void )
{
    s32 i;

    m_Age       = 0.0f;
    m_Done      = FALSE;
    m_Suspended = FALSE;
    m_BBoxDirty = TRUE;
    m_BBox.Clear();

    for( i = 0; i < m_pEffectDef->NControllers; i++ )
    {
        m_pCtrl[i]->Initialize( m_pEffectDef->pCtrlDef[i], m_pStagingArea );
        m_pCtrl[i]->Evaluate( m_pCtrl[i]->ComputeLogicalTime() );
    }

    for( i = 0; i < m_pEffectDef->NElements; i++ )
    {
        m_pElement[i]->Reset();
    }
}

//==============================================================================

RVA(0x2baba0, 0xf3)
void fx_effect::AdvanceLogic( f32 DeltaTime )
{
    s32 i;
    s32 Finished = 0;

    if( m_Done )
        return;

    // Clear the bounding box.  It will get rebuilt during the logic.
    m_BBoxDirty = TRUE;

    // Advance each controller.
    for( i = 0; i < m_pEffectDef->NControllers; i++ )
    {
        m_pCtrl[i]->AdvanceLogic( DeltaTime );
    }

    // Advance each element.
    for( i = 0; i < m_pEffectDef->NElements; i++ )
    {
              fx_element*      pElement    = m_pElement[i];
        const fx_element_def*  pElementDef = m_pEffectDef->pElementDef[i];

        if( GetAge() >= pElementDef->TimeStart )
        {
            if( !pElement->IsFinished( this ) )
            {
                if( m_EL2WDirty )
                    pElement->BaseL2W();

                pElement->BaseLogic();
                pElement->AdvanceLogic( this, DeltaTime );
            }
            else
            {
                Finished++;
            }
        }
    }

    m_EL2WDirty = FALSE;

    // Advance the effect's age.
    m_Age += DeltaTime;

    // All elements are finished?
    if( Finished == m_pEffectDef->NElements )
    {
        m_Done = TRUE;
    }
}

//==============================================================================
//  FORCED CONSTRUCTION FUNCTIONS
//==============================================================================
#undef new
//==============================================================================

RVA(0x2baca0, 0x37)
void fx_effect::ForceConstruct( void* pAddress )
{
    new( pAddress ) fx_effect;
}

//==============================================================================

RVA(0x2bace0, 0x37)
void fx_effect_clone::ForceConstruct( void* pAddress )
{
    new( pAddress ) fx_effect_clone;
}

//==============================================================================

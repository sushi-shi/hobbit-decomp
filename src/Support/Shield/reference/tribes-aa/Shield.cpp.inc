//=========================================================================
//
// SHIELD.CPP
//
//=========================================================================


//=========================================================================
// INCLUDES
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include "Shield.hpp"
#include "AADS/Globals.hpp"


//=========================================================================
// FUNCTIONS
//=========================================================================



//=========================================================================
// Constructor/destructor
//=========================================================================
shield::shield()
{
    Initialize() ;
}

shield::~shield()
{
}


//=========================================================================
// Initialize functions
//=========================================================================

void shield::Initialize(  u32 DirtyBit /* = 0 */ )
{
    m_DirtyBit = DirtyBit ;
    m_Instance.SetShape(NULL) ;
    m_FlashTime = 0 ;
    m_Color     = XCOLOR_WHITE ;
    m_Alpha     = 0 ;
    m_FadeSpeed = 0 ;
}


//=========================================================================
// "Look" functions
//=========================================================================

void shield::SetShape   ( s32 ShapeType )
{
    // Already using this shape (saves a lookup)
    shape* pShape = m_Instance.GetShape() ;
    if ( (pShape) && (pShape->GetType() == ShapeType) )
        return ;

    // Setup shape
    m_Instance.SetShape(tgl.GameShapeManager.GetShapeByType(ShapeType)) ;
}

//=========================================================================

void shield::SetPos     ( vector3 Pos )
{
    m_Instance.SetPos(Pos) ;
}

//=========================================================================

void shield::SetScale   ( vector3 Scale )
{
    m_Instance.SetScale(Scale) ;
}

//=========================================================================

void shield::SetRot     ( radian3 Rot )
{
    m_Instance.SetRot(Rot) ;
}

//=========================================================================

void shield::SetColor   ( xcolor Col ) 
{
    m_Instance.SetColor(Col) ;
}

    
//=========================================================================
// Logic functions
//=========================================================================

void shield::Advance    ( f32 DeltaTime )
{
    // Alpha none zero?
    if (m_Alpha > 0)
    {
        // Fader out alpha
        m_Alpha -= m_FadeSpeed * DeltaTime ;
        if (m_Alpha < 0)
            m_Alpha = 0 ;
    }
}

//=========================================================================

void shield::Render     ( void )
{
    // Temp
    //m_Alpha = 0.25f ;

    // Not visible?
    if (m_Alpha == 0)
        return ;

    // Get current view
    const view *View = &tgl.GetView();
	ASSERT(View) ;

    // Is shape in active view?
    if (!View->BBoxInView(m_Instance.GetWorldBounds()))
        return ;

    // Setup the shape color taking the alpha fade into account
    xcolor Col = m_Color ;
    Col.A = (u8)((f32)Col.A * m_Alpha) ;
    m_Instance.SetColor(Col) ;

    // Draw that puppy
    m_Instance.Draw(shape::DRAW_FLAG_CLIP | shape::DRAW_FLAG_REF_RANDOM | shape::DRAW_FLAG_TURN_OFF_LIGHTING) ;
}

//=========================================================================

void shield::Flash      ( u32& DirtyBits, f32 Time, xcolor Color )
{
    // Flag dirty bits
    DirtyBits |= m_DirtyBit ;

    // Start a fade
    m_FlashTime = Time ;
    m_FadeSpeed = 1.0f / Time ;
    m_Alpha     = 1 ;
    m_Color     = Color ;
}

//=========================================================================

// Net functions
void shield::OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority ;

    // Make sure dirty bit has been initialized
    ASSERT(m_DirtyBit) ;

    // Has shield been flashed since last send?
    if (BitStream.WriteFlag((DirtyBits & m_DirtyBit) != 0))
    {
        // Clear shield dirty bit
        DirtyBits &= ~m_DirtyBit ;

        // Write flash data
        BitStream.WriteF32(m_FlashTime) ;
        BitStream.WriteColor(m_Color) ;
    }
}
    
//=========================================================================

void shield::OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast )
{
    (void)SecInPast ;

    // Temp dirty bits for flash function call
    u32 DirtyBits = 0 ;

    // Flash it?
    if (BitStream.ReadFlag())
    {
        // Read flash data
        BitStream.ReadF32(m_FlashTime) ;
        BitStream.ReadColor(m_Color) ;
           
        // Do the flash
        Flash(DirtyBits, m_FlashTime, m_Color) ;
    }
}

//=========================================================================



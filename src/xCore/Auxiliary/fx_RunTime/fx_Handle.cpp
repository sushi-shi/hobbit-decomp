//==============================================================================
//
//  fx_handle.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Auxiliary/fx_RunTime/fx_Mgr.hpp>

//==============================================================================

RVA(0x2b75e0, 0x9)
fx_handle::fx_handle( void )
{
    Index = -1;
}

//==============================================================================

fx_handle::fx_handle( const fx_handle& Handle )
{
    FXMgr.BindHandle( *this, Handle.Index );
}

//==============================================================================

fx_handle::~fx_handle( void )
{
    FXMgr.UnbindHandle( *this );
}

//==============================================================================

const fx_handle& fx_handle::operator = ( const fx_handle& Handle )
{
    FXMgr.BindHandle( *this, Handle.Index );
    return( *this );
}

//==============================================================================

RVA(0x2b75f0, 0x4a)
xbool fx_handle::InitInstance( const char* pName )
{
    FXMgr.UnbindHandle( *this );

    s32   Index   = -1;
    xbool Success = FXMgr.InitEffect( Index, pName );

    if( Success )
    {
        FXMgr.BindHandle( *this, Index );
    }

    return( Success );
}

//==============================================================================

RVA(0x2b7640, 0x13)
void fx_handle::AdvanceLogic( f32 DeltaTime )
{
    FXMgr.AdvanceLogic( *this, DeltaTime );
}

//==============================================================================

void fx_handle::KillInstance( void )
{
    FXMgr.UnbindHandle( *this );
}

//==============================================================================

RVA(0x2b7670, 0xc)
void fx_handle::Render( void ) const
{
    FXMgr.Render( *this );
}

//==============================================================================

RVA(0x2b7680, 0xc)
void fx_handle::Restart( void )
{
    FXMgr.RestartEffect( *this );
}

//==============================================================================

RVA(0x2b7690, 0xc)
xbool fx_handle::IsFinished( void ) const
{
    return( FXMgr.IsFinished( *this ) );
}

//==============================================================================



//==============================================================================

RVA(0x2b76a0, 0x13)
void fx_handle::SetScale( const vector3& Scale )
{
    FXMgr.SetScale( *this, Scale );
}

//==============================================================================

RVA(0x2b76c0, 0x13)
void fx_handle::SetRotation( const radian3& Rotation )
{
    FXMgr.SetRotation( *this, Rotation );
}

//==============================================================================

RVA(0x2b76e0, 0x13)
void fx_handle::SetTranslation( const vector3& Translation )
{
    FXMgr.SetTranslation( *this, Translation );
}

//==============================================================================

vector3 fx_handle::GetTranslation( void ) const
{
    return FXMgr.GetTranslation( *this );
}

//==============================================================================

void fx_handle::SetTransform( const matrix4& L2W )
{
    FXMgr.SetScale      ( *this, L2W.GetScale()         );
    FXMgr.SetRotation   ( *this, L2W.GetRotation()      );
    FXMgr.SetTranslation( *this, L2W.GetTranslation()   );
}

//==============================================================================

RVA(0x2b7700, 0x13)
void fx_handle::SetColor( const xcolor& Color )
{
    FXMgr.SetColor( *this, Color );
}

//==============================================================================

xcolor fx_handle::GetColor( void )
{
    return FXMgr.GetColor( *this );
}

//==============================================================================

RVA(0x2b7720, 0x13)
void fx_handle::SetSuspended( xbool Suspended )
{
    FXMgr.SetSuspended( *this, Suspended );
}

//==============================================================================

RVA(0x2b7740, 0xc)
const bbox& fx_handle::GetBounds( void ) const
{
    return( FXMgr.GetBounds( *this ) );
}

//==============================================================================

xbool fx_handle::Validate( void ) const
{
    return( FXMgr.Validate( *this ) );
}

//==============================================================================

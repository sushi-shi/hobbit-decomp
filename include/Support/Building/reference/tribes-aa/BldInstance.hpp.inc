#ifndef BLD_INSTANCE_HPP
#define BLD_INSTANCE_HPP

///////////////////////////////////////////////////////////////////////////
// 
// This is the only public interace for the buildings.
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////

#include <xCore/x_files/x_math.hpp>
#include "bldmanager.hpp"

///////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////

class ps2building;

///////////////////////////////////////////////////////////////////////////
// BUILDING INSTANCE
///////////////////////////////////////////////////////////////////////////
//
//  Initialzie          - Initialize the buildng module. 
//  Kill                - Kills the building module
//  LoadBuilding        - Returns an instance of a building. After you are done
//                        using it you should delete it.    
//  Render              - Renders the building.
//  SetLocalMatrix      - Sets the matrix of the building.
//  GetZoneFromPoint    - Gets the zone where the given point is.
//                        Zone 0 you are out any other zone you are in the building.
//
///////////////////////////////////////////////////////////////////////////

class bld_instance : public bld_manager::instance
{
///////////////////////////////////////////////////////////////////////////
public:

    typedef building::raycol            raycol;
    typedef building::prep_render_info  prep_info;


///////////////////////////////////////////////////////////////////////////
public:
    
                        bld_instance        ( void );
                       ~bld_instance        ( void );

    void                LoadBuilding        ( const char* pFileName, xbool LoadAlarmMaps );
    void                LoadBuilding        ( const char* pFileName, const matrix4& L2W, xbool LoadAlarmMaps );
	void				DeleteBuilding		( void );
    void                SetLocalMatrix      ( const matrix4& L2W );
    void                GetBBox             ( bbox& BBox );

    void                PrepRender          ( void ); 
    const building&     GetBuilding         ( void );
    u32                 IsBuildingInView    ( const view& View ) const;

    const matrix4&      GetL2W              ( void );
    const matrix4&      GetW2L              ( void );
    void                RefreshTextures     ( void );

    s32                 GetZoneFromPoint    ( const vector3& Point ) const;
    xbool               CastRay             ( const vector3& S, const vector3& E, raycol& Info ) const;
	
	void				LightBuilding		( void );
    void                SetAlarmLighting    ( xbool State );
};

///////////////////////////////////////////////////////////////////////////
// END
///////////////////////////////////////////////////////////////////////////
#endif
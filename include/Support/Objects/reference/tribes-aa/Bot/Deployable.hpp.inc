#ifndef DEPLOYABLE_HPP
#define DEPLOYABLE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/x_files/x_files.hpp>
#include <Support/ObjectMgr/Object.hpp>
#include <Support/ObjectMgr/ObjectMgr.hpp>

//==============================================================================
//  ABSTRACT BASE CLASS
//==============================================================================
class asset_spot
{
    public:
//------------------------------------------------------------------------------
//  TYPES
//------------------------------------------------------------------------------
    enum team
    {
        TEAM_STORM,
        TEAM_INFERNO,

        TEAM_NEUTRAL = 0x04
    };

    enum
    {
        MAX_INVENS = 5,
        MAX_TURRETS = 50,
        MAX_MINES   = 50,
        MAX_SENSORS = 50,
        FLAG_BAD_SPOT = (1<<31),
    };

    enum priority_value
    {
        PRIORITY_LOW = 1,
        PRIORITY_MEDIUM = 3,
        PRIORITY_HIGH = 9,
    };

//------------------------------------------------------------------------------
//  Public Functions
    public:
                        asset_spot  ( void );
virtual                ~asset_spot  ( void ) { }
virtual void            Clear       ( void );
virtual void            Set         ( const vector3& Pos, priority_value Pval, 
                                     u32 Flags, object::id SwitchID);
virtual void            operator=   ( const asset_spot& RHS);

const   vector3&        GetStandPos ( void )   { return m_StandPos; }
        priority_value  GetPriority ( void )   { return m_Priority; }
        u32             GetFlags    ( void )   { return m_Flags; }
        void            AddFlag     ( u32 Flag ) { m_Flags |= Flag; }
        void            ClearFlag   ( u32 Flag ) { m_Flags &= ~Flag; }

virtual const   vector3&        GetAssetPos ( void ) = 0;

        object::id      GetSwitch   ( void )    { return m_SwitchID; }

//------------------------------------------------------------------------------
// Protected Members    
    protected:
        vector3         m_StandPos;

        priority_value  m_Priority;

        u32             m_Flags;

        object::id      m_SwitchID;

};

//==============================================================================
//  DEPLOYABLE INVENTORY STATION
//==============================================================================

class inven_spot : public asset_spot
{
//------------------------------------------------------------------------------
//  Public Functions
    public:
        
                        inven_spot  ( void );
                       ~inven_spot  ( void ) {}
        void            Clear       ( void );
        void            Set         ( const vector3& Pos, 
                                      const vector3& AssetPos, 
                                      radian   Yaw,
                                      priority_value Pval, 
                                      u32 Flags = 0,
                                      object::id SwitchID = obj_mgr::NullID);

        void            Set         ( const inven_spot& Inven );
        void            operator=   ( const inven_spot& RHS );

const   vector3&        GetAssetPos ( void )    { return m_AssetPos; }
        radian          GetYaw      ( void )    { return m_Yaw; }

//------------------------------------------------------------------------------
// Protected Members    
    protected:
        vector3         m_AssetPos;
        radian          m_Yaw;             
};

//==============================================================================
//  DEPLOYABLE TURRET 
//==============================================================================

class turret_spot : public asset_spot
{
//------------------------------------------------------------------------------
// Public Functions
    public:
                        turret_spot ( void );
                        ~turret_spot( void ) {}
        void            Clear       ( void );
        void            Set         ( const vector3& Pos, 
                                      const vector3& AssetPos, 
                                      const vector3& Normal,
                                      priority_value Pval, 
                                      u32 Flags = 0,
                                      object::id SwitchID = obj_mgr::NullID);
        void            Set         ( const turret_spot& Turret );
        void            operator=   ( const turret_spot& RHS );

const   vector3&        GetAssetPos ( void )    { return m_AssetPos; }
const   vector3&        GetNormal   ( void )    { return m_Normal; }

//------------------------------------------------------------------------------
// Protected Members    
    protected:
        vector3         m_AssetPos;
        vector3         m_Normal;
};

//==============================================================================
//  DEPLOYABLE SENSOR
//==============================================================================

class sensor_spot : public asset_spot
{
//------------------------------------------------------------------------------
// Public Functions
    public:
                        sensor_spot ( void );
                        ~sensor_spot( void ) {}
        void            Clear       ( void );
        void            Set         ( const vector3& Pos, 
                                      const vector3& AssetPos, 
                                      const vector3& Normal,
                                      priority_value Pval, 
                                      u32 Flags = 0,
                                      object::id SwitchID = obj_mgr::NullID);
        void            Set         ( const sensor_spot& Sensor );
        void            operator=   ( const sensor_spot& RHS );

const   vector3&        GetAssetPos ( void )    { return m_AssetPos; }
const   vector3&        GetNormal   ( void )    { return m_Normal; }

//------------------------------------------------------------------------------
// Protected Members    
    protected:
        vector3         m_AssetPos;
        vector3         m_Normal;
};

//==============================================================================
//  MINES
//==============================================================================

class mine_spot : public asset_spot
{
//------------------------------------------------------------------------------
// Public Functions
    public:
                        mine_spot   ( void );
                        ~mine_spot  ( void ) {}
        void            Clear       ( void );
        void            Set         ( const vector3& Pos, 
                                      const radian3& PlayerRot,
                                      priority_value Pval, 
                                      u32 Flags = 0,
                                      object::id SwitchID = obj_mgr::NullID);
        void            Set         ( const mine_spot& Mine);
        void            operator=   ( const mine_spot& RHS);
const   vector3&        GetAssetPos ( void )    { return m_StandPos; }

const   radian3&        GetPlayerRot( void )    { return m_PlayerRot; }

//------------------------------------------------------------------------------
// Protected Members    
    protected:
        radian3         m_PlayerRot;
};

//==============================================================================

#endif
#include "Deployable.hpp"

//==============================================================================

asset_spot::asset_spot( void )
{
    Clear();    
}

//==============================================================================
void asset_spot::Clear( void )
{
     m_StandPos.Zero();
     m_Priority = PRIORITY_LOW;
     m_Flags = 0;
     m_SwitchID = obj_mgr::NullID;
}

//==============================================================================

void asset_spot::operator =(const asset_spot& RHS)
{
    Set(RHS.m_StandPos, RHS.m_Priority, RHS.m_Flags, RHS.m_SwitchID);
}

//==============================================================================

void asset_spot::Set(const vector3& Pos, priority_value Pval, u32 Flags, 
                            object::id SwitchID)
{
    m_StandPos = Pos;
    m_Priority = Pval;
    m_Flags = Flags;
    m_SwitchID = SwitchID;
}

//==============================================================================

inven_spot::inven_spot( void )
{
    Clear();
}

//==============================================================================

void inven_spot::Clear( void )
{
    asset_spot::Clear();
    m_AssetPos.Zero();
    m_Yaw = 0.0f;
}

//==============================================================================

void inven_spot::operator =(const inven_spot& RHS)
{
    Set(RHS);
}

//==============================================================================

void inven_spot::Set(    const  vector3& Pos, 
                         const  vector3& AssetPos, 
                                radian   Yaw,
                                priority_value Pval, 
                                u32 Flags,
                                object::id SwitchID)
{
    asset_spot::Set(Pos, Pval, Flags, SwitchID);
    m_AssetPos = AssetPos;
    m_Yaw = Yaw;
}

//==============================================================================

void inven_spot::Set(const inven_spot& Inven)
{
    Set(Inven.m_StandPos, Inven.m_AssetPos, Inven.m_Yaw, Inven.m_Priority, 
        Inven.m_Flags, Inven.m_SwitchID);
}

//==============================================================================

turret_spot::turret_spot( void )
{
    Clear();
}

//==============================================================================

void turret_spot::Clear( void )
{
    asset_spot::Clear();
    m_AssetPos.Zero();
    m_Normal.Zero();
}

//==============================================================================

void turret_spot::operator =(const turret_spot& RHS)
{
    Set(RHS);
}

//==============================================================================

void turret_spot::Set(   const  vector3& Pos, 
                         const  vector3& AssetPos, 
                         const  vector3& Normal,
                                priority_value Pval, 
                                u32 Flags,                                
                                object::id SwitchID)
{
    asset_spot::Set(Pos, Pval, Flags, SwitchID);

    m_AssetPos = AssetPos;
    m_Normal = Normal;

    ASSERT(Normal.LengthSquared() < 100.0f);
}

//==============================================================================

void turret_spot::Set(const turret_spot& Turret)
{
    Set(Turret.m_StandPos, Turret.m_AssetPos, Turret.m_Normal, Turret.m_Priority, 
        Turret.m_Flags, Turret.m_SwitchID);
}

//==============================================================================

mine_spot::mine_spot( void )
{
    Clear();
}

//==============================================================================

void mine_spot::Clear( void )
{
    asset_spot::Clear();
    m_PlayerRot.Zero();
}

//==============================================================================

void mine_spot::operator =(const mine_spot& RHS)
{
    Set(RHS);
}

//==============================================================================

void mine_spot::Set(    const  vector3& Pos, 
                        const   radian3& PlayerRot,
                                priority_value Pval, 
                                u32 Flags,                                
                                object::id SwitchID)
{
    asset_spot::Set(Pos, Pval, Flags, SwitchID);
    m_PlayerRot = PlayerRot;
}

//==============================================================================

void mine_spot::Set(const mine_spot& Mine)
{
    Set(Mine.m_StandPos, Mine.m_PlayerRot, Mine.m_Priority, Mine.m_Flags,
        Mine.m_SwitchID);
}

//==============================================================================

sensor_spot::sensor_spot( void )
{
    Clear();
}

//==============================================================================

void sensor_spot::Clear( void )
{
    asset_spot::Clear();
    m_AssetPos.Zero();
    m_Normal.Zero();
}

//==============================================================================

void sensor_spot::operator =(const sensor_spot& RHS)
{
    Set(RHS);
}

//==============================================================================

void sensor_spot::Set(   const  vector3& Pos, 
                         const  vector3& AssetPos, 
                         const  vector3& Normal,
                                priority_value Pval, 
                                u32 Flags,                                
                                object::id SwitchID)
{
    asset_spot::Set(Pos, Pval, Flags, SwitchID);
    m_AssetPos = AssetPos;
    m_Normal = Normal;
    ASSERT(Normal.LengthSquared() < 100.0f);
}

//==============================================================================

void sensor_spot::Set(const sensor_spot& Sensor)
{
    Set(Sensor.m_StandPos, Sensor.m_AssetPos, Sensor.m_Normal, 
        Sensor.m_Priority, Sensor.m_Flags, Sensor.m_SwitchID);
}

//==============================================================================


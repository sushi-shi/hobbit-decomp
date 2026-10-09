#ifndef AIMER_HPP
#define AIMER_HPP

xbool CalculateLinearAimDirection(
      const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    ,       f32      VelInheritFactor
    ,       f32      LinearSpeed
    ,       f32      LifetimeS
    ,       vector3& AimDirection 
    ,       vector3& AimPosition
    ,       xbool    Verbose = FALSE );

xbool CalculateArcingAimDirection(
      const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    ,       f32      VelInheritFactor
    ,       f32      MuzzleSpeed
    ,       f32      LifetimeS
    ,       vector3& AimDirection
    ,       vector3& AimPosition
    ,       f32      Gravity = 25.0f );

xbool CalculateArcingAimDirectionSlow(
      const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    ,       f32      VelInheritFactor
    ,       f32      MuzzleSpeed
    ,       f32      LifetimeS
    ,       vector3& AimDirection
    ,       vector3& AimPosition
    ,       f32      Gravity );

f32 GetLastArcingTime( void );

void CalculateArcingImpactPos(
      const vector3&    SourcePos
    , const vector3&    SourceVel
    ,       vector3&    Impact
    ,       object::id& ObjectHitID
    ,       void*       pMovingObj
    ,       s32         ExcludeObjID
    ,       f32         Gravity );

#endif //ndef AIMER_HPP

// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/Object.cpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  Object.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <xCore/Entropy/Entropy.hpp> // For DebugRender().
#include <xCore/Auxiliary/Parsing/bitstream.hpp>

//==============================================================================
//  OBJECT FUNCTIONS
//==============================================================================

object::object(void) {
    m_WorldPos(0, 0, 0);
    m_WorldBBox(vector3(-5, -5, -5), vector3(5, 5, 5));
    m_TeamBits = 0;
}

//==============================================================================

object::~object(void) {
    ASSERT(m_ObjectID.Slot = -1); // Object not removed from ObjMgr!
}

//==============================================================================

void object::OnAdd(void) {}

//==============================================================================

void object::OnRemove(void) {}

//==============================================================================

object::update_code object::OnAdvanceLogic(f32 DeltaTime) {
    (void)DeltaTime;
    return (NO_UPDATE);
}

//==============================================================================

void object::OnCollide(u32 AttrBits, collider& Collider) {
    (void)AttrBits;

    if (m_AttrBits & ATTR_PERMEABLE) {
        Collider.ApplyPermeableBBox(m_WorldBBox);
    } else {
        Collider.ApplyBBox(m_WorldBBox);
    }
}

//==============================================================================

void object::Render(void) {
    draw_BBox(m_WorldBBox, XCOLOR_GREEN);
}

//==============================================================================

void object::DebugRender(void) {
    draw_BBox(m_WorldBBox, XCOLOR_RED);
}

//==============================================================================

u32 object::GetTeamBits(void) const {
    return (m_TeamBits);
}

//==============================================================================

u32 object::GetAttrBits(void) const {
    return (m_AttrBits);
}

//==============================================================================

u32 object::GetDirtyBits(void) const {
    return (m_DirtyBits);
}

//==============================================================================

void object::ClearDirtyBits(void) {
    m_DirtyBits = 0x00000000;
}

//==============================================================================

object::id object::GetObjectID(void) const {
    return (m_ObjectID);
}

//==============================================================================

object::type object::GetType(void) const {
    return (m_pTypeInfo->Type);
}

//==============================================================================

const char* object::GetTypeName(void) const {
    return (m_pTypeInfo->pTypeName);
}

//==============================================================================

obj_type_info* object::GetTypeInfoPtr(void) const {
    return (m_pTypeInfo);
}

//==============================================================================

void object::OnAcceptInit(const bitstream& BitStream) {
    (void)BitStream;
}

//==============================================================================

void object::OnProvideInit(bitstream& BitStream) {
    (void)BitStream;
}

//==============================================================================

object::update_code object::OnAcceptUpdate(const bitstream& BitStream, f32 SecInPast) {
    (void)BitStream;
    (void)SecInPast;
    return (NO_UPDATE);
}

//==============================================================================

void object::OnProvideUpdate(bitstream& BitStream, u32& DirtyBits, f32 Priority) {
    (void)BitStream;
    (void)Priority;
    DirtyBits = 0;
}

//==============================================================================

void object::OnPain(const pain& Pain) {
    (void)Pain;
}

//==============================================================================

void object::OnAddVelocity(const vector3& Velocity) {
    (void)Velocity;
}

//==============================================================================

f32 object::GetHealth(void) const {
    return (-1.0f);
}

//==============================================================================

f32 object::GetEnergy(void) const {
    return (0.0f);
}

//==============================================================================

xbool object::IsShielded(void) const {
    return (FALSE);
}

//==============================================================================

f32 object::GetPainRadius(void) const {
    return (0.0f);
}

//==============================================================================

vector3 object::GetPainPoint(void) const {
    return (m_WorldBBox.GetCenter());
}

//==============================================================================

vector3 object::GetBlendPainPoint(void) const {
    vector3 Result = GetPainPoint();
    Result -= GetPosition();
    Result += GetBlendPos();
    return (Result);
}

//==============================================================================

vector3 object::GetBlendFirePos(s32 Index) {
    (void)Index;
    ASSERT(FALSE);
    return (vector3(0, 0, 0));
}

//==============================================================================

const xwchar* object::GetLabel(void) const {
    return (NULL);
}

//==============================================================================

radian3 object::GetRotation(void) const {
    return (radian3(R_0, R_0, R_0));
}

//==============================================================================

vector3 object::GetVelocity(void) const {
    return (vector3(0.0f, 0.0f, 0.0f));
}

//==============================================================================

vector3 object::GetAverageVelocity(void) const {
    return GetVelocity();
}

//==============================================================================

f32 object::GetMass(void) const {
    return (0.0f);
}

//==============================================================================

const bbox& object::GetBBox(void) const {
    return (m_WorldBBox);
}

//==============================================================================

const vector3& object::GetPosition(void) const {
    return (m_WorldPos);
}

//==============================================================================

vector3 object::GetBlendPos(void) const {
    return (m_WorldPos);
}

//==============================================================================

radian3 object::GetBlendRot(void) const {
    return (radian3(R_0, R_0, R_0));
}

//==============================================================================

f32 object::GetSenseRadius(void) const {
    return (0.0f);
}

//==============================================================================

void object::ClearSensedBits(void) {}

//==============================================================================

void object::AddSensedBits(u32 SensedBits) {
    (void)SensedBits;
}

//==============================================================================

u32 object::GetSensedBits(void) const {
    return (0x00);
}

//==============================================================================

void object::MissileTrack(void) {}

//==============================================================================

void object::MissileLock(void) {}

//==============================================================================

void object::UnbindOriginID(object::id OriginID) {
    (void)OriginID;
}

//==============================================================================

void object::UnbindTargetID(object::id TargetID) {
    (void)TargetID;
}

//==============================================================================

matrix4* object::GetNodeL2W(s32 ID, s32 NodeIndex) {
    (void)ID;
    (void)NodeIndex;

    return (NULL);
}

//==============================================================================

void object::SetTeamBits(u32 TeamBits) {
    m_TeamBits = TeamBits;
}

//==============================================================================

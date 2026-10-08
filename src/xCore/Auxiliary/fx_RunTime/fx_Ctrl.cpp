//==============================================================================
//
//  fx_Ctrl.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Auxiliary/fx_RunTime/fx_Mgr.hpp>
#include <xCore/x_files/x_math.hpp>

//==============================================================================

RVA(0x002bf700, 0x55)
void fx_ctrl::Initialize(fx_ctrl_def* pCtrlDef, float* pOutput) {
    float CycleSize = pCtrlDef->DataEnd - pCtrlDef->DataBegin;
    float StartAge = -pCtrlDef->DataBegin / CycleSize;

    m_pCtrlDef = pCtrlDef;
    m_AgeRate = 1.0f / CycleSize;
    m_Cycle = static_cast<int>(x_floor(StartAge));
    m_CycleAge = StartAge - m_Cycle;
    m_pOutput = pOutput;
}

//==============================================================================

RVA(0x002bf760, 0x8a)
void fx_ctrl::AdvanceLogic(float DeltaTime) {
    m_CycleAge += m_AgeRate * DeltaTime;

    while (m_CycleAge >= 1.0f) {
        m_CycleAge -= 1.0f;
        m_Cycle += 1;
    }
    while (m_CycleAge < 0.0f) {
        m_CycleAge += 1.0f;
        m_Cycle -= 1;
    }

    Evaluate(ComputeLogicalTime());
}

//==============================================================================

RVA(0x002bf7f0, 0x44)
float fx_ctrl::ComputeLogicalTime(void) const {
    int LoopCode = FX_TILE;

    if (m_Cycle < 0) {
        LoopCode = m_pCtrlDef->LeadIn;
    }
    if (m_Cycle > 0) {
        LoopCode = m_pCtrlDef->LeadOut;
    }

    switch (LoopCode) {
        default:
        case FX_CLAMP:
            return ((m_Cycle < 0) ? 0.0f : 1.0f);
            break;

        case FX_TILE:
            return (m_CycleAge);
            break;

        case FX_MIRROR:
            return ((m_Cycle & 0x01) ? (1.0f - m_CycleAge) : (m_CycleAge));
            break;
    }
}

//==============================================================================

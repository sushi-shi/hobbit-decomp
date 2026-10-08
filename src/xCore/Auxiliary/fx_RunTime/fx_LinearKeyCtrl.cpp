//==============================================================================
//
//  fx_LinearKeyCtrl.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Auxiliary/fx_RunTime/fx_Mgr.hpp>

//==============================================================================
//  DAMN LINKER!
//==============================================================================

DATA(0x00415cc8)
int fx_LinearKeyCtrl;

//==============================================================================
//  TYPES
//==============================================================================

struct fx_cdef_linear_keys : public fx_ctrl_def {
    int NKeyFrames;
};

//==============================================================================

class linear_key_ctrl : public fx_ctrl {
public:
    void Evaluate(float Time) const;
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

RVA(0x002b7750, 0x11a)
void linear_key_ctrl::Evaluate(float Time) const {
    const fx_cdef_linear_keys* pDef = static_cast<const fx_cdef_linear_keys*>(m_pCtrlDef);
    const float* pKeyTime = reinterpret_cast<const float*>(pDef + 1);
    const float* pKeyData = pKeyTime + pDef->NKeyFrames;
    int i;
    int LowerKey, UpperKey;
    float Blend;

    // The first and last key frames must be at time 0 and 1 respectively.

    //
    // Easy cases first.
    //

    // At the 0.0 key?
    if (Time <= 0.0f) {
        for (i = 0; i < pDef->NOutputValues; i++) {
            m_pOutput[pDef->OutputIndex + i] = *pKeyData;
            pKeyData += pDef->NKeyFrames;
        }

        return;
    }

    // At the 1.0 key?
    if (Time >= 1.0f) {
        // Need last key frame.
        pKeyData += (pDef->NKeyFrames - 1);

        for (i = 0; i < pDef->NOutputValues; i++) {
            m_pOutput[pDef->OutputIndex + i] = *pKeyData;
            pKeyData += pDef->NKeyFrames;
        }

        return;
    }

    //
    // Figure out which two key frames we are between.
    // [[Optimization: Use a local binary search.]]
    //

    for (i = 1; i < pDef->NKeyFrames - 1; i++) {
        if (Time < pKeyTime[i]) {
            break;
        }
    }
    UpperKey = i;
    LowerKey = i - 1;

    // Determine amount of contribution from the upper key.
    Blend = (Time - pKeyTime[LowerKey]) / (pKeyTime[UpperKey] - pKeyTime[LowerKey]);

    //
    // Go ahead and generate the values.
    //
    pKeyData += LowerKey;

    for (i = 0; i < pDef->NOutputValues; i++) {
        m_pOutput[pDef->OutputIndex + i] =
            (*(pKeyData) * (1.0f - Blend)) + (*(pKeyData + 1) * (Blend));
        pKeyData += pDef->NKeyFrames;
    }
}

//==============================================================================

DATA(0x00415cc4)
extern fx_ctrl_reg CtrlReg_linear_key_ctrl;

#undef new
REGISTER_FX_CONTROLLER_CLASS(linear_key_ctrl, "LINEAR KEY");

//==============================================================================

// Natural macro/template and startup contributions, exact decoded extents.
RVA_DYNINIT(0x002b7870, 0x5, CtrlReg_linear_key_ctrl)
RVA_DYNINIT(0x002b7880, 0x15, CtrlReg_linear_key_ctrl)
RVA_COMPGEN(0x002b78a0, 0x5, ?fx_Construct_linear_key_ctrl@@YAXPAX@Z)
RVA_COMPGEN(0x002b78b0, 0xf, ?xConstruct@@YAXPAVlinear_key_ctrl@@@Z)

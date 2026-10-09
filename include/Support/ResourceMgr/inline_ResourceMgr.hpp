// Imported surviving engine source: Area51 pristine431f72b9, Support/ResourceMgr/inline_ResourceMgr.hpp
// Provisional later-version interfaces; no PC identity claims. See IMPORT-NOTES.md.

#ifndef INLINE_RESOURCE_MANAGER_HPP
#define INLINE_RESOURCE_MANAGER_HPP

//==============================================================================
//==============================================================================
// RSC_LOADER
//==============================================================================
//==============================================================================

inline rsc_loader::rsc_loader(const char* pType, const char* pExt) {
    // Confirm that extension begins with a dot
    ASSERT(pExt[0] == '.');

    m_pType = pType;
    m_pExt = pExt;
    m_pNext = rsc_mgr::m_pLoader;
    rsc_mgr::m_pLoader = this;
    rsc_mgr::m_NumLoaders++;
}

//==============================================================================
//==============================================================================
//  RHANDLE FUNCTIONS
//==============================================================================
//==============================================================================

//==============================================================================

template<class T> inline T* rhandle<T>::GetPointer(void) const {
    return (T*)rhandle_base::GetPointer();
}

//==============================================================================
//==============================================================================
//  RHANDLE BASE FUNCTIONS
//==============================================================================
//==============================================================================

//==============================================================================

inline rhandle_base::rhandle_base(void) {
#if defined(HOBBIT_RESOURCE_LATER_LAYOUT)
    SetIndex(-1);
#else
    m_Data = (u32)-1;
#endif
}

//==============================================================================
inline rhandle_base::rhandle_base(const char* pResourceName) {
#if defined(HOBBIT_RESOURCE_LATER_LAYOUT)
    SetIndex(-1);
#else
    m_Data = (u32)-1;
#endif
    g_RscMgr.AddRHandle(*this, pResourceName);
}

//==============================================================================
inline rhandle_base::~rhandle_base(void) {}

//==============================================================================
inline void rhandle_base::Destroy(void) {
#if defined(HOBBIT_RESOURCE_LATER_LAYOUT)
    SetIndex(-1);
#else
    m_Data = (u32)-1;
#endif
}

//==============================================================================
inline void rhandle_base::SetName(const char* pResourceName) {
    if (pResourceName != NULL) {
        g_RscMgr.AddRHandle(*this, pResourceName);
    }
}

//==============================================================================
inline const char* rhandle_base::GetName(void) const {
    return g_RscMgr.GetRHandleName(*this);
}

//==============================================================================
inline void* rhandle_base::GetPointer(void) const {
    return g_RscMgr.GetPointer(*this);
}

//==============================================================================
inline xbool rhandle_base::IsLoaded(void) const {
    return g_RscMgr.IsRHandleLoaded(*this);
}

//==============================================================================
#if defined(HOBBIT_RESOURCE_LATER_LAYOUT)
inline s16 rhandle_base::GetIndex(void) const {
    return (s16)m_Data;
}
#else
inline s32 rhandle_base::GetIndex(void) const {
    return (s32)((m_Data & 0x7fffffff) | ((m_Data & 0x40000000) << 1));
}
#endif

//==============================================================================
#if defined(HOBBIT_RESOURCE_LATER_LAYOUT)
inline void rhandle_base::SetIndex(s16 I) {
    m_Data = I;
}
#else
inline void rhandle_base::SetIndex(s32 I) {
    m_Data = (m_Data & 0x80000000) | ((u32)I & 0x7fffffff);
}

inline void rhandle_base::SetLocked(xbool Locked) {
    if (Locked) m_Data |= 0x80000000;
    else m_Data &= 0x7fffffff;
}
#endif

//==============================================================================

inline void* rsc_mgr::GetPointer(const rhandle_base& RHandle) {
    s16 I = RHandle.GetIndex();

    // Handle null handle
    if (I == -1) {
        return NULL;
    }

    // Confirm range
    ASSERT((I >= 0) && (I < m_nResourcesAllocated));
    ASSERT(m_pResource[I].State != NOT_USED);

    if (m_pResource[I].pData) {
        return m_pResource[I].pData;
    }

    return GetPointerSlow(RHandle);
}

//==============================================================================
// END
//==============================================================================
#endif
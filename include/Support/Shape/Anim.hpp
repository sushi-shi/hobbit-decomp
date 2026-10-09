#ifndef __ANIM_HPP__
#define __ANIM_HPP__

#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/ShapeBinFile.hpp>
#include <Support/Shape/UserData.hpp>

#ifdef TARGET_PS2
#include "libvu0.h"
#endif


//==============================================================================
// Defines
//==============================================================================

#define ANIM_MAX_FNAME  48



//==============================================================================
// Util functions
//==============================================================================

void BlendLinear( const vector3& vA, const vector3& vB, f32 T, vector3& vOut) ;

// PS2 vu0 version used by "matrix4* shape_instance::CalculateNodeMatrices()"
// NOTE: Leaves vOut.W untouched
void BlendLinear( const vector4& vA, const vector4& vB, f32 T, vector4& vOut) ;

// PS2 vu0 version used by "matrix4* shape_instance::CalculateNodeMatrices()"
void BlendFast( const quaternion& Qa, const quaternion& Qb, f32 T, quaternion& qOut ) ;

// PS2 vu0 version used by "matrix4* shape_instance::CalculateNodeMatrices()"
void BlendIdentityFast( const quaternion& Qa, f32 T, quaternion& qOut ) ;


//==============================================================================
// KeySet - A stream of key frames that are used by a single node
//==============================================================================

class key_set : public shape_bin_file_class
{
// Structures
private:

	// Properties
	struct key_set_flags
	{
		u32 NeverAnimate:1 ;		// TRUE if node never animates
        u32 PosKeysCompressed:1 ;   // TRUE if pos keys are in 8bit compressed format
        u32 RotKeysCompressed:1 ;   // TRUE if rot keys are in 8bit compressed format
	} ;


// Members
private:
	key_set_flags	m_Flags ;		        // Flags
	f32				m_AnimWeight ;	        // Used when applying masked anims

    byte*           m_PosKeys ;	            // List of position key frames (vector3's or u8's)
	byte*           m_RotKeys ;             // List of rotation key frames (quaternion's or u8's)

    s32				m_nPosKeys ;	        // Number of position keys
	s32				m_nRotKeys ;	        // Number of rotation keys
                               
    s32             m_nVisKeys ;            // Number of visiblity keys
    byte*           m_VisKeys ;             // List of visibility keys (u8)

    byte            PADDING[12] ;           // Align to 16 bytes for shape_bin_file_class
    
// Functions
public:
	
    // Constructor/destructor
    key_set() ;
	~key_set() ;

	// Flag functions
    xbool	    GetNeverAnimateFlag ( void )			{ return m_Flags.NeverAnimate ; } 
    void	    SetNeverAnimateFlag	( xbool Value )     { m_Flags.NeverAnimate = Value ; }

	// Anim weight functions
	f32		    GetAnimWeight		( void )			{ return m_AnimWeight ;	 }
	void	    SetAnimWeight		( f32 Value )		{ m_AnimWeight = Value ; }

    // Functions
    s32         MemoryUsed          ( void ) ;

    // Initialization functions
    void        SetPosKeyCount      (s32 Count) ;
    void        SetRotKeyCount      (s32 Count) ;
    void        SetVisKeyCount      (s32 Count) ;

    vector3&    GetPosKey           (s32 Index) ;
    void        SetPosKey           (s32 Index, vector3 vPos) ;

    quaternion& GetRotKey           (s32 Index) ;
    void        SetRotKey           (s32 Index, quaternion qRot) ;

    void        SetVisKey           (s32 Index, f32 F) ;
    f32         GetVisKey           (s32 Index) ;

    // Remove x,z translation from position keys
    void        RemoveXZTranslation() ;

    // Optimize memory usage by collapsing a bunch of same keys into one
    void        CollapseKeys() ;

    // Puts keys into compressed 8 bit per component format
    void        Compress() ;

    // Operators
    key_set&  operator =  ( key_set& KeySet );

    // Indexed key access functions
    
    // Returns position key
    void GetPosKey(s32 Index, vector3& PosKey) ;

    // Use for PS2 optimization (leaves PosKey.W untouched)
    void GetPosKey(s32 Index, vector4& PosKey) ;

    // Returns quaternion key
    void GetRotKey(s32 Index, quaternion& RotKey) ;

    // Returns visiblity key
    void GetVisKey(s32 Index, f32& VisKey) ;

    // Animation key access functions

	// Given time A, time B, and the fraction between them (from 0->1, where 0 = time A, 1 = time B)
	// these functions return the interpolated key frame
    void GetPosKey(vector3 &vPos, s32 tA, s32 tB, f32 Frac) ;
    void GetPosKey(vector4 &vPos, s32 tA, s32 tB, f32 Frac) ;
    void GetRotKey(quaternion &qRot, s32 tA, s32 tB, f32 Frac) ;
    void GetVisKey(f32 &Vis, s32 tA, s32 tB, f32 Frac) ;

    // Count functions
    s32         GetPosKeyCount      ()      { return m_nPosKeys ;   }
    s32         GetRotKeyCount      ()      { return m_nRotKeys ;   }
    s32         GetVisKeyCount      ()      { return m_nVisKeys ;   }

    // File IO
    void        ReadWrite(shape_bin_file &File) ;
} ;



//==============================================================================
// Anim - A list of key sets that can be applied to a set of nodes
//==============================================================================



class anim : public shape_bin_file_class
{
// Structures
private:
	// Properties
	struct anim_flags
	{
		u32 Additive:1 ;
		u32 RemoveRootTrans:1 ;
		u32 Compress:1 ;
        u32 HasAnimatedVis:1 ;
	} ;

// Members
private:
    char            m_Name[ANIM_MAX_FNAME] ;// Filename
    time_stamp      m_TimeStamp ;           // Time stamp from .skel file

    s32             m_Index ;               // Index of anim inside shape list
    s32             m_Type ;                // Type of animation
    s32             m_AdditiveRefType ;     // Additive reference type

    f32             m_Weight ;              // Weight used for choosing same types
    anim_flags      m_Flags ;               // Animation flags

    f32             m_Fps ;                 // Frames per sec anim was recorded at
    key_set         *m_KeySets ;	    	// List of key sets
	s32				m_nKeySets ;	    	// Number of key sets
	s32				m_nFrames ;		        // Total number of frames in animation
	s32				m_nLoopToFrame ;    	// Frame to loop back to after completion
   
    //byte            PADDING[4] ;            // Align to 16 bytes for shape_bin_file_class


// Functions
public:

	// Constructors/Destructor
	anim() ;
    anim(anim &SourceAnim, user_data_anim_info &AnimInfo) ;
	~anim() ;

    // Operators
    anim&  operator =  ( anim& Anim ) ;

    // Flag functions
    anim_flags &GetFlags         ( void )       { return m_Flags ;                  }

    xbool GetAdditiveFlag        ( void )       { return m_Flags.Additive ;         } 
    xbool GetRemoveRootTransFlag ( void )       { return m_Flags.RemoveRootTrans ;  } 
    xbool GetCompressFlag        ( void )       { return m_Flags.Compress ;         } 
    xbool GetHasAnimatedVisFlag  ( void )       { return m_Flags.HasAnimatedVis ;   } 

    void SetAdditiveFlag         (xbool Value)  { m_Flags.Additive        = Value ; }
    void SetRemoveRootTransFlag  (xbool Value)  { m_Flags.RemoveRootTrans = Value ; }
    void SetCompressFlag         (xbool Value)  { m_Flags.Compress        = Value ; }
    void SetHasAnimatedVisFlag   (xbool Value)  { m_Flags.HasAnimatedVis  = Value ; }

    // Functions

    // Returns amount of bytes used
    s32 MemoryUsed() ;
    
    // Allocates key set list
    void SetKeySetCount(s32 nKeySets) ;

    // Removes key set from list
    void DeleteKeySet(s32 Index) ;

    // Applies parent keys to given child keys (in preparation for deleting the parent key set)
    void ApplyKeySetToChildKeySet(s32 Parent, s32 Child) ;

	// Returns pointer to requested key set
	key_set &GetKeySet(s32 KeySet)
	{
		ASSERT((KeySet >= 0) && (KeySet < m_nKeySets)) ;
		return m_KeySets[KeySet] ;
	}

    // Inline functions
    const char *GetName         ()                      { return m_Name ;           }
    void        SetName         (const char *Name) ;
                                                        
    time_stamp  GetTimeStamp    ()                      { return m_TimeStamp ;      }
    void        SetTimeStamp    (time_stamp TimeStamp)  { m_TimeStamp = TimeStamp ; }

    s32         GetIndex         ()                     { return m_Index ;          }
    void        SetIndex         (s32 Index)            { m_Index = Index ;         }

    s32         GetType         ()                      { return m_Type ;           }
    void        SetType         (s32 Type)              { m_Type = Type ;           }

    s32         GetAdditiveRefType()                    { return m_AdditiveRefType ;}
    
    f32         GetWeight       ()                      { return m_Weight ;         }
    void        SetWeight       (f32 Weight)            { m_Weight = Weight ;       }
                                                        
    f32         GetFps          ()                      { return m_Fps ;            }
    void        SetFps          (f32 Fps)               { m_Fps = Fps ;             }
                                                        
    s32         GetKeySetCount  ()                      { return m_nKeySets ;       }

    s32         GetFrameCount   ()                      { return m_nFrames ;        }
    void        SetFrameCount   (s32 Count)             { m_nFrames = Count ;       }
    
    s32         GetLoopToFrame  ()                      { return m_nLoopToFrame ;   }
    
    void SetLoopToFrame (s32 Frame)
    {
        ASSERT(Frame >= 0) ;
        ASSERT(Frame < m_nFrames) ;

        m_nLoopToFrame = Frame ;
    }

    // Makes animation additive by removing the first frames orientation from all the keys
    // See the playback code in ShapeInstance.cpp for more details
    void        ConvertToAdditive(anim &RootAnim) ;

    // Scale anim to new fps
    void        Scale(f32 NewFps) ;

    // Optimize memory usage by collapsing a bunch of same keys into one
    void        CollapseKeys() ;

    // Returns TRUE if animation contains no movement!
    xbool       IsStatic() ;

    // Puts keys into compressed 8 bit per component format
    void        Compress() ;

    // Sets up vars ready for accessing keys
    void        SetupKeyAccessVars(f32 Frame, s32& tA, s32& tB, f32& Frac) ;

    // File IO
    void        ReadWrite               (shape_bin_file &File) ;

} ;


//==============================================================================
// Structure used to speed up looking up of animation keys
//==============================================================================
struct anim_info
{
    s32     tA ;        // Current frame time
    s32     tB ;        // Next frame time
    f32     Frac ;      // Blend fraction (0->1) between times
    key_set *KeySet ;   // Current key set (assumes key sets are accessed in node order)
} ;


//==============================================================================
// Animation state - ssed to track playback of an animation
//==============================================================================
class anim_state
{
// Data
private:

    anim_state  *m_Next ;           // Linked list pointer
    s32         m_ID ;              // ID identifier for linked list

	anim		*m_pAnim ;	        // Animation that is being tracked
	f32		    m_Frame ;		    // Current frame within animation
    f32         m_Speed ;           // Playback speed
	f32			m_Weight ;			// Total animation weight multiplier

    s32         m_AnimPlayed ;      // Gets increment every time anim completes loop

// Functions
public:
	// Constructor
	anim_state() ;

	// Functions

    // Linked list functions
    s32         GetID       ()                          { return m_ID ;         }
    anim_state  *GetNext    ()                          { return m_Next ;       }

    void        SetID       (s32 ID)                    { m_ID = ID ;           }
    void        SetNext     (anim_state *AnimState)     { m_Next = AnimState ;  }

	// Sets the current anim
	void SetAnim(anim *pAnim) ;

	// Advance to next frame in current anim
	void Advance(f32 DeltaTime) ;

    // Use the results from this function to call GetKeys()
    void        SetupAnimInfo(anim_info& AnimInfo) ;

    // Inline functions

    // Returns animation
    anim    *GetAnim()          { return m_pAnim ;  }

    // Returns current position of animation playback
    f32     GetFrame()          { return m_Frame ;  } 

    // Sets the current position of animation playback
    void    SetFrame            (f32 Frame) ;

    // Sets the current position of animation playback (use 0->1, where 0=start frame, 1=end frame)
    void    SetFrameParametric  (f32 Frame) ;
    
    // Gets the current position of animation playback (use 0->1, where 0=start frame, 1=end frame)
    f32     GetFrameParametric  ( void ) ;
    
    // Sets playback speed scaler
    void    SetSpeed(f32 Speed) { m_Speed = Speed ; }

    // Returns playback speed scaler
    f32     GetSpeed()          { return m_Speed ; }

    // Sets playback weight scaler
    void    SetWeight(f32 Weight) { m_Weight = Weight ; }

    // Returns playback weight scaler
    f32     GetWeight()          { return m_Weight ; }

    // Returns # of times anim is played
    void    SetAnimPlayed(s32 AnimPlayed)   { m_AnimPlayed = AnimPlayed ; }
    s32     GetAnimPlayed()                 { return m_AnimPlayed ; }

    // Returns position key frame
    inline void GetPosKey(anim_info& AnimInfo, key_set &KeySet, vector3& vPos)
    {
        // Go get the key...
	    KeySet.GetPosKey(vPos, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
    }

    inline void GetPosKey(anim_info& AnimInfo, key_set &KeySet, vector4& vPos)
    {
        // Go get the key...
	    KeySet.GetPosKey(vPos, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
    }

    // Returns rotation key frame
    inline void GetRotKey(anim_info& AnimInfo, key_set &KeySet, quaternion& qRot)
    {
        // Go get the key...
	    KeySet.GetRotKey(qRot, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
    }
    
    // Returns position, scale, and rotation key of requested keyset
    inline void GetKeys(anim_info& AnimInfo, s32 Set, vector3& vPos, quaternion& qRot)
    {
        // Must be anim!
        ASSERT(m_pAnim) ;

	    // Go get the key frames...
	    key_set &KeySet = m_pAnim->GetKeySet(Set) ;
	    KeySet.GetPosKey    (vPos,   AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    KeySet.GetRotKey    (qRot,   AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
    }

    // Regular
    inline void GetKeys(anim_info& AnimInfo, s32 Set, vector4& vPos, quaternion& qRot)
    {
        // Must be anim!
        ASSERT(m_pAnim) ;

	    // Go get the key frames...
	    key_set &KeySet = m_pAnim->GetKeySet(Set) ;
	    KeySet.GetPosKey    (vPos,   AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    KeySet.GetRotKey    (qRot,   AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
    }

    // With visiblity
    inline void GetKeys(anim_info& AnimInfo, s32 Set, vector4& vPos, quaternion& qRot, f32& Vis)
    {
        // Must be anim!
        ASSERT(m_pAnim) ;

	    // Go get the key frames...
	    key_set &KeySet = m_pAnim->GetKeySet(Set) ;
	    KeySet.GetPosKey    (vPos,   AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    KeySet.GetRotKey    (qRot,   AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    KeySet.GetVisKey    (Vis,    AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
    }


    // Returns position, scale, and rotation key of current anim info keyset
    inline void GetKeys(anim_info& AnimInfo, vector3& vPos, quaternion& qRot)
    {
	    // Go get the key frames...
	    AnimInfo.KeySet->GetPosKey(vPos, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    AnimInfo.KeySet->GetRotKey(qRot, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;

        // Next key set for next node
        AnimInfo.KeySet++ ;
    }

    // Regular
    inline void GetKeys(anim_info& AnimInfo, vector4& vPos, quaternion& qRot)
    {
	    // Go get the key frames...
	    AnimInfo.KeySet->GetPosKey(vPos, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    AnimInfo.KeySet->GetRotKey(qRot, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;

        // Next key set for next node
        AnimInfo.KeySet++ ;
    }

    // With visiblity
    inline void GetKeys(anim_info& AnimInfo, vector4& vPos, quaternion& qRot, f32& Vis)
    {
	    // Go get the key frames...
	    AnimInfo.KeySet->GetPosKey(vPos, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    AnimInfo.KeySet->GetRotKey(qRot, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;
	    AnimInfo.KeySet->GetVisKey(Vis,  AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;

        // Next key set for next node
        AnimInfo.KeySet++ ;
    }

} ;

//==============================================================================


#endif  //#ifndef __ANIM_HPP__

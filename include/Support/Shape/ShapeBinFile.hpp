#ifndef __SHAPE_BIN_FILE_HPP__
#define __SHAPE_BIN_FILE_HPP__

//==============================================================================
// A simple class that allows easy binary file IO for shape data
//==============================================================================

#include <xCore/x_files/x_files.hpp>






//==============================================================================
// Structures
//==============================================================================

// Used to check for out of date binary file
struct time_stamp
{
// Data 

    // Makes a nice total of 32 bits so we can cast to a s32
    s32 Seconds:6 ; // 0->59
	s32 Minutes:6 ; // 0->59
	s32 Hours:5 ;   // 0->23
	s32 Day:3 ;     // 0->6
	s32 Month:4 ;   // 0->11
	s32 Year:8 ;    // 0->255   (Beware - this timestamp wraps every Year%256 !)

// Functions
    time_stamp()
    {
        ASSERT(sizeof(time_stamp) == 4) ;
		Clear() ;
    }

    void Clear()
    {
        Seconds = 0 ;
        Minutes = 0 ;
        Hours   = 0 ;
        Day     = 0 ;
        Month   = 0 ;
        Year    = 0 ;
    }

// Operators
    
    // Equal
    xbool operator == ( const time_stamp &T ) const
    {
        return (        (Seconds == T.Seconds)
                    &&  (Minutes == T.Minutes)
                    &&  (Hours   == T.Hours)
                    &&  (Day     == T.Day)
                    &&  (Month   == T.Month)
                    &&  (Year    == T.Year) ) ;
    }

    // Not equal
    xbool operator != ( const time_stamp &T ) const
    {
        return (        (Seconds != T.Seconds)
                    ||  (Minutes != T.Minutes)
                    ||  (Hours   != T.Hours)
                    ||  (Day     != T.Day)
                    ||  (Month   != T.Month)
                    ||  (Year    != T.Year) ) ;
    }
} ;


//==============================================================================
// Classes
//==============================================================================

// Class used to read and write binary shape file
class shape_bin_file
{
// Defines
public:
    enum mode_types
    {
        SHAPE_BIN_FILE_READING,
        SHAPE_BIN_FILE_WRITING
    } ;


// Structures
private:
	// Simple structure used to keep track of allocating memory when creating binary files
	struct shape_alloc_info
	{
		void    *Addr ;     // Address of allocated memory
		s32     Size ;      // Size of allocated memory
	} ;

// Static data
private:
	static X_FILE*					s_File	;			// System file pointer
	static s32                      s_Mode ;			// Mode (read/write)
	static byte*	                s_Mem ;				// Memory allocated for reading
	static byte*                    s_MemAllocPos ;		// Current allocated position
	static s32                      s_MemSize ;			// Total allocation size needed
	static byte*					s_FileBuffer ;		// Allocated file buffer
	static s32						s_FileBufferSize ;	// Total size of file
	static s32						s_FileBufferPos ;	// Current file buffer position

// Private functions
private:
    // The big read/write function that every thing goes through...
    void        ReadWriteMemory(byte *Bytes, s32 Length) ;

    // Call to reserve memory for reading file
    void        Reserve     (s32 Size) ;

// Functions
public:

    // Called from new operator to hookup pointer allocations
    static  void        *Alloc      (s32 Size) ;
    static  void        Free        (void *Memory) ;
    static  s32         GetMode     () ;

    // Constructor/destructor
    shape_bin_file() ;
    ~shape_bin_file() ;
    
    // File functions
    s32         OpenForWriting(const char *Filename) ;
    byte        *OpenForReading(const char *Filename) ;
    void        Close       () ;
    xbool       EndOfFile   () ;
   
    // Read/Write a single class/structure
    template<class T>
    void ReadWrite(T &Struct)
    {
        ReadWriteMemory((byte *)&Struct, sizeof(Struct)) ;
    }

    // Read/Write an array of classes
    template<class T>
    void ReadWriteClassArray(T* &ClassArray, s32 &Count)
    {
        // Read/Write count
        ReadWrite(Count) ;

        // Any there?
        if (Count)
        {
            // Allocate array?
            if (shape_bin_file::GetMode() == SHAPE_BIN_FILE_READING)
            {
                ClassArray = new T[Count] ;
                ASSERT(ClassArray) ;
            }

            // Read/write class array
            for (s32 i = 0 ; i < Count ; i++)
                ClassArray[i].ReadWrite(*this) ;
        }
        else
            ClassArray = NULL ;
    }
    
    // Read/Write an array of structures
    template<class T>
    void ReadWriteStructArray(T* &StructArray, s32 &Count)
    {
        // Read/Write count
        ReadWrite(Count) ;

        // Any there?
        if (Count)
        {
            // Allocate array?
            if (shape_bin_file::GetMode() == SHAPE_BIN_FILE_READING)
            {
                StructArray = (T*)Alloc(sizeof(T) * Count) ;
                ASSERT(StructArray) ;
            }
            else
            {
                ASSERT(shape_bin_file::GetMode() == SHAPE_BIN_FILE_WRITING) ;
                Reserve(sizeof(T) * Count) ;    // Reserve for reading...
            }

            // Read/write whole array in one chunk
            ReadWriteMemory((byte *)StructArray, sizeof(T) * Count) ;
        }
        else
            StructArray = NULL ;
    }
    
   
} ;



// Base all classes that are written to binary shape file here...
class shape_bin_file_class
{
// Data
private:
    // If no padding is added, the PS2 compiler assumes sizeof(shape_bin_file_class) is 4,
    // but the PC 0. So I've added this to force the PC compiler to make it a size 4!
    byte    PADDING[4] ;


// Defines
public:

    // Targets supported
    enum target
    {
        DEFAULT=-1,   

        PC,
        PS2,

        TOTAL
    } ;


// Functions
public:

    // Returns compile target
    static shape_bin_file_class::target GetDefaultTarget() ;

    // Remove x files macro so prototypes compile
    #ifdef new
    #undef new
    #endif

    // Override new and delete operator so pointers are hooked up if reading from a binary file
    
    // Release versions
    void* operator new []   ( u32 Size ) ;
    void* operator new      ( u32 Size ) ;

    void operator delete    ( void* pMemory ) ;
    void operator delete [] ( void* pMemory ) ;

    // Debug versions
    void* operator new []   ( u32 Size, char* pFileName, s32 LineNumber  ) ;
    void* operator new      ( u32 Size, char* pFileName, s32 LineNumber  ) ;

    void operator delete    ( void* pMemory, char* pFileName, s32 LineNumber  ) ;
    void operator delete [] ( void* pMemory, char* pFileName, s32 LineNumber  ) ;
} ;

// Re-include x-types so we get the proper definition of 'new' overridden on a debug build
#undef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>

//==============================================================================

#endif  //#ifndef __SHAPE_BIN_FILE_HPP__


#include <xCore/Entropy/Entropy.hpp>
#include <Support/Shape/ShapeBinFile.hpp>
#include <Support/Shape/Shape.hpp>


//==============================================================================
// Defines
//==============================================================================

//#define SHOW_DEBUG_INFO
// Version (increment every time file format changes or dlist build algorithm changes)
#define SHAPE_FILE_VERSION      31   // Increment every time data format changes!

//==============================================================================
// Static variables
//==============================================================================

X_FILE*                  shape_bin_file::s_File				= NULL ;    // System file pointer
s32                      shape_bin_file::s_Mode				= -1 ;      // Mode (read/write)
byte*                    shape_bin_file::s_Mem				= NULL;     // Memory allocated for reading
byte*                    shape_bin_file::s_MemAllocPos		= NULL ;    // Current allocated position
s32                      shape_bin_file::s_MemSize			= 0 ;       // Total allocation size needed
byte*					 shape_bin_file::s_FileBuffer		= NULL;		// Allocated file buffer
s32						 shape_bin_file::s_FileBufferSize	= 0 ;		// Total size of file
s32						 shape_bin_file::s_FileBufferPos	= 0 ;		// Current file buffer position



//==============================================================================
//
// Class static functions
//
//==============================================================================


// Allocates memory from the heap when not reading a binary file,
// or allocates from single allocation if reading from a binary file
void *shape_bin_file::Alloc(s32 Size)
{
    // Make sure size is aligned for PS2 etc
    Size = ALIGN_16(Size) ;

    // Show info
    #ifdef SHOW_DEBUG_INFO
        shape::printf("   shape_bin_file::Allocating %d bytes...\n", Size) ;
    #endif

    // If reading, return next available memory
    if (s_Mem)
    {
        // Should never over allocate!
        ASSERT((s_MemAllocPos + Size) <= (s_Mem + s_MemSize)) ;

        // Keep copy of current pos ready for returning
        byte *MemAlloc = s_MemAllocPos ;

        // Advance pointer
        s_MemAllocPos += Size ;

        // Should never over allocate!
        ASSERT(s_MemAllocPos <= (s_Mem + s_MemSize)) ;

        // Return allocated mem
        return MemAlloc ;
    }
    else
    {
        // Update memory size ready for reading back....
        s_MemSize += Size ;

        // Return allocated memory
        return x_malloc(Size) ;
    }
}

//==============================================================================

// Should only be called when writing a binary file
void shape_bin_file::Free(void *Memory)
{
    // Should never be called when reading!
    if (s_Mem)
    {
        ASSERT(0) ;
    }
    else
    {
        // Any memory?
        if (Memory)
        {
            // Update memory size for reading back binary...
            if (s_Mode != -1)
            {
                s_MemSize -= x_msize(Memory) ;
                ASSERT(s_MemSize > 0) ;
            }

            // Free memory
            x_free(Memory) ;
        }
    }
}

//==============================================================================

// Call to reserve memory for reading file
void shape_bin_file::Reserve(s32 Size)
{
    // Make sure size is aligned for PS2 etc
    Size = ALIGN_16(Size) ;

    // Show info
    #ifdef SHOW_DEBUG_INFO
        shape::printf("   shape_bin_file::Reserving %d bytes...\n", Size) ;
    #endif

    // Update memory needed for reading
    s_MemSize += Size ;
}

//==============================================================================

// Returns current file mode
s32 shape_bin_file::GetMode()
{
    return s_Mode ;    
}


//==============================================================================
//
// Class functions
//
//==============================================================================

// Constructor
shape_bin_file::shape_bin_file()
{
    // Should only ever be called from one thread!
    ASSERT(s_File           == NULL) ;
    ASSERT(s_Mode           == -1) ;
    ASSERT(s_Mem            == NULL) ;
    ASSERT(s_MemAllocPos    == NULL) ;
    ASSERT(s_MemSize        == 0) ;
}

//==============================================================================

// Destructor
shape_bin_file::~shape_bin_file()
{
    // Close the file?
    if (s_File)
        Close() ;

    // Clear vars
    s_File           = NULL ;
    s_Mode           = -1 ;
    s_Mem            = NULL ;
    s_MemAllocPos    = NULL ;
    s_MemSize        = 0 ;
}

//==============================================================================

// Opens file ready for reading
byte *shape_bin_file::OpenForReading(const char *Filename)
{
    // Should only ever be called from one thread!
    ASSERT(s_File           == NULL) ;
    ASSERT(s_Mode           == -1) ;
    ASSERT(s_Mem            == NULL) ;
    ASSERT(s_MemAllocPos    == NULL) ;
    ASSERT(s_MemSize        == 0) ;

    // Try open the file
    s_File = x_fopen(Filename, "rb") ;
    
    // Fail?
    if (s_File == NULL)
        return NULL ;

    // Begin reading...
    s_Mode = SHAPE_BIN_FILE_READING ;

    // Read version
    s32 Version ;
    ReadWrite(Version) ;

    // Is file newer than this build?
    if (Version > SHAPE_FILE_VERSION)
    {
        shape::printf("   either your source code is old (update from source safe!) or file is corrupt (delete it) %s - it's a newer version\n\n", Filename) ;
        ASSERT(0) ;

		x_fclose(s_File) ;
        return NULL ;
    }

    // Older version?
    if (Version < SHAPE_FILE_VERSION)
    {
        shape::printf("   skipping file %s - it's an old version\n\n", Filename) ;

		x_fclose(s_File) ;
		return NULL ;
    }

    // Read amount of memory to allocate
    ReadWrite(s_MemSize) ;

    // If no memory to alloc then shape file must have been aborted by the shape builder
    if (s_MemSize == 0)
    {
        s_MemSize = 0 ;

        shape::printf("   skipping file %s - it's empty (was probably aborted by shape builder last time)\n\n", Filename) ;

		x_fclose(s_File) ;
		return NULL ;
    }

	// Allocate!
    s_Mem         = new byte[s_MemSize] ;
    s_MemAllocPos = s_Mem ; // Reset alloc pointer
    ASSERT(s_Mem) ;

    // Keep current file pos
	s_FileBufferPos = x_ftell(s_File) ;
    
	// Find out length of file
	x_fseek( s_File, 0, X_SEEK_END ) ;
    s_FileBufferSize = x_ftell(s_File) ;

	// Try allocate memory to read whole file
    s_FileBuffer = new byte[s_FileBufferSize] ;
	if (s_FileBuffer)
	{
		// Put file back to start
		x_fseek( s_File, 0, X_SEEK_SET );

	    // Read whole file in one go...
	    s_FileBufferSize = x_fread((void *)s_FileBuffer, 1, s_FileBufferSize, s_File) ;
	}
	else
	{
		// Put file back to the correct position
		x_fseek( s_File, s_FileBufferPos, X_SEEK_SET );

		// Clear file buffer vars
		s_FileBuffer	 = NULL ;
		s_FileBufferSize = 0 ;
		s_FileBufferPos  = 0 ;
	}

    // Return allocated memory
    return (s_Mem) ;
}

//==============================================================================

// Opens file
s32 shape_bin_file::OpenForWriting(const char *Filename)
{
    // Should only ever be called from one thread!
    ASSERT(s_File           == NULL) ;
    ASSERT(s_Mode           == -1) ;
    ASSERT(s_Mem            == NULL) ;
    ASSERT(s_MemAllocPos    == NULL) ;
    ASSERT(s_MemSize        == 0) ;

    // Try open file
    s_File = x_fopen(Filename, "wb") ;

    // Success?
    if (s_File)
    {
        // Begin writing
        s_Mode = SHAPE_BIN_FILE_WRITING ;

        // Write version
        s32 Version = SHAPE_FILE_VERSION ;
        ReadWrite(Version) ;

        // Write amount of memory to allocate when reading back in...
        // (this will be filled in later...)
        ReadWrite(s_MemSize) ;
    }
    else
    {
        // Show warning
        shape::printf("\n\nWARNING: Could not create %s - File may be read only!\n\n", Filename) ;
    }

    return (s_File != NULL) ;
}

//==============================================================================

// Closes file
void shape_bin_file::Close()
{
    ASSERT(s_File) ;

    // If reading, make sure we used all the allocated memory!
    if (s_Mode == SHAPE_BIN_FILE_READING)
    {
        ASSERT(s_MemAllocPos == (s_Mem + s_MemSize)) ;
    }
    else
    {
        ASSERT(s_Mode == SHAPE_BIN_FILE_WRITING) ;

        // Go back to start of file where mem size was written
        x_fseek( s_File, 4, X_SEEK_SET );

        // Write final allocation size ready for reading...
        ReadWrite(s_MemSize) ;

        // Show info
        shape::printf("   shape_bin_file::Close - Written Version:%d MemSize:%d\n", SHAPE_FILE_VERSION, s_MemSize) ;
    }

    // Finally close the file
    x_fclose(s_File) ;

	// Free file buffer
	if (s_FileBuffer)
	{
		// Make sure all data in file was read
		ASSERT(s_FileBufferPos == s_FileBufferSize) ;

		// Delete file buffer
		delete [] s_FileBuffer ;
		s_FileBuffer	 = NULL ;
		s_FileBufferSize = 0 ;
		s_FileBufferPos  = 0 ;
	}

    // Clear vars
    s_File           = NULL ;
    s_Mode           = -1 ;
    s_Mem            = NULL ;
    s_MemAllocPos    = NULL ;
    s_MemSize        = 0 ;
}

//==============================================================================

// Checks for end of file
xbool shape_bin_file::EndOfFile()
{
    ASSERT(s_File) ;
    return x_feof(s_File) ;
}

//==============================================================================

void shape_bin_file::ReadWriteMemory(byte *Bytes, s32 Length)
{
    // Write stream
    if (s_Mode == SHAPE_BIN_FILE_WRITING)
        x_fwrite(Bytes, Length, 1, s_File) ;
    else
    if (s_Mode == SHAPE_BIN_FILE_READING)
	{
		// Read from buffer?
		if (s_FileBuffer)
		{
			// Check for reading past end of file
			ASSERT((s_FileBufferPos + Length) <= s_FileBufferSize) ;

			// Read from memory
			x_memcpy((void *)Bytes, &s_FileBuffer[s_FileBufferPos], Length) ;

			// Move file pointer ready for next read
			s_FileBufferPos += Length ;
		}
		else
			x_fread(Bytes, Length, 1, s_File) ;	// Regular read
	}
}

//==============================================================================
//
// shape_bin_file_class implementation
//
//==============================================================================

shape_bin_file_class::target   shape_bin_file_class::GetDefaultTarget()
{
    shape_bin_file_class::target    Target = shape_bin_file_class::DEFAULT ;

    // Choose target of given build
    #if defined(TARGET_PC) || defined(TARGET_LINUX)
        Target = shape_bin_file_class::PC ;
    #endif

    #ifdef TARGET_PS2
        Target = shape_bin_file_class::PS2 ;
    #endif

    ASSERTS(Target != shape_bin_file_class::DEFAULT, "Shape library does not support this build!") ;

    return Target ;
}

//==============================================================================

#undef new
// Override new and delete operator so pointers are hooked up if reading from a binary file
void* shape_bin_file_class::operator new []   ( u32 Size )
{
    return shape_bin_file::Alloc(Size) ;
}

//==============================================================================

void* shape_bin_file_class::operator new      ( u32 Size )
{
    return shape_bin_file::Alloc(Size) ;
}

//==============================================================================

void* shape_bin_file_class::operator new []   ( u32 Size, char* pFileName, s32 LineNumber  )
{
    // Suppress warnings
    (void)pFileName ;
    (void)LineNumber ;
    return shape_bin_file::Alloc(Size) ;
}

//==============================================================================

void* shape_bin_file_class::operator new      ( u32 Size, char* pFileName, s32 LineNumber  )
{
    // Suppress warnings
    (void)pFileName ;
    (void)LineNumber ;

    return shape_bin_file::Alloc(Size) ;
}

//==============================================================================

void shape_bin_file_class::operator delete    ( void* pMemory )   
{ 
    shape_bin_file::Free(pMemory) ;
}

//==============================================================================

void shape_bin_file_class::operator delete [] ( void* pMemory )   
{
    shape_bin_file::Free(pMemory) ;       
}

//==============================================================================

void shape_bin_file_class::operator delete    ( void* pMemory, char* pFileName, s32 LineNumber )   
{
    // Suppress warnings
    (void)pFileName ;
    (void)LineNumber ;

    shape_bin_file::Free(pMemory) ;       
}

//==============================================================================

void shape_bin_file_class::operator delete [] ( void* pMemory, char* pFileName, s32 LineNumber )   
{ 
    // Suppress warnings
    (void)pFileName ;
    (void)LineNumber ;

    shape_bin_file::Free(pMemory) ;       
}

//==============================================================================


#ifndef BLD_FILE_SYSTEM_HPP
#define BLD_FILE_SYSTEM_HPP

///////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_stdio.hpp>

///////////////////////////////////////////////////////////////////////////
// bld_fileio
///////////////////////////////////////////////////////////////////////////
class bld_fileio
{
///////////////////////////////////////////////////////////////////////////
public:

    //=====================================================================
    inline  bld_fileio( void ) {  }
    inline ~bld_fileio( void ) {  }

    //=====================================================================
    static inline void Read( void* pBuff, s32 Size, X_FILE* Fp )
    {
        #ifdef X_DEBUG
            int i = x_fread( pBuff, 1, Size, Fp );
            ASSERT( i == Size );
        #else
            x_fread( pBuff, 1, Size, Fp );
        #endif
    }

    //=====================================================================
    static inline void Write( const void* pBuff, s32 Size, X_FILE* Fp )
    {
        #ifdef X_DEBUG
            int i = x_fwrite( pBuff, 1, Size, Fp );
            ASSERT( i == Size );
        #else
            x_fwrite( pBuff, 1, Size, Fp );
        #endif
    }

    //=====================================================================
    static inline void Seek( s32 Pos, X_FILE* Fp )
    {
        #ifdef X_DEBUG
            int i = x_fseek( Fp, Pos, X_SEEK_SET );
            ASSERT( i == 0 );
        #else
            x_fseek( Fp, Pos, X_SEEK_SET );
        #endif
    }

    //=====================================================================
    static inline int Tell( X_FILE* Fp )
    {
        return x_ftell( Fp );
    }
    
    //=====================================================================
    static inline int IsEOF( X_FILE* Fp )
    {
        return x_feof( Fp );
    }

    //=====================================================================
    template< class ta > inline
    void HandleIOField( ta& Field, s32 Count )
    {
        if( m_Field_Mode )
        {
            int Size;

            // Find how much memory we have to copy
            ASSERT( Count >= 0 );
            ASSERT( sizeof(Field) == 4 );            
            Size = sizeof(*Field) * Count;

            // Save it to disk
            if( Count > 0 )
            {
                // Store where does this field start
                m_FielPos[ m_Field_Count ] = ( Tell( m_Field_Fp ) - m_WriteFieldPos );

                // Write the field section
                Write( Field, sizeof(*Field)*Count, m_Field_Fp );

                //
                // Pad the structure to 16 bytes
                //
                {
                    int Loc = Tell( m_Field_Fp );

                    int Padding = (16 - ( Loc - m_WriteFieldPos )) & 15;

                    if( Padding )
                    {
                        static const s32 Pad[]={ 0xDDDDDDDD, 0xDDDDDDDD, 0xDDDDDDDD, 0xDDDDDDDD };
                        Write( Pad, Padding, m_Field_Fp );
                    }
                }
            }
            else
            {
                m_FielPos[ m_Field_Count ] = -1;
            }
            
            // Mark as a written field
            m_Field_Count++;
        }
        else
        {
            if( m_pFIOFieldPos[ m_Field_Count ] == -1 )
            {
                Field = NULL;
            }
            else
            {
                Field = (ta)(m_Field_pMem + m_pFIOFieldPos[ m_Field_Count ]);
            }

            m_Field_Count++;
        }
    }

    //=====================================================================
    inline void WriteHeader( const char* Version, X_FILE* Fp )
    {
        ASSERT( x_strlen( Version ) == 8 );
        Write( Version, 8, Fp );     
        
        m_CurrentChunkPos = Tell( Fp );
        m_ChunckCount     = 0;

        Write( &m_ChunckCount, sizeof(m_ChunckCount), Fp ); 
    }

    //=====================================================================
    template< class ta > inline 
    void WriteFields( ta& Class, X_FILE* Fp )
    {
        s32 FinalPos, Pos;
        s32 Size;

        // Update how many chuncks we are writting to disk
        UpdateChunkCount( Fp );

        // Get the curent Pos
        Pos = Tell( Fp );

        // Save have size of the chunck.
        Size                 = 0;
        m_Field_Count     = 0;
        Write( &Size,    sizeof(Size), Fp );
        Write( &m_Field_Count, sizeof(m_Field_Count), Fp );

        // Write the rest fields
        m_Field_Mode     = 1;
        m_Field_Fp       = Fp;
        m_WriteFieldPos  = Tell( Fp );
        Class.IOFields( *this );

        // Write the offset for each of the fields
        Write( m_FielPos, sizeof(s32) * m_Field_Count, Fp );

        // get the final position
        FinalPos = Tell( Fp );

        // Fix the chunck size
        Seek( Pos, Fp );
        Size = FinalPos - Pos - sizeof(Size) - sizeof(m_Field_Count);
        Write( &Size, sizeof(Size), Fp );
        Write( &m_Field_Count, sizeof(m_Field_Count), Fp );

        // Go to the end
        Seek( FinalPos, Fp );
    }

    //=====================================================================
    void BeginWChunck( X_FILE* Fp )
    {        
        s32 Size = 0;

        // Update how many chuncks we are writting to disk
        UpdateChunkCount( Fp );

        m_StartChunck = Tell( Fp );
        Write( &Size, sizeof(Size), Fp );
    }

    //=====================================================================
    void EndWChunck( X_FILE* Fp )
    {
        s32 EndChunk;
        s32 Size;

        EndChunk = Tell( Fp );

        Seek( m_StartChunck, Fp );

        Size = EndChunk - m_StartChunck - sizeof(Size);
        Write( &Size, sizeof(Size),  Fp );

        Seek( EndChunk, Fp );
    }

    //=====================================================================
    void WriteChunck( void* Buff, s32 Size, X_FILE* Fp )
    {
        // Update how many chuncks we are writting to disk
        UpdateChunkCount( Fp );

        Write( &Size, sizeof(Size), Fp );
        Write( Buff, Size, Fp );
    }

    //=====================================================================
    template< class ta > inline
    void WriteClass( ta& Class, X_FILE* Fp )
    {
        WriteChunck( &Class, sizeof(ta), Fp );
    }

    //=====================================================================
    s32 ReadHeader( const char* Version, X_FILE* Fp )
    {
        (void)Version;
        char String[16];
        s32  NChunks;
        Read( String, 8, Fp );         
        String[8]=0;
        ASSERT( x_strcmp( String, Version ) == 0 );
        Read( &NChunks, sizeof(NChunks), Fp ); 
        return NChunks;
    }

    //=====================================================================
    template< class ta > inline
    void* ReadFields( ta& Class, X_FILE* Fp )
    {
        s32 Size;
        s32 nFields;
        void* pData;

        // Get the size of the chunck
        Read( &Size, sizeof(Size), Fp );
        Read( &nFields, sizeof(nFields), Fp );
        
        // Allocate the field memory
        pData = new char[ Size ];
        ASSERT( pData );

        // Read all the fields in
        Read( pData, Size, Fp );

        // Read the size of the 
        m_Field_Count    = 0;
        m_Field_Mode     = 0;
        m_Field_pMem     = (char*)pData;
        m_pFIOFieldPos      = (s32*)(&m_Field_pMem[Size] - (nFields*sizeof(s32)) );

        Class.IOFields( *this );

        ASSERT( m_Field_Count == nFields );

        return pData;
    }

    //=====================================================================
    int BeginRChunck( X_FILE* Fp )
    {
        m_StartChunck = Tell( Fp );
        Read( &m_SizeChunck, sizeof(m_SizeChunck), Fp );
        return m_SizeChunck;
    }

    //=====================================================================
    void EndRChunck( X_FILE* Fp )
    {
        (void)Fp;
        ASSERT( (IsEOF( Fp ) == false) || 
                (m_SizeChunck == ( Tell( Fp ) - m_StartChunck )) );
    }

    //=====================================================================
    void* ReadChunck( X_FILE* Fp )
    {
        void* pBuff;
        s32   Size;

        Read( &Size, sizeof(Size), Fp );
        pBuff = new char[ Size ];
        ASSERT( pBuff );
        Read( pBuff, Size, Fp );
        return pBuff;
    }

    //=====================================================================
    template< class ta > inline
    void ReadClass( ta& Class, X_FILE* Fp )
    {
        s32   Size;

        Read( &Size, sizeof(Size), Fp );
        ASSERT( Size == sizeof(Class) );

        // Can't handle virtual functions
        Read( &Class, sizeof(Class), Fp );
    }

///////////////////////////////////////////////////////////////////////////
protected:

    //=====================================================================
    inline void UpdateChunkCount( X_FILE* Fp )
    {
        s32 Pos = Tell( Fp );

        Seek( m_CurrentChunkPos, Fp );

        m_ChunckCount++;
        Write( &m_ChunckCount, sizeof(m_ChunckCount), Fp ); 

        Seek( Pos, Fp );        
    }

///////////////////////////////////////////////////////////////////////////
protected:

    s32*     m_pFIOFieldPos;
    s32      m_WriteFieldPos;
    s32      m_FielPos[1024];
    s32      m_CurrentChunkPos;
    s32      m_ChunckCount;
    s32      m_StartChunck;
    s32      m_SizeChunck;
    s32      m_Field_Count;
    s32      m_Field_Mode;
    X_FILE*    m_Field_Fp;
    char*    m_Field_pMem;
};

///////////////////////////////////////////////////////////////////////////
// END
///////////////////////////////////////////////////////////////////////////
#endif
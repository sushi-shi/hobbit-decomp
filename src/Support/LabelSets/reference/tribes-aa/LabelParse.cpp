#include "entropy.hpp"
#include "../tokenizer/tokenizer.hpp"
#include "labelparse.hpp"



//=============================================================================
//  Load a LabelSet
//=============================================================================

xbool labels::LoadLabelSet( const char *pFileName )
{
    token_stream    ts;

    // open the file
    if ( ts.OpenFile( pFileName ) == FALSE )
        return FALSE;

    // look for labels
    if( ts.Find( "BEGIN_LABEL_SET", TRUE ) != FALSE )
    {
        ts.Read();
        ASSERT( ts.Delimiter() == '(' );
        ts.Read();
        m_Name = ts.String();

        // Search all the labels
        s32 ID=0 ;
    	while(ts.Type() != token_stream::TOKEN_EOF)
        {
            ts.Read() ;

            // LABEL or LABEL_VALUE?
            if (        (ts.Type() == token_stream::TOKEN_SYMBOL)
                    &&  (x_strncmp(ts.String(), "LABEL", 5) == 0) )
            {
                label_data new_label;

                // What type of label?
                xbool LabelValue = (x_strcmp(ts.String(), "LABEL_VALUE") == 0) ;

                // Skip the delimiter
                ts.Read();
                ASSERT( ts.Delimiter() == '(' );
            
                // Read the label name
                ts.Read();
                new_label.m_Label = ts.String();
            
                // Skip delim
                ts.Read();
                ASSERT( ts.Delimiter() == ',' );

                // Does label have an associated value?
                if (LabelValue)
                {
                    ID = ts.ReadInt() ; // Read value

                    // Skip delim
                    ts.Read();
                    ASSERT( ts.Delimiter() == ',' );
                }

                // Set the label value
                new_label.m_ID = ID++ ;

                // Read the description
                ts.ReadString();
                new_label.m_Desc = ts.String();

                // Add the label to the labels
                m_Labels.Append( new_label );
            }
        }
    }
    else
        return FALSE;

    // success
    return TRUE;
}


//=============================================================================
//  Find a label
//=============================================================================
s32 labels::FindLabel( const char *pLabel )
{
    s32 i;

    for ( i = 0; i < m_Labels.GetCount(); i++ )
    {
        if ( m_Labels[i].m_Label == pLabel )
            return m_Labels[i].m_ID;
    }

    return -1;
}
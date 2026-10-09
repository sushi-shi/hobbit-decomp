#include <xCore/x_files/x_files.hpp>
#include "../cardmgr.hpp"

static s32 last_error;

//-----------------------------------------------------------------------------
void        card_Init(void)
{
	last_error = CARD_ERR_CHANGED;
}

//-----------------------------------------------------------------------------
void        card_Kill(void)
{
}

//-----------------------------------------------------------------------------
s32 card_Delete(const char* pFilename)
{
    return 0 ;
}

//-----------------------------------------------------------------------------
card_file  *card_Open(const char *pFilename,const char *pMode)
{
	const char *pName;
	char	mode[32];
	char* pNewMode;

	pName = pFilename+x_strlen(pFilename);

	while ( (pName != pFilename) && (*pName != '/') )
	{
		pName--;
	}
	if (*pName=='/')
		pName++;

	pNewMode = mode;
	while (*pMode)
	{
		switch (*pMode)
		{
		case 'w':
		case 'W':
			*pNewMode++='w';
			break;
		case 'r':
		case 'R':
			*pNewMode++='r';
			break;
		default:
			break;
		}
		pMode++;
	}
	*pNewMode++='b';
	*pNewMode++=0x0;
	return (card_file* )x_fopen(pName,mode);
}

//-----------------------------------------------------------------------------
void        card_Close(card_file *pFile)
{
	x_fclose((X_FILE* )pFile);
}

//-----------------------------------------------------------------------------
s32         card_Write(card_file *pFile,const void *pBase,s32 Length)
{
	last_error = CARD_ERR_CHANGED;
	return x_fwrite(pBase,Length,1,(X_FILE*)pFile);
}

//-----------------------------------------------------------------------------
s32         card_Read(card_file *pFile,void *pBase, s32 Length)
{
    return x_fread(pBase,Length,1,(X_FILE*)pFile);
}

//-----------------------------------------------------------------------------
s32         card_Seek(card_file *pFile,s32 Offset,s32 Mode)
{
	return x_fseek((X_FILE*)pFile,Offset,Mode);
}

//-----------------------------------------------------------------------------
s32         card_Length(card_file *pFile)
{
	return x_flength((X_FILE*)pFile);
}

//-----------------------------------------------------------------------------
s32         card_Check(char *pFilename,card_info *pInfo)
{
    (void)pFilename;
    (void)pInfo;
    return NULL;
}


//-----------------------------------------------------------------------------
s32         card_Search(char *pFilename,card_file *pBuffer,s32 mode)
{
    (void)pFilename;
    (void)pBuffer;
    (void)mode;
    return NULL;
}

//-----------------------------------------------------------------------------
s32         card_Check(const char *pFilename,card_info *pInfo)
{
	s32 error;
    (void)pFilename;
    (void)pInfo;
	error = last_error;
	last_error = CARD_ERR_OK;
    return error;
}

//-----------------------------------------------------------------------------
s32         card_Search(const char *pFilename,card_file *pBuffer,s32 mode)
{
    (void)pFilename;
    (void)pBuffer;
    (void)mode;

    return CARD_ERR_OK;
}

//-----------------------------------------------------------------------------
s32         card_Format(const char *pFilename)
{
    (void)pFilename;
    return CARD_ERR_OK;
}

//-----------------------------------------------------------------------------
s32         card_MkDir(const char *pDirname)
{
    (void)pDirname;
    return CARD_ERR_OK;
}

//-----------------------------------------------------------------------------
s32         card_ChDir(const char *pDirname)
{
    (void)pDirname;
    return CARD_ERR_OK;
}

s32     card_MkIcon(const char *pTitlename)
{
    ASSERT(pTitlename);

    return CARD_ERR_OK;
}

s32 card_Checksum(u8 *pBuffer,s32 Length,s32 ChecksumSeed)
{
    s32 i;

    for (i=0;i<Length;i++)
    {
        ChecksumSeed += (*pBuffer++)*i;
    }
    return ChecksumSeed;
}
#include <xCore/x_files/x_files.hpp>
#include "../cardmgr.hpp"
#include "ps2/iop/iop.hpp"
#include "libmc.h"
#include "libvu0.h"

//-------------------------------------------
// Internal defines
//-------------------------------------------
#define CARD_MAX_PORTS  2
#define CARD_MAX_SLOTS  1
#define CARD_DIR_ENTRIES    8
//-------------------------------------------
// Internal structures
//-------------------------------------------
typedef struct s_memcard_vars
{
    s32             CurrentPort;
    s32             CurrentSlot;
    s32             MaxSlots;
    s32             MaxSpace;
    s32             AvailableSpace;
    s32             Type;
    xbool           ForceUpdate;
    s32             DirectoryLength;
    s32             DirectoryIndex;
    sceMcTblGetDir  *pDirectory;
} memcard_vars;

//-------------------------------------------
// Internal function prototypes
//-------------------------------------------
const char      *NameToPortAndSlot(const char *pName,s32 &Port,s32 &Slot);
s32             CardSync(void);
s32             SonyToInternalError(s32 ErrorCode);
void            AsciiToSJIS(const char *pTitleName,s16 *SJISBuffer);
//-------------------------------------------
// Internal variables
//-------------------------------------------

memcard_vars    *gpMemcard=NULL;

//-----------------------------------------------------------------------------
void        card_Init(void)
{
    ASSERT(gpMemcard==NULL);

    gpMemcard = (memcard_vars *)x_malloc(sizeof(memcard_vars));
    ASSERT(gpMemcard);
    sceMcInit();
    gpMemcard->CurrentPort = 0;
    gpMemcard->CurrentSlot = 0;
    gpMemcard->pDirectory  = (sceMcTblGetDir *)x_malloc(sizeof(sceMcTblGetDir) * CARD_DIR_ENTRIES);
    ASSERT(gpMemcard->pDirectory);
    gpMemcard->DirectoryIndex = 0;
    gpMemcard->DirectoryLength = 0;
}

//-----------------------------------------------------------------------------
void        card_Kill(void)
{
    ASSERT(gpMemcard);
    x_free(gpMemcard);
    gpMemcard = NULL;
}

//-----------------------------------------------------------------------------
s32 card_Delete(const char* pFilename)
{
	s32 status;
	s32 port,slot;

    pFilename = NameToPortAndSlot(pFilename,port,slot);

    sceMcDelete(port,slot,(char *)pFilename);
    status = CardSync();
    return SonyToInternalError(status);
}

card_file  *card_Open(const char *pFilename,const char *pMode)
{
    s32 mode;
    s32 status;
    card_file *pFile;

    ASSERT(gpMemcard);

    mode = 0;
    while (*pMode)
    {
        switch(*pMode)
        {
        case 'w':
        case 'W':
            gpMemcard->ForceUpdate=TRUE;
            mode |= (SCE_WRONLY|SCE_CREAT);
            break;
        case 'r':
        case 'R':
            mode |= SCE_RDONLY;
            break;
        case 'a':
        case 'A':
            mode |= SCE_RDWR;
            break;
        default:
            break;
        }
        pMode++;
    }
    //
    // First, check to see if a memory card is present
    //

    pFile = (card_file *)x_malloc(sizeof(card_file));
    ASSERT(pFile);
    
    pFilename = NameToPortAndSlot(pFilename,pFile->Port,pFile->Slot);
    pFile->Position = 0;

    sceMcOpen(pFile->Port,pFile->Slot,(char *)pFilename,mode);
    status = CardSync();

    if (status >=0)
    {
        pFile->Handle = status;        
        sceMcGetDir(pFile->Port,pFile->Slot,(char *)pFilename,0,1,gpMemcard->pDirectory);
        CardSync();
        pFile->Length = gpMemcard->pDirectory->FileSizeByte;
    }
    else
    {
        x_free(pFile);
        return (card_file *)SonyToInternalError(status);
    }
    //
    // Now we go fetch a bunch of file information to fill in internal structs
    //
    return pFile;
}

//-----------------------------------------------------------------------------
void        card_Close(card_file *pFile)
{
    ASSERT(gpMemcard);
    ASSERT(pFile);

    sceMcClose(pFile->Handle);
    CardSync();

    x_free(pFile);
;
}

//-----------------------------------------------------------------------------
s32         card_Write(card_file *pFile,const void *pBase,s32 Length)
{
    s32 status;
    ASSERT(gpMemcard);
    ASSERT(pFile);

    sceMcWrite(pFile->Handle,(void *)pBase,Length);
    //
    // Keep the length of the current file updated
    //
    if ( (pFile->Position + Length) > pFile->Length)
    {
        pFile->Length = pFile->Position+Length;
    }
    status = CardSync();
    gpMemcard->ForceUpdate=TRUE;
    return SonyToInternalError(status);
}

//-----------------------------------------------------------------------------
s32         card_Read(card_file *pFile,void *pBase, s32 Length)
{
    s32 status;

    ASSERT(gpMemcard);
    ASSERT(pFile);

    sceMcRead(pFile->Handle,pBase,Length);

    status = CardSync();
    return SonyToInternalError(status);
}

//-----------------------------------------------------------------------------
s32         card_Seek(card_file *pFile,s32 Offset,s32 Mode)
{
    s32 status;
    ASSERT(gpMemcard);
    ASSERT(pFile);

    sceMcSeek(pFile->Handle,Offset,Mode);
    status = CardSync();
    if (status >=0)
    {
        pFile->Position = status;
    }
    return SonyToInternalError(status);
}

//-----------------------------------------------------------------------------
s32         card_Length(card_file *pFile)
{
    ASSERT(gpMemcard);
    ASSERT(pFile);

    return pFile->Length;
}

//-----------------------------------------------------------------------------
s32         card_Check(const char *pFilename,card_info *pInfo)
{
    s32 Formatted;
    s32 status;

    ASSERT(gpMemcard);
    ASSERT(pFilename);
    ASSERT(pInfo);

    NameToPortAndSlot(pFilename,gpMemcard->CurrentPort,gpMemcard->CurrentSlot);
    //
    // Now fill in information about the current card activated
    //
    sceMcGetInfo(gpMemcard->CurrentPort,gpMemcard->CurrentSlot,&pInfo->Type,&pInfo->Free,&Formatted);
    status = CardSync();

    if (status < -10)
        return CARD_ERR_UNPLUGGED;

    if ( (status == -1) || (status == -2) || (gpMemcard->ForceUpdate))
    {
        gpMemcard->ForceUpdate = FALSE;
        return CARD_ERR_CHANGED;
    }

    if ( (pInfo->Type != 2) && (pInfo->Type != 0) )
    {
        return CARD_ERR_WRONGTYPE;
    }
    gpMemcard->MaxSlots = sceMcGetSlotMax(gpMemcard->CurrentPort);
    return SonyToInternalError(status);
    return NULL;
}

//-----------------------------------------------------------------------------
s32         card_Search(const char *pFilename,card_file *pBuffer,s32 mode)
{
    s32 Port,Slot;
    s32 status;
    sceMcTblGetDir  *pDir;

    ASSERT(gpMemcard);
    ASSERT(pFilename);
    ASSERT(pBuffer);

    pFilename = NameToPortAndSlot(pFilename,Port,Slot);

    // Do we need to reload a new chunk of directory entries?
    if ( (mode==0) || (gpMemcard->DirectoryIndex == CARD_DIR_ENTRIES) )
    {
        sceMcGetDir(Port,Slot,(char *)pFilename,mode,CARD_DIR_ENTRIES,gpMemcard->pDirectory);
        status = CardSync();
        if (status < 0)
        {
            return SonyToInternalError(status);
        }
        gpMemcard->DirectoryIndex = 0;
        gpMemcard->DirectoryLength = status;
    }

    if (gpMemcard->DirectoryIndex >= gpMemcard->DirectoryLength)
        return CARD_ERR_NOTFOUND;

    pDir = &gpMemcard->pDirectory[gpMemcard->DirectoryIndex];
    pBuffer->Length = pDir->FileSizeByte;
    x_strncpy(pBuffer->Filename,(char *)pDir->EntryName,CARD_NAME_LENGTH);
    pBuffer->Port = Port;
    pBuffer->Slot = Slot;
    pBuffer->Position = 0;
    
    gpMemcard->DirectoryIndex++;
  
    return CARD_ERR_OK;
}

//-----------------------------------------------------------------------------
s32         card_Format(const char *pFilename)
{
    s32 port,slot;
    s32 status;

    gpMemcard->ForceUpdate=TRUE;
    NameToPortAndSlot(pFilename,port,slot);
    sceMcFormat(port,slot);
    status = CardSync();
    return SonyToInternalError(status);
}

//-----------------------------------------------------------------------------
s32         card_MkDir(const char *pDirname)
{
    s32 port,slot;
    s32 status;

    ASSERT(pDirname);
    pDirname = NameToPortAndSlot(pDirname,port,slot);
    sceMcMkdir(port,slot,(char *)pDirname);
    status = CardSync();
    gpMemcard->ForceUpdate=TRUE;
    return SonyToInternalError(status);
}

//-----------------------------------------------------------------------------
s32         card_ChDir(const char *pDirname)
{
    s32 port,slot;
    s32 status;

    ASSERT(pDirname);
    pDirname = NameToPortAndSlot(pDirname,port,slot);
    sceMcChdir(port,slot,(char *)pDirname,NULL);
    status = CardSync();
    if (status==0)
    {
        gpMemcard->ForceUpdate=TRUE;
    }

    return SonyToInternalError(status);
}

s32     card_MkIcon(const char *pTitlename)
{
    ASSERT(pTitlename);
	static sceMcIconSys icon;
    s16 SJISBuffer[32];
    card_file *fp;
    s32 i;

	static sceVu0IVECTOR bgcolor[4] = {	/* 0xff Ç™ç≈ëÂãPìx */
		{ 0x80,    0,    0, 0 },		/* îwåiêF(ç∂è„) */
		{    0, 0x80,    0, 0 },		/* îwåiêF(âEè„) */
		{    0,    0, 0x80, 0 },		/* îwåiêF(ç∂â∫) */
		{ 0x80, 0x80, 0x80, 0 },		/* îwåiêF(âEè„) */
	};
	static sceVu0FVECTOR lightdir[3] = {
		{ 0.5, 0.5, 0.5, 0.0 },
		{ 0.0,-0.4,-0.1, 0.0 },
		{-0.5,-0.5, 0.5, 0.0 },
	};
	static sceVu0FVECTOR lightcol[3] = {
		{ 0.48, 0.48, 0.03, 0.00 },
		{ 0.50, 0.33, 0.20, 0.00 },
		{ 0.14, 0.14, 0.38, 0.00 },
	};
	static sceVu0FVECTOR ambient = { 0.50, 0.50, 0.50, 0.00 };

    static char *IconName[]=
    {
        "view.ico",
        "view.ico",
        "view.ico",
    };

    x_memset(&icon, 0, sizeof(icon));
	x_strcpy((char *)icon.Head, "PS2D");
    AsciiToSJIS(pTitlename,SJISBuffer);
    x_memcpy(icon.TitleName,SJISBuffer,34*sizeof(s16));

    // Find a good place to do a linebreak in the title name as close to the
    // middle as possible.
    i = x_strlen(pTitlename) / 2 ;
    while (pTitlename[i]!=' ')
    {
        i--;
        if (i<0)
        {
            i=x_strlen(pTitlename) / 2 ;
            break;
        }
    }
	icon.OffsLF = i*2;
	icon.TransRate = 0x60;
	x_memcpy(icon.BgColor, bgcolor, sizeof(bgcolor));
	x_memcpy(icon.LightDir, lightdir, sizeof(lightdir));
	x_memcpy(icon.LightColor, lightcol, sizeof(lightcol));
	x_memcpy(icon.Ambient, ambient, sizeof(ambient));

	x_strcpy((char *)icon.FnameView, IconName[0]);
    x_strcpy((char *)icon.FnameCopy, IconName[1]);
    x_strcpy((char *)icon.FnameDel,  IconName[2]);

    fp = card_Open("icon.sys","w");
    if (fp <=0)
    {
        return CARD_ERR_GENERAL;
    }

    card_Write(fp,&icon,sizeof(icon));
    card_Close(fp);

//    for (i=0;i<3;i++)
    for (i=0;i<1;i++)
    {
        X_FILE *pFile;
        s32 length;
        u8      *pBuffer;

        pFile=x_fopen(xfs("data/memcard/%s",IconName[i]),"rb");
        if (!pFile)
        {
            return CARD_ERR_GENERAL;
        }
        x_fseek(pFile,0,X_SEEK_END);
        length = x_ftell(pFile);
        x_fseek(pFile,0,X_SEEK_SET);
        pBuffer = (u8 *)x_malloc(length);
        ASSERT(pBuffer);
        x_fread(pBuffer,length,1,pFile);
        x_fclose(pFile);
        fp = card_Open(IconName[i],"w");
        if ((s32)fp<=0)
        {
            x_free(pBuffer);
            return CARD_ERR_GENERAL;
        }
        card_Write(fp,pBuffer,length);
        card_Close(fp);
        x_free(pBuffer);
    }

    return CARD_ERR_OK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Internal function bodies
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const char    *NameToPortAndSlot(const char *pName,s32 &Port,s32 &Slot)
{
    ASSERT(pName);

    if ( (pName[0]=='c') && (pName[1]=='a') &&
         (pName[2]=='r') && (pName[3]=='d') &&
         (pName[6]==':') )
    {
        Slot = pName[4]-'0';
        Port = pName[5]-'0';
        ASSERT( (Port >=0) && (Port < CARD_MAX_PORTS) );
        ASSERT( (Slot >=0) && (Slot < CARD_MAX_SLOTS) );
        pName = &pName[7];
        gpMemcard->CurrentPort = Port;
        gpMemcard->CurrentSlot = Slot;
    }
    else
    {
        Port = gpMemcard->CurrentPort;
        Slot = gpMemcard->CurrentSlot;
    }
    return pName;
}

//-----------------------------------------------------------------------------
s32     CardSync(void)
{
    s32 status;
    s32 Result;
    s32 Function;

    do
    {
        x_DelayThread(1);
        status = sceMcSync(0,&Function,&Result);
    } while (status == 0);

    return Result;
}

//-----------------------------------------------------------------------------
s32             SonyToInternalError(s32 ErrorCode)
{
    if (ErrorCode >= 0)
        return ErrorCode;

    switch (ErrorCode)
    {
    case -1:
        return CARD_ERR_GENERAL;
        break;
    case -2:
        return CARD_ERR_UNFORMATTED;
        break;
    case -3:
        return CARD_ERR_FULL;
        break;
    case -4:
        return CARD_ERR_NOTFOUND;
        break;
    case -5:
        return CARD_ERR_PROTECTED;
        break;
    case -7:
        return CARD_ERR_TOOMANYOPEN;
        break;
    case -8:
        return CARD_ERR_WRITEFAILED;
        break;
    case -10:
        return CARD_ERR_UNPLUGGED;
        break;
    default:
        return CARD_ERR_UNPLUGGED;
    }
}

u16 sjis_table[]=
{
    0x8140,0x8149,0x8168,0x8194,0x8190,0x8193,0x8195,0x8166,0x8169,0x816a,0x8196,0x817b,0x8143,0x817c,0x8144,0x815e,    //  !"#$%&'()*+,-./
    0x824f,0x8250,0x8251,0x8252,0x8253,0x8254,0x8255,0x8256,0x8257,0x8258,0x8146,0x8147,0x8171,0x8181,0x8172,0x8148,    // 0123456789:;<=>?
    0x8197,0x8260,0x8261,0x8262,0x8263,0x8264,0x8265,0x8266,0x8267,0x8268,0x8269,0x826a,0x826b,0x826c,0x826d,0x826e,    // @ABCDEFGHIJKLMNO
    0x826f,0x8270,0x8271,0x8272,0x8273,0x8274,0x8275,0x8276,0x8277,0x8278,0x8279,0x816d,0x815f,0x816e,0x814f,0x8151,    // PQRSTUVWXYZ[\]^_
    0x814d,0x8281,0x8282,0x8283,0x8284,0x8285,0x8286,0x8287,0x8288,0x8289,0x828a,0x828b,0x828c,0x828d,0x828e,0x828f,    // `abcdefghijklmno
    0x8290,0x8291,0x8292,0x8293,0x8294,0x8295,0x8296,0x8297,0x8298,0x8299,0x829a,0x816f,0x8162,0x8170,0x8160,0x829f,    // pqrstuvwxyz{|}~

};

void AsciiToSJIS(const char *pTitleName,s16 *pSJISBuffer)
{
    s32 count;

    for (count=0;count<34;count++)
    {
        if (*pTitleName==0x0)
        {
            pSJISBuffer[count]=0x0;
        }
        else
        {
            pSJISBuffer[count]=(sjis_table[*pTitleName-32]>>8)|(sjis_table[*pTitleName-32]<<8);
            pTitleName++;
        }
    }
    // Make sure it's zero delimited
    pSJISBuffer[33]=0x0;
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
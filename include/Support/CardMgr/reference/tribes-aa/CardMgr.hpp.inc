#ifndef __CARDMGR_HPP
#define __CARDMGR_HPP

#include <xCore/x_files/x_types.hpp>

#define CARD_NAME_LENGTH    32

typedef struct s_card_file
{
    char    Filename[CARD_NAME_LENGTH];
    s32     Port;
    s32     Slot;
    s32     Handle;
    s32     Position;
    s32     Length;
} card_file;

typedef struct s_card_info
{
    s32     Location;
    s32     Capacity;
    s32     Used;
    s32     Free;
    s32     Type;
} card_info;

#define CARD_SEARCH_RESET   (0)
#define CARD_SEARCH_NEXT    (1)

#define CARD_SEEK_SET       (0)
#define CARD_SEEK_CUR       (1)
#define CARD_SEEK_END       (2)


#define CARD_ERR_OK             0
#define CARD_ERR_GENERAL        -1
#define CARD_ERR_NOTFOUND       -2
#define CARD_ERR_FAILED         -3
#define CARD_ERR_FULL           -4
#define CARD_ERR_DIRFULL        -5
#define CARD_ERR_UNFORMATTED    -6
#define CARD_ERR_CORRUPT        -7
#define CARD_ERR_UNPLUGGED      -8
#define CARD_ERR_CHANGED        -9
#define CARD_ERR_EXISTS         -10
#define CARD_ERR_PROTECTED      -11
#define CARD_ERR_WRITEFAILED    -12
#define CARD_ERR_TOOMANYOPEN    -13
#define CARD_ERR_WRONGTYPE      -14

//-----------------------------------------------------------------------------
// Notes:
//      The card_file structure between the PC and PS2 will vary slightly. As of
//      now, the PC will have nothing but dummy functions which will return a
//      not found error code status.
//
//      The filenames passed in to card_Check and card_Search are used to build
//      an identifier to whichever card is to be utilized. On the PC, this is
//      intended to be a directory path which will be used to store all the saved
//      game information.
// The open mode can be r(ead),w(rite),a(synchronous)

void        card_Init   (void);
void        card_Kill   (void);
card_file  *card_Open   (const char *pFilename,const char *pMode);
void        card_Close  (card_file *pFile);
s32         card_Write  (card_file *pFile,const void *pBase,s32 Length);
s32         card_Read   (card_file *pFile,void *pBase, s32 Length);
s32         card_Seek   (card_file *pFile,s32 Offset,s32 Mode);
s32         card_Length (card_file *pFile);
s32			card_Delete (const char *pFilename);
s32         card_Check  (const char *pFilename,card_info *pInfo);
s32         card_Search (const char *pFilename,card_file *pBuffer,s32 mode);
s32         card_MkDir  (const char *pDirname);
s32         card_MkIcon (const char *pTitlename);
s32         card_ChDir  (const char *pDirname);
s32         card_Format (const char *pFilename);
s32         card_Checksum(u8 *pBuffer,s32 Length,s32 ChecksumSeed);

#endif
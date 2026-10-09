#ifndef _E_NETFSDEFS_HPP
#define _E_NETFSDEFS_HPP
//-----------------------------------------------------------------------------
// Definitions used internally within the netfs client/server. This includes
// packet format and communication protocol tags
//-----------------------------------------------------------------------------
enum
{
    FSRV_TYPE_PASSTHRU  = 0,                // This file is passed through to the previous fs
    FSRV_TYPE_LOCAL,                        // This file has to be processed by our fs.
};

#define FILESERVER_PORT         32000
#define FILESERVER_TIMEOUT      1000          // Timeout in milliseconds
#define FILESERVER_RETRIES      3           // Number of retries before giving up
#define FILESERVER_CONCURRENT_REQUESTS  8
//
// The fileserver block size is LESS than the maximum packet size because we need to leave a little space for a header.
// Our header is ONLY 6 bytes but this will leave us a 24 byte space for future header expansion.
//
#define FILESERVER_BLOCK_SIZE   1000
#define FILESERVER_MAX_FILENAME 256
#define FILESERVER_MAX_MODE     10

//-----------------------------------------------------------------------------
// Fileserver requests
//-----------------------------------------------------------------------------
enum
{
    FSRV_REQ_RESET   = 0x1000,
    FSRV_REQ_OPEN,
    FSRV_REQ_CLOSE,
    FSRV_REQ_READ,
    FSRV_REQ_WRITE,
    FSRV_REQ_FLUSH,
    FSRV_REQ_FIND,
};

typedef struct s_fsrv_reset
{
} fsrv_reset;

typedef struct s_fsrv_open
{
    char    Filename[FILESERVER_MAX_FILENAME];
    char    Mode[FILESERVER_MAX_MODE];
} fsrv_open;

typedef struct s_fsrv_close
{
    s32     Handle;
} fsrv_close;

typedef struct s_fsrv_read
{
    s32     Handle;
    s32     Length;
    s32     Position;
} fsrv_read;

typedef struct s_fsrv_write
{
    s32     Handle;
    s32     Length;
    s32     Position;
    byte    Data[FILESERVER_BLOCK_SIZE];
} fsrv_write;

typedef struct s_fsrv_find
{
    s32     Address;
    s32     Port;
} fsrv_find;

typedef struct s_fsrv_header
{
        s32             Type;
        s32             Sequence;
} fsrv_header;

typedef struct s_fileserver_request
{
    fsrv_header         Header;
    union
    {
        fsrv_reset      Reset;
        fsrv_open       Open;
        fsrv_close      Close;
        fsrv_read       Read;
        fsrv_write      Write;
        fsrv_find       Find;
    };
    u8  Padding[1024-sizeof(fsrv_write)-sizeof(fsrv_header)];
} fileserver_request;

//-----------------------------------------------------------------------------
// Fileserver replies
//-----------------------------------------------------------------------------
enum
{
    FSRV_ERR_OK = 0,
    FSRV_ERR_NOTFOUND,
    FSRV_ERR_NOTOPEN,
    FSRV_ERR_READONLY,
    FSRV_ERR_BADCOMMAND,
};

typedef struct s_fsrv_open_reply
{
    s32     Status;                     // Status MUST always be the first field
    s32     Handle;
    s32     Length;
} fsrv_open_reply;

typedef struct s_fsrv_reply
{
    s32     Status;                     // Status MUST always be the first field
} fsrv_reply;

typedef struct s_fsrv_read_reply
{
    s32     Status;                     // Status MUST always be the first field
    s32     Length;
    byte    Data[FILESERVER_BLOCK_SIZE];
} fsrv_read_reply;

typedef struct s_fsrv_find_reply
{
    s32     Address;
    s32     Port;
} fsrv_find_reply;

typedef struct s_fsrv_write_reply
{
    s32     Status;                     // Status MUST always be the first field
    s32     Length;
} fsrv_write_reply;

typedef struct s_fileserver_reply
{
    struct
    {
        s32                 Sequence;
    } Header;
    union
    {
        fsrv_reply          Reset;
        fsrv_open_reply     Open;
        fsrv_reply          Close;
        fsrv_read_reply     Read;
        fsrv_write_reply    Write;
        fsrv_find_reply     Find;
    };
    u8  Padding[1024-sizeof(fsrv_read_reply)-sizeof(s32)];
} fileserver_reply;

#endif
#ifndef HOBBIT_X_STDIO_HPP
#define HOBBIT_X_STDIO_HPP

// Xbox decorated interfaces use unsigned-int pointers for opaque file handles.
// PC callsites independently establish each argument's order and width.
typedef unsigned int X_FILE;
// Xbox type back-reference 0 names the first char* parameter. PC calls pass
// the same mutable 32-bit va_list stack cursor.
int x_vsprintf(char* buffer, const char* format, char* arguments);
int x_printf(const char* format, ...);
int x_printfxy(int x, int y, const char* format, ...);
int x_sprintf(char* buffer, const char* format, ...);
X_FILE* x_fopen(const char* name, const char* mode);
void x_fclose(X_FILE* file);
int x_fread(void* buffer, int size, int count, X_FILE* file);
int x_fwrite(const void* buffer, int size, int count, X_FILE* file);
int x_flength(X_FILE* file);

int x_fprintf(X_FILE* file, const char* format, ...);
int x_fflush(X_FILE* file);
int x_fseek(X_FILE* file, int offset, int origin);
int x_ftell(X_FILE* file);
int x_feof(X_FILE* file);
int x_fgetc(X_FILE* file);
int x_fputc(int character, X_FILE* file);
void x_IOInit();
void x_IOKill();
int x_GetFileTime(const char* name, unsigned __int64& time);

typedef X_FILE* open_fn(const char*, const char*);
typedef void close_fn(X_FILE*);
typedef int read_fn(X_FILE*, unsigned char*, int);
typedef int write_fn(X_FILE*, const unsigned char*, int);
typedef int seek_fn(X_FILE*, int, int);
typedef int tell_fn(X_FILE*);
typedef int flush_fn(X_FILE*);
typedef int eof_fn(X_FILE*);
typedef int length_fn(X_FILE*);
typedef void print_fn(const char*);
typedef void print_at_fn(const char*, int, int);
void x_SetFileIOHooks(open_fn*, close_fn*, read_fn*, write_fn*, seek_fn*, tell_fn*, flush_fn*, eof_fn*, length_fn*);
void x_GetFileIOHooks(open_fn*&, close_fn*&, read_fn*&, write_fn*&, seek_fn*&, tell_fn*&, flush_fn*&, eof_fn*&, length_fn*&);
void x_SetPrintHook(print_fn*);
void x_SetPrintAtHook(print_at_fn*);

// Original seek-origin values used by real xmemfile/x_files providers.
#define X_EOF -1
#define X_SEEK_SET 0
#define X_SEEK_CUR 1
#define X_SEEK_END 2

#endif

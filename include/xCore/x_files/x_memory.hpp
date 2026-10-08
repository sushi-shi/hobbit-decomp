#ifndef HOBBIT_X_MEMORY_HPP
#define HOBBIT_X_MEMORY_HPP

// Debug allocation entry points retained by Hobbit's PC build. Xbox decorated
// names establish these interfaces; PC callers retain filename/line arguments.
void* x_malloc_info(int bytes, const char* file, int line);
void* x_malloc_fn(int bytes, const char* file, int line);
void* x_realloc_info(void* memory, int bytes, const char* file, int line);
void* x_realloc_fn(void* memory, int bytes, const char* file, int line);
void x_free_fn(void* memory, const char* file, int line);

void x_MemInit();
void x_MemKill();
int x_MemAddMark(const char* comment);
int x_MemGetAllocated();
int x_MemGetUsed();
void x_MemDump();
void x_MemDump(const char* filename);
void x_debug_MemSanity(const char* file, int line);
void x_MemGrabOwnerStack(unsigned char* buffer, int bufferSize);
void x_MemGetFree(int& freeBytes, int& largest, int& fragments);
int x_MemGetFree();

// Retail owner scopes have no fields or retained push/pop operations. PC
// constructor and destructor call pairs establish the one-byte empty class.
class x_mem_owner
{
public:
    x_mem_owner(const char* name);
    ~x_mem_owner();
};

// The imported source API uses the original filename/line allocation wrappers.
#ifndef x_malloc
#define x_malloc(bytes) x_malloc_fn((bytes), __FILE__, __LINE__)
#define x_realloc(memory,bytes) x_realloc_fn((memory), (bytes), __FILE__, __LINE__)
#define x_free(memory) x_free_fn((memory), __FILE__, __LINE__)
#endif

// Sibling memory-owner profile switch; retain the recovered Hobbit class name.
#ifdef X_MEM_DEBUG
#define MEMORY_OWNER(name) x_mem_owner __owner__(name)
#else
#define MEMORY_OWNER(name)
#endif
#define MEMORY_OWNER_DETAIL(name)

#endif

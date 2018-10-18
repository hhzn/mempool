#include <stdlib.h>

#define ALIGNMENT 8

struct mempool_header_t
{
    size_t size;
    size_t free_size;
    unsigned int first_offset;
    unsigned int first_free_offset;
};

struct mempool_internal_t
{
    unsigned int offset;
    size_t total_size;
    size_t payload_size;
    unsigned int prev_offset;
    unsigned int next_offset;
    unsigned int prev_free_offset;
    unsigned int next_free_offset;
};
void mempool_header_init(size_t memory_pool_size);
void mempool_freeblock_init();
size_t mempool_align_size(size_t size);
size_t mempool_calc_needed_size(size_t size);

static char *pool = NULL;

int mempool_init(size_t memory_pool_size)
{
    int r = 0;
    //Allocate memory for the pool
    char *init_pool = malloc(memory_pool_size);
    if (init_pool == NULL)
    {
        return 1;
    }
    //Place the header and the first free block
    {
        pool = pool_init;
        mempool_header_init(memory_pool_size);
        mempool_freeblock_init();
    }
    return 0;
}
int mempool_alloc(mempool_handle_t *handle, size_t size)
{
    size_t needed_size = 0;
    mempool_header_t *header = (mempool_header_t *)pool;
    //Check if there has been an init
    if(pool == NULL)
    {
        return 1;
    }
    //Check if the amount of memory can possibly be allocated
    needed_size = mempool_calc_needed_size(size);
    if(needed_size > header.free_size){
        return 2;
    }
    
}
size_t mempool_align_size(size_t size){
    return ((size -1) | (ALIGNMENT -1)) + 1;
}
size_t mempool_calc_needed_size(size_t size){
    return mempool_align_size(size + sizeof(mempool_internal_t));
}
void mempool_header_init(size_t memory_pool_size)
{
    mempool_header_t *header = (mempool_header_t *)pool;
    header.size = memory_pool_size;
    header.free_size = memory_pool_size - sizeof(mempool_header_t);
    header.first_offset = 0;
    header.first_free_offset = sizeof(mempool_header_t);
}
void mempool_freeblock_init()
{
    mempool_header_t *header = (mempool_header_t *)pool;
    mempool_internal_t *freeblock = (mempool_internal_t *)(pool + header.first_free_offset);
    freeblock.offset = header.first_free_offset;
    freeblock.size = header.free_size;
    freeblock.prev_free_offset = 0;
    freeblock.next_free_offset = 0;
    freeblock.next_offset = 0;
    freeblock.prev_offset = 0;
}

int mempool_exit()
{
    free(pool);
    pool = NULL;
}
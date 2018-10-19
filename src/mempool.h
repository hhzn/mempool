#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <stddef.h>

typedef struct
{
    unsigned int offset;
} mempool_handle_t;

enum mempool_err
{
    MEMPOOL_SUCCESS,
    MEMPOOL_FAILED_INIT,
    MEMPOOL_INVALID_HANDLE,
    MEMPOOL_NO_SIZE,
    MEMPOOL_NO_SPACE,
    MEMPOOL_INVALID_OFFSET,
    MEMPOOL_INVALID_SIZE
};
    
int mempool_init(size_t memory_pool_size);
int mempool_exit();
int mempool_alloc(mempool_handle_t *handle, size_t size);
int mempool_free(mempool_handle_t *handle);
int mempool_shared_to_pointer(mempool_handle_t *handle, size_t* size, void **mem_pointer);

#endif
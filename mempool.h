#ifndef MEMPOOL_H_
#define MEMPOOL_H_

struct mempool_handle_t{
    unsigned int offset;
};

int mempool_init(size_t memory_pool_size);
int mempool_exit();
int mempool_alloc(mempool_handle_t* handle, size_t size);
int mempool_free(mempool_handle_t* handle);
int mempool_shared_to_pointer(mempool_handle_t* handle, size_t* size, void** mem_pointer);

#endif
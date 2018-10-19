#include <stdlib.h>
#include <string.h>
#include "mempool.h"

#define ALIGNMENT 8
#define MIN_BLOCK_SIZE ALIGNMENT + sizeof(mempool_internal_t)

typedef struct
{
    size_t size;
    size_t free_size;
    unsigned int first_offset;
    unsigned int first_free_offset;
} mempool_header_t;

typedef struct internal
{
    unsigned int offset;
    size_t total_size;
    size_t payload_size;
    unsigned int prev_offset;
    unsigned int next_offset;
    unsigned int prev_free_offset;
    unsigned int next_free_offset;
} mempool_internal_t;

void mempool_header_init(size_t memory_pool_size);
void mempool_freeblock_init();
void mempool_move_freeblock(unsigned int offset,
                            int total_size,
                            mempool_internal_t *baseblock,
                            mempool_header_t *header);
size_t mempool_align_size(size_t size);
size_t mempool_calc_needed_size(size_t size);
int mempool_check_legal_offset(unsigned int offset,
                               mempool_header_t *header);
void mempool_free_right(mempool_internal_t *block,
                        mempool_header_t *header);
void mempool_free_left(mempool_internal_t *block,
                       mempool_header_t *header,
                       mempool_internal_t **moved_block);

static char *pool = NULL;

/*
 *  Initializes the memory pool
 *      in:
 *          memory_pool_size:       The size of the memory pool
 *      returns:
 *          MEMPOOL_FAILED_INIT     Failure
 *          MEMPOOL_SUCCESS         Success
 */
int mempool_init(size_t memory_pool_size)
{
    //Allocate memory for the pool
    char *init_pool = malloc(memory_pool_size);
    if (init_pool == NULL)
    {
        return MEMPOOL_FAILED_INIT;
    }
    //Place the header and the first free block
    {
        pool = init_pool;
        mempool_header_init(memory_pool_size);
        mempool_freeblock_init();
    }
    return MEMPOOL_SUCCESS;
}
/*
 *  Get the handle for accessing payload
 *      in:
 *          handle:         handle for the block
 *      out:
 *          size:           size of the payload
 *          mem_pointer:     pointer to the payload
 *      returns:
 *          MEMPOOL_FAILED_INIT     The mempool was not initialized
 *          MEMPOOL_INVALID_SIZE    Attemting to allocate 0 memory
 *          MEMPOOL_INVALID_HANDLE  The handle was a null pointer
 *          MEMPOOL_NO_SIZE         The memory to allocate is larger then the size of the buffer
 *          MEMPOOL_NO_SPACE        There was no space for this block
 *          MEMPOOL_SUCCESS         Success
 */
int mempool_alloc(mempool_handle_t *handle, size_t size)
{
    //Check if the client didn't give a nonsense handle
    if (handle == NULL)
    {
        return MEMPOOL_INVALID_HANDLE;
    }
    size_t needed_size = 0;
    mempool_header_t *header = (mempool_header_t *)pool;
    //Check if there has been an init
    if (pool == NULL)
    {
        return MEMPOOL_FAILED_INIT;
    }
    //Check against allocating empty administration
    if (size == 0)
    {
        return MEMPOOL_INVALID_SIZE;
    }
    //Check if the amount of memory can possibly be allocated
    needed_size = mempool_calc_needed_size(size);
    if (needed_size > header->free_size)
    {
        return MEMPOOL_NO_SIZE;
    }
    //Get the first free item from the freelist
    mempool_internal_t *free_block = (mempool_internal_t *)(pool + header->first_free_offset);
    int found = 1;
    //Loop through the freelist, searching for the first block that fits needed_size
    do
    {
        if (free_block->total_size >= needed_size)
        {
            found = 0;
        }
        //Go to the next block
        free_block = (mempool_internal_t *)(pool + free_block->next_free_offset);
    } while (free_block->next_free_offset != 0 && found == 1);
    if (found == 1)
    {
        return MEMPOOL_NO_SPACE;
    }
    //A free block has been found! If there is an empty space after placing our block, allocate a new free block
    if (free_block->total_size >= MIN_BLOCK_SIZE)
    {
        mempool_move_freeblock(free_block->offset + needed_size,
                               free_block->total_size - needed_size,
                               free_block,
                               header);
    }
    //Update size administration
    free_block->total_size = needed_size;
    free_block->payload_size = size;
    //Decrease the total free size
    header->free_size = header->free_size - free_block->total_size;
    //And done
    handle->offset = free_block->offset;
    return MEMPOOL_SUCCESS;
}
int mempool_free(mempool_handle_t *handle)
{
    if (pool == NULL)
    {
        return MEMPOOL_FAILED_INIT;
    }
    mempool_header_t *header = (mempool_header_t *)pool;
    if (handle == NULL)
    {
        return MEMPOOL_INVALID_HANDLE;
    }
    if (mempool_check_legal_offset(handle->offset, header) != 0)
    {
        return MEMPOOL_INVALID_OFFSET;
    }
    //get block
    mempool_internal_t *block = (mempool_header_t *)pool + handle->offset;
    mempool_internal_t *moved_block = NULL;
    mempool_free_left(block, header, &moved_block);
    block = moved_block;
    mempool_free_right(block, header);

    //Update size adminstration
    block->payload_size = 0;

    //Increase total free size
    header->free_size = header->free_size + block->total_size;

    //And done
    return MEMPOOL_SUCCESS;
}
/*
 *  Get the handle for accessing payload
 *      in:
 *          handle:         handle for the block
 *      out:
 *          size:           size of the payload
 *          mem_pointer:     pointer to the payload
 *      returns:
 *          MEMPOOL_FAILED_INIT     The mempool was not initialized
 *          MEMPOOL_INVALID_HANDLE  There handle was a null pointer
 *          MEMPOOL_INVALID_OFFSET  The handle contained a invalid offset
 */
int mempool_shared_to_pointer(mempool_handle_t *handle, size_t* size, void **mem_pointer)
{
    if (pool == NULL)
    {
        return MEMPOOL_FAILED_INIT;
    }
    mempool_header_t *header = (mempool_header_t *)pool;
    if (handle == NULL)
    {
        return MEMPOOL_INVALID_HANDLE;
    }
    if (mempool_check_legal_offset(handle->offset, header) != 0)
    {
        return MEMPOOL_INVALID_OFFSET;
    }
    mempool_internal_t *block = (mempool_header_t *)pool + handle->offset;
    void *payload = pool + block->offset + sizeof(mempool_internal_t);
    *mem_pointer = &payload;
    size = block->payload_size;
    return MEMPOOL_SUCCESS;
}
/*
 *  Updates the administration to the 'left' of a freed block, and recombines if needed
 *      in:
 *          block:          block that is being freed
 *          header:         header of the mempool
 *      out:
 *          moved_block:    new reference to the block being freed
 */
void mempool_free_left(mempool_internal_t *block, mempool_header_t *header, mempool_internal_t **moved_block)
{
    //check if there is a previous block
    if (block->prev_offset == 0)
    {
        mempool_internal_t *prev_block = (mempool_header_t *)pool + block->prev_free_offset;
        //check if the previous block has size 0, if so, combine
        if (prev_block->payload_size == 0)
        {
            prev_block->next_offset = block->next_offset;
            prev_block->next_free_offset = block->next_free_offset;
            prev_block->total_size = block->total_size + prev_block->total_size;
            block = prev_block;
        }
        //If there is a block before this one, it needs to point to this one
        if(block->prev_offset != 0){
            mempool_internal_t *new_prev_block = (mempool_header_t *)pool + block->prev_offset;
            new_prev_block->next_offset = block->offset;
            //If there is a free block before this one, it needs to point to this one
            if(block->prev_free_offset != 0)
            {
                mempool_internal_t *prev_free_block = (mempool_header_t *)pool + block->prev_free_offset;
                prev_free_block->next_free_offset = block->offset;
            }
        }
    }
    //This is the first block
    else
    {
        header->first_offset = block->offset;
        header->first_offset = block->offset;
    }
    //Ensure that moved block points to the correct mempool_internal
    *moved_block = &block;
}
/*
 *  Updates the administration to the 'right' of a freed block, and recombines if needed
 *      in:
 *          block:          block that is being freed
 *          header:         header of the mempool
 */
void mempool_free_right(mempool_internal_t *block, mempool_header_t *header)
{
    //check if there is a next block, or if we are at the edge of allocated memory
    if (block->next_offset != 0)
    {
        mempool_internal_t *next_block = (mempool_header_t *)pool + block->next_offset;
        //check if the next block has size 0, if so, combine
        if (next_block->payload_size == 0)
        {
            block->next_offset = next_block->next_offset;
            block->next_free_offset = next_block->next_free_offset;
            block->total_size = block->total_size + next_block->total_size;
        }
        //If there is a block further in memory, it needs to point to this block
        if (block->next_offset != 0)
        {
            mempool_internal_t *new_next_block = (mempool_header_t *)pool + block->next_offset;
            new_next_block->prev_offset = block->offset;

            //If there is a free block further in memory, it needs to point back to this block
            if (block->next_free_offset != 0)
            {
                mempool_internal_t *new_next_free_block = (mempool_header_t *)pool + block->next_free_offset;
                new_next_free_block->prev_free_offset = block->offset;
            }
        }
    }
}

int mempool_check_legal_offset(unsigned int offset,
                               mempool_header_t *header)
{
    if (offset > (header->size + MIN_BLOCK_SIZE && offset > sizeof(mempool_header_t)))
    {
        return 1;
    }
    return 0;
}
/*
 *  'Moves' the starting point of a freeblock further into the mempool
 *      in:
 *          offset:         new offset for the freeblock            must be larger then baseblock->offset and smaller then header->size
 *          total_size:     new total size for the freeblock        total_size + offset must be smaller then header->size and larger then sizeof(mempool_internal_t)
 *          baseblock:      block to move
 *          header:         header of the mempool
 */
void mempool_move_freeblock(unsigned int offset,
                            int total_size,
                            mempool_internal_t *baseblock,
                            mempool_header_t *header)
{
    //Set this freeblock
    mempool_internal_t *freeblock = (mempool_internal_t *)pool + offset;
    memcpy((void *)freeblock, (void *)baseblock, sizeof(mempool_internal_t));
    freeblock->offset = offset;
    freeblock->total_size = total_size;
    //If there is a previous freeblock, it needs to point to the new offset
    //Otherwize, this is the first free block, and the header needs to point to this one
    if (freeblock->prev_free_offset != 0)
    {
        mempool_internal_t *prev_freeblock = (mempool_internal_t *)pool + freeblock->prev_free_offset;
        prev_freeblock->next_free_offset = offset;
    }
    else
    {
        header->first_free_offset = offset;
    }
    //If there is a next block, it needs to point back to this one
    if (freeblock->next_offset != 0)
    {
        mempool_internal_t *next_block = (mempool_internal_t *)pool + freeblock->next_offset;
        next_block->prev_offset = offset;
        //If there is a next freeblock, it needs to point back to this one
        if (freeblock->next_free_offset != 0)
        {
            mempool_internal_t *next_freeblock = (mempool_internal_t *)pool + freeblock->next_free_offset;
            next_freeblock->prev_free_offset = offset;
        }
    }
    //Point back to the block before us, and make it point to this one
    freeblock->prev_offset = baseblock->offset;
    baseblock->next_free_offset = offset;
}
size_t mempool_align_size(size_t size)
{
    return ((size - 1) | (ALIGNMENT - 1)) + 1;
}
size_t mempool_calc_needed_size(size_t size)
{
    return mempool_align_size(size + sizeof(mempool_internal_t));
}
void mempool_header_init(size_t memory_pool_size)
{
    mempool_header_t *header = (mempool_header_t *)pool;
    header->size = memory_pool_size;
    header->free_size = memory_pool_size - sizeof(mempool_header_t);
    header->first_offset = 0;
    header->first_free_offset = sizeof(mempool_header_t);
}
void mempool_freeblock_init()
{
    mempool_header_t *header = (mempool_header_t *)pool;
    mempool_internal_t *freeblock = (mempool_internal_t *)(pool + header->first_free_offset);
    freeblock->offset = header->first_free_offset;
    freeblock->total_size = header->free_size;
    freeblock->payload_size = 0;
    freeblock->prev_free_offset = 0;
    freeblock->next_free_offset = 0;
    freeblock->next_offset = 0;
    freeblock->prev_offset = 0;
}

int mempool_exit()
{
    free(pool);
    pool = NULL;
}
#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#ifdef ALLOCATOR_IMPLEMENTATION
#   define ALLOCATOR_IMPLEMENTATION__
#endif // ALLOCATOR_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>

typedef struct Allocator Allocator;

Allocator  *allocator_new();
void       *allocator_alloc(Allocator *a, size_t size);
void       *allocator_zalloc(Allocator *a, size_t size);
void       *allocator_realloc(Allocator *a, void *base, size_t size);
void        allocator_free(Allocator *a, void *base);
void        allocator_reset(Allocator *a);
void        allocator_kill(Allocator *a);

#ifdef ALLOCATOR_IMPLEMENTATION__

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct FreeNode {
    void    *base;
    size_t   size;
    struct Block *block;
    struct FreeNode *prev, *next;
};

typedef struct {
    struct FreeNode *head, *tail;
    size_t count;
} FreeList;

struct Block {
    void    *base;
    size_t  offset;
    size_t  user_allocated_size; 
    size_t  capacity;
};

typedef struct {
    struct Block   **items;
    size_t          len;
    size_t          size;
} BlockList;

struct Allocator {
    FreeList    fl;
    BlockList   blocks;
    size_t      user_allocated_size;
    size_t      capacity;
};

#include "array.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

void *__int_malloc(size_t size) {
    void *ptr = malloc(size);
    if(ptr == NULL) {
        perror("malloc failed");
        abort();
    }
    return ptr;
}

void *__int_zalloc(size_t size) {
    void *ptr = __int_malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

struct AllocationHeader {
    void            *base;
    struct Block    *block;
    size_t          size;
    bool            is_free;
    bool            is_allocated;
};


FreeList    __int_freelist_sort(FreeList fl);
void        __int_freelist_split(FreeList fl, FreeList *left, FreeList *right);
FreeList    __int_freelist_merge(FreeList left, FreeList right);
void        __int_allocator_reduce_freelist(Allocator *this);

void __int_init_header(struct AllocationHeader *header, void *base, size_t size, struct Block *block) {
    header->size = size;
    header->base = base;
    header->is_free = false;
    header->is_allocated = true;
    header->block = block;
}

#define ALLOCATION_SIZE(size) ((size) + sizeof(struct AllocationHeader))

struct FreeNode *__int__freenode_new(void *base, size_t size, struct Block *block) {
    struct FreeNode *freenode = __int_zalloc(sizeof(*freenode));    

    freenode->base = base;
    freenode->size = size;
    freenode->block = block;

    return freenode;
}

struct Block *__int_block_new(size_t capacity) {
    struct Block *block = __int_malloc(sizeof(*block));
    block->base = __int_zalloc(capacity);
    block->user_allocated_size = 0; 
    block->capacity = capacity;
    block->offset = 0;
    return block;
}


#define ALLOCATOR_DEFAULT_BLOCK_CAPACITY 1024

struct Block *__int_block_default_new() {
    return __int_block_new(ALLOCATION_SIZE(ALLOCATOR_DEFAULT_BLOCK_CAPACITY));
}

Allocator *allocator_new() {
    Allocator *allocator = __int_zalloc(sizeof(*allocator));

    allocator->fl = (FreeList) {0};

    struct Block *initial_block = __int_block_default_new();
    allocator->blocks = (BlockList) {0};
    arrappend(allocator->blocks, initial_block); 
    
    allocator->user_allocated_size = 0;
    allocator->capacity = initial_block->capacity;

    return allocator;
}

void *__int_allocator_push_block(Allocator *this, size_t size) {
    assert(size != 0);

    struct Block *block = __int_block_new(ALLOCATION_SIZE(max(ALLOCATOR_DEFAULT_BLOCK_CAPACITY, size)));
    arrappend(this->blocks, block);

    void *base = (void *)((struct AllocationHeader *)block->base + 1);
    block->user_allocated_size += ALLOCATION_SIZE(size);
    block->offset += ALLOCATION_SIZE(size);
    __int_init_header(block->base, base, size, block);
    
    this->user_allocated_size += ALLOCATION_SIZE(size);
    this->capacity += block->capacity;

    return base;
}

void *allocator_alloc(Allocator *this, size_t size) {
    if(size == 0) return NULL;

    if(this->user_allocated_size + ALLOCATION_SIZE(size) > this->capacity) {
        return __int_allocator_push_block(this, size);
    }

    for(size_t i = 0; i < this->blocks.len; ++i) {
        struct Block *block = this->blocks.items[i];
        if(block->offset + ALLOCATION_SIZE(size) > block->capacity) continue;

        // get the offset pointer
        void *ptr = block->base + block->offset;
        
        // mark the region as allocated
        block->offset += ALLOCATION_SIZE(size);
        block->user_allocated_size += ALLOCATION_SIZE(size);

        // update the allocator stats
        this->user_allocated_size += ALLOCATION_SIZE(size);

        // make the essential stuff for header and return the base pointer back
        struct AllocationHeader *header = (struct AllocationHeader *)ptr;
        void *base = (void *)(header + 1);

        __int_init_header(header, base, size, block);
        return base;
    }

    struct FreeNode *current = this->fl.head;
    while(current != NULL) {
        if(current->size >= ALLOCATION_SIZE(size)) break;
        current = current->next;
    }

    if(current == NULL) {
        return __int_allocator_push_block(this, size);
    }

    void *base = (void *)((struct AllocationHeader *)current->base + 1);
    __int_init_header(current->base, base, size, current->block);

    current->block->user_allocated_size += ALLOCATION_SIZE(size);
    this->user_allocated_size += ALLOCATION_SIZE(size);

    current->size -= ALLOCATION_SIZE(size);
    current->base += ALLOCATION_SIZE(size);

    if(current->size != 0) return base;

    if(current->prev != NULL) current->prev->next = current->next;
    else this->fl.head = current->next;
    
    if(current->next != NULL) current->next->prev = current->prev;
    else this->fl.tail = current->next;

    free(current);
    return base;
}

void *allocator_zalloc(Allocator *this, size_t size) {
    void *base = allocator_alloc(this, size);
    memset(base, 0, size);
    return base;
}

void __int__allocator_append_freenode(Allocator *this, struct FreeNode *freenode) {
    assert(freenode != NULL);

    this->fl.count += 1;
    this->user_allocated_size -= freenode->size;
    freenode->block->user_allocated_size -= freenode->size;
    
    if(this->fl.head == NULL) {
        this->fl.head = freenode;
        this->fl.tail = freenode;
        return;
    } 

    freenode->prev = this->fl.tail;
    this->fl.tail->next = freenode;
    this->fl.tail = freenode;
}

void *allocator_realloc(Allocator *this, void *base, size_t size) {
    if(base == NULL) return allocator_alloc(this, size);

    struct AllocationHeader *header = ((struct AllocationHeader *)base - 1);
    if(header->is_free) {
        fprintf(stderr, "cannot reallocated free pointer\n");
        abort();
    }

    if(!header->is_allocated) {
        fprintf(stderr, "failed to reallocate, pointer not produced by allocator\n");
        abort();
    }

    if(header->base != base) {
        fprintf(stderr, "failed to reallocate, invalid base pointer\n");
        abort();
    }

    if(size == 0) {
        allocator_free(this, base);
        return NULL;
    }

    if(header->size == size) return header->base;

    if(size < header->size) {
        struct FreeNode *freenode = __int__freenode_new(header->base + header->size, header->size - size, header->block);
        __int__allocator_append_freenode(this, freenode);
        header->size -= freenode->size;
        __int_allocator_reduce_freelist(this);
        return header->base;
    }

    void *ptr = allocator_alloc(this, size);
    memcpy(ptr, header->base, header->size);
    allocator_free(this, header->base);

    return ptr;
}

void allocator_free(Allocator *this, void *base) {
    if(base == NULL) return;

    struct AllocationHeader *header = ((struct AllocationHeader *)base - 1);
    if(header->is_free) {
        fprintf(stderr, "failed to free, double free calls\n");
        abort();
    }

    if(!header->is_allocated) {
        fprintf(stderr, "failed to free, region not marked as allocated\n");
        abort();
    }

    if(header->base != base) {
        fprintf(stderr, "failed to free, invalid base pointer\n");
        abort();
    }

    struct FreeNode *freenode = __int__freenode_new((void *)header, ALLOCATION_SIZE(header->size), header->block);
    
    __int__allocator_append_freenode(this, freenode);

    header->is_free = true;
    header->is_allocated = false;
    header->base = NULL;
    header->size = 0;

    __int_allocator_reduce_freelist(this);
}

void allocator_kill(Allocator *this) {
    struct FreeNode *current = this->fl.head;
    while(current != NULL) {
        struct FreeNode *next = current->next;
        free(current);
        current = next;
    }
    
    for(size_t i = 0; i < this->blocks.len; ++i) {
        free(this->blocks.items[i]->base);
        free(this->blocks.items[i]);
    }

    free(this);
} 


FreeList __int_freelist_merge(FreeList left, FreeList right) {
    struct FreeNode *left_current   = left.head;
    struct FreeNode *right_current  = right.head;

    struct FreeNode *min_node = NULL;
    if(left_current->base > right_current->base) {
        min_node = right_current;
        right_current = right_current->next;
    } else {
        min_node = left_current;
        left_current = left_current->next;
    }

    FreeList merged = (FreeList) {
        .count = 1,
        .head = min_node,
        .tail = min_node,
    };
    
    while(left_current != NULL && right_current != NULL) {
        if(left_current->base > right_current->base) {
            min_node = right_current;
            right_current = right_current->next;
        } else {
            min_node = left_current;
            left_current = left_current->next;
        }

        min_node->prev = merged.tail;
        merged.tail->next = min_node;
        merged.tail = min_node;
        merged.count += 1;
    }

    while(left_current != NULL) {
        left_current->prev = merged.tail;
        merged.tail->next = left_current;
        merged.tail = left_current;
        merged.count += 1;
        left_current = left_current->next;
    }

    while(right_current != NULL) {
        right_current->prev = merged.tail;
        merged.tail->next = right_current;
        merged.tail = right_current;
        merged.count += 1;
        right_current = right_current->next;
    }

    return merged;
}

void __int_freelist_split(FreeList fl, FreeList *left, FreeList *right) {
    struct FreeNode *slow = fl.head;
    struct FreeNode *fast = fl.head;

    while(fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }

    size_t left_count = fl.count / 2;
    size_t right_count = fl.count - left_count;

    *left = (FreeList) {
        .count  = left_count,
        .head   = fl.head,
        .tail   = slow->prev,
    };

    *right = (FreeList) {
        .count  = right_count,
        .head   = left->tail->next,
        .tail   = fl.tail
    };

    left->tail->next = NULL;
    right->head->prev = NULL;
}


FreeList __int_freelist_sort(FreeList fl) {
    if(fl.count < 2) return fl; 

    if(fl.count == 2) {
        if(fl.head->base <= fl.tail->base) return fl;

        fl.tail->next = fl.head;
        fl.tail->prev = NULL;

        fl.head->prev = fl.tail;
        fl.head->next = NULL;

        return fl;
    } 
    
    FreeList left = {0};
    FreeList right = {0};
    __int_freelist_split(fl, &left, &right);

    FreeList sorted_left = __int_freelist_sort(left);
    FreeList sorted_right = __int_freelist_sort(right);

    return __int_freelist_merge(sorted_left, sorted_right);
}

void __int_allocator_reduce_freelist(Allocator *this) {
    this->fl = __int_freelist_sort(this->fl);
    
    struct FreeNode *current = this->fl.head;
    while(current != NULL && current->next != NULL) {
        struct FreeNode *next = current->next;
        
        if(current->block != next->block) {
            current = next;
            continue;
        }
        
        void *end = current->base + current->size;
        void *start = next->base;

        if(start != end) {
            current = next;
            continue;
        }

        current->size += next->size;
        current->next = next->next;
        
        if(next->next == NULL) {
            this->fl.tail = current;
        } else {
            next->next->prev = current;
        }

        free(next);
        this->fl.count -= 1;
    }
}

void allocator_reset(Allocator *this) {
    struct FreeNode *current = this->fl.head;
    while(current != NULL) {
        struct FreeNode *next = current->next;
        free(current);
        current = next;
    }

    this->fl.head = NULL;
    this->fl.tail = NULL;
    this->fl.count = 0;

    
    for(size_t i = 0; i < this->blocks.len; ++i) {
        memset(this->blocks.items[i]->base, 0, this->blocks.items[i]->capacity);
        this->blocks.items[i]->user_allocated_size = 0;
        this->blocks.items[i]->offset = 0;
    }

    this->user_allocated_size = 0;
}


#endif // ALLOCATOR_IMPLEMENTATION

#endif // ALLOCATOR_H_
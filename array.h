#ifndef ARRAY_H_
#define ARRAY_H_

#include <stdio.h>
#include <stdlib.h>

#ifndef ARRAY_INITIAL_SIZE
#   define ARRAY_INITIAL_SIZE 100
#endif // ARRAY_INITIAL_SIZE

/**
 * type Array<T> = {
 *      T *items;
 *      size_t len;
 *      size_t size;
 */

#define arrappend(array, item) \
    do { \
        if((array).len >= (array).size) { \
            size_t new_size = (array).size * 2; \
            if(new_size == 0) new_size = ARRAY_INITIAL_SIZE; \
            \
            (array).items = realloc((array).items, sizeof(*(array).items) * new_size); \
            if((array).items == NULL) perror("malloc error"); \
            (array).size = new_size; \
        } \
        \
        (array).items[(array).len] = item; \
        (array).len += 1; \
    } while(0)


/** 
 * arrappend_allocator: append item to the array using allocator
 * expect alloc_realloc_func: function(Allocator *allocator, void *ptr, size_t new_size) void *;
 */
#define arrappend_allocator(array, item, alloc_realloc_func, allocator) \
    do { \
        if((array).len >= (array).size) { \
            size_t new_size = (array).size * 2; \
            if(new_size == 0) new_size = ARRAY_INITIAL_SIZE; \
            \
            (array).items = alloc_realloc_func((allocator), (array).items, sizeof(*(array).items) * new_size); \
            if((array).items == NULL) perror("allocator error"); \
            (array).size = new_size; \
        } \
        \
        (array).items[(array).len] = item; \
        (array).len += 1; \
    } while(0)

#define arrclean(array) { (array).len = 0; }

#define arrfree(array) { free((array).items); (array).len = 0; (array).size = 0; }

/** 
 * arrfree_allocator: free array using allocator
 * expect alloc_free_func: function(Allocator *allocator, void *ptr) void;
 */
#define arrfree_allocator(array, alloc_free_func, allocator) { alloc_free_func((allocator), (array).items); (array).len = 0; (array).size = 0; }


#endif
#ifndef STRING_BUILDER_H_
#define STRING_BUILDER_H_

#include <stdio.h>

#ifdef STRING_BUILDER_IMPLEMENTATION

#   define STRING_BUILDER_IMPLEMENTATION__ 

#   ifndef ALLOCATOR_IMPLEMENTATION
#       define ALLOCATOR_IMPLEMENTATION
#   endif // ALLOCATOR_IMPLEMENTATION

#endif  // STRING_BUILDER_IMPLEMENTATION

#include "allocator.h"

typedef struct {
    char        *data;
    size_t      len;
    size_t      size;
    Allocator   *allocator;
} StringBuilder;

StringBuilder *sb_new();
StringBuilder *sb_new_from_allocator(Allocator *allocator);
void           sb_reset(StringBuilder *sb);
void           sb_append(StringBuilder *sb, char *data, size_t len);
void           sb_append_cstr(StringBuilder *sb, const char *cstr);
char          *sb_collect(StringBuilder *sb);
void           sb_free(StringBuilder *sb);

#ifdef STRING_BUILDER_IMPLEMENTATION__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define __int__sb_max(a, b) ((a) > (b) ? (a) : (b))
#define __int__sb_min(a, b) ((a) < (b) ? (a) : (b))


StringBuilder *sb_new() {
    StringBuilder *sb = (StringBuilder *)malloc(sizeof(*sb));
    if(sb == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    
    sb->size = 1024;
    sb->data = (char *)malloc(sizeof(char) * sb->size);
    if(sb->data == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    sb->allocator = NULL;
    sb->len = 0;

    return sb;
}

StringBuilder *sb_new_from_allocator(Allocator *allocator) {
    StringBuilder *sb = (StringBuilder *)allocator_alloc(allocator, sizeof(*sb));
    
    sb->size = 1024;
    sb->data = (char *)allocator_alloc(allocator, sizeof(char) * sb->size);
    sb->len = 0;
    sb->allocator = allocator;

    return sb;
}

void __int__sb_resize(StringBuilder *sb, size_t min_required_new_size) {
    size_t new_size = __int__sb_max(sb->size * 2, min_required_new_size) + 1; // extra one for null-terminator on string collection

    if(sb->allocator != NULL) {
        sb->data = (char *)allocator_realloc(sb->allocator, sb->data, new_size);
        sb->size = new_size;
        return;
    }

    sb->data = (char *)realloc(sb->data, new_size);
    sb->size = new_size;
}

void __int__sb_clamp(StringBuilder *sb, size_t size) {
    assert(size <= sb->size);
    assert(sb->len < size);

    if(sb->allocator != NULL) {
        sb->data = (char *)allocator_realloc(sb->allocator, sb->data, size);
        sb->size = size;
        return;
    }

    sb->data = (char *)realloc(sb->data, size);
    sb->size = size;
}


void sb_append(StringBuilder *sb, char *data, size_t len) {
    if(sb->len + len > sb->size) __int__sb_resize(sb, sb->len + len);
    memcpy(sb->data + sb->len, data, len);
    sb->len += len;
}

void sb_append_cstr(StringBuilder *sb, const char *cstr) {
    return sb_append(sb, (char *)cstr, strlen(cstr));
}

char *sb_collect(StringBuilder *sb) {
    __int__sb_clamp(sb, sb->len + 1);
    sb->data[sb->len] = 0;    
    return sb->data;
}

void sb_reset(StringBuilder *sb) {
    sb->len = 0;
}

void sb_free(StringBuilder *sb) {
    if(sb->allocator == NULL) {
        free(sb->data);
        return free(sb);
    }

    allocator_free(sb->allocator, sb->data);
    allocator_free(sb->allocator, sb);
}

#endif // STRING_BUILDER_IMPLEMENTATION

#endif // STRING_BUILDER_H_

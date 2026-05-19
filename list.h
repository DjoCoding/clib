#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct Node Node;

struct Node {
    void *value;
    void *prev, *next;
};

#define list_append(self, val) \
    do { \
        Node *node = __int__node_create(sizeof(*(self).head)); \
        ((typeof((self).head))node->value)[0] = (val); \
        \
        if((self).head == NULL) { \
            (self).head = (typeof((self).head))node; \
            (self).tail = (typeof((self).tail))node; \
            break; \
        } \
        \
        ((Node *)(self).tail)->next = node; \
        node->prev = (self).tail; \
        (self).tail = (typeof((self).tail))node; \
    } while (0);


#define list_foreach(self, index, item, ...) \
    do { \
        size_t index = 0; \
        Node *current = (Node *)((self).head); \
        while(current != NULL) { \
            typeof(*(self).head) item = ((typeof((self).head))current->value)[0]; \
            __VA_ARGS__ \
            index += 1; \
            current = current->next; \
        } \
    } while(0)

#define list_map(self, mapped, index, item, mapped_item, ...) \
    do { \
        size_t index = 0; \
        Node *current = (Node *)((self).head); \
        while(current) { \
            typeof(*(self).head) item = ((typeof((self).head))current->value)[0]; \
            typeof(*(mapped).head) mapped_item; \
            __VA_ARGS__ \
            list_append(mapped, mapped_item); \
            index += 1; \
            current = current->next; \
        } \
    } while(0)

#define list_find(self, item, index, found,...) \
    do { \
        index = 0; \
        Node *current = (Node *)((self).head); \
        while(current != NULL) { \
            typeof(*(self).head) item = ((typeof((self).head))current->value)[0]; \
            bool found = false; \
            __VA_ARGS__ \
            if(found) break; \
            current = current->next; \
            index += 1; \
        } \
    } while(0)

#define list_free(self, item,...) \
    do { \
        Node *current = (Node *)((self).head); \
        while(current != NULL) { \
            typeof(*(self).head) item = ((typeof((self).head))current->value)[0]; \
            __VA_ARGS__ \
            Node *next = current->next; \
            free(current); \
            current = next; \
        } \
    } while(0)

#define list_filter(self, filtered, item, pred, ...) \
    do { \
        size_t index = 0; \
        Node *current = (Node *)((self).head); \
        while(current != NULL) { \
            typeof(*(self).head) item = ((typeof((self).head))current->value)[0]; \
            bool pred = false; \
            __VA_ARGS__ \
            if(pred) list_append(filtered, item); \
            index += 1; \
            current = current->next; \
        } \
    } while(0)

#define list_at(self, index, item) \
    do { \
        size_t _index = 0; \
        Node *current = (Node *)((self).head); \
        while(current != NULL && _index <= (size_t)index) { \
            item = (typeof((self).head))current->value; \
            _index += 1; \
            current = current->next; \
        } \
    } while(0)

#ifdef LINKEDLIST_IMPLEMENTATION

Node *__int__node_create(size_t T_size) {
    Node *node = malloc(sizeof(*node));
    if(node == NULL) {
        perror("malloc failed");
        exit(1);
    }

    node->value = malloc(T_size);
    if(node->value == NULL) {
        perror("malloc failed");
        exit(1);
    }

    node->next = NULL;
    node->prev = NULL;

    return node;
}

#endif // LINKEDLIST_IMPLEMENTATION


#endif // LINKEDLIST_H_

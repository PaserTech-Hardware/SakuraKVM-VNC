#ifndef __SAKURAKVMD_LIST_H__
#define __SAKURAKVMD_LIST_H__

#include <stddef.h>

typedef struct _list_node_t {
  struct _list_node_t* next;
  struct _list_node_t* prev;
  void* data;
} list_node_t;

typedef struct _list_ctx_t {
  list_node_t* head;
  list_node_t* tail;
  size_t size;
} list_ctx_t;

/**
 * @brief create a new list
 *
 * @param ctx return list context if success
 * @return int error code
 */
int list_create(list_ctx_t** ctx);

/**
 * @brief put data into list
 *
 * @param ctx list context pointer
 * @param data data pointer
 * @param length data length
 * @param node return node pointer
 * @return int error code
 */
int list_put(list_ctx_t* ctx, void* data, size_t length, list_node_t** node);

int list_delete(list_ctx_t* ctx, list_node_t* node);

/**
 * @brief destroy list
 *
 * @param ctx list context pointer
 * @return int error code
 */
int list_destroy(list_ctx_t* ctx);


#endif

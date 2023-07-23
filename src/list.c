#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "list.h"
#include "logging.h"

int list_create(list_ctx_t** ctx) {

  // validate parameters
  if(ctx == NULL)
    return -EINVAL;

  // initialize context
  list_ctx_t* _ctx = malloc(sizeof(list_ctx_t)); {
    if(_ctx == NULL)
      return -ENOMEM;

    // clear buffer
    memset(_ctx, 0, sizeof(list_ctx_t));
    *ctx = _ctx;
  }

  LOGV("chain list created %p", _ctx);

  return 0;
}

int list_destroy(list_ctx_t* ctx) {
  if(ctx == NULL)
    return -EINVAL;

  // free all nodes
  list_node_t* _node = ctx->head;
  while(_node != NULL) {
    list_node_t* _next = _node->next;
    free(_node);
    _node = _next;
  }

  // free context
  free(ctx);
  LOGV("chain list destroyed %p", ctx);

  return 0;
}

int list_put(list_ctx_t* ctx, void* data, size_t length, list_node_t** node) {
  if(ctx == NULL || data == NULL)
    return -EINVAL;

  if(ctx->size == SIZE_MAX) {
    LOGW("list size overflow: SIZE_MAX reached");
    return -ENOMEM;
  }

  // allocate an node + userdata length buffer
  // to reduce memory fragmentation
  size_t _length = sizeof(list_node_t) + length;
  list_node_t* _node = malloc(_length); {
    if(_node == NULL)
      return -ENOMEM;
  }

  // initialize node and copy data into
  memset(_node, 0, _length); {

    // setup next node pointer
    _node->next = NULL;

    // setup prev node pointer
    if(ctx->head == NULL) _node->prev = NULL;
    else _node->prev = ctx->tail;

    // copy data
    _node->data = (&_node->data) + 1;
    memcpy(_node->data, data, length);
  }

  // append node to list
  if(ctx->head == NULL) {
    ctx->head = _node;
    ctx->tail = _node;
  }
  else {
    ctx->tail->next = _node;
    ctx->tail = _node;
  }

  ++ctx->size;

  // return node pointer
  if(node != NULL) *node = _node;

  return 0;
}

int list_delete(list_ctx_t* ctx, list_node_t* node) {

  if(ctx == NULL || node == NULL)
    return -EINVAL;

  LOGV("delete node %p", node);
  LOGV("node prev %p, node next %p", node->prev, node->next);

  if(node->prev == NULL)
    ctx->head = node->next;
  else
    node->prev->next = node->next;

  if(node->next == NULL)
    ctx->tail = node->prev;
  else
    node->next->prev = node->prev;

  free(node);

  --ctx->size;

  return 0;
}

#ifndef LIBFLUXUS_H
#define LIBFLUXUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libhash/libhash.h"
#include "libutil/libutil.h"

typedef char* action_t;

typedef void* handler_t(void*);

typedef void* subscriber_t(void* state, action_t action);

typedef struct {
  array_t*    subscribers;
  hash_table* handlers;
  void*       state;
  void* (*dispatch)(void*);
} __store_t;

typedef __store_t* store_t;

store_t* create_store(void* initial_state);

void create_action(store_t* store, char* action, handler_t* handler);

void subscribe(store_t* store, void* (*subscriber)(void*));

void unsubscribe(store_t* store, subscriber_t* subscriber);

void* dispatch(store_t* store, action_t action);

void* get_state(store_t* store);

#ifdef __cplusplus
}
#endif

#endif /* LIBFLUXUS_H */

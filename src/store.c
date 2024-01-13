#include <stdlib.h>

#include "libfluxus.h"

static void*
xmalloc (size_t sz)
{
  void* mem;
  if ((mem = malloc(sz)) == NULL) {
    exit(EXIT_FAILURE);
  }

  return mem;
}

store_t*
create_store (void* initial_state)
{
  __store_t* store   = xmalloc(sizeof(__store_t));
  store->handlers    = ht_init(0);
  store->subscribers = array_init();
  store->state       = initial_state;

  return store;
}

void
create_action (store_t* store, char* action, handler_t* handler)
{
  ht_insert(((__store_t*)store)->handlers, action, handler);
}

void
subscribe (store_t* store, void* (*subscriber)(void*))
{
  array_push(((__store_t*)store)->subscribers, subscriber);
}

void
unsubscribe (store_t* store, subscriber_t* subscriber)
{
  __store_t* store_internal = (__store_t*)(store);

  int idx = array_find(store_internal->subscribers, int_comparator, subscriber);
  if (idx > -1) {
    array_remove(store_internal->subscribers, idx);
  }
}

void*
dispatch (store_t* store, action_t action)
{
  __store_t* store_internal = (__store_t*)(store);

  ht_record* entry          = ht_search(store_internal->handlers, action);
  if (entry) {
    handler_t* handler = entry->value;

    void* result       = handler(store_internal->state);

    foreach (store_internal->subscribers, i) {
      ((subscriber_t*)array_get(store_internal->subscribers, i))(
        result,
        action
      );
    }

    return result;
  }

  return store_internal->state;
}

void*
get_state (store_t* store)
{
  return ((__store_t*)store)->state;
}

#include "libfluxus.h"
#include "tests.h"

typedef struct {
  int   num;
  char* str;
  int   subscribe_count;
  int   subscribe_count2;
} state;

const char* INCREMENT = "INCREMENT";
const char* DECREMENT = "DECREMENT";
const char* PRINT     = "PRINT";

void*
increment_num (state* state)
{
  state->num++;

  return state;
}

void*
decrement_num (state* state)
{
  state->num--;

  return state;
}

void*
print_str (state* state)
{
  printf("%s\n", state->str);

  return state;
}

void*
on_event (state* state, action_t action)
{
  state->subscribe_count++;
}

void*
on_event2 (state* state, action_t action)
{
  state->subscribe_count2++;
}

store_t*
create_test_store ()
{
  state* s            = malloc(sizeof(state));
  s->num              = 1;
  s->str              = "hello";
  s->subscribe_count  = 0;
  s->subscribe_count2 = 0;

  store_t* store      = create_store(s);
  create_action(store, INCREMENT, increment_num);
  create_action(store, DECREMENT, decrement_num);
  create_action(store, PRINT, print_str);

  return store;
}

void
create_store_test (void)
{
  store_t* store = create_test_store();

  ok(((state*)get_state(store))->num == 1);
  is(((state*)get_state(store))->str, "hello");
  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);
}

void
dispatch_test (void)
{
  store_t* store = create_test_store();

  ok(((state*)get_state(store))->num == 1);
  is(((state*)get_state(store))->str, "hello");
  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, INCREMENT);

  ok(((state*)get_state(store))->num == 2);
  is(((state*)get_state(store))->str, "hello");
  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, INCREMENT);

  ok(((state*)dispatch(store, INCREMENT))->num == 4);
  is(((state*)get_state(store))->str, "hello");
  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, DECREMENT);
  dispatch(store, DECREMENT);
  dispatch(store, DECREMENT);

  ok(((state*)get_state(store))->num == 1);
  is(((state*)get_state(store))->str, "hello");
  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);
}

void
subscribe_test ()
{
  store_t* store = create_test_store();

  subscribe(store, on_event);

  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, INCREMENT);
  ok(((state*)get_state(store))->subscribe_count == 1);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, DECREMENT);
  ok(((state*)get_state(store))->subscribe_count == 2);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, PRINT);
  ok(((state*)get_state(store))->subscribe_count == 3);
  ok(((state*)get_state(store))->subscribe_count2 == 0);
}

void
unsubscribe_test ()
{
  store_t* store = create_test_store();

  subscribe(store, on_event);
  subscribe(store, on_event2);

  ok(((state*)get_state(store))->subscribe_count == 0);
  ok(((state*)get_state(store))->subscribe_count2 == 0);

  dispatch(store, INCREMENT);
  ok(((state*)get_state(store))->subscribe_count == 1);
  ok(((state*)get_state(store))->subscribe_count2 == 1);

  unsubscribe(store, on_event);

  dispatch(store, INCREMENT);
  ok(((state*)get_state(store))->subscribe_count == 1);
  ok(((state*)get_state(store))->subscribe_count2 == 2);
}

void
run_store_tests (void)
{
  create_store_test();
  dispatch_test();
  subscribe_test();
  unsubscribe_test();
}

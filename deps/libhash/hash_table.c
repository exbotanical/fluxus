#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "libhash.h"
#include "prime.h"
#include "strdup/strdup.h"

static ht_record HT_RECORD_SENTINEL = {NULL, NULL};

const int HT_DEFAULT_CAPACITY = 50;

static void __ht_insert(hash_table *ht, const char *key, void *value,
                        bool free_value);
static int __ht_delete(hash_table *ht, const char *key, bool free_value);
static void __ht_delete_table(hash_table *ht, bool free_value);

/**
 * Resize the hash table. This implementation has a set capacity;
 * hash collisions rise beyond the capacity and `ht_insert` will fail.
 * To mitigate this, we resize up if the load (measured as the ratio of
 * records count to capacity) is less than .1, or down if the load exceeds
 * .7. To resize, we create a new table approx. 1/2x or 2x times the current
 * table size, then insert into it all non-deleted records.
 *
 * @param ht
 * @param base_capacity
 * @return int
 */
static void ht_resize(hash_table *ht, const int base_capacity,
                      bool free_value) {
  if (base_capacity < 0) {
    return;
  }

  hash_table *new_ht = ht_init(base_capacity);

  for (int i = 0; i < ht->capacity; i++) {
    ht_record *r = ht->records[i];

    if (r != NULL && r != &HT_RECORD_SENTINEL) {
      __ht_insert(new_ht, r->key, r->value, free_value);
    }
  }

  ht->base_capacity = new_ht->base_capacity;
  ht->count = new_ht->count;

  const int tmp_capacity = ht->capacity;

  ht->capacity = new_ht->capacity;
  new_ht->capacity = tmp_capacity;

  ht_record **tmp_records = ht->records;
  ht->records = new_ht->records;
  new_ht->records = tmp_records;

  ht_delete_table(new_ht);
}

/**
 * Resize the table to a larger size, the first prime subsequent
 * to approx. 2x the base capacity.
 *
 * @param ht
 */
static void ht_resize_up(hash_table *ht, bool free_value) {
  const int new_capacity = ht->base_capacity * 2;

  ht_resize(ht, new_capacity, free_value);
}

/**
 * Resize the table to a smaller size, the first prime subsequent
 * to approx. 1/2x the base capacity.
 *
 * @param ht
 */
static void ht_resize_down(hash_table *ht, bool free_value) {
  const int new_capacity = ht->base_capacity / 2;

  ht_resize(ht, new_capacity, free_value);
}

/**
 * Initialize a new hash table record with the given k, v pair
 *
 * @param k Record key
 * @param v Record value
 * @return ht_record*
 */
static ht_record *ht_record_init(const char *k, void *v) {
  ht_record *r = malloc(sizeof(ht_record));
  r->key = strdup(k);
  r->value = v;

  return r;
}

/**
 * Delete a record and deallocate its memory
 *
 * @param r Record to delete
 */
static void ht_delete_record(ht_record *r, bool free_value) {
  free(r->key);
  if (free_value) {
    free(r->value);
    r->value = NULL;
  }
  free(r);
}

static void __ht_insert(hash_table *ht, const char *key, void *value,
                        bool free_value) {
  if (ht == NULL) {
    return;
  }

  const int load = ht->count * 100 / ht->capacity;
  if (load > 70) {
    ht_resize_up(ht, free_value);
  }

  ht_record *new_record = ht_record_init(key, value);

  int idx = h_resolve_hash(new_record->key, ht->capacity, 0);

  ht_record *current_record = ht->records[idx];
  int i = 1;

  // i.e. if there was a collision
  while (current_record != NULL && current_record != &HT_RECORD_SENTINEL) {
    // update existing key/value
    if (strcmp(current_record->key, key) == 0) {
      ht_delete_record(current_record, free_value);
      ht->records[idx] = new_record;

      return;
    }

    // TODO verify i is 1..
    idx = h_resolve_hash(new_record->key, ht->capacity, i);
    current_record = ht->records[idx];
    i++;
  }

  ht->records[idx] = new_record;
  ht->count++;
}

static int __ht_delete(hash_table *ht, const char *key, bool free_value) {
  const int load = ht->count * 100 / ht->capacity;

  if (load < 10) {
    ht_resize_down(ht, free_value);
  }

  int i = 0;
  int idx = h_resolve_hash(key, ht->capacity, i);

  ht_record *current_record = ht->records[idx];

  while (current_record != NULL && current_record != &HT_RECORD_SENTINEL) {
    if (strcmp(current_record->key, key) == 0) {
      ht_delete_record(current_record, free_value);
      ht->records[idx] = &HT_RECORD_SENTINEL;

      ht->count--;

      return 1;
    }

    idx = h_resolve_hash(key, ht->capacity, ++i);
    current_record = ht->records[idx];
  }

  return 0;
}

static void __ht_delete_table(hash_table *ht, bool free_value) {
  for (int i = 0; i < ht->capacity; i++) {
    ht_record *r = ht->records[i];

    if (r != NULL && r != &HT_RECORD_SENTINEL) {
      ht_delete_record(r, free_value);
    }
  }

  free(ht->records);
  free(ht);
}

hash_table *ht_init(int base_capacity) {
  if (!base_capacity) {
    base_capacity = HT_DEFAULT_CAPACITY;
  }

  hash_table *ht = malloc(sizeof(hash_table));
  ht->base_capacity = base_capacity;

  ht->capacity = next_prime(ht->base_capacity);
  ht->count = 0;
  ht->records = calloc((size_t)ht->capacity, sizeof(ht_record *));

  return ht;
}

void ht_insert(hash_table *ht, const char *key, void *value) {
  __ht_insert(ht, key, value, false);
}

void ht_insert_ptr(hash_table *ht, const char *key, void *value) {
  __ht_insert(ht, key, value, true);
}

ht_record *ht_search(hash_table *ht, const char *key) {
  int idx = h_resolve_hash(key, ht->capacity, 0);
  ht_record *current_record = ht->records[idx];
  int i = 1;

  while (current_record != NULL && current_record != &HT_RECORD_SENTINEL) {
    if (strcmp(current_record->key, key) == 0) {
      return current_record;
    }

    idx = h_resolve_hash(key, ht->capacity, i);
    current_record = ht->records[idx];
    i++;
  }

  return NULL;
}

char *ht_get(hash_table *ht, const char *key) {
  ht_record *r = ht_search(ht, key);

  return r ? r->value : NULL;
}

void ht_delete_table(hash_table *ht) { __ht_delete_table(ht, false); }

void ht_delete_table_ptr(hash_table *ht) { __ht_delete_table(ht, true); }

int ht_delete(hash_table *ht, const char *key) {
  return __ht_delete(ht, key, false);
}

int ht_delete_ptr(hash_table *ht, const char *key) {
  return __ht_delete(ht, key, true);
}

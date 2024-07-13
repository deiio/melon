/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "table.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
 * Generic hash function (a popular one from Bernstein)
 */
static unsigned str_hash(const char *s) {
  unsigned int h = 0;
  while (*s) {
    h = ((h << 5) + h) + (*s++); // hash * 33 + c
  }
  return h;
}

/*
 * Used for safe string hash table.
 */
typedef struct X1 {
  int size;           /* The number of availble slots. Must be
                       * power of 2 greater that or equal to 1 */
  int count;          /* Number of currently slots filled */
  struct X1Node *tbl; /* The data stored here */
  struct X1Node **ht; /* Hash table for loopup */
} X1;

/*
 * Safe string hash node.
 */
typedef struct X1Node {
  char *data;           /* The data */
  struct X1Node *next;  /* Next entry with the same hash */
  struct X1Node **from; /* Previous link */
} X1Node;

static X1 *x1a = NULL;
static const int kStrTableSize = 1024;

/*
 * Works like strdup, sort of. Save a string in malloced memory, but
 * keep strings in a table so that the same string is not in more than
 * one place.
 */
char *MlnStrSafe(const char *s) {
  char *z = MlnStrSafeFind(s);
  if (z == NULL && (z = malloc(strlen(s) + 1)) != NULL) {
    strcpy(z, s);
    MlnStrSafeInsert(z);
  }
  MlnMemoryCheck(z);
  return z;
}

/*
 * Allocate a new associative array
 */
void MlnStrSafeInit() {
  if (x1a != NULL) {
    return;
  }

  x1a = (X1 *)malloc(sizeof(X1));
  if (x1a == NULL) {
    return;
  }

  x1a->size = kStrTableSize;
  x1a->count = 0;
  x1a->tbl = malloc((sizeof(X1Node) + sizeof(X1Node *)) * kStrTableSize);
  if (x1a->tbl == NULL) {
    free(x1a);
    x1a = NULL;
  } else {
    int i = 0;
    x1a->ht = (X1Node **)&(x1a->tbl[kStrTableSize]);
    for (i = 0; i < kStrTableSize; i++) {
      x1a->ht[i] = NULL;
    }
  }
}

/*
 * Insert a new record into the array. Return MLN_TRUE if successful.
 * Prior data with the same key is NOT overwritten.
 */
int MlnStrSafeInsert(char *data) {
  X1Node *node;
  unsigned h;
  unsigned index;

  if (x1a == NULL) {
    return MLN_FALSE;
  }

  h = str_hash(data);
  index = h & (x1a->size - 1);
  node = x1a->ht[index];
  while (node) {
    if (strcmp(node->data, data) == 0) {
      /* An existing entry with the same key is found.
       * Fail because overwrite is not allowed.
       */
      return MLN_FALSE;
    }
    node = node->next;
  }

  if (x1a->count >= x1a->size) {
    /* Need to make the hash table bigger */
    int i, size;
    X1 array;
    array.size = size = x1a->size * 2;
    array.count = x1a->count;
    array.tbl = malloc((sizeof(X1Node) + sizeof(X1Node *)) * size);
    if (array.tbl == NULL) {
      return MLN_FALSE; /* Fail due to malloc failure */
    }
    array.ht = (X1Node **)&(array.tbl[size]);
    for (i = 0; i < size; i++) {
      array.ht[i] = NULL;
    }
    for (i = 0; i < x1a->count; i++) {
      X1Node *old, *new;
      old = &(x1a->tbl[i]);
      index = str_hash(old->data) & (size - 1);
      new = &(array.tbl[i]);
      if (array.ht[index]) {
        array.ht[index]->from = &(new->next);
      }
      new->next = array.ht[index];
      new->data = old->data;
      new->from = &(array.ht[index]);
      array.ht[index] = new;
    }
    free(x1a->tbl);
    *x1a = array;
  }

  /* Insert the new data */
  index = h & (x1a->size - 1);
  node = &(x1a->tbl[x1a->count++]);
  node->data = data;
  if (x1a->ht[index]) {
    x1a->ht[index]->from = &(node->next);
  }
  node->next = x1a->ht[index];
  x1a->ht[index] = node;
  node->from = &(x1a->ht[index]);

  return MLN_TRUE;
}

/*
 * Return a pointer to data assigned to the given key. Return NULL
 * if no such key.
 */
char *MlnStrSafeFind(const char *key) {
  unsigned h;
  X1Node *node;

  if (x1a == NULL) {
    return NULL;
  }

  h = str_hash(key) & (x1a->size - 1);
  node = x1a->ht[h];
  while (node) {
    if (strcmp(node->data, key) == 0) {
      break;
    }
    node = node->next;
  }

  return node ? node->data : NULL;
}

/*
 * Symbols
 */

/*
 * Used for MlnSymbol hash table.
 */
typedef struct X2 {
  int size;           /* The nubmer of available slots. Must be a power
                       * of 2 greater than or equal to 1 */
  int count;          /* Number of currently slots filled */
  struct X2Node *tbl; /* The data stored here */
  struct X2Node **ht; /* Hash table for lookups */
} X2;

/*
 * MlnSymbol hash node.
 */
typedef struct X2Node {
  MlnSymbol *data;      /* The data */
  char *key;            /* The key */
  struct X2Node *next;  /* Next entry with the same hash */
  struct X2Node **from; /* Previous link */
} X2Node;

static X2 *x2a;
static const int kSymTableSize = 128;

/*
 * Return a pointer to the (terminal or non-terminal) symbol "x".
 * Create a new symbol if this is the first time "x" has been seen.
 */
MlnSymbol *MlnSymbolNew(const char *x) {
  MlnSymbol *sym;

  sym = MlnSymbolFind(x);
  if (sym != NULL) {
    return sym;
  }

  sym = malloc(sizeof(MlnSymbol));
  MlnMemoryCheck(sym);
  sym->name = MlnStrSafe(x);
  sym->index = 0;
  sym->type = isupper(*x) ? MLN_SYM_TERMINAL : MLN_SYM_NON_TERMINAL;
  sym->rule = NULL;
  sym->fallback = NULL;
  sym->prec = -1;
  sym->assoc = MLN_ASSOC_UNK;
  sym->first_set = NULL;
  sym->lambda = MLN_FALSE;
  sym->destructor = NULL;
  sym->destructor_line = 0;
  sym->data_type = NULL;
  sym->data_type_num = 0;
  MlnSymbolInsert(sym, sym->name);

  return sym;
}

/* Compare two symbols for working purposes.
 *
 * Symbols that begin with uppper case letters (terminals or tokens)
 * must sort before symbols that begin with lower case letters
 * (non-terminals). Other than that, the order does not matter.
 *
 * We find experimentally that leaving the symbols is their original
 * order (the order they appeared in the grammer file) gives the smallest
 * parser tables in CutDB.
 */
int MlnSymbolCmp(MlnSymbol **a, MlnSymbol **b) {
  int i1 = (**a).index + 10000000 * ((**a).name[0] > 'Z');
  int i2 = (**b).index + 10000000 * ((**b).name[0] > 'Z');
  return i1 - i2;
}

/*
 * Allocate a new associative array
 */
void MlnSymbolInit() {
  if (x2a != NULL) {
    return;
  }

  x2a = malloc(sizeof(X2));
  if (x2a == NULL) {
    return;
  }

  x2a->size = kSymTableSize;
  x2a->count = 0;
  x2a->tbl = malloc((sizeof(X2Node) + sizeof(X2Node *)) * kSymTableSize);
  if (x2a->tbl == NULL) {
    free(x2a);
    x2a = NULL;
  } else {
    int i;
    x2a->ht = (X2Node **)&(x2a->tbl[kSymTableSize]);
    for (i = 0; i < kSymTableSize; i++) {
      x2a->ht[i] = NULL;
    }
  }
}

/* Insert a new record into the array. Return MLN_TRUE if successful.
 * Prior data with the same key is NOT overwritten.
 */
int MlnSymbolInsert(MlnSymbol *data, char *key) {
  X2Node *node;
  unsigned h;
  unsigned index;

  if (x2a == NULL) {
    return MLN_FALSE;
  }
  h = str_hash(key);
  index = h & (x2a->size - 1);
  node = x2a->ht[index];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      /* An existing entry with the same key is found.
       * Fail because overwrite is not allowed.
       */
      return MLN_FALSE;
    }
    node = node->next;
  }
  if (x2a->count >= x2a->size) {
    /* Need to make the hash table bigger */
    int i, size;
    X2 array;
    array.size = size = x2a->size * 2;
    array.count = x2a->count;
    array.tbl = malloc((sizeof(X2Node) + sizeof(X2Node *)) * size);
    if (array.tbl == NULL) {
      return MLN_FALSE; /* Fail due to malloc failure */
    }
    array.ht = (X2Node **)&(array.tbl[size]);
    for (i = 0; i < size; i++) {
      array.ht[i] = NULL;
    }
    for (i = 0; i < x2a->count; i++) {
      X2Node *old, *new;
      old = &(x2a->tbl[i]);
      index = str_hash(old->key) & (size - 1);
      new = &(array.tbl[i]);
      if (array.ht[index]) {
        array.ht[index]->from = &(new->next);
      }
      new->next = array.ht[index];
      new->data = old->data;
      new->key = old->key;
      new->from = &(array.ht[index]);
      array.ht[index] = new;
    }
    free(x2a);
    *x2a = array;
  }

  /* Insert the new data */
  index = h & (x2a->size - 1);
  node = &(x2a->tbl[x2a->count++]);
  node->key = key;
  node->data = data;
  if (x2a->ht[index]) {
    x2a->ht[index]->from = &(node->next);
  }
  node->next = x2a->ht[index];
  x2a->ht[index] = node;
  node->from = &(x2a->ht[index]);

  return MLN_TRUE;
}

MlnSymbol *MlnSymbolFind(const char *key) {
  unsigned h;
  X2Node *node;

  if (x2a == NULL) {
    return NULL;
  }
  h = str_hash(key) & (x2a->size - 1);
  node = x2a->ht[h];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      break;
    }
    node = node->next;
  }
  return node ? node->data : NULL;
}

/*
 * Return the size of the array.
 */
int MlnSymbolCount() { return x2a ? x2a->count : 0; }

/*
 * Return an array of pointers to all data in the table.
 * The array is obtained from malloc. Return NULL if memory allocation
 * problems, or if the array is empty.
 */
MlnSymbol **MlnSymbolArrayOf() {
  MlnSymbol **array;
  int i, size;
  if (x2a == NULL) {
    return NULL;
  }
  size = x2a->count;
  array = malloc(sizeof(MlnSymbol *) * size);
  if (array) {
    for (i = 0; i < size; i++) {
      array[i] = x2a->tbl[i].data;
    }
  }
  return array;
}

/*
 * State
 */

/*
 * Used for MlnState hash table.
 */
typedef struct X3 {
  int size;           /* The nubmer of available slots. Must be a power
                       * of 2 greater than or equal to 1 */
  int count;          /* Number of currently slots filled */
  struct X3Node *tbl; /* The data stored here */
  struct X3Node **ht; /* Hash table for lookups */
} X3;

/*
 * MlnState hash node.
 */
typedef struct X3Node {
  MlnState *data;       /* The data */
  MlnConfig *key;       /* The key */
  struct X3Node *next;  /* Next entry with the same hash */
  struct X3Node **from; /* Previous link */
} X3Node;

static X3 *x3a;
static const int kStateTableSize = 128;

/* Hash a state */
static unsigned state_hash(MlnConfig *c) {
  unsigned h = 0;
  while (c) {
    h = h * 571 + c->rule->index * 37 + c->dot;
    c = c->bp;
  }
  return h;
}

/* Compare two states. */
int MlnStateCmp(MlnConfig *a, MlnConfig *b) {
  int rc;
  for (rc = 0; rc == 0 && a && b; a = a->bp, b = b->bp) {
    rc = a->rule->index - b->rule->index;
    if (rc == 0) {
      rc = a->dot - b->dot;
    }
  }
  if (rc == 0) {
    if (a) {
      rc = 1;
    }
    if (b) {
      rc = -1;
    }
  }
  return rc;
}

/* Allocacte a new state structure. */
MlnState *MlnStateNew() {
  MlnState *new = malloc(sizeof(MlnState));
  MlnMemoryCheck(new);
  return new;
}

/* Allocate a new associative array. */
void MlnStateInit() {
  if (x3a != NULL) {
    return;
  }

  x3a = malloc(sizeof(X3));
  if (x3a == NULL) {
    return;
  }

  x3a->size = kStateTableSize;
  x3a->count = 0;
  x3a->tbl = malloc((sizeof(X3Node) + sizeof(X3Node *)) * kStateTableSize);
  if (x3a->tbl == NULL) {
    free(x3a);
    x3a = NULL;
  } else {
    int i = 0;
    x3a->ht = (X3Node **)&(x3a->tbl[kStateTableSize]);
    for (i = 0; i < kStateTableSize; i++) {
      x3a->ht[i] = NULL;
    }
  }
}

/*
 * Insert a new record into the array. Return MLN_TRUE if successful.
 * Prior data with the same key is NOT overwritten.
 */
int MlnStateInsert(MlnState *state, MlnConfig *config) {
  X3Node *node;
  unsigned h;
  unsigned index;

  if (x3a == NULL) {
    return MLN_FALSE;
  }

  h = state_hash(config);
  index = h & (x3a->size - 1);
  node = x3a->ht[index];
  while (node) {
    if (MlnStateCmp(node->key, config) == 0) {
      /* An existing entry with the same key is found.
       * Fail because overwrite is not allows. */
      return MLN_FALSE;
    }
    node = node->next;
  }

  if (x3a->count >= x3a->size) {
    /* Need to make the hash table bigger */
    int i, size;
    X3 array;
    array.size = size = x3a->size * 2;
    array.count = x3a->count;
    array.tbl = malloc((sizeof(X3Node) + sizeof(X3Node *)) * size);
    if (array.tbl == NULL) {
      return MLN_FALSE; /* Fail due to malloc failure */
    }
    array.ht = (X3Node **)&(array.tbl[size]);
    for (i = 0; i < size; i++) {
      array.ht[i] = NULL;
    }
    for (i = 0; i < x3a->count; i++) {
      X3Node *old, *new;
      old = &(x3a->tbl[i]);
      index = state_hash(old->key) & (size - 1);
      new = &(array.tbl[i]);
      if (array.ht[index]) {
        array.ht[index]->from = &(new->next);
      }
      new->next = array.ht[index];
      new->key = old->key;
      new->data = old->data;
      new->from = &(array.ht[index]);
      array.ht[index] = new;
    }
    free(x3a->tbl);
    *x3a = array;
  }

  /* Insert the new data */
  index = h & (x3a->size - 1);
  node = &(x3a->tbl[x3a->count++]);
  node->key = config;
  node->data = state;
  if (x3a->ht[index]) {
    x3a->ht[index]->from = &(node->next);
  }
  node->next = x3a->ht[index];
  x3a->ht[index] = node;
  node->from = &(x3a->ht[index]);

  return MLN_TRUE;
}

/*
 * Return a pointer to data assigned to the given key. Return NULL
 * if no such key.
 */
MlnState *MlnStateFind(MlnConfig *config) {
  unsigned h;
  X3Node *node;

  if (x3a == NULL) {
    return NULL;
  }

  h = state_hash(config) & (x3a->size - 1);
  node = x3a->ht[h];
  while (node) {
    if (MlnStateCmp(node->key, config) == 0) {
      break;
    }
    node = node->next;
  }

  return node ? node->data : NULL;
}

/*
 * Return an array of pointers to all data in the table.
 * The array is obtained from malloc. Return NULL if memory allocation
 * problems, or if the array is empty.
 */
MlnState **MlnStateArrayOf() {
  MlnState **array;
  int i, size;

  if (x3a == NULL) {
    return NULL;
  }
  size = x3a->count;
  array = malloc(sizeof(MlnState *) * size);
  if (array) {
    for (i = 0; i < size; i++) {
      array[i] = x3a->tbl[i].data;
    }
  }
  return array;
}

/*
 * Used for MlnConfig hash table.
 */
typedef struct X4 {
  int size;           /* The number of available slots. Must be a
                         power of 2 greater than or equal to 1 */
  int count;          /* Number of currently slots filled */
  struct X4Node *tbl; /* The data stored here */
  struct X4Node **ht; /* Hash table for lookups */
} X4;

/*
 * MlnConfig hash node.
 */
typedef struct X4Node {
  MlnConfig *data;      /* The data */
  struct X4Node *next;  /* Next entry with the same hash */
  struct X4Node **from; /* Previous link */
} X4Node;

static X4 *x4a = NULL;
static const int kConfigTableSize = 64;

/* Hash a configuration */
static unsigned config_hash(MlnConfig *c) {
  return c->rule->index * 37 + c->dot;
}

/* Compare two configurations */
int MlnConfigCmp(MlnConfig *a, MlnConfig *b) {
  int x = a->rule->index - b->rule->index;
  if (x == 0) {
    x = a->dot - b->dot;
  }
  return x;
}

/* Allocate a new associative array */
void MlnConfigTableInit() {
  if (x4a != NULL) {
    return;
  }

  x4a = malloc(sizeof(X4));
  if (x4a == NULL) {
    return;
  }

  x4a->size = kConfigTableSize;
  x4a->count = 0;
  x4a->tbl = malloc((sizeof(X4Node) + sizeof(X4Node *)) * kConfigTableSize);
  if (x4a->tbl == NULL) {
    free(x4a);
    x4a = NULL;
  } else {
    int i;
    x4a->ht = (X4Node **)&(x4a->tbl[kConfigTableSize]);
    for (i = 0; i < kConfigTableSize; i++) {
      x4a->ht[i] = NULL;
    }
  }
}

/*
 * Insert a new record into the array. Return MLN_TRUE if successful.
 * Prior data with the same key is NOT overwritten.
 */
int MlnConfigTableInsert(MlnConfig *config) {
  X4Node *node;
  unsigned h;
  unsigned index;

  if (x4a == NULL) {
    return MLN_FALSE;
  }

  h = config_hash(config);
  index = h & (x4a->size - 1);
  node = x4a->ht[index];
  while (node) {
    if (MlnConfigCmp(node->data, config) == 0) {
      /* An existing entry with the same key is found.
       * Fail because overwrite is not allowed. */
      return MLN_FALSE;
    }
    node = node->next;
  }

  if (x4a->count >= x4a->size) {
    /* Need to make the hash table bigger */
    int i, size;
    X4 array;
    array.size = size = x4a->size * 2;
    array.count = x4a->count;
    array.tbl = malloc((sizeof(X4Node) + sizeof(X4Node *)) * size);
    if (array.tbl == NULL) {
      return MLN_FALSE; /* Fail due to malloc failure */
    }
    array.ht = (X4Node **)&(array.tbl[size]);
    for (i = 0; i < size; i++) {
      array.ht[i] = NULL;
    }
    for (i = 0; i < x4a->count; i++) {
      X4Node *old, *new;
      old = &(x4a->tbl[i]);
      index = config_hash(old->data) & (size - 1);
      new = &(array.tbl[i]);
      if (array.ht[index]) {
        array.ht[index]->from = &(new->next);
      }
      new->next = array.ht[index];
      new->data = old->data;
      new->from = &(array.ht[index]);
      array.ht[index] = new;
    }
    free(x4a->tbl);
    *x4a = array;
  }

  /* Insert the new data */
  index = h & (x4a->size - 1);
  node = &(x4a->tbl[x4a->count++]);
  node->data = config;
  if (x4a->ht[index]) {
    x4a->ht[index]->from = &(node->next);
  }
  node->next = x4a->ht[index];
  x4a->ht[index] = node;
  node->from = &(x4a->ht[index]);

  return MLN_TRUE;
}

/*
 * Return a pointer to data assigned to the given key. Return NULL
 * if no such key.
 */
MlnConfig *MlnConfigTableFind(MlnConfig *config) {
  unsigned h;
  X4Node *node;

  if (x4a == NULL) {
    return NULL;
  }

  h = config_hash(config) & (x4a->size - 1);
  node = x4a->ht[h];
  while (node) {
    if (MlnConfigCmp(node->data, config) == 0) {
      break;
    }
    node = node->next;
  }

  return node ? node->data : NULL;
}

/*
 * Remove all data from the table. Pass each data to the function "clear"
 * as it is removed. ("clear" may be null to avoid this step.)
 */
void MlnConfigTableClear(int (*clear)(MlnConfig *)) {
  int i;
  if (x4a == NULL || x4a->count == 0) {
    return;
  }
  if (clear != NULL) {
    for (i = 0; i < x4a->count; i++) {
      (*clear)(x4a->tbl[i].data);
    }
  }
  for (i = 0; i < x4a->size; i++) {
    x4a->ht[i] = NULL;
  }
  x4a->count = 0;
}

/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "action.h"

#include "assert.h"
#include "msort.h"

#include <stdlib.h>

const int kDefaultActionSize = 100;

MlnAction *MlnActionNew() {
  static MlnAction *free_list = NULL;
  MlnAction *new;

  if (free_list == NULL) {
    int i;
    free_list = calloc(kDefaultActionSize, sizeof(MlnAction));
    MlnMemoryCheck(free_list);
    for (i = 0; i < kDefaultActionSize - 1; i++) {
      free_list[i].next = &free_list[i + 1];
    }
  }
  new = free_list;
  free_list = free_list->next;
  return new;
}

static int MlnActionCmp(void *a, void *b) {
  MlnAction *ap1 = a, *ap2 = b;
  int rc = ap1->sym->index - ap2->sym->index;
  if (rc == 0) {
    rc = (int)ap1->type - (int)ap2->type;
  }
  if (rc == 0) {
    assert(ap1->type == MLN_REDUCE || ap1->type == MLN_RD_RESOLVED ||
           ap1->type == MLN_CONFLICT);
    assert(ap2->type == MLN_REDUCE || ap2->type == MLN_RD_RESOLVED ||
           ap2->type == MLN_CONFLICT);
    rc = ap1->x.rule->index - ap2->x.rule->index;
  }
  return rc;
}

/* Sort parser actions */
MlnAction *MlnActionSort(MlnAction *ap) {
  ap = MlnMSort(ap, &ap->next, MlnActionCmp);
  return ap;
}

void MlnActionAdd(MlnAction **app, MlnActionState type, MlnSymbol *sym,
                  void *arg) {
  MlnAction *new = MlnActionNew();
  new->next = *app;
  *app = new;
  new->type = type;
  new->sym = sym;
  if (type == MLN_SHIFT) {
    new->x.state = arg;
  } else {
    new->x.rule = arg;
  }
}

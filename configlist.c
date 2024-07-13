/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "configlist.h"

#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "error.h"
#include "msort.h"
#include "plink.h"
#include "set.h"
#include "struct.h"
#include "table.h"

static MlnConfig *free_list = NULL;    /* List of free configurations */
static MlnConfig *current = NULL;      /* Top of list of configurations */
static MlnConfig **current_end = NULL; /* Last on list of configs */
static MlnConfig *basis = NULL;        /* Top of list of basis configs */
static MlnConfig **basis_end = NULL;   /* End of list of basis configs */

static const int kDefaultConfigSize = 3;

/*
 * Return a pointer to a new configuration.
 */
static MlnConfig *NewConfig() {
  MlnConfig *cfg;
  if (free_list == NULL) {
    int i;
    free_list = calloc(kDefaultConfigSize, sizeof(MlnConfig));
    if (free_list == NULL) {
      fprintf(stderr, "Unable to allocate memory for a new configuration.");
      exit(1);
    }

    for (i = 0; i < kDefaultConfigSize - 1; i++) {
      free_list[i].next = &free_list[i + 1];
    }
  }
  cfg = free_list;
  free_list = free_list->next;
  return cfg;
}

/*
 * Recycle a configuration to free list.
 */
static void DeleteConfig(MlnConfig *c) {
  c->next = free_list;
  free_list = c;
}

/*
 * Initialized the configuration list builder.
 */
void MlnConfigListInit() {
  current = NULL;
  current_end = &current;
  basis = NULL;
  basis_end = &basis;
  MlnConfigTableInit();
}

/*
 * Add another configuration to the configuration list
 */
MlnConfig *MlnConfigListAdd(MlnRule *rule, int dot) {
  MlnConfig *cfp, model;

  assert(current_end != NULL);
  model.rule = rule;
  model.dot = dot;
  cfp = MlnConfigTableFind(&model);
  if (cfp == NULL) {
    cfp = NewConfig();
    cfp->rule = rule;
    cfp->dot = dot;
    cfp->fws = MlnSetNew();
    cfp->st = NULL;
    cfp->fpl = cfp->bpl = NULL;
    cfp->next = NULL;
    cfp->bp = NULL;

    *current_end = cfp;
    current_end = &cfp->next;
    MlnConfigTableInsert(cfp);
  }

  return cfp;
}

/*
 * Add a basis configuration to the configuration list.
 */
MlnConfig *MlnConfigListAddBasis(MlnRule *rule, int dot) {
  MlnConfig *cfp, model;

  assert(basis_end != NULL);
  assert(current_end != NULL);
  model.rule = rule;
  model.dot = dot;
  cfp = MlnConfigTableFind(&model);
  if (cfp == NULL) {
    cfp = NewConfig();
    cfp->rule = rule;
    cfp->dot = dot;
    cfp->fws = MlnSetNew();
    cfp->st = NULL;
    cfp->fpl = cfp->bpl = NULL;
    cfp->next = NULL;
    cfp->bp = NULL;

    *current_end = cfp;
    current_end = &cfp->next;

    *basis_end = cfp;
    basis_end = &cfp->bp;
    MlnConfigTableInsert(cfp);
  }

  return cfp;
}

/*
 * Compute the closure of the configuration list.
 */
void MlnConfigListClosure(Melon *melon) {
  MlnConfig *cfp, *newcfp;
  MlnRule *rp, *newrp;
  MlnSymbol *sp, *xsp;
  int i, dot;

  assert(current_end != NULL);
  for (cfp = current; cfp != NULL; cfp = cfp->next) {
    rp = cfp->rule;
    dot = cfp->dot;
    if (dot >= rp->nrhs) {
      continue;
    }
    sp = rp->rhs[dot];
    if (sp->type == MLN_SYM_NON_TERMINAL) {
      if (sp->rule == NULL && sp != melon->err_sym) {
        MlnErrorMsg(melon->filename, rp->line,
                    "Non-terninal \"%s\" has no rules.", sp->name);
        melon->error_cnt++;
      }
      for (newrp = sp->rule; newrp != NULL; newrp = newrp->next_lhs) {
        newcfp = MlnConfigListAdd(newrp, 0);
        for (i = dot + 1; i < rp->nrhs; i++) {
          xsp = rp->rhs[i];
          if (xsp->type == MLN_SYM_TERMINAL) {
            MlnSetAdd(newcfp->fws, xsp->index);
            break;
          } else {
            MlnSetUnion(newcfp->fws, xsp->first_set);
            if (xsp->lambda == MLN_FALSE) {
              break;
            }
          }
        }
        if (i == rp->nrhs) {
          MlnPLinkAdd(&cfp->fpl, newcfp);
        }
      }
    }
  }
}

static int config_cmp(void *a, void *b) { return MlnConfigCmp(a, b); }

/*
 * Sort the configuration list.
 */
void MlnConfigListSort() {
  current = MlnMSort(current, &current->next, &config_cmp);
  current_end = NULL;
}

/*
 * Sort the basis configuration list.
 */
void MlnConfigListSortBasis() {
  basis = MlnMSort(current, &current->bp, &config_cmp);
  basis_end = NULL;
}

MlnConfig *MlnConfigListReturn() {
  MlnConfig *old = current;
  current = NULL;
  current_end = NULL;
  return old;
}

/*
 * Return a pointer to the head of the configuration list and
 * reset the list.
 */
MlnConfig *MlnConfigListBasis() {
  MlnConfig *old = basis;
  basis = NULL;
  basis_end = NULL;
  return old;
}

/*
 * Free all elements of the given configuration list.
 */
void MlnConfigListEat(MlnConfig *config) {
  MlnConfig *next;
  for (; config; config = next) {
    next = config->next;
    assert(config->fpl == NULL);
    assert(config->bpl == NULL);
    if (config->fws) {
      MlnSetFree(config->fws);
    }
    DeleteConfig(config);
  }
}

/*
 * Initialized the configuration list builder.
 */
void MlnConfigListReset() {
  current = NULL;
  current_end = &current;
  basis = NULL;
  basis_end = &basis;
  MlnConfigTableClear(NULL);
}

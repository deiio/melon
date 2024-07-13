/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

/*
 * This module implements routines use to construct the yy_action[] table.
 */

#include "acttab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"

/*
 * Allocate a new MlnAtionTable structure.
 */
MlnActionTable *MlnActionTableAlloc() {
  MlnActionTable *at = malloc(sizeof(MlnActionTable));
  if (at == NULL) {
    fprintf(stderr, "Unable to allocate memory for a new MlnActionTable.\n");
    exit(1);
  }
  memset(at, 0, sizeof(*at));
  return at;
}

/*
 * Free all memory associated with the given MlnActionTable.
 */
void MlnActionTableFree(MlnActionTable *at) {
  free(at->actions);
  free(at->lookaheads);
  free(at);
}

/*
 * Add a new action to the current transcation set.
 */
void MlnActionTableAddAction(MlnActionTable *at, int lookahead, int action) {
  if (at->nlookahead >= at->nlookahead_alloc) {
    at->nlookahead_alloc += 25;
    at->lookaheads = realloc(at->lookaheads,
                             sizeof(at->lookaheads[0]) * at->nlookahead_alloc);
    if (at->lookaheads == NULL) {
      fprintf(stderr, "malloc failed\n");
      exit(1);
    }
  }
  if (at->nlookahead == 0) {
    at->max_lookahead = lookahead;
    at->min_lookahead = lookahead;
    at->min_action = action;
  } else {
    if (at->max_lookahead < lookahead) {
      at->max_lookahead = lookahead;
    }
    if (at->min_lookahead > lookahead) {
      at->min_lookahead = lookahead;
      at->min_action = action;
    }
  }

  at->lookaheads[at->nlookahead].lookahead = lookahead;
  at->lookaheads[at->nlookahead].action = action;
  at->nlookahead++;
}

/*
 * Add the transaction set built up with prior calls to
 * MlnActionTableAddAction into the current action table. Then reset the
 * transaction set back to an empty set in preparation for a new round
 * of MlnActionTableAddAction calls.
 *
 * Return the offset into the action table of the new transaction.
 */
int MlnActionTableInsert(MlnActionTable *at) {
  int i, j, n;
  assert(at->nlookahead > 0);

  /* Make sure we have enough space to hold the expanded action table
   * in the worst case. The wrost case occurs if the transaction set
   * must be appended to the current action table.
   */
  n = at->max_lookahead + 1;
  if (at->naction + n >= at->naction_alloc) {
    int old_alloc = at->naction_alloc;
    at->naction_alloc = at->naction + n + at->naction_alloc + 20;
    at->actions =
        realloc(at->actions, sizeof(at->actions[0]) * at->naction_alloc);
    if (at->actions == NULL) {
      fprintf(stderr, "malloc failed\n");
      exit(1);
    }
    for (i = old_alloc; i < at->naction_alloc; i++) {
      at->actions[i].lookahead = -1;
      at->actions[i].action = -1;
    }
  }

  /* Scan the existing action table looking for an offset where we can
   * insert the current transaction set. Fall out of the loop when that
   * offset is found. In the worst case, we fall out of the loop when
   * i reaches at->naction, which means we append the new transaction
   * set.
   *
   * i is the inde in at->actions[] where at->min_lookahead is inserted.
   */
  for (i = 0; i < at->naction + at->min_lookahead; i++) {
    if (at->actions[i].lookahead < 0) {
      for (j = 0; j < at->nlookahead; j++) {
        int k = at->lookaheads[j].lookahead - at->min_lookahead + i;
        if (k < 0) {
          break;
        }
        if (at->actions[k].lookahead >= 0) {
          break;
        }
      }
      if (j < at->nlookahead) {
        continue;
      }
      for (j = 0; j < at->naction; j++) {
        if (at->actions[j].lookahead == j + at->min_lookahead - i) {
          break;
        }
      }
      if (j == at->naction) {
        break; /* Fits in empty slots */
      }
    } else if (at->actions[i].lookahead == at->min_lookahead) {
      if (at->actions[i].action != at->min_action) {
        continue;
      }
      for (j = 0; j < at->nlookahead; j++) {
        int k = at->lookaheads[j].lookahead - at->min_lookahead + i;
        if (k < 0 || k >= at->naction) {
          break;
        }
        if (at->lookaheads[j].lookahead != at->actions[k].lookahead) {
          break;
        }
        if (at->lookaheads[j].action != at->actions[k].action) {
          break;
        }
      }
      if (j < at->nlookahead) {
        continue;
      }
      n = 0;
      for (j = 0; j < at->naction; j++) {
        if (at->actions[j].lookahead < 0) {
          continue;
        }
        if (at->actions[j].lookahead == j + at->min_lookahead - i) {
          n++;
        }
      }
      if (n == at->nlookahead) {
        break; /* Same as a prior transaction set */
      }
    }
  }

  /* Insert transaction set at index i. */
  for (j = 0; j < at->nlookahead; j++) {
    int k = at->lookaheads[j].lookahead - at->min_lookahead + i;
    at->actions[k] = at->lookaheads[j];
    if (k >= at->naction) {
      at->naction = k + 1;
    }
  }
  at->nlookahead = 0;

  /* Return the offset that is added to the lookahead in order to get
   * the index into yy_action of the action */
  return i - at->min_lookahead;
}

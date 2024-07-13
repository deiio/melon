/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_ACTTAB_H_
#define MELON_ACTTAB_H_

/*
 * The state of the yy_action table under construction is an instance
 * of the following structure.
 */
typedef struct MlnActionTable {
  int naction;          /* Number of used slots in actions */
  int naction_alloc;    /* Slots allocated for actions */
  int nlookahead;       /* Number of used slots in lookaheads */
  int nlookahead_alloc; /* Slots allocated for lookaheads */
  int min_lookahead;    /* Minimum lookaheads[].lookahead */
  int min_action;       /* Action associated with min_lookahead */
  int max_lookahead;    /* Maximum lookaheads[].lookahead */
  struct {
    int lookahead; /* Value of the lookahead token */
    int action;    /* Action to take on the given lookahead */
  } *actions,      /* The yy_action[] table under construction */
      *lookaheads; /* A single new transaction set */
} MlnActionTable;

MlnActionTable *MlnActionTableAlloc();
void MlnActionTableFree(MlnActionTable *at);
void MlnActionTableAddAction(MlnActionTable *at, int lookahead, int action);
int MlnActionTableInsert(MlnActionTable *at);

/* Return the number of entries in the yy_action table */
#define MlnActionTableSize(X) ((X)->naction)

/* The value for the N-th entry in yy_action */
#define MlnActionTableAction(X, N) ((X)->actions[N].action)

/* The value for the N-th entry in yy_lookahead */
#define MlnActionTableLookahead(X, N) ((X)->actions[N].lookahead)

#endif

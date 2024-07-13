/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "plink.h"

#include <stdio.h>
#include <stdlib.h>

static MlnPLink *plink_free_list = NULL;
static const int kDefaultPLinkSize = 100;

/*
 * Allocate a new plink.
 */
MlnPLink *MlnPLinkNew() {
  MlnPLink *new;
  if (plink_free_list == NULL) {
    int i;
    plink_free_list = calloc(kDefaultPLinkSize, sizeof(MlnPLink));
    if (plink_free_list == NULL) {
      fprintf(stderr, "Unable to allocate memory for a new follow-set "
                      "propagation link.\n");
      exit(1);
    }
    for (i = 0; i < kDefaultPLinkSize - 1; i++) {
      plink_free_list[i].next = &plink_free_list[i + 1];
    }
  }

  new = plink_free_list;
  plink_free_list = plink_free_list->next;
  return new;
}

/*
 * Add a plink to a plink list.
 */
void MlnPLinkAdd(MlnPLink **plpp, MlnConfig *config) {
  MlnPLink *new;
  new = MlnPLinkNew();
  new->next = *plpp;
  *plpp = new;
  new->config = config;
}

/*
 * Transfer every plink on the list "from" to the list "to".
 */
void MlnPLinkCopy(MlnPLink **to, MlnPLink *from) {
  MlnPLink *next;
  while (from != NULL) {
    next = from->next;
    from->next = *to;
    *to = from;
    from = next;
  }
}

/*
 * Delete every plink on the list.
 */
void MlnPLinkDelete(MlnPLink *plp) {
  MlnPLink *next;

  while (plp != NULL) {
    next = plp->next;
    plp->next = plink_free_list;
    plink_free_list = plp;
    plp = next;
  }
}

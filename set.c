/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "set.h"

#include <stdlib.h>

static int size = 0;

/*
 * Set the set size.
 */
void MlnSetSize(int n) { size = n + 1; }

/*
 * Allocate a new set.
 */
void *MlnSetNew() {
  void *s = calloc(1, size);
  if (s == NULL) {
    extern void memory_error();
    memory_error();
  }
  return s;
}

/*
 * Deallocate a set.
 */
void MlnSetFree(void *set) { free(set); }

/*
 * Add a new element to the set. Return MLN_TRUE if the element was added
 * and MLN_FALSE if it was already there.
 */
int MlnSetAdd(void *set, int n) {
  char *s = set;
  int v = s[n];
  s[n] = 1;
  return v == 0 ? MLN_TRUE : MLN_FALSE;
}

/*
 * Add every element of sb to sa.  Return MLN_TRUE if sa changes.
 */
int MlnSetUnion(void *sa, void *sb) {
  char *s1 = sa, *s2 = sb;
  int i;
  MlnBoolean ret = MLN_FALSE;
  for (i = 0; i < size; i++) {
    if (s2[i] == 0) {
      continue;
    }
    if (s1[i] == 0) {
      s1[i] = 1;
      ret = MLN_TRUE;
    }
  }
  return ret;
}

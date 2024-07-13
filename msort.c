/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 *
 * Merge Sort
 *
 * Usage:
 *  Let "ptr' be a pointer to some structure which is at the head of
 *  a null-terminated list. Then to sort the list call:
 *
 *      ptr = MlnMSort(ptr, &(ptr->next), cmp_func);
 *
 *  In the above, "cmp_func" is a pointer to a function which compares
 *  two instances of the structure and returns an integer, as in
 *  strcmp. The second argument is a pointer to the pointer to the
 *  second element of the linked list. This address is used to compute
 *  the offset to the "next" filed within the strutures. The offset to
 *  the "next" filed most be constant for all structures in the list.
 *
 *  The function returns a new pointer which is the head of the list
 *  after sorting.
 */

#include "msort.h"

#include <stdlib.h>

/*
 * Return a pointer to the next structure in the linked list.
 */
#define Next(addr, offset) (*(char **)((char *)addr + offset))

/*
 * Inputs:
 *  addr1:    A sorted, null-terminated linked list. (May be null)
 *  addr2:    A sorted, null-terminated linked list. (May be null)
 *  cmp_func: A pointer to the comparison function.
 *  offset:   Offset in the structure to the "next" field.
 *
 * Return value:
 *  A pointer to the head of a sorted list containing the elements
 *  of both addr1 and addr2.
 *
 * Side effects:
 *  The "next" pointers for elements in the lists addr1 and addr2 are
 *  changed.
 */
static void *merge(void *addr1, void *addr2, MlnCmpFunc *cmp_func, int offset) {
  void *ptr, *head;

  if (addr1 == NULL) {
    return addr2;
  }
  if (addr2 == NULL) {
    return addr1;
  }
  if (cmp_func(addr1, addr2) < 0) {
    ptr = addr1;
    addr1 = Next(ptr, offset);
  } else {
    ptr = addr2;
    addr2 = Next(ptr, offset);
  }

  head = ptr;
  while (addr1 != NULL && addr2 != NULL) {
    if (cmp_func(addr1, addr2) < 0) {
      Next(ptr, offset) = addr1;
      ptr = addr1;
      addr1 = Next(addr1, offset);
    } else {
      Next(ptr, offset) = addr2;
      ptr = addr2;
      addr2 = Next(addr2, offset);
    }
  }
  if (addr1) {
    Next(ptr, offset) = addr1;
  } else {
    Next(ptr, offset) = addr2;
  }
  return head;
}

static const int kListSize = 30;

/*
 * Input:
 *  list:     Pointer to a singly-linked list of structures.
 *  next:     Pointer to pointer to the second element of the list.
 *  cmp_func: A comparision function.
 *
 * Return value:
 *  A pointer to the head of a sorted list containing the elements
 *  originally in list.
 *
 * Side effects:
 *  The "next" pointers ofr elements in list are changed.
 */
void *MlnMSort(void *list, void *next, MlnCmpFunc *cmp_func) {
  unsigned long offset = (unsigned long)next - (unsigned long)list;
  int i;
  void *set[kListSize];
  void *ep;

  for (i = 0; i < kListSize; i++) {
    set[i] = NULL;
  }
  while (list) {
    ep = list;
    list = Next(list, offset);
    Next(ep, offset) = NULL;
    for (i = 0; i < kListSize - 1 && set[i] != NULL; i++) {
      ep = merge(ep, set[i], cmp_func, offset);
      set[i] = NULL;
    }
    set[i] = ep;
  }

  ep = NULL;
  for (i = 0; i < kListSize; i++) {
    if (set[i] != NULL) {
      ep = merge(ep, set[i], cmp_func, offset);
    }
  }
  return ep;
}

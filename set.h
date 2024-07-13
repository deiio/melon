/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_SET_H_
#define MELON_SET_H_

#include "struct.h"

void MlnSetSize(int n);     /* All sets will be of size n */
void *MlnSetNew();          /* A new set for element 0..N */
void MlnSetFree(void *set); /* Deallocate a set */

int MlnSetAdd(void *set, int n);     /* Add element to a set */
int MlnSetUnion(void *sa, void *sb); /* A <- A U B, thru element N */

#define MlnSetFind(X, Y) (((char *)X)[Y]) /* True if Y is in set X */

#endif

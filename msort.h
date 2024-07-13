/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_MSORT_H_
#define MELON_MSORT_H_

typedef int MlnCmpFunc(void *, void *);

void *MlnMSort(void *list, void *next, MlnCmpFunc *cmp_func);

#endif

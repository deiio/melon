/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_ACTION_H_
#define MELON_ACTION_H_

#include "struct.h"

MlnAction *MlnActionNew();
MlnAction *MlnActionSort(MlnAction *ap);
void MlnActionAdd(MlnAction **app, MlnActionState type, MlnSymbol *sym,
                  void *arg);

#endif

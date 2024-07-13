/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_CONFIG_LIST_H_
#define MELON_CONFIG_LIST_H_

#include "struct.h"

void MlnConfigListInit();
MlnConfig *MlnConfigListAdd(MlnRule *rule, int dot);
MlnConfig *MlnConfigListAddBasis(MlnRule *rule, int dot);
void MlnConfigListClosure(Melon *melon);
void MlnConfigListSort();
void MlnConfigListSortBasis();
MlnConfig *MlnConfigListReturn();
MlnConfig *MlnConfigListBasis();
void MlnConfigListEat(MlnConfig *config);
void MlnConfigListReset();

#endif

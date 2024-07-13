/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_BUILD_H_
#define MELON_BUILD_H_

#include "struct.h"

void MlnFindRulePrecedences(Melon *melon);
void MlnFindFirstSets(Melon *melon);
void MlnFindStates(Melon *melon);
void MlnFindLinks(Melon *melon);
void MlnFindFollowSets(Melon *melon);
void MlnFindActions(Melon *melon);

#endif

/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_PLINK_H_
#define MELON_PLINK_H_

#include "struct.h"

MlnPLink *MlnPLinkNew();
void MlnPLinkAdd(MlnPLink **plpp, MlnConfig *config);
void MlnPLinkCopy(MlnPLink **to, MlnPLink *from);
void MlnPLinkDelete(MlnPLink *plp);

#endif

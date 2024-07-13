/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_REPORT_H_
#define MELON_REPORT_H_

#include "struct.h"

void MlnReprint(Melon *melon);
void MlnReportOutput(Melon *melon);
void MlnReportTable(Melon *melon, int mhflag);
void MlnReportHeader(Melon *melon);
void MlnCompressTables(Melon *melon);

#endif

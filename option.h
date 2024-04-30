/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_OPTION_H_
#define MELON_OPTION_H_

#include <stdio.h>

typedef struct {
  enum {
    MLN_OPT_FLAG = 1,
    MLN_OPT_INT,
    MLN_OPT_DBL,
    MLN_OPT_STR,
    MLN_OPT_FFLAG,
    MLN_OPT_FINT,
    MLN_OPT_FDBL,
    MLN_OPT_FSTR,
  } type;
  char *label;
  char *arg;
  char *message;
} MlnOption;

int MlnOptInit(char **argv, MlnOption *options, FILE *err);
int MlnOptNArgs();
char *MlnOptArg(int n);
void MlnOptErr(int n);
void MlnOptPrint();

#endif

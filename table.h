/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_TABLE_H_
#define MELON_TABLE_H_

#include "struct.h"

/* Routines for handling a strings */

char *MlnStrSafe(const char *s);
void MlnStrSafeInit();
int MlnStrSafeInsert(char *data);
char *MlnStrSafeFind(const char *key);

/* Routines for handling symbols of grammar */

MlnSymbol *MlnSymbolNew(const char *x);
int MlnSymbolCmp(MlnSymbol **a, MlnSymbol **b);
void MlnSymbolInit();
int MlnSymbolInsert(MlnSymbol *data, char *key);
MlnSymbol *MlnSymbolFind(const char *key);
int MlnSymbolCount();
MlnSymbol **MlnSymbolArrayOf();

/* Routines for manage the state table */

MlnState *MlnStateNew();
void MlnStateInit();
int MlnStateInsert(MlnState *state, MlnConfig *config);
MlnState *MlnStateFind(MlnConfig *config);
MlnState **MlnStateArrayOf();

/* Routines used for efficiency in MlnConfigListAdd */

int MlnConfigCmp(MlnConfig *a, MlnConfig *b);
void MlnConfigTableInit();
int MlnConfigTableInsert(MlnConfig *config);
MlnConfig *MlnConfigTableFind(MlnConfig *config);
void MlnConfigTableClear(int (*clear)(MlnConfig *));

#endif

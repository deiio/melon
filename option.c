/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "option.h"

#include <stdlib.h>
#include <string.h>

#define MLN_IS_OPT(x)                                                          \
  ((x)[0] == '-' || (x)[0] == '+' || strchr((x), '=') != NULL)

static char **argv;
static MlnOption *opts;
static FILE *err;

/*
 * Print the command line with a carrot pointing to the k-th character
 * of the n-th field.
 */
static void MlnErrLine(int n, int k, FILE *err) {
  int i;
  int spcnt;
  if (argv[0]) {
    fprintf(err, "%s", argv[0]);
  }
  spcnt = strlen(argv[0]) + 1;
  for (i = 1; i < n && argv[i]; i++) {
    fprintf(err, " %s", argv[i]);
    spcnt += strlen(argv[i] + 1);
  }
  spcnt += k;
  for (; argv[i]; i++) {
    fprintf(err, " %s", argv[i]);
  }
  if (spcnt < 20) {
    fprintf(err, "\n%*s^-- here\n", spcnt, "");
  } else {
    fprintf(err, "\n%*shere --^\n", spcnt - 7, "");
  }
}

/*
 * Print the index of the n-th non-switch argument. Return -1
 * if n is out of range.
 */
static int MlnArgIndex(int n) {
  int i;
  int dash_dash = 0;
  if (argv != NULL && argv[0] != NULL) {
    for (i = 1; argv[i] != NULL; i++) {
      if (dash_dash || !MLN_IS_OPT(argv[i])) {
        if (n == 0) {
          return i;
        }
        n--;
      }
      if (strcmp(argv[i], "--") == 0) {
        dash_dash = 1;
      }
    }
  }
  return -1;
}

static char emsg[] = "Command line syntax error: ";

/*
 * Process a flag command line argument.
 */
static int MlnHandleFlags(int n, FILE *err) {
  int i;
  int v;
  int err_cnt = 0;

  for (i = 0; opts[i].label; i++) {
    if (strcmp(&argv[n][1], opts[i].label) == 0) {
      break;
    }
  }
  v = argv[n][0] == '-' ? 1 : 0;
  if (opts[i].label == NULL) {
    if (err) {
      fprintf(err, "%sundefined option.\n", emsg);
      MlnErrLine(n, 1, err);
    }
    err_cnt++;
  } else if (opts[i].type == MLN_OPT_FLAG) {
    *((int *)opts[i].arg) = v;
  } else if (opts[i].type == MLN_OPT_FFLAG) {
    (*(void (*)(int))(opts[i].arg))(v);
  } else {
    if (err) {
      fprintf(err, "%smissing argument on switch.\n", emsg);
      MlnErrLine(n, 1, err);
    }
    err_cnt++;
  }
  return err_cnt;
}

/*
 * Process a command line switch which has an argument.
 */
static int MlnHandleSwitch(int n, FILE *err) {
  int i;
  int err_cnt = 0;
  char *cp = strchr(argv[n], '=');

  *cp = '\0';
  for (i = 0; opts[i].label; i++) {
    if (strcmp(argv[n], opts[i].label) == 0) {
      break;
    }
  }
  *cp = '=';
  if (opts[i].label == NULL) {
    if (err) {
      fprintf(err, "%sundefined option.\n", emsg);
      MlnErrLine(n, 0, err);
    }
    err_cnt++;
  } else {
    int lv = 0;
    double dv = 0;
    char *sv = NULL, *end;

    cp++;
    switch (opts[i].type) {
    case MLN_OPT_FLAG:
    case MLN_OPT_FFLAG:
      if (err) {
        fprintf(err, "%soption requires an argument.\n", emsg);
        MlnErrLine(n, 0, err);
      }
      err_cnt++;
      break;
    case MLN_OPT_DBL:
    case MLN_OPT_FDBL:
      dv = strtod(cp, &end);
      if (*end) {
        if (err) {
          fprintf(err, "%sillegal character in floating-point argument.\n",
                  emsg);
          MlnErrLine(n, (int)(end - argv[n]), err);
        }
        err_cnt++;
      }
      break;
    case MLN_OPT_INT:
    case MLN_OPT_FINT:
      lv = strtol(cp, &end, 0);
      if (*end) {
        if (err) {
          fprintf(err, "%sillegal character in integer argument.\n", emsg);
          MlnErrLine(n, (int)(end - argv[n]), err);
        }
        err_cnt++;
      }
      break;
    case MLN_OPT_STR:
    case MLN_OPT_FSTR:
      sv = cp;
      break;
    }

    switch (opts[i].type) {
    case MLN_OPT_FLAG:
    case MLN_OPT_FFLAG:
      break;
    case MLN_OPT_DBL:
      *(double *)(opts[i].arg) = dv;
      break;
    case MLN_OPT_FDBL:
      (*(void (*)(double))(opts[i].arg))(dv);
      break;
    case MLN_OPT_INT:
      *(int *)(opts[i].arg) = lv;
      break;
    case MLN_OPT_FINT:
      (*(void (*)(int))(opts[i].arg))(lv);
      break;
    case MLN_OPT_STR:
      *(char **)(opts[i].arg) = sv;
      break;
    case MLN_OPT_FSTR:
      (*(void (*)(char *))(opts[i].arg))(sv);
      break;
    }
  }

  return err_cnt;
}

int MlnOptInit(char **a, MlnOption *option, FILE *e) {
  int err_cnt = 0;

  argv = a;
  opts = option;
  err = e;

  if (argv && *argv && opts) {
    int i;
    for (i = 1; argv[i] != NULL; i++) {
      if (argv[i][0] == '+' || argv[i][0] == '-') {
        err_cnt += MlnHandleFlags(i, err);
      } else if (strchr(argv[i], '=')) {
        err_cnt += MlnHandleSwitch(i, err);
      }
    }
  }
  if (err_cnt > 0) {
    fprintf(err, "Valid command line options for \"%s\" are: \n", *a);
    MlnOptPrint();
    return -1;
  }

  return 0;
}

int MlnOptNArgs() {
  int i;
  int dash_dash = 0;
  int cnt = 0;

  if (argv != NULL && argv[0] != NULL) {
    for (i = 1; argv[i] != NULL; i++) {
      if (dash_dash || !MLN_IS_OPT(argv[i])) {
        cnt++;
      }
      if (strcmp(argv[i], "--") == 0) {
        dash_dash = 1;
      }
    }
  }
  return cnt;
}

char *MlnOptArg(int n) {
  int i = MlnArgIndex(n);
  return i >= 0 ? argv[i] : NULL;
}

void MlnOptErr(int n) {
  int i = MlnArgIndex(n);
  if (i >= 0) {
    MlnErrLine(i, 0, err);
  }
}

void MlnOptPrint() {
  int i;
  int max = 0;

  for (i = 0; opts[i].label; i++) {
    int len = strlen(opts[i].label) + 1;
    switch (opts[i].type) {
    case MLN_OPT_FLAG:
    case MLN_OPT_FFLAG:
      break;
    case MLN_OPT_INT:
    case MLN_OPT_FINT:
      len += 9; /* length of "<integer>" */
      break;
    case MLN_OPT_DBL:
    case MLN_OPT_FDBL:
      len += 6; /* length of "<real>" */
      break;
    case MLN_OPT_STR:
    case MLN_OPT_FSTR:
      len += 8; /* length of "<string>" */
      break;
    }
    if (len > max) {
      max = len;
    }
  }

  for (i = 0; opts[i].label; i++) {
    switch (opts[i].type) {
    case MLN_OPT_FLAG:
    case MLN_OPT_FFLAG:
      fprintf(err, "  -%-*s  %s\n", max, opts[i].label, opts[i].message);
      break;
    case MLN_OPT_INT:
    case MLN_OPT_FINT:
      fprintf(err, "  %s=<integer>%*s  %s\n", opts[i].label,
              (int)(max - strlen(opts[i].label) - 9), "", opts[i].message);
      break;
    case MLN_OPT_DBL:
    case MLN_OPT_FDBL:
      fprintf(err, "  %s=<real>%*s  %s\n", opts[i].label,
              (int)(max - strlen(opts[i].label) - 6), "", opts[i].message);
      break;
    case MLN_OPT_STR:
    case MLN_OPT_FSTR:
      fprintf(err, "  %s=<string>%*s  %s\n", opts[i].label,
              (int)(max - strlen(opts[i].label) - 8), "", opts[i].message);
      break;
    }
  }
}

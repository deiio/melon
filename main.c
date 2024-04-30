/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 *
 * TODO
 */

#include "option.h"

/* The main program. Parse the command line and do it... */
int main(int argc, char *argv[]) {
  int version = 0;
  int rpflag = 0;
  int basis_flag = 0;
  int compress = 0;
  int quiet = 0;
  int statistics = 0;
  int mhflag = 0;
  MlnOption options[] = {
      {MLN_OPT_FLAG, "b", (char *)&basis_flag,
       "Print only the basis in report."},
      {MLN_OPT_FLAG, "c", (char *)&compress,
       "Don't compress the action table."},
      {MLN_OPT_FLAG, "g", (char *)&rpflag, "Print grammer without actions."},
      {MLN_OPT_FLAG, "m", (char *)&mhflag,
       "Output a makeheaders compatible file."},
      {MLN_OPT_FLAG, "q", (char *)&quiet,
       "(Quiet) Don't print the report file."},
      {MLN_OPT_FLAG, "s", (char *)&statistics,
       "Print parser stats to standard output."},
      {MLN_OPT_FLAG, "x", (char *)&version, "Print the version number."},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };

  if (MlnOptInit(argv, options, stderr) < 0) {
    return -1;
  }
  if (version) {
    printf("Melon version 0.1\n");
    return 0;
  }

  if (MlnOptNArgs() != 1) {
    fprintf(stderr, "Exactly one filename argument is required.\n");
    return -1;
  }

  return 0;
}

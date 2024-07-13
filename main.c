/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include <ctype.h>
#include <stdlib.h>

#include "build.h"
#include "error.h"
#include "option.h"
#include "parse.h"
#include "report.h"
#include "set.h"
#include "struct.h"
#include "table.h"
#include "version.h"

/*
 * Report an out-of-memory condition and abort. This function
 * is used mostly by the "MlnMemoryCheck" function in struct.h
 */
void memory_error() {
  fprintf(stderr, "Out of memory. Aborting...\n");
  exit(1);
}

/* The main program. Parse the command line and do it... */
int main(int argc, char *argv[]) {
  int version = 0;
  int rpflag = 0;
  int basis_flag = 0;
  int compress = 0;
  int quiet = 0;
  int statistics = 0;
  int mhflag = 0;
  int i;
  MlnOption options[] = {
      {MLN_OPT_FLAG, "b", &basis_flag, "Print only the basis in report."},
      {MLN_OPT_FLAG, "c", &compress, "Don't compress the action table."},
      {MLN_OPT_FSTR, "D", MlnHandleDOption, "Define an %ifdef macro."},
      {MLN_OPT_FLAG, "g", &rpflag, "Print grammer without actions."},
      {MLN_OPT_FLAG, "m", &mhflag, "Output a makeheaders compatible file."},
      {MLN_OPT_FLAG, "q", &quiet, "(Quiet) Don't print the report file."},
      {MLN_OPT_FLAG, "s", &statistics,
       "Print parser stats to standard output."},
      {MLN_OPT_FLAG, "v", &version, "Print the version number."},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };
  Melon melon;

  if (MlnOptInit(argv, options, stderr) < 0) {
    return -1;
  }
  if (version) {
    printf("Melon version %s\n", MLN_VERSION);
    return 0;
  }

  if (MlnOptNArgs() != 1) {
    fprintf(stderr, "Exactly one filename argument is required.\n");
    return -1;
  }

  /* Initialize the machine */
  MlnStrSafeInit();
  MlnSymbolInit();
  MlnStateInit();

  melon.error_cnt = 0;
  melon.argv0 = argv[0];
  melon.filename = MlnOptArg(0);
  melon.basis_flag = basis_flag;
  melon.has_fallback = 0;
  melon.nconflict = 0;
  melon.name = NULL;
  melon.arg = NULL;
  melon.token_type = NULL;
  melon.var_type = NULL;
  melon.start = NULL;
  melon.stack_size = NULL;
  melon.include = NULL;

  melon.error = NULL;
  melon.overflow = NULL;
  melon.failure = NULL;
  melon.accept = NULL;
  melon.extra_code = NULL;
  melon.token_dest = NULL;
  melon.var_dest = NULL;
  melon.output_file = NULL;
  melon.token_prefix = NULL;

  melon.table_size = 0;

  MlnSymbolNew("$");
  melon.err_sym = MlnSymbolNew("error");

  /* Parse the input file */
  MlnParse(&melon);

  if (melon.error_cnt > 0) {
    return melon.error_cnt;
  }
  if (melon.rule == 0) {
    fprintf(stderr, "Empty grammar.\n");
    return 1;
  }

  /* Count and index the symbols of the grammar */
  melon.nsymbol = MlnSymbolCount();
  MlnSymbolNew("{default}");
  melon.symbols = MlnSymbolArrayOf();
  for (i = 0; i <= melon.nsymbol; i++) {
    melon.symbols[i]->index = i;
  }
  qsort(melon.symbols, melon.nsymbol + 1, sizeof(MlnSymbol *),
        (int (*)(const void *, const void *))MlnSymbolCmp);
  for (i = 0; i <= melon.nsymbol; i++) {
    melon.symbols[i]->index = i;
  }
  for (i = 1; isupper(melon.symbols[i]->name[0]); i++) {
  }
  melon.nterminal = i;

  /* Generate a reprint of the grammar, if requested on the command line */
  if (rpflag) {
    MlnReprint(&melon);
  } else {
    /* Initialize the size for all follow and first sets */
    MlnSetSize(melon.nterminal);

    /* Find the precedence for every production rule (that has one) */
    MlnFindRulePrecedences(&melon);

    /* Compute the lambda-non-terminals and the first-sets for every
     * non-terminal */
    MlnFindFirstSets(&melon);

    /* Compute all LR(0) states. Also record follow-set propagation
     * links so that the follow-set can be computed later */
    melon.nstate = 0;
    MlnFindStates(&melon);
    melon.sorted = MlnStateArrayOf();

    /* Tie up loose ends on the propagation links */
    MlnFindLinks(&melon);

    /* Compute the follow set of every reducible configuration */
    MlnFindFollowSets(&melon);

    /* Compute the action tables */
    MlnFindActions(&melon);

    /* Compress the action tables */
    if (compress == 0) {
      MlnCompressTables(&melon);
    }

    /* Generate a report of the parser generated. (the "y.output" file) */
    if (quiet == 0) {
      MlnReportOutput(&melon);
    }

    /* Generate the source code for the parser */
    MlnReportTable(&melon, mhflag);

    /* Produce a header file for use by the scanner. (This step is
     * ommited if the "-m" option is used because makeheaders will
     * generate the file for us.) */
    if (!mhflag) {
      MlnReportHeader(&melon);
    }
  }

  if (statistics != 0) {
    printf("Parser statistics: %d terminals, %d nonterminals, %d rules\n",
           melon.nterminal - 1, melon.nsymbol - melon.nterminal - 1,
           melon.nrule);
    printf("                   %d states, %d parser table entries, "
           "%d conflicts\n",
           melon.nstate, melon.table_size, melon.nconflict);
  }

  return melon.error_cnt + melon.nconflict;
}

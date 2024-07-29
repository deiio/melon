/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "report.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "action.h"
#include "acttab.h"
#include "assert.h"
#include "error.h"
#include "set.h"
#include "table.h"

const char *kDefaultTemplateFile = "mlt_parser.c";

/*
 * Generate a fielname with the given suffix. Space to hold the
 * name comes from malloc() and must be freed by calling function.
 */
static char *MlnFileMakeName(Melon *melon, const char *suffix) {
  char *cp;
  char *name = malloc(strlen(melon->filename) + strlen(suffix) + 5);

  if (name == NULL) {
    fprintf(stderr, "Can't allocate space for a filename.\n");
    exit(1);
  }
  strcpy(name, melon->filename);
  cp = strrchr(name, '.');
  if (cp != NULL) {
    *cp = '\0';
  }
  strcat(name, suffix);
  return name;
}

/*
 * Open a file with a name based on the name of the input file,
 * but with a different (specified) suffix, and return a pointer
 * to the stream.
 */
static FILE *MlnFileOpen(Melon *melon, const char *suffix, const char *mode) {
  FILE *fp;

  if (melon->output_file != NULL) {
    free(melon->output_file);
  }
  melon->output_file = MlnFileMakeName(melon, suffix);
  fp = fopen(melon->output_file, mode);
  if (fp == NULL && *mode == 'w') {
    fprintf(stderr, "Can't open file \"%s\".\n", melon->output_file);
    melon->error_cnt++;
    return NULL;
  }
  return fp;
}

/*
 * Print the configuration to file.
 */
static void MlnConfigPrint(FILE *file, MlnConfig *config) {
  int i;
  MlnRule *rule = config->rule;
  fprintf(file, "%s ::=", rule->lhs->name);
  for (i = 0; i <= rule->nrhs; i++) {
    if (i == config->dot) {
      fprintf(file, " *");
    }
    if (i == rule->nrhs) {
      break;
    }
    fprintf(file, " %s", rule->rhs[i]->name);
  }
}
/*
 * Print an action to the given file stream. Return 0 if nothing
 * was actually printed.
 */
static int MlnPrintAction(MlnAction *action, FILE *file, int indent) {
  int ret = 1;
  switch (action->type) {
  case MLN_SHIFT:
    fprintf(file, "%*s shift  %d", indent, action->sym->name,
            action->x.state->index);
    break;
  case MLN_REDUCE:
    fprintf(file, "%*s reduce %d", indent, action->sym->name,
            action->x.rule->index);
    break;
  case MLN_ACCEPT:
    fprintf(file, "%*s accept", indent, action->sym->name);
    break;
  case MLN_ERROR:
    fprintf(file, "%*s error", indent, action->sym->name);
    break;
  case MLN_CONFLICT:
    fprintf(file, "%*s reduce %-3d ** Parsing conflict **", indent,
            action->sym->name, action->x.rule->index);
    break;
  case MLN_SH_RESOLVED:
  case MLN_RD_RESOLVED:
  case NOT_USED:
    ret = 0;
    break;
  }
  return ret;
}

#ifdef TEST

/*
 * Print a set.
 */
static void MlnSetPrint(FILE *file, void *set, Melon *melon) {
  int i;
  char *space = "";
  fprintf(file, "%12s[", "");
  for (i = 0; i < melon->nterminal; i++) {
    if (MlnSetFind(set, i)) {
      fprintf(file, "%s%s", space, melon->symbols[i]->name);
      space = " ";
    }
  }
  fprintf(file, "]\n");
}

/*
 * Print a PLink chain.
 */
static void MlnPLinkPrint(FILE *file, MlnPLink *pl, const char *tag) {
  while (pl != NULL) {
    fprintf(file, "%12s%s (state %2d) ", "", tag, pl->config->st->index);
    MlnConfigPrint(file, pl->config);
    fprintf(file, "\n");
    pl = pl->next;
  }
}

#endif /* TEST */

/*
 * Search for the file "name" which is in the same directory as
 * the executable.
 */
static char *MlnPathSearch(const char *argv0, const char *name, int mode) {
  const char *cp;
  char *path;

  cp = strrchr(argv0, '/');
  if (cp != NULL) {
    int len = (int)(cp - argv0);
    path = malloc(len + strlen(name) + 2);
    if (path != NULL) {
      sprintf(path, "%.*s/%s", len, argv0, name);
    }
  } else {
    const char *paths = getenv("PATH");
    if (paths == NULL) {
      paths = ".:/bin:/usr/bin";
    }
    path = malloc(strlen(paths) + strlen(name) + 2);
    if (path != NULL) {
      while (paths != NULL) {
        int len;
        cp = strchr(paths, ':');
        if (cp == NULL) {
          cp = &paths[strlen(paths)];
        }
        len = (int)(cp - paths);
        sprintf(path, "%.*s/%s", len, paths, name);
        if (*cp == '\0') {
          paths = NULL;
        } else {
          paths = &cp[1];
        }
        if (access(path, mode) == 0) {
          break;
        }
      }
    }
  }

  return path;
}

/*
 * The next function finds the template file and opens it, returning
 * a pointer to the opened file.
 */
static FILE *MlnTplOpen(Melon *melon) {
  char buf[1024];
  FILE *in;
  char *cp;
  const char *tpl_name;
  int need_free = 0;

  cp = strrchr(melon->filename, '.');
  if (cp != NULL) {
    sprintf(buf, "%.*s.mtpl", (int)(cp - melon->filename), melon->filename);
  } else {
    sprintf(buf, "%s.mtpl", melon->filename);
  }

  if (access(buf, 0004) == 0) {
    tpl_name = buf;
  } else if (access(kDefaultTemplateFile, 0004) == 0) {
    tpl_name = kDefaultTemplateFile;
  } else {
    tpl_name = MlnPathSearch(melon->argv0, kDefaultTemplateFile, 0004);
    need_free = 1;
  }

  if (tpl_name == NULL) {
    fprintf(stderr, "Can't find the parser driver template file \"%s\".\n",
            kDefaultTemplateFile);
    melon->error_cnt++;
    return NULL;
  }

  in = fopen(tpl_name, "r");

  if (in == NULL) {
    fprintf(stderr, "Can't open the template file \"%s\".\n", tpl_name);
    melon->error_cnt++;
  }

  if (need_free) {
    free((char *)tpl_name);
  }

  return in;
}

/*
 * Given an action, compute the integer value for that action
 * which is to be put in the action table of the generated
 * machine. Return negative if no action should be generated.
 */
static int MlnComputeAction(Melon *melon, MlnAction *ap) {
  switch (ap->type) {
  case MLN_SHIFT:
    return ap->x.state->index;
  case MLN_REDUCE:
    return ap->x.rule->index + melon->nstate;
  case MLN_ERROR:
    return melon->nstate + melon->nrule;
  case MLN_ACCEPT:
    return melon->nstate + melon->nrule + 1;
  default:
    return -1;
  }
}

const int kLineSize = 1000;

/*
 * The next cluster of routines are for reading the template file
 * and writing the results to the generated parser.
 */

/*
 * The first function transfers data from "in" to "out" until
 * a line is seen which begins with "%%". The line number is
 * tracked.
 *
 * if name != NULL, the any word that begin with "Parse" is changed
 * to begin with *name instead.
 */
static void MlnTplXfer(const char *name, FILE *in, FILE *out, int *lineno) {
  char line[kLineSize];
  while (fgets(line, kLineSize, in) && (line[0] != '%' || line[1] != '%')) {
    int start = 0;
    (*lineno)++;
    if (name != NULL) {
      int i = 0;
      while (line[i] != '\0') {
        if (line[i] == 'P' && strncmp(&line[i], "Parse", 5) == 0 &&
            (i == 0 || !isalpha(line[i - 1]))) {
          if (i > start) {
            fprintf(out, "%.*s", i - start, &line[start]);
          }
          fprintf(out, "%s", name);
          i += 4;
          start = i + 1;
        }
        i++;
      }
    }
    fprintf(out, "%s", &line[start]);
  }
}

/*
 * Print a string to the file and keep the line number up to date.
 */
static void MlnTplPrint(FILE *out, Melon *melon, const char *str, int str_line,
                        int *lineno) {
  if (str == NULL) {
    return;
  }
  fprintf(out, "#line %d \"%s\"\n", str_line, melon->filename);
  (*lineno)++;
  while (*str != '\0') {
    if (*str == '\n') {
      (*lineno)++;
    }
    putc(*str, out);
    str++;
  }
  fprintf(out, "\n#line %d \"%s\"\n", *lineno + 2, melon->output_file);
  (*lineno) += 2;
}

/*
 * The following routine emits code for the destructor for the symbol sym
 */
static void MlnEmitDestructorCode(FILE *out, MlnSymbol *sym, Melon *melon,
                                  int *lineno) {
  char *cp = NULL;
  if (sym->type == MLN_SYM_TERMINAL) {
    cp = melon->token_dest;
    if (cp == NULL) {
      return;
    }
    fprintf(out, "#line %d \"%s\"\n{", melon->token_dest_line, melon->filename);
  } else if (sym->destructor != NULL) {
    cp = sym->destructor;
    fprintf(out, "#line %d \"%s\"\n{", sym->destructor_line, melon->filename);
  } else if (melon->var_dest != NULL) {
    cp = melon->var_dest;
    fprintf(out, "#line %d \"%s\"\n{", melon->var_dest_line, melon->filename);
  } else {
    assert(0); /* Cannot happen */
  }

  for (; *cp != '\0'; cp++) {
    if (*cp == '$' && cp[1] == '$') {
      fprintf(out, "(yypminor->yy%d)", sym->data_type_num);
      cp++;
      continue;
    }
    if (*cp == '\n') {
      (*lineno)++;
    }
    fputc(*cp, out);
  }
  (*lineno) += 3;
  fprintf(out, "}\n#line %d \"%s\"\n", *lineno, melon->output_file);
}

/*
 * Duplicate the input file without comments.
 */
void MlnReprint(Melon *melon) {
  MlnRule *rp;
  MlnSymbol *sp;
  int i, j, maxlen, len, ncolumns, skip;
  printf("// Reprint of input file \"%s\".\n// Symbols:\n", melon->filename);
  maxlen = 10;

  for (i = 0; i < melon->nsymbol; i++) {
    sp = melon->symbols[i];
    len = strlen(sp->name);
    if (len > maxlen) {
      maxlen = len;
    }
  }
  ncolumns = 76 / (maxlen + 5);
  if (ncolumns < 1) {
    ncolumns = 1;
  }
  skip = (melon->nsymbol + ncolumns - 1) / ncolumns;
  for (i = 0; i < skip; i++) {
    printf("//");
    for (j = i; j < melon->nsymbol; j += skip) {
      sp = melon->symbols[j];
      assert(sp->index == j);
      printf(" %3d %-*.*s", j, maxlen, maxlen, sp->name);
    }
    printf("\n");
  }

  for (rp = melon->rule; rp; rp = rp->next) {
    printf("%s", rp->lhs->name);
    if (rp->lhs_alias) {
      printf("(%s)", rp->lhs_alias);
    }

    printf(" ::=");
    for (i = 0; i < rp->nrhs; i++) {
      printf(" %s", rp->rhs[i]->name);
      if (rp->rhs_alias[i]) {
        printf("(%s)", rp->rhs_alias[i]);
      }
    }
    printf(".");
    if (rp->prec_sym) {
      printf(" [%s]", rp->prec_sym->name);
    }
    if (rp->code) {
      printf("\n    %s", rp->code);
    }
    printf("\n");
  }
}

/*
 * Generate the "y.output" log file.
 */
void MlnReportOutput(Melon *melon) {
  int i;
  FILE *fp = MlnFileOpen(melon, ".out", "w");

  if (fp == NULL) {
    return;
  }

  for (i = 0; i < melon->nstate; i++) {
    MlnConfig *cfp;
    MlnAction *ap;
    MlnState *state = melon->sorted[i];
    fprintf(fp, "State %d:\n", state->index);
    if (melon->basis_flag) {
      cfp = state->bp;
    } else {
      cfp = state->cfp;
    }
    while (cfp != NULL) {
      char buf[20];
      if (cfp->dot == cfp->rule->nrhs) {
        sprintf(buf, "(%d)", cfp->rule->index);
        fprintf(fp, "%9s ", buf);
      } else {
        fprintf(fp, "%10s", "");
      }
      MlnConfigPrint(fp, cfp);
      fprintf(fp, "\n");
#ifdef TEST
      MlnSetPrint(fp, cfp->fws, melon);
      MlnPLinkPrint(fp, cfp->fpl, "To  ");
      MlnPLinkPrint(fp, cfp->bpl, "From");
#endif
      if (melon->basis_flag) {
        cfp = cfp->bp;
      } else {
        cfp = cfp->next;
      }
    }

    fprintf(fp, "\n");
    for (ap = state->ap; ap != NULL; ap = ap->next) {
      if (MlnPrintAction(ap, fp, 30)) {
        fprintf(fp, "\n");
      }
    }
    fprintf(fp, "\n");
  }

  fclose(fp);
  return;
}

/*
 * Return true (non-zero) if the given symbol has a destructor
 */
static int MlnHasDestructor(MlnSymbol *sym, Melon *melon) {
  if (sym->type == MLN_SYM_TERMINAL) {
    return melon->token_dest != NULL;
  } else {
    return melon->var_dest != NULL || sym->destructor != NULL;
  }
}

/*
 * Generate code which executes when the rule "rule" is reduced.
 * Write the code to "out", Make sure lineno stays up-to-date.
 */
void MlnEmitCode(FILE *out, MlnRule *rule, Melon *melon, int *lineno) {
  char used[MLN_MAX_RHS] = {0};
  char *cp;
  int i;
  int lhs_used = 0;

  /* Generate code to do the reduce action */
  if (rule->code != NULL) {
    fprintf(out, "#line %d \"%s\"\n{", rule->line, melon->filename);
    for (cp = rule->code; *cp != '\0'; cp++) {
      if (isalpha(*cp) &&
          (cp == rule->code || (!isalnum(cp[-1]) && cp[-1] != '_'))) {
        char saved;
        char *xp;
        for (xp = &cp[1]; isalnum(*xp) || *xp == '_'; xp++) {
        }
        saved = *xp;
        *xp = '\0';
        if (rule->lhs_alias != NULL && strcmp(cp, rule->lhs_alias) == 0) {
          fprintf(out, "yygotominor.yy%d", rule->lhs->data_type_num);
          cp = xp;
          lhs_used = 1;
        } else {
          for (i = 0; i < rule->nrhs; i++) {
            if (rule->rhs_alias[i] != NULL &&
                strcmp(cp, rule->rhs_alias[i]) == 0) {
              fprintf(out, "yymsp[%d].minor.yy%d", i - rule->nrhs + 1,
                      rule->rhs[i]->data_type_num);
              cp = xp;
              used[i] = 1;
              break;
            }
          }
        }
        *xp = saved;
      }
      if (*cp == '\n') {
        (*lineno)++;
      }
      fputc(*cp, out);
    }
    (*lineno) += 3;
    fprintf(out, "}\n#line %d \"%s\"\n", *lineno, melon->output_file);
  }

  /* Check to make sure the LHS has been used */
  if (rule->lhs_alias != NULL && !lhs_used) {
    MlnErrorMsg(melon->filename, rule->rule_line,
                "Label \"%s\" for \"%s(%s)\" is never used.", rule->lhs_alias,
                rule->lhs->name, rule->lhs_alias);
    melon->error_cnt++;
  }

  /*
   * Generate destructor code for RHS symbols which are not used in the
   * reduce code.
   */
  for (i = 0; i < rule->nrhs; i++) {
    if (rule->rhs_alias[i] != NULL && !used[i]) {
      MlnErrorMsg(melon->filename, rule->rule_line,
                  "Label \"%s\" for \"%s(%s)\" is never used.",
                  rule->rhs_alias[i], rule->rhs[i]->name, rule->rhs_alias[i]);
      melon->error_cnt++;
    } else if (rule->rhs_alias[i] == NULL) {
      if (MlnHasDestructor(rule->rhs[i], melon)) {
        fprintf(out, "  yy_destructor(%d, &yymsp[%d].minor);\n",
                rule->rhs[i]->index, i - rule->nrhs + 1);
        (*lineno)++;
      } else {
        fprintf(out, "        /* No destructor defined for %s */\n",
                rule->rhs[i]->name);
        (*lineno)++;
      }
    }
  }
}

/*
 * Print the definitation of the union used for the parser's data stack.
 * This union contains fields for every possible data type for tokens
 * and nonterminals. In the process of computing and printing this
 * union, also set the ".data_type_num" filed of every terminal and
 * nonterminal symbol.
 */
static void MlnPrintStackUnion(FILE *out, Melon *melon, int *lineno,
                               int mhflag) {
  char **types;   /* A hash table of datatypes */
  int type_size;  /* Size of types array */
  int max_dt_len; /* Maximum length of any ".data_type" filed */
  char *stddt;    /* Standardized name for a datatype */
  char *name;     /* Name of the parser */
  int hash;       /* For hashing the name of a type */
  int i, j;       /* Loop counters */

  /* Allocate and initialize types[] and allocate stddt[]. */
  type_size = melon->nsymbol * 2;
  types = calloc(type_size, sizeof(char *));
  max_dt_len = 0;
  if (melon->var_type) {
    max_dt_len = strlen(melon->var_type);
  }
  for (i = 0; i < melon->nsymbol; i++) {
    int len;
    MlnSymbol *sp = melon->symbols[i];
    if (sp->data_type == NULL) {
      continue;
    }
    len = strlen(sp->data_type);
    if (len > max_dt_len) {
      max_dt_len = len;
    }
  }
  stddt = malloc(max_dt_len * 2 + 1);
  if (types == NULL || stddt == NULL) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  /*
   * Build a hash table of datatypes. The ".data_type_num" field of
   * each symbol is filled in with the hash index plus 1. A
   * ".data_type_num" value of 0 is used for terminal symbols. If
   * there is no %default_type defined then 0 is also used as the
   * ".data_type_num" value for nonterminals which do not specify
   * a datatype using the %type directive.
   */
  for (i = 0; i < melon->nsymbol; i++) {
    MlnSymbol *sp = melon->symbols[i];
    char *cp;
    if (sp == melon->err_sym) {
      sp->data_type_num = type_size + 1;
      continue;
    }
    if (sp->type != MLN_SYM_NON_TERMINAL ||
        (sp->data_type == NULL && melon->var_type == NULL)) {
      sp->data_type_num = 0;
      continue;
    }
    cp = sp->data_type;
    if (cp == NULL) {
      cp = melon->var_type;
    }
    j = 0;
    while (isspace(*cp)) {
      cp++;
    }
    while (*cp != '\0') {
      stddt[j++] = *cp++;
    }
    while (j > 0 && isspace(stddt[j - 1])) {
      j--;
    }
    stddt[j] = '\0';
    hash = 0;
    for (j = 0; stddt[j]; j++) {
      hash = hash * 53 + stddt[j];
    }
    hash = (hash & 0x7FFFFFFF) % type_size;
    while (types[hash] != NULL) {
      if (strcmp(types[hash], stddt) == 0) {
        sp->data_type_num = hash + 1;
        break;
      }
      hash++;
      if (hash >= type_size) {
        hash = 0;
      }
    }
    if (types[hash] == NULL) {
      sp->data_type_num = hash + 1;
      types[hash] = malloc(strlen(stddt) + 1);
      if (types[hash] == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
      }
      strcpy(types[hash], stddt);
    }
  }

  /* Print out the definitation of YYTOKENTYPE and YYMINORTYPE */
  name = melon->name == NULL ? "Parse" : melon->name;
  if (mhflag) {
    fprintf(out, "#if INTERFACE\n");
    (*lineno)++;
  }
  fprintf(out, "#define %sTOKENTYPE %s\n", name,
          melon->token_type ? melon->token_type : "void *");
  (*lineno)++;
  if (mhflag) {
    fprintf(out, "#endif /* INTERFACE */\n");
    (*lineno)++;
  }
  fprintf(out, "typedef union {\n");
  (*lineno)++;
  fprintf(out, "  %sTOKENTYPE yy0;\n", name);
  (*lineno)++;
  for (i = 0; i < type_size; i++) {
    if (types[i] == NULL) {
      continue;
    }
    fprintf(out, "  %s yy%d;\n", types[i], i + 1);
    (*lineno)++;
    free(types[i]);
  }
  fprintf(out, "  int yy%d;\n", melon->err_sym->data_type_num);
  (*lineno)++;
  fprintf(out, "} YYMINORTYPE;\n");
  (*lineno)++;
  free(stddt);
  free(types);
}

/*
 * Return the name of a C data type able to represent values between
 * lwr and upr, inclusive.
 */
static const char *MlnMinimumSizeType(int lwr, int upr) {
  if (lwr >= 0) {
    if (upr <= 0xFF) {
      return "unsigned char";
    } else if (upr < 0xFFFF) {
      return "unsigned short";
    } else {
      return "unsigned";
    }
  } else if (lwr >= -0xFF && upr <= 0xFF) {
    return "signed char";
  } else if (lwr >= -0xFFFF && upr <= 0xFFFF) {
    return "short";
  } else {
    return "int";
  }
}

/*
 * Each state contains a set of token transaction and a set of
 * nonterminal transactions. Each of these sets makes an instance
 * of the following structure. An array of these structures is used
 * to order the creation of entries in the yy_action[] table.
 */
typedef struct {
  MlnState *state; /* A pointer to a state */
  int is_token;    /* True to use tokens. False for non-terminals */
  int naction;     /* Number of actions */
} MlnAxSet;

/*
 * Compare to MlnAxSet structures for sorting pruposes.
 */
static int MlnAxSetCompare(const void *a, const void *b) {
  const MlnAxSet *p1 = a, *p2 = b;
  return p2->naction - p1->naction;
}

/*
 * Generate C source code for the parser.
 */
void MlnReportTable(Melon *melon, int mhflag) {
  char *name;
  char line[kLineSize];
  FILE *in, *out;
  int line_no;
  int i, j, n;
  int min_tkn_offset, max_tkn_offset;
  int min_ntkn_offset, max_ntkn_offset;
  MlnAxSet *ax;
  MlnActionTable *at;
  MlnRule *rule;

  in = MlnTplOpen(melon);
  if (in == NULL) {
    return;
  }
  out = MlnFileOpen(melon, ".c", "w");
  if (out == NULL) {
    fclose(in);
    return;
  }
  line_no = 1;
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate the include code, if any */
  MlnTplPrint(out, melon, melon->include, melon->include_line, &line_no);
  if (mhflag) {
    name = MlnFileMakeName(melon, ".h");
    fprintf(out, "#include \"%s\"\n", name);
    line_no++;
    free(name);
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate #defines for all tokens */
  if (mhflag) {
    char *prefix;
    int i;
    fprintf(out, "#if INTERFACE\n");
    line_no++;
    if (melon->token_prefix) {
      prefix = melon->token_prefix;
    } else {
      prefix = "";
    }
    for (i = 1; i < melon->nterminal; i++) {
      fprintf(out, "#define %s%-30s %2d\n", prefix, melon->symbols[i]->name, i);
      line_no++;
    }
    fprintf(out, "#endif /* INTERFACE */\n");
    line_no++;
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate the defines */
  fprintf(out, "#define YYCODETYPE %s\n",
          MlnMinimumSizeType(0, melon->nsymbol + 5));
  line_no++;
  fprintf(out, "#define YYNOCODE %d\n", melon->nsymbol + 1);
  line_no++;
  fprintf(out, "#define YYACTIONTYPE %s\n",
          MlnMinimumSizeType(0, melon->nstate + melon->nrule + 5));
  line_no++;
  MlnPrintStackUnion(out, melon, &line_no, mhflag);

  if (melon->stack_size) {
    if (atoi(melon->stack_size) <= 0) {
      MlnErrorMsg(melon->filename, 0,
                  "Illegal stack size: [%s]. "
                  "The stack size should be an integer constant.",
                  melon->stack_size);
      melon->error_cnt++;
      melon->stack_size = "100";
    }
    fprintf(out, "#define YYSTACKDEPTH %s\n", melon->stack_size);
    line_no++;
  } else {
    fprintf(out, "#define YYSTACKDEPTH 100\n");
    line_no++;
  }
  if (mhflag) {
    fprintf(out, "#if INTERFACE\n");
    line_no++;
  }
  name = melon->name ? melon->name : "Parse";
  if (melon->arg && melon->arg[0] != '\0') {
    i = strlen(melon->arg);
    while (i >= 1 && isspace(melon->arg[i - 1])) {
      i--;
    }
    while (i >= 1 && (isalnum(melon->arg[i - 1]) || melon->arg[i - 1] == '_')) {
      i--;
    }
    fprintf(out, "#define %sARG_SDECL %s;\n", name, melon->arg);
    fprintf(out, "#define %sARG_PDECL ,%s\n", name, melon->arg);
    fprintf(out, "#define %sARG_FETCH %s = yypParser->%s\n", name, melon->arg,
            &melon->arg[i]);
    fprintf(out, "#define %sARG_STORE yypParser->%s = %s\n", name,
            &melon->arg[i], &melon->arg[i]);
    line_no += 4;
  } else {
    fprintf(out, "#define %sARG_SDECL\n", name);
    fprintf(out, "#define %sARG_PDECL\n", name);
    fprintf(out, "#define %sARG_FETCH\n", name);
    fprintf(out, "#define %sARG_STORE\n", name);
    line_no += 4;
  }
  if (mhflag) {
    fprintf(out, "#endif /* INTERFACE */\n");
    line_no++;
  }
  fprintf(out, "#define YYNSTATE %d\n", melon->nstate);
  line_no++;
  fprintf(out, "#define YYNRULE %d\n", melon->nrule);
  line_no++;
  fprintf(out, "#define YYERRORSYMBOL %d\n", melon->err_sym->index);
  line_no++;
  fprintf(out, "#define YYERRSYMDT yy%d\n", melon->err_sym->data_type_num);
  line_no++;
  if (melon->has_fallback) {
    fprintf(out, "#define YYFALLBACK 1\n");
    line_no++;
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate the action table and its associates:
   *
   *  yy_action[]       A single table containing all actions.
   *  yy_lookahead[]    A table containing the lookahead for each entry
   *                    in yy_action. Used to detect hash collisions.
   *  yy_shift_ofst[]   For each state, the offset into yy_action for
   *                    shfiting terminals.
   *  yy_reduce_ofst[]  For each state, the offset into yy_action for
   *                    shfiting non-terminals after a reduce.
   *  yy_default[]      Default action for each state.
   */

  /* Compute the actions on all states and count them up */
  ax = malloc(sizeof(ax[0]) * melon->nstate * 2);
  if (ax == NULL) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  for (i = 0; i < melon->nstate; i++) {
    MlnAction *ap;
    MlnState *state = melon->sorted[i];
    state->ntkn_act = 0;
    state->nntkn_act = 0;
    state->dflt_act = melon->nstate + melon->nrule;
    state->tkn_off = MLN_NO_OFFSET;
    state->ntkn_off = MLN_NO_OFFSET;
    for (ap = state->ap; ap != NULL; ap = ap->next) {
      if (MlnComputeAction(melon, ap) > 0) {
        if (ap->sym->index < melon->nterminal) {
          state->ntkn_act++;
        } else if (ap->sym->index < melon->nsymbol) {
          state->nntkn_act++;
        } else {
          state->dflt_act = MlnComputeAction(melon, ap);
        }
      }
    }
    ax[i * 2].state = state;
    ax[i * 2].is_token = 1;
    ax[i * 2].naction = state->ntkn_act;
    ax[i * 2 + 1].state = state;
    ax[i * 2 + 1].is_token = 0;
    ax[i * 2 + 1].naction = state->nntkn_act;
  }
  min_tkn_offset = 0;
  max_tkn_offset = 0;
  min_ntkn_offset = 0;
  max_ntkn_offset = 0;

  /*
   * Compute the action table. In order to try to keep the size of the
   * action table to a minimum, the heuristic of placing the largest
   * action sets first is used.
   */
  qsort(ax, melon->nstate * 2, sizeof(ax[0]), MlnAxSetCompare);
  at = MlnActionTableAlloc();
  for (i = 0; i < melon->nstate * 2 && ax[i].naction > 0; i++) {
    MlnAction *ap;
    MlnState *state = ax[i].state;
    if (ax[i].is_token) {
      for (ap = state->ap; ap != NULL; ap = ap->next) {
        int action;
        if (ap->sym->index >= melon->nterminal) {
          continue;
        }
        action = MlnComputeAction(melon, ap);
        if (action < 0) {
          continue;
        }
        MlnActionTableAddAction(at, ap->sym->index, action);
      }
      state->tkn_off = MlnActionTableInsert(at);
      if (state->tkn_off < min_tkn_offset) {
        min_tkn_offset = state->tkn_off;
      }
      if (state->tkn_off > max_tkn_offset) {
        max_tkn_offset = state->tkn_off;
      }
    } else {
      for (ap = state->ap; ap != NULL; ap = ap->next) {
        int action;
        if (ap->sym->index < melon->nterminal) {
          continue;
        }
        if (ap->sym->index == melon->nsymbol) {
          continue;
        }
        action = MlnComputeAction(melon, ap);
        if (action < 0) {
          continue;
        }
        MlnActionTableAddAction(at, ap->sym->index, action);
      }
      state->ntkn_off = MlnActionTableInsert(at);
      if (state->ntkn_off < min_ntkn_offset) {
        min_ntkn_offset = state->ntkn_off;
      }
      if (state->ntkn_off > max_ntkn_offset) {
        max_ntkn_offset = state->ntkn_off;
      }
    }
  }
  free(ax);

  /* Output the yy_action table */
  fprintf(out, "static YYACTIONTYPE yy_action[] = {\n");
  line_no++;
  n = MlnActionTableSize(at);
  for (i = 0, j = 0; i < n; i++) {
    int action = MlnActionTableAction(at, i);
    if (action < 0) {
      action = melon->nsymbol + melon->nrule + 2;
    }
    if (j == 0) {
      fprintf(out, " /* %5d */ ", i);
    }
    fprintf(out, " %4d,", action);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      line_no++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  line_no++;

  /* Output the yy_lookahead table */
  fprintf(out, "static YYCODETYPE yy_lookahead[] = {\n");
  line_no++;
  for (i = 0, j = 0; i < n; i++) {
    int la = MlnActionTableLookahead(at, i);
    if (la < 0) {
      la = melon->nsymbol;
    }
    if (j == 0) {
      fprintf(out, " /* %5d */ ", i);
    }
    fprintf(out, " %4d,", la);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      line_no++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  line_no++;

  /* Output the yy_shift_ofst[] table */
  fprintf(out, "#define YY_SHIFT_USE_DFLT (%d)\n", min_tkn_offset - 1);
  line_no++;
  fprintf(out, "static %s yy_shift_ofst[] = {\n",
          MlnMinimumSizeType(min_tkn_offset - 1, max_tkn_offset));
  line_no++;
  n = melon->nstate;
  for (i = 0, j = 0; i < n; i++) {
    MlnState *state = melon->sorted[i];
    int ofst = state->tkn_off;
    if (ofst == MLN_NO_OFFSET) {
      ofst = min_tkn_offset - 1;
    }
    if (j == 0) {
      fprintf(out, " /* %5d */ ", i);
    }
    fprintf(out, " %4d,", ofst);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      line_no++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  line_no++;

  /* Output the yy_reduce_ofst[] table */
  fprintf(out, "#define YY_REDUCE_USE_DFLT (%d)\n", min_ntkn_offset - 1);
  line_no++;
  fprintf(out, "static %s yy_reduce_ofst[] = {\n",
          MlnMinimumSizeType(min_ntkn_offset - 1, max_ntkn_offset));
  line_no++;
  n = melon->nstate;
  for (i = 0, j = 0; i < n; i++) {
    MlnState *state = melon->sorted[i];
    int ofst = state->ntkn_off;
    if (ofst == MLN_NO_OFFSET) {
      ofst = min_ntkn_offset - 1;
    }
    if (j == 0) {
      fprintf(out, " /* %5d */ ", i);
    }
    fprintf(out, " %4d,", ofst);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      line_no++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  line_no++;

  /* Output the default actoin table */
  fprintf(out, "static YYACTIONTYPE yy_default[] = {\n");
  line_no++;
  n = melon->nstate;
  for (i = 0, j = 0; i < n; i++) {
    MlnState *state = melon->sorted[i];
    if (j == 0) {
      fprintf(out, " /* %5d */ ", i);
    }
    fprintf(out, " %4d,", state->dflt_act);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      line_no++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  line_no++;
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate the table of fallback tokens */
  if (melon->has_fallback) {
    for (i = 0; i < melon->nterminal; i++) {
      MlnSymbol *sym = melon->symbols[i];
      if (sym->fallback == NULL) {
        fprintf(out, "    0,  /* %10s => nothing */\n", sym->name);
      } else {
        fprintf(out, "  %3d,  /* %10s => %s */\n", sym->fallback->index,
                sym->name, sym->fallback->name);
      }
      line_no++;
    }
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate a table containing the symbolic name of every symbol */
  for (i = 0; i < melon->nsymbol; i++) {
    sprintf(line, "\"%s\",", melon->symbols[i]->name);
    fprintf(out, "  %-15s", line);
    if ((i & 3) == 3) {
      fprintf(out, "\n");
      line_no++;
    }
  }
  if ((i & 3) != 0) {
    fprintf(out, "\n");
    line_no++;
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate a table containing a text string that describes every
   * rule in the rule set fo the grammar. This information is used
   * when tracing REDUCE actions.
   */
  for (i = 0, rule = melon->rule; rule != NULL; rule = rule->next, i++) {
    assert(rule->index == i);
    fprintf(out, " /* %3d */ \"%s ::=", i, rule->lhs->name);
    for (j = 0; j < rule->nrhs; j++) {
      fprintf(out, " %s", rule->rhs[j]->name);
    }
    fprintf(out, "\",\n");
    line_no++;
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate code which executes every time a symbol is popped from
   * the stack while processing errors or while destroying the parser.
   * (In other words, generate the %destructor actions)
   */
  if (melon->token_dest != NULL) {
    for (i = 0; i < melon->nsymbol; i++) {
      MlnSymbol *sp = melon->symbols[i];
      if (sp == NULL || sp->type != MLN_SYM_TERMINAL) {
        continue;
      }
      fprintf(out, "    case %d:\n", sp->index);
      line_no++;
    }
    for (i = 0;
         i < melon->nsymbol && melon->symbols[i]->type != MLN_SYM_TERMINAL;
         i++) {
    }
    if (i < melon->nsymbol) {
      MlnEmitDestructorCode(out, melon->symbols[i], melon, &line_no);
      fprintf(out, "      break;\n");
      line_no++;
    }
  }
  for (i = 0; i < melon->nsymbol; i++) {
    MlnSymbol *sp = melon->symbols[i];
    if (sp == NULL || sp->type == MLN_SYM_TERMINAL || sp->destructor == NULL) {
      continue;
    }
    fprintf(out, "    case %d:\n", sp->index);
    line_no++;
    MlnEmitDestructorCode(out, sp, melon, &line_no);
    fprintf(out, "      break;\n");
    line_no++;
  }
  if (melon->var_dest != NULL) {
    MlnSymbol *dflt_sp = NULL;
    for (i = 0; i < melon->nsymbol; i++) {
      MlnSymbol *sp = melon->symbols[i];
      if (sp == NULL || sp->type == MLN_SYM_TERMINAL || sp->index <= 0 ||
          sp->destructor != NULL) {
        continue;
      }
      fprintf(out, "    case %d:\n", sp->index);
      line_no++;
      dflt_sp = sp;
    }
    if (dflt_sp != NULL) {
      MlnEmitDestructorCode(out, dflt_sp, melon, &line_no);
      fprintf(out, "      break;\n");
      line_no++;
    }
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate code which executes whenever the parser stack overflows */
  MlnTplPrint(out, melon, melon->overflow, melon->overflow_line, &line_no);
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate the table of rule information.
   *
   * Note: This code depends on the fact that rules are number
   * sequentually beginning with 0.
   */
  for (rule = melon->rule; rule != NULL; rule = rule->next) {
    fprintf(out, "  { %d, %d },\n", rule->lhs->index, rule->nrhs);
    line_no++;
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate code which execution during each REDUCE action */
  for (rule = melon->rule; rule != NULL; rule = rule->next) {
    fprintf(out, "      case %d:\n", rule->index);
    line_no++;
    MlnEmitCode(out, rule, melon, &line_no);
    fprintf(out, "        break;\n");
    line_no++;
  }
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate code which executes if a parse fails */
  MlnTplPrint(out, melon, melon->failure, melon->failure_line, &line_no);
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate code which executes when a syntax error occurs */
  MlnTplPrint(out, melon, melon->error, melon->error_line, &line_no);
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Generate code which executes when the parser accepts its input */
  MlnTplPrint(out, melon, melon->accept, melon->accept_line, &line_no);
  MlnTplXfer(melon->name, in, out, &line_no);

  /* Append any addition code the user desires */
  MlnTplPrint(out, melon, melon->extra_code, melon->extra_code_line, &line_no);

  fclose(out);
  fclose(in);
}

/*
 * Generate a header file for the parser.
 */
void MlnReportHeader(Melon *melon) {
  FILE *out, *in;
  const char *prefix;
  char line[kLineSize];
  char pattern[kLineSize];
  int i;

  if (melon->token_prefix) {
    prefix = melon->token_prefix;
  } else {
    prefix = "";
  }

  in = MlnFileOpen(melon, ".h", "r");
  if (in) {
    for (i = 1; i < melon->nterminal && fgets(line, kLineSize, in); i++) {
      snprintf(pattern, kLineSize, "#define %s%-30s %2d\n", prefix,
               melon->symbols[i]->name, i);
      if (strcmp(line, pattern)) {
        break;
      }
      fclose(in);
      if (i == melon->nterminal) {
        /* No change in the file. Don't rewrite it. */
        return;
      }
    }
  }

  out = MlnFileOpen(melon, ".h", "w");
  if (out) {
    for (i = 1; i < melon->nterminal; i++) {
      fprintf(out, "#define %s%-30s %2d\n", prefix, melon->symbols[i]->name, i);
    }
    fclose(out);
  }
}

/*
 * Reduce the size of the action tables, if possible, by making use
 * of defaults.
 *
 * In this version, we take the most frequent REDUCE action and make
 * it the default. Only default a reduce if there are more than one.
 */
void MlnCompressTables(Melon *melon) {
  int i;
  for (i = 0; i < melon->nstate; i++) {
    MlnState *state = melon->sorted[i];
    int nbest = 0;
    MlnRule *rbest = NULL;
    MlnAction *ap;

    for (ap = state->ap; ap != NULL; ap = ap->next) {
      MlnRule *rp;
      MlnAction *ap2;
      int n = 1;
      if (ap->type != MLN_REDUCE) {
        continue;
      }
      rp = ap->x.rule;
      if (rp == rbest) {
        continue;
      }

      for (ap2 = ap->next; ap2 != NULL; ap2 = ap2->next) {
        MlnRule *rp2;
        if (ap2->type != MLN_REDUCE) {
          continue;
        }
        rp2 = ap2->x.rule;
        if (rp2 == rbest) {
          continue;
        }
        if (rp2 == rp) {
          n++;
        }
      }
      if (n > nbest) {
        nbest = n;
        rbest = rp;
      }
    }

    /* Do not make default if the number of rules to default
     * is not at leaset 2 */
    if (nbest < 2) {
      continue;
    }

    /* Compress matching REDUCE actions into a single default */
    for (ap = state->ap; ap != NULL; ap = ap->next) {
      if (ap->type == MLN_REDUCE && ap->x.rule == rbest) {
        break;
      }
    }
    assert(ap);
    ap->sym = MlnSymbolNew("{default}");
    for (ap = ap->next; ap != NULL; ap = ap->next) {
      if (ap->type == MLN_REDUCE && ap->x.rule == rbest) {
        ap->type = NOT_USED;
      }
    }
    state->ap = MlnActionSort(state->ap);
  }
}

/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "parse.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "table.h"

/*
 * The state of the parser
 */
typedef struct {
  char *filename;    /* Name of the input file */
  int token_line;    /* Line number at which current token starts */
  int error_cnt;     /* Number of errors so far */
  char *token_start; /* Text of current token */
  Melon *melon;      /* Global state vector */
  enum MlnParserState {
    MLN_PS_INITIALIZE,
    MLN_PS_WAITING_FOR_DECL_OR_RULE,
    MLN_PS_WAITING_FOR_DECL_KEYWORD,
    MLN_PS_WAITING_FOR_DECL_ARG,
    MLN_PS_WAITING_FOR_PRECEDENCE_SYMBOL,
    MLN_PS_WAITING_FOR_ARROW,
    MLN_PS_IN_RHS,
    MLN_PS_LHS_ALIAS_1,
    MLN_PS_LHS_ALIAS_2,
    MLN_PS_LHS_ALIAS_3,
    MLN_PS_RHS_ALIAS_1,
    MLN_PS_RHS_ALIAS_2,
    MLN_PS_PRECEDENCE_MARK_1,
    MLN_PS_PRECEDENCE_MARK_2,
    MLN_PS_RESYNC_AFTER_RULE_ERROR,
    MLN_PS_RESYNC_AFTER_DECL_ERROR,
    MLN_PS_WAITING_FOR_DESTRUCTOR_SYMBOL,
    MLN_PS_WAITING_FOR_DATATYPE_SYMBOL,
    MLN_PS_WAITING_FOR_FALLBACK_ID,
  } state;                     /* The state of the parser */
  MlnSymbol *fallback;         /* The fallback token */
  MlnSymbol *lhs;              /* Left-hand side of current rule */
  char *lhs_alias;             /* Alias for the LHS */
  int rhs_count;               /* Number of right-hand side symbols seen */
  MlnSymbol *rhs[MLN_MAX_RHS]; /* RHS symbols */
  char *alias[MLN_MAX_RHS];    /* Aliases for each RHS symbol (or NULL) */
  MlnRule *prev_rule;          /* Previous rule parsed */
  char *decl_keyword;          /* Keyword of a declaration */
  char **decl_arg_slot;    /* Where the declaration argument should be put */
  int *decl_ln_slot;       /* Where the declaration linenumber is put */
  MlnAssocType decl_assoc; /* Assign this association to decl arguments */
  int prec_counter;        /* Assign this precedence to decl arguments */
  MlnRule *first_rule;     /* Pointer to first rule in the grammer */
  MlnRule *last_rule;      /* Pointer to the most recently parsed rule */
} pstate;

/* Number of -D options on the command line. */
static int define_cnt = 0;
static char **define_array = NULL; /* Name of the -D macros */

#pragma GCC diagnostic push
#if defined(__clang__)
#else
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

/* Parse a single token */
static void ParseOneToken(pstate *ps) {
  char *x = MlnStrSafe(ps->token_start); /* save the token permanently */
#if TEST
  printf("%s:%d: Token=[%s] state=%d\n", ps->filename, ps->token_line, x,
         ps->state);
#endif
  switch (ps->state) {
  case MLN_PS_INITIALIZE:
    ps->prec_counter = 0;
    ps->prev_rule = NULL;
    ps->first_rule = NULL;
    ps->last_rule = NULL;
    ps->melon->nrule = 0;
    /* Fall through */

  case MLN_PS_WAITING_FOR_DECL_OR_RULE:
    if (x[0] == '%') {
      ps->state = MLN_PS_WAITING_FOR_DECL_KEYWORD;
    } else if (islower(x[0])) {
      ps->lhs = MlnSymbolNew(x);
      ps->rhs_count = 0;
      ps->lhs_alias = NULL;
      ps->state = MLN_PS_WAITING_FOR_ARROW;
    } else if (x[0] == '{') {
      if (ps->prev_rule == NULL) {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "There is not prior rule upon which to attach the code "
                    "fragment which begins on this line.");
        ps->error_cnt++;
      } else if (ps->prev_rule->code != NULL) {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "Code fragment beginning on this line is not the frist to "
                    "follow the previous rule.");
        ps->error_cnt++;
      } else {
        ps->prev_rule->line = ps->token_line;
        ps->prev_rule->code = &x[1];
      }
    } else if (x[0] == '[') {
      ps->state = MLN_PS_PRECEDENCE_MARK_1;
    } else {
      MlnErrorMsg(
          ps->filename, ps->token_line,
          "Token \"%s\" should be either \"%%\" or a non-terminal name.", x);
      ps->error_cnt++;
    }
    break;

  case MLN_PS_PRECEDENCE_MARK_1:
    if (!isupper(x[0])) {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "The precedence symbol must be a terminal.");
      ps->error_cnt++;
    } else if (ps->prev_rule == NULL) {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "There is no prior rule to assign precedence \"[%s]\".", x);
      ps->error_cnt++;
    } else if (ps->prev_rule->prec_sym != NULL) {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Precedence mark on this line is not the first to follow the "
                  "previous rule.");
      ps->error_cnt++;
    } else {
      ps->prev_rule->prec_sym = MlnSymbolNew(x);
    }
    ps->state = MLN_PS_PRECEDENCE_MARK_2;
    break;

  case MLN_PS_PRECEDENCE_MARK_2:
    if (x[0] != ']') {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Missing \"]\" on precedence mark.");
      ps->error_cnt++;
    }
    ps->state = MLN_PS_WAITING_FOR_DECL_OR_RULE;
    break;

  case MLN_PS_WAITING_FOR_ARROW:
    if (x[0] == ':' && x[1] == ':' && x[2] == '=') {
      ps->state = MLN_PS_IN_RHS;
    } else if (x[0] == '(') {
      ps->state = MLN_PS_LHS_ALIAS_1;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Expected to see a \":\" following the LHS symbol \"%s\".",
                  ps->lhs->name);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;

  case MLN_PS_LHS_ALIAS_1:
    if (isalpha(x[0])) {
      ps->lhs_alias = x;
      ps->state = MLN_PS_LHS_ALIAS_2;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "\"%s\" is not a valid alias for LHS \"%s\".", x,
                  ps->lhs->name);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;

  case MLN_PS_LHS_ALIAS_2:
    if (x[0] == ')') {
      ps->state = MLN_PS_LHS_ALIAS_3;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Missing \")\" following LHS alias name \"%s\".",
                  ps->lhs_alias);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;

  case MLN_PS_LHS_ALIAS_3:
    if (x[0] == ':' && x[1] == ':' && x[2] == '=') {
      ps->state = MLN_PS_IN_RHS;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Missing \"->\" following: \"%s(%s)\".", ps->lhs->name,
                  ps->lhs_alias);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case MLN_PS_IN_RHS:
    if (x[0] == '.') {
      MlnRule *rp;
      rp = malloc(sizeof(MlnRule) + sizeof(MlnSymbol *) * ps->rhs_count +
                  sizeof(char *) * ps->rhs_count);
      if (rp == NULL) {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "Can't allocate enough memory for this rule.");
        ps->error_cnt++;
        ps->prev_rule = NULL;
      } else {
        int i;
        rp->rule_line = ps->token_line;
        rp->rhs = (MlnSymbol **)&rp[1];
        rp->rhs_alias = (char **)&(rp->rhs[ps->rhs_count]);
        for (i = 0; i < ps->rhs_count; i++) {
          rp->rhs[i] = ps->rhs[i];
          rp->rhs_alias[i] = ps->alias[i];
        }
        rp->lhs = ps->lhs;
        rp->lhs_alias = ps->lhs_alias;
        rp->nrhs = ps->rhs_count;
        rp->code = NULL;
        rp->prec_sym = NULL;
        rp->index = ps->melon->nrule++;
        rp->next_lhs = rp->lhs->rule;
        rp->lhs->rule = rp;
        rp->next = 0;
        if (ps->first_rule == NULL) {
          ps->first_rule = ps->last_rule = rp;
        } else {
          ps->last_rule->next = rp;
          ps->last_rule = rp;
        }
        ps->prev_rule = rp;
      }
      ps->state = MLN_PS_WAITING_FOR_DECL_OR_RULE;
    } else if (isalpha(x[0])) {
      if (ps->rhs_count >= MLN_MAX_RHS) {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "Too many symbol on RHS or rule beginning at \"%s\".", x);
        ps->error_cnt++;
        ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
      } else {
        ps->rhs[ps->rhs_count] = MlnSymbolNew(x);
        ps->alias[ps->rhs_count] = NULL;
        ps->rhs_count++;
      }
    } else if (x[0] == '(' && ps->rhs_count > 0) {
      ps->state = MLN_PS_RHS_ALIAS_1;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Illegal character on RHS or rule: \"%s\".", x);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;

  case MLN_PS_RHS_ALIAS_1:
    if (isalpha(x[0])) {
      ps->alias[ps->rhs_count - 1] = x;
      ps->state = MLN_PS_RHS_ALIAS_2;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "\"%s\" is not a valid alias for the RHS symbol \"%s\".", x,
                  ps->rhs[ps->rhs_count - 1]->name);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;

  case MLN_PS_RHS_ALIAS_2:
    if (x[0] == ')') {
      ps->state = MLN_PS_IN_RHS;
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Missing \")\" following RHS alias name \"%s\".",
                  ps->alias[ps->rhs_count - 1]);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_RULE_ERROR;
    }
    break;

  case MLN_PS_WAITING_FOR_DECL_KEYWORD:
    if (isalpha(x[0])) {
      ps->decl_keyword = x;
      ps->decl_arg_slot = NULL;
      ps->decl_ln_slot = 0;
      ps->state = MLN_PS_WAITING_FOR_DECL_ARG;
      if (strcmp(x, "name") == 0) {
        ps->decl_arg_slot = &(ps->melon->name);
      } else if (strcmp(x, "include") == 0) {
        ps->decl_arg_slot = &(ps->melon->include);
        ps->decl_ln_slot = &(ps->melon->include_line);
      } else if (strcmp(x, "code") == 0) {
        ps->decl_arg_slot = &(ps->melon->extra_code);
        ps->decl_ln_slot = &(ps->melon->extra_code_line);
      } else if (strcmp(x, "token_destructor") == 0) {
        ps->decl_arg_slot = &(ps->melon->token_dest);
        ps->decl_ln_slot = &(ps->melon->token_dest_line);
      } else if (strcmp(x, "default_destructor") == 0) {
        ps->decl_arg_slot = &(ps->melon->var_dest);
        ps->decl_ln_slot = &(ps->melon->var_dest_line);
      } else if (strcmp(x, "token_prefix") == 0) {
        ps->decl_arg_slot = &(ps->melon->token_prefix);
      } else if (strcmp(x, "syntax_error") == 0) {
        ps->decl_arg_slot = &(ps->melon->error);
        ps->decl_ln_slot = &(ps->melon->error_line);
      } else if (strcmp(x, "parse_accept") == 0) {
        ps->decl_arg_slot = &(ps->melon->accept);
        ps->decl_ln_slot = &(ps->melon->accept_line);
      } else if (strcmp(x, "parse_failure") == 0) {
        ps->decl_arg_slot = &(ps->melon->failure);
        ps->decl_ln_slot = &(ps->melon->failure_line);
      } else if (strcmp(x, "stack_overflow") == 0) {
        ps->decl_arg_slot = &(ps->melon->overflow);
        ps->decl_ln_slot = &(ps->melon->overflow_line);
      } else if (strcmp(x, "extra_argument") == 0) {
        ps->decl_arg_slot = &(ps->melon->arg);
      } else if (strcmp(x, "token_type") == 0) {
        ps->decl_arg_slot = &(ps->melon->token_type);
      } else if (strcmp(x, "default_type") == 0) {
        ps->decl_arg_slot = &(ps->melon->var_type);
      } else if (strcmp(x, "stack_size") == 0) {
        ps->decl_arg_slot = &(ps->melon->stack_size);
      } else if (strcmp(x, "start_symbol") == 0) {
        ps->decl_arg_slot = &(ps->melon->start);
      } else if (strcmp(x, "left") == 0) {
        ps->prec_counter++;
        ps->decl_assoc = MLN_ASSOC_LEFT;
        ps->state = MLN_PS_WAITING_FOR_PRECEDENCE_SYMBOL;
      } else if (strcmp(x, "right") == 0) {
        ps->prec_counter++;
        ps->decl_assoc = MLN_ASSOC_RIGHT;
        ps->state = MLN_PS_WAITING_FOR_PRECEDENCE_SYMBOL;
      } else if (strcmp(x, "nonassoc") == 0) {
        ps->prec_counter++;
        ps->decl_assoc = MLN_ASSOC_NONE;
        ps->state = MLN_PS_WAITING_FOR_PRECEDENCE_SYMBOL;
      } else if (strcmp(x, "destructor") == 0) {
        ps->state = MLN_PS_WAITING_FOR_DESTRUCTOR_SYMBOL;
      } else if (strcmp(x, "type") == 0) {
        ps->state = MLN_PS_WAITING_FOR_DATATYPE_SYMBOL;
      } else if (strcmp(x, "fallback") == 0) {
        ps->fallback = NULL;
        ps->state = MLN_PS_WAITING_FOR_FALLBACK_ID;
      } else {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "Unknown declaration keyword: \"%%%s\".", x);
        ps->error_cnt++;
        ps->state = MLN_PS_RESYNC_AFTER_DECL_ERROR;
      }
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Illegal declaration keyword: \"%%%s\".", x);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_DECL_ERROR;
    }
    break;

  case MLN_PS_WAITING_FOR_DESTRUCTOR_SYMBOL:
    if (!isalpha(x[0])) {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Symbol name missing after %%destructor keyword.");
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_DECL_ERROR;
    } else {
      MlnSymbol *sym = MlnSymbolNew(x);
      ps->decl_arg_slot = &sym->destructor;
      ps->decl_ln_slot = &sym->destructor_line;
      ps->state = MLN_PS_WAITING_FOR_DECL_ARG;
    }
    break;

  case MLN_PS_WAITING_FOR_DATATYPE_SYMBOL:
    if (!isalpha(x[0])) {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Symbol name missing after %%type keyword.");
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_DECL_ERROR;
    } else {
      MlnSymbol *sym = MlnSymbolNew(x);
      ps->decl_arg_slot = &sym->data_type;
      ps->decl_ln_slot = 0;
      ps->state = MLN_PS_WAITING_FOR_DECL_ARG;
    }
    break;

  case MLN_PS_WAITING_FOR_PRECEDENCE_SYMBOL:
    if (x[0] == '.') {
      ps->state = MLN_PS_WAITING_FOR_DECL_OR_RULE;
    } else if (isupper(x[0])) {
      MlnSymbol *sym = MlnSymbolNew(x);
      if (sym->prec >= 0) {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "Symbol \"%s\" has already be given a precedence.", x);
        ps->error_cnt++;
      } else {
        sym->prec = ps->prec_counter;
        sym->assoc = ps->decl_assoc;
      }
    } else {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "Can't assign a precedence to \"%s\".", x);
      ps->error_cnt++;
    }
    break;

  case MLN_PS_WAITING_FOR_DECL_ARG:
    if (x[0] == '{' || x[0] == '\"' || isalnum(x[0])) {
      if (*ps->decl_arg_slot != NULL) {
        MlnErrorMsg(
            ps->filename, ps->token_line,
            "The argument \"%s\" to declaration \"%%%s\" is not the first.",
            x[0] == '\"' ? &x[1] : x, ps->decl_keyword);
        ps->error_cnt++;
        ps->state = MLN_PS_RESYNC_AFTER_DECL_ERROR;
      } else {
        *(ps->decl_arg_slot) = (x[0] == '\"' || x[0] == '{') ? &x[1] : x;
        if (ps->decl_ln_slot != NULL) {
          *ps->decl_ln_slot = ps->token_line;
        }
        ps->state = MLN_PS_WAITING_FOR_DECL_OR_RULE;
      }
    } else {
      MlnErrorMsg(ps->filename, ps->token_line, "Illegal argument to %%%s: %s",
                  ps->decl_keyword, x);
      ps->error_cnt++;
      ps->state = MLN_PS_RESYNC_AFTER_DECL_ERROR;
    }
    break;

  case MLN_PS_WAITING_FOR_FALLBACK_ID:
    if (x[0] == '.') {
      ps->state = MLN_PS_WAITING_FOR_DECL_OR_RULE;
    } else if (!isupper(x[0])) {
      MlnErrorMsg(ps->filename, ps->token_line,
                  "%%fallback argument \"%s\" should be a token.", x);
      ps->error_cnt++;
    } else {
      MlnSymbol *sym = MlnSymbolNew(x);
      if (ps->fallback == NULL) {
        ps->fallback = sym;
      } else if (sym->fallback != NULL) {
        MlnErrorMsg(ps->filename, ps->token_line,
                    "More than one fallback assigned to token %s", x);
        ps->error_cnt++;
      } else {
        sym->fallback = ps->fallback;
        ps->melon->has_fallback = 1;
      }
    }
    break;

  case MLN_PS_RESYNC_AFTER_RULE_ERROR: /* Fall through */
  case MLN_PS_RESYNC_AFTER_DECL_ERROR:
    if (x[0] == '.') {
      ps->state = MLN_PS_WAITING_FOR_DECL_OR_RULE;
    }
    if (x[0] == '%') {
      ps->state = MLN_PS_WAITING_FOR_DECL_KEYWORD;
    }
    break;
  }
}
#pragma GCC diagnostic pop

/*
 * Run the preprocessor over the input file text. The global variables
 * define_array[0] through define_array[define-cnt] contains the names
 * of all defined macros. This routine looks for "%ifdef" and "%ifndef"
 * and "%endif" and comments them out. Text in between is also commented
 * out as appropriate.
 *
 * TODO(mn): optimize un-exclude endif
 */
static void MlnPreprocessInput(char *z) {
  int i, j, k, n;
  int exclude = 0;
  int start = 0;
  int line_no = 1;
  int start_line_no = 0;

  for (i = 0; z[i] != '\0'; i++) {
    if (z[i] == '\n') {
      line_no++;
    }
    if (z[i] != '%' || (i > 0 && z[i - 1] != '\n')) {
      continue;
    }
    if (strncmp(&z[i], "%endif", 6) == 0 && isspace(z[i + 6])) {
      if (exclude) {
        exclude--;
        if (exclude == 0) {
          for (j = start; j < i; j++) {
            if (z[j] != '\n') {
              z[j] = ' ';
            }
          }
        }
      }
      for (j = i; z[j] != '\0' && z[j] != '\n'; j++) {
        z[j] = ' ';
      }
    } else if ((strncmp(&z[i], "%ifdef", 6) == 0 && isspace(z[i + 6])) ||
               (strncmp(&z[i], "%ifndef", 7) == 0 && isspace(z[i + 7]))) {
      if (exclude) {
        exclude++;
      } else {
        for (j = i + 7; isspace(z[j]); j++) {
        }
        for (n = 0; z[j + n] != '\0' && !isspace(z[j + n]); n++) {
        }
        exclude = 1;
        for (k = 0; k < define_cnt; k++) {
          if (strncmp(define_array[k], &z[j], n) == 0 &&
              strlen(define_array[k]) == n) {
            exclude = 0;
            break;
          }
        }
        if (z[i + 3] == 'n') {
          exclude = !exclude;
        }
        if (exclude) {
          start = i;
          start_line_no = line_no;
        }
      }
      for (j = i; z[j] != '\0' && z[j] != '\n'; j++) {
        z[j] = ' ';
      }
    }
  }
  if (exclude) {
    fprintf(stderr, "unterminated %%ifdef starting on line %d\n",
            start_line_no);
    exit(1);
  }
}

/*
 * This routine is called with the argument to each -D command-line option.
 * Add the macro defined to the define_array array.
 */
void MlnHandleDOption(char *z) {
  define_cnt++;
  define_array = realloc(define_array, sizeof(define_array[0]) * define_cnt);
  if (define_array == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }

  define_array[define_cnt - 1] = strdup(z);
  if (define_array[define_cnt - 1] == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  for (z = define_array[define_cnt - 1]; *z && *z != '='; z++) {
  }
  *z = '\0';
}

/*
 * In spite of its name, this function is really a scanner. It read
 * int the entire input file (all at once) then tokenizes it. Each
 * token is passed to the function "ParseOneToken" which builds
 * all the approprite data structures in the global state vector
 * "melon".
 */
void MlnParse(Melon *melon) {
  pstate ps;
  FILE *fp;
  char *buf;
  char *cp, *nextcp;
  int file_size;
  int line_no;
  int c;
  int start_line = 0;

  ps.melon = melon;
  ps.filename = melon->filename;
  ps.error_cnt = 0;
  ps.state = MLN_PS_INITIALIZE;
  ps.first_rule = NULL;

  /* Begin by reading the input file */
  fp = fopen(ps.filename, "rb");
  if (fp == NULL) {
    MlnErrorMsg(ps.filename, 0, "Can't open this file for reading.");
    melon->error_cnt++;
    return;
  }

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  rewind(fp);
  buf = malloc(file_size + 1);
  if (buf == NULL) {
    MlnErrorMsg(ps.filename, 0,
                "Can't allocate %d of memory to hold this file.",
                file_size + 1);
    melon->error_cnt++;
    return;
  }
  if (fread(buf, 1, file_size, fp) != file_size) {
    MlnErrorMsg(ps.filename, 0, "Can't read in all %d bytes of this file.",
                file_size);
    melon->error_cnt++;
    return;
  }
  fclose(fp);
  buf[file_size] = '\0';

  /* Make an initial pass through the file to handle %ifdef and %ifndef */
  MlnPreprocessInput(buf);

  /* Now scan the next of the input file */
  line_no = 1;
  for (cp = buf; (c = *cp) != '\0';) {
    /* Keep track of the line number */
    if (c == '\n') {
      line_no++;
    }
    /* Skip all white space */
    if (isspace(c)) {
      cp++;
      continue;
    }
    /* Skip C++ style comments */
    if (c == '/' && cp[1] == '/') {
      cp += 2;
      while ((c = *cp) != '\0' && c != '\n') {
        cp++;
      }
      continue;
    }
    /* Skip C style comments */
    if (c == '/' && cp[1] == '*') {
      cp += 2;
      while ((c = *cp) != '\0' && (c != '/' || cp[-1] != '*')) {
        if (c == '\n') {
          line_no++;
        }
        cp++;
      }
      if (c) {
        cp++;
      }
      continue;
    }
    ps.token_start = cp;     /* Mark the beginning of the token */
    ps.token_line = line_no; /* Line number on which token begins */
    /* String literals */
    if (c == '\"') {
      cp++;
      while ((c = *cp) != '\0' && c != '\"') {
        if (c == '\n') {
          line_no++;
        }
        cp++;
      }
      if (c == '\0') {
        MlnErrorMsg(ps.filename, start_line,
                    "String starting on this line is not terminated before the "
                    "end of the file.");
        ps.error_cnt++;
        nextcp = cp;
      } else {
        nextcp = cp + 1;
      }
    } else if (c == '{') {
      /* A block of C code */
      int level;
      cp++;
      for (level = 1; (c = *cp) != '\0' && (level > 1 || c != '}'); cp++) {
        if (c == '\n') {
          line_no++;
        } else if (c == '{') {
          level++;
        } else if (c == '}') {
          level--;
        } else if (c == '/' && cp[1] == '*') { /* Skip C comments */
          int prevc = 0;
          cp += 2;
          while ((c = *cp) != '\0' && (c != '/' || prevc != '*')) {
            if (c == '\n') {
              line_no++;
            }
            prevc = c;
            cp++;
          }
        } else if (c == '/' && cp[1] == '/') {
          /* Skip C++ sytle comments too */
          cp += 2;
          while ((c = *cp) != '\0' && c != '\n') {
            cp++;
          }
          if (c == '\n') {
            line_no++;
          }
        } else if (c == '\'' || c == '\"') {
          /* String a character literals */
          int start_char, prevc;
          start_char = c;
          prevc = 0;
          for (cp++; (c = *cp) != '\0' && (c != start_char || prevc == '\\');
               cp++) {
            if (c == '\n') {
              line_no++;
            }
            if (prevc == '\\') {
              prevc = 0;
            } else {
              prevc = c;
            }
          }
        }
      }
      if (c == '\0') {
        MlnErrorMsg(ps.filename, ps.token_line,
                    "C code starting on this line is not terminated before the "
                    "end of the file.");
        ps.error_cnt++;
        nextcp = cp;
      } else {
        nextcp = cp + 1;
      }
    } else if (isalnum(c)) {
      /* Identifiers */
      while ((c = *cp) != '\0' && (isalnum(c) || c == '_')) {
        cp++;
      }
      nextcp = cp;
    } else if (c == ':' && cp[1] == ':' && cp[2] == '=') {
      /* The operator "::=" */
      cp += 3;
      nextcp = cp;
    } else {
      /* All other (one character) operators */
      cp++;
      nextcp = cp;
    }

    c = *cp;
    *cp = '\0';         /* Null terminate the token */
    ParseOneToken(&ps); /* Parse the token */
    *cp = c;            /* Restore the buffer */
    cp = nextcp;
  }

  free(buf); /* Release the buffer after parsing */
  melon->rule = ps.first_rule;
  melon->error_cnt = ps.error_cnt;
}

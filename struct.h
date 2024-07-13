/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_STRUCT_H_
#define MELON_STRUCT_H_

#ifdef TEST
#define MLN_MAX_RHS 5
#else
#define MLN_MAX_RHS 1024
#endif

struct MlnState;
struct MlnConfig;

typedef enum { MLN_FALSE = 0, MLN_TRUE = 1 } MlnBoolean;

typedef enum MlnAssocType {
  MLN_ASSOC_LEFT,
  MLN_ASSOC_RIGHT,
  MLN_ASSOC_NONE,
  MLN_ASSOC_UNK,
} MlnAssocType;

typedef enum MlnActionState {
  MLN_SHIFT,
  MLN_ACCEPT,
  MLN_REDUCE,
  MLN_ERROR,
  MLN_CONFLICT,    /* Was a reduce, but part of a conflict */
  MLN_SH_RESOLVED, /* Was a shift. Precedence resolved conflict */
  MLN_RD_RESOLVED, /* Was a reduce. Precedence resolved conflict */
  NOT_USED,        /* Deleted by compression */
} MlnActionState;

/*
 * Symbols (terminals and non-terminals) of the grammer are stored
 * in the following.
 */
typedef struct MlnSymbol {
  char *name; /* Name of the symbol */
  int index;  /* Index number for this symbol */
  enum {
    MLN_SYM_TERMINAL,
    MLN_SYM_NON_TERMINAL,
  } type;               /* Symbols are all either terminals or non-terminals */
  struct MlnRule *rule; /* Linked list of rules of this (if an NT) */
  struct MlnSymbol *fallback; /* Fallback token while the token doesn't parse */
  int prec;                   /* Precedence if defined (-1 otherwise) */
  MlnAssocType assoc;         /* Associativity if predecence is defined */
  void *first_set;            /* First-set for all rules of this symbol */
  MlnBoolean lambda;          /* True if NT and can generate an empty string */
  char *destructor;           /* Code which executes whenever this symbol is
                               * popped from the stack during error processing */
  int destructor_line;        /* Line number of destructor code */
  char *data_type;            /* The data type of information held by this
                               * object. Only used if an NT */
  int data_type_num;          /* The data type number. In the parser, the value
                               * stack is a union. The .yy%d element of this union
                               * is the correct data type ofr this object. */
} MlnSymbol;

/*
 * Each production rule in the grammer is stored in the following
 * structure.
 */
typedef struct MlnRule {
  MlnSymbol *lhs;           /* Left-hand side of the rule */
  char *lhs_alias;          /* Alias for the LHS (NULL if none) */
  int rule_line;            /* Line nubmer for the rule */
  int nrhs;                 /* Number of RHS symbols */
  MlnSymbol **rhs;          /* The RHS symbols */
  char **rhs_alias;         /* An alias for each RHS symbol (NULL if none) */
  int line;                 /* Line number at which code begin */
  char *code;               /* The code executed when this rule is reduced */
  MlnSymbol *prec_sym;      /* Precedence symbol for this rule */
  int index;                /* An index number for this rule */
  MlnBoolean can_reduce;    /* True if this rule is ever reduced */
  struct MlnRule *next_lhs; /* Next rule with the same LHS */
  struct MlnRule *next;     /* Next rule in the global list */
} MlnRule;

/*
 * A followset propagation link indicates that the contents of one
 * configuration followset should be propagated to another whenever
 * the first changes.
 */
typedef struct MlnPLink {
  struct MlnConfig *config; /* The configuration to which linked */
  struct MlnPLink *next;    /* The next propagate link */
} MlnPLink;

/* A configuration is a production rule of the grammer together with
 * a mark (dot) showing how nuch of that rule has been processed so far.
 * Configuration also contain a follow-set which is a list of terminal
 * symbols which are allowed to immediately follow the end of the rule.
 * Every configuration is recorded as an instance of the following.
 */
typedef struct MlnConfig {
  MlnRule *rule;       /* The rule upon which the configuration is based */
  int dot;             /* The parse point */
  char *fws;           /* Follow-set for this configuration only */
  MlnPLink *fpl;       /* Follow-set forward propagation links */
  MlnPLink *bpl;       /* Follow-set backward propagation links */
  struct MlnState *st; /* Pointer to state which contains this */
  enum {
    MLN_COMPLETE,
    MLN_INCOMPLETE
  } status;               /* Used during followset and shift computations */
  struct MlnConfig *next; /* Next configuration in the state */
  struct MlnConfig *bp;   /* The next basis configuration */
} MlnConfig;

/*
 * Every shift or reduce operation is stored as one of the following
 */
typedef struct MlnAction {
  MlnSymbol *sym; /* The look-ahead symbol */
  MlnActionState type;
  union {
    struct MlnState *state; /* The new state, if a shift */
    struct MlnRule *rule;   /* The rule, if a reduce */
  } x;
  struct MlnAction *next;    /* Next action for this state */
  struct MlnAction *collide; /* Next action with the same hash */
} MlnAction;

/*
 * Each state of the generated parser's finite state machine
 * is encoded as an instance of the following structure.
 */
typedef struct MlnState {
  MlnConfig *bp;  /* The basis configurations for this state */
  MlnConfig *cfp; /* All configurations in this set */
  int index;      /* Sequencial number for this state */
  MlnAction *ap;  /* Array of actions for this state */
  int ntkn_act;   /* Number of actions on terminals */
  int nntkn_act;  /* Number of actions on non-terminals */
  int tkn_off;    /* yy_action[] offset for terminals */
  int ntkn_off;   /* yy_action[] offset for non-terminals */
  int dflt_act;   /* Default action */
} MlnState;

#define MLN_NO_OFFSET (-0x7FFFFFFF)

/*
 * The state vector for the entire parser generator is recorded as
 * follows.
 *
 *    SPECIAL DIRECTIVES  VARIABLE
 *    ----------------------------
 *    %name               name
 *    %extra_argument     arg
 *    %token_type         token_type
 *    %default_type       var_type
 *    %start_symbol       start
 *    %stack_size         stack_size
 *    %include            include
 *    %syntax_error       error
 *    %stack_overflow     overflow
 *    %parse_failure      failure
 *    %parse_accept       accept
 *    %code               extra_code
 *    %token_destructor   token_dest
 *    %default_destructor var_dest
 *    %token_prefix       token_prefix
 *    %fallback           has_fallback
 */
typedef struct Melon {
  MlnState **sorted;   /* Table of states sorted by state number */
  int nstate;          /* Number of states */
  MlnRule *rule;       /* List of all rules */
  int nrule;           /* Number of rules */
  MlnSymbol **symbols; /* Sorted array of pointers to symbols */
  int nsymbol;         /* Number of terminal and non-terminal symbols */
  int nterminal;       /* Number of terminal symbols */
  MlnSymbol *err_sym;  /* The error symbol */
  int error_cnt;       /* Number of errors */

  char *name;          /* Name of the generated parser */
  char *arg;           /* Declaration of the 3rd argument to parser */
  char *token_type;    /* Type of terminal symbols in the parser stack */
  char *var_type;      /* The default type of non-terminal symbols */
  char *start;         /* Name of the start symbol for the grammer */
  char *stack_size;    /* Size of the parser stack */
  char *include;       /* Code to put at the start of the C file */
  int include_line;    /* Line number for start of include code */
  char *error;         /* Code to execute when an error is seen */
  int error_line;      /* Line number for start of error code */
  char *overflow;      /* Code to execute on a stack overflow */
  int overflow_line;   /* Line number for start of overflow code */
  char *failure;       /* Code to execute on parser failure */
  int failure_line;    /* Line number for start of failure code */
  char *accept;        /* Code to execute when the parser eccepts */
  int accept_line;     /* Line number for start of the accept code */
  char *extra_code;    /* Code appended to the generated file */
  int extra_code_line; /* Line number for start of the extra code */
  char *token_dest;    /* Code to execute to destroy token data */
  int token_dest_line; /* Line number for token destroyer code */
  char *var_dest;      /* Code for the default non-terminal destructor */
  int var_dest_line;  /* Line number for default non-terminal destructor code */
  char *token_prefix; /* A prefix added to token names in the *.h file */
  int has_fallback;   /* True if any %fallback is seen in the grammer */

  char *filename;    /* Name of the input file */
  char *output_file; /* Name of the current output file */
  int nconflict;     /* Number of parsing conflicts */
  int table_size;    /* Szie of the parse tables */
  int basis_flag;    /* Print only basis configurations */
  char *argv0;       /* Name of the program */
} Melon;

#define MlnMemoryCheck(x)                                                      \
  if ((x) == NULL) {                                                           \
    extern void memory_error();                                                \
    memory_error();                                                            \
  }

#endif

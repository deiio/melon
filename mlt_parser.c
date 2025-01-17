/*
 * Driver template for the MELON parser generator.
 * The author disclaims copyright to this source code.
 */
/* First off, code is include which follows the "include" declaration
 * in the input file.
 */
#include <stdio.h>
%%
/*
 * Next is all token values, in a form suitable for use by makeheaders.
 * This section will be null unless melon is run with the -m switch.
 */
/*
 * These contains (all generated automatically by the parser generator)
 * specify the various kinds of tokens (terminals) that the parser
 * understands.
 *
 * Each symbol here is a terminal symbol in the grammar.
 */
%%
/*
 * Make sure the INTERFACE macro is defined.
 */
#ifndef INTERFACE
#define INTERFACE 1
#endif
/*
 * The next thing included is series of defines which control
 * various aspects of the generated parser.
 *    YYCODETYPE          is the data type used for storing terminal
 *                        and nonterminal numbers. "unsigned char" is
 *                        used if there are fewer than 250 terminals
 *                        and nonterminals. "int" is used otherwise.
 *    YYNOCODE            is a number of type YYCODETYPE which corresponds
 *                        to no legal terminal or nonterminal number. This
 *                        number is used to fill in empty slots of the hash
 *                        table.
 *    YYFALLBACK          If defined, this indicates that one or more tokens
 *                        have fall-back values which should be used if the
 *                        original value of the token will not parse.
 *    YYACTIONTYPE        is the data type used for storing terminal
 *                        and nonterminal numbers. "unsigned char" is used
 *                        if there are fewer than 250 rules and states
 *                        combines. "int" is used otherwise.
 *    ParseTOKENTYPE      is the data type used for minor tokens given
 *                        directly to the parser from the tokenizer.
 *    YYMINORTYPE         is the data type used for all minor tokens.
 *                        This is typically a union of many types, one of
 *                        which is ParseTOKENTYPE. The entry in the union
 *                        for base tokens is called "yy0".
 *    YYSTACKDEPTH        is the maxmium depth of the parser's stack.
 *    ParseARG_SDECL      A static variable declaration for the
 *    ParseARG_PDECL      A parameter declaration for the
 *    ParseARG_STORE      Code to store %extra_argument into
 *    ParseARG_FETCH      Code to extract %extra_argument from
 *    YYNSTATE            the combined number of states.
 *    YYNRULE             the number of rules in the grammer.
 *    YYERRORSYMBOL       is the code number of the error symbol. If not
 *                        defined, then to no error processing.
 */
%%
#define YY_NO_ACTION      (YYNSTATE + YYNRULE + 2)
#define YY_ACCEPT_ACTION  (YYNSTATE + YYNRULE + 1)
#define YY_ERROR_ACTION   (YYNSTATE + YYNRULE)

/*
 * Next are that tables used to determine what action to take based on
 * the current state and lookahead token. These tables are used to
 * implement functions that take a state number and lookahead value and
 * return an action integer.
 *
 * Suppose the action integer is N. Then the action is determine as
 * follows
 *
 *  0 <= N < YYNSTATE                 Shift N. That is, push the lookahead
 *                                    token onto the stack and goto state N.
 *
 *  YYNSTATE <= N < YYNSTATE+YYNRULE  Reduce by rule N - YYNSTATE.
 *
 *  N == YY_ERROR_ACTION              A syntax error has occurred.
 *
 *  N == YY_ACCEPT_ACTION             The parser accepts its input.
 *
 *  N == YY_NO_ACTION                 No such action. Denotes unused slots
 *                                    in the yy_action[] table.
 *
 *  The action table is constructed as a single large table named
 *  yy_action[]. Given state S and lookahead X, the action is computed
 *  as
 *
 *      yy_action[ yy_shift_ofst[S] + X ]
 *
 *  If the index value yy_shift_ofst[S]+X is out of range or if the
 *  value yy_lookahead[yy_sfhit_ofst[S]+X is not equal to X or if
 *  yy_shift_ofst[S] is equal to YY_SHIFT_USE_DFLT, it means that
 *  the action is not in the table and that yy_default[S] should be
 *  used instead.
 *
 *  The formula above is for compuating the action when the lookahead
 *  is a terminal symbol. If the lookahead is a non-terminal (as occurs
 *  after a reduce action) then the yy_reduce_ofst[] array is used in
 *  place of the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used
 *  in place of YY_SHIFT_USE_DFLT.
 *
 *  The following are the tables generated in this section:
 *
 *    yy_action[]       A single table containing all actions.
 *    yy_lookahead[]    A table containing the lookahead for each entry
 *                      in yy_action. Used to detect hash collisions.
 *    yy_shift_ofst[]   For each state, the offset into yy_action for
 *                      shifting terminals.
 *    yy_reduce_ofst[]  For each state, the offset into yy_action for
 *                      shifting non-terminals after a reduce.
 *    yy_default[]      Default action for each state.
 */
%%
#define YY_SZ_ACTTAB (sizeof(yy_action) / sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens. If a construct
 * like the following:
 *
 *      %fallback ID X Y Z.
 *
 * appears in the grammar, then ID becomes a fallback token for X, Y,
 * and Z. Whenever one of the tokens X, Y or Z is input to the parser
 * but it does not parse, the type of the token is changed to ID and
 * the parse is retried before an error is thrown.
 */
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
%%
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
 * parser's stack. Information sotred includes:
 *
 *    + The state number for the parser at this level of the stack.
 *
 *    + The value of the token stored at this level of the stack.
 *      (In other words, the "major" token)
 *
 *    + The semantic value stored at this level of the stack. This is
 *      the information used by the action routines in the grammar.
 *      It is sometime called the "minor" token.
 */
typedef struct {
  int state_no;       /* The state number */
  int major;          /* The major token value. This is the code
                         number for the token at this stack level */
  YYMINORTYPE minor;  /* The user-supplied minor token value. This
                         is the value of the token */
} yyStackEntry;

/* The state of the parser is completely contained in an instance of
 * the following structure */
typedef struct {
  int yyidx;                  /* Index of top element in stack */
  int yyerrcnt;               /* Shifts left before out of the error */
  ParseARG_SDECL              /* A place to hold %extra_argument */
  yyStackEntry yystack[YYSTACKDEPTH]; /* The parser's stack */
} yyParser;

#ifndef NDEBUG
static FILE *yyTraceFILE = NULL;
static const char *yyTracePrompt = NULL;

/* Turn parser tracing on by giving a stream to which to write the trace
 * and a prompt to preface each trace message. Tracing is truned off
 * by making either argument NULL.
 *
 *    + file is a FILE* to which trace output should be written.
 *      If NULL, then tracing is truned off.
 *    + prompt is a prefix string written at the beginning of every
 *      line of trace output. If NULL, then tracing is truned off.
 */
void ParseTrace(FILE *file, const char *prompt) {
  yyTraceFILE = file;
  yyTracePrompt = prompt;
  if (yyTraceFILE == NULL) {
    yyTracePrompt = NULL;
  } else if (yyTracePrompt == NULL) {
    yyTraceFILE = NULL;
  }
}

/* For tracing shifts, the names of all terminals and nonterminals
 * are required. The following table supplies these names
 */
static const char *yyTokenName[] = {
%%
};

/*
 * For tracing reduce actions, the names of all rules are requred.
 */
static const char *yyRuleName[] = {
%%
};
#endif /* NDEBUG */

/*
 * This function returns the symbolic name associated with a token
 * value.
 */
const char *PraseTokenName(int token_type) {
#ifndef NDEBUG
  if (token_type > 0 &&
      token_type < (sizeof(yyTokenName) / sizeof(yyTokenName[0]))) {
    return yyTokenName[token_type];
  } else {
    return "Unknown";
  }
#else
  return "";
#endif
}

/*
 * This function allocates a new parser.
 * The only argument is a pointer to a function which works like
 * malloc.
 *
 *    + alloc is a pointer to the function used to allocate memory.
 *    + returns a pointer to a parser. This pointer is used in
 *      subsequent calls to Parse and ParseFree.
 */
void *ParseAlloc(void *(alloc)(size_t)) {
  yyParser *parser = (yyParser *)alloc(sizeof(yyParser));
  if (parser != NULL) {
    parser->yyidx = -1;
  }
  return parser;
}

/*
 * The following function deletes the value associated with a
 * symbol. The symbol can be either a terminal or nonterminal.
 * "yymajor" is the symbol code, and "yyminor" is a pointer to
 * the value.
 */
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor) {
  switch (yymajor) {
    /* Here is inserted the actions which take place when a
     * terminal or non-terminal is destroyed. This can happen
     * when the symbol is popped from the stack during a
     * reduce or during error processing or when a parser is
     * being destroyed before it is finished parsing.
     *
     * Note: during a reduce, the only symbols destroyed are those
     * which appear on the RHS of the rule, but which are not used
     * inside the C code.
     */
%%
    default:
      break; /* If no destructor action specified: do nothing */
  }
}

/*
 * Pop the parser's stack once.
 *
 * If there is a destructor routine associated with the token which
 * is popped from the stack, then call it.
 *
 * Return the major token number for the symbol popped.
 */ static int yy_pop_parser_stack(yyParser *pParser) {
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if (pParser->yyidx < 0) {
    return 0;
  }
#ifndef NDEBUG
  if (yyTraceFILE != NULL) {
    fprintf(yyTraceFILE, "%sPopping %s\n",
        yyTracePrompt,
        yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/*
 * Deallocate and destroy a parser. Destructors are all called for
 * all stack elements before shutting the parser down.
 *
 *    + p is a pointer to the parser. This should be a pointer
 *      obtained from ParseAlloc.
 *    + free_proc is a pointer to a function used to reclaim memory
 *      obtained from malloc.
 */
void ParseFree(void *p, void (free_proc)(void*)) {
  yyParser *pParser = (yyParser *)p;
  if (pParser == NULL) {
    return;
  }
  while (pParser->yyidx >= 0) {
    yy_pop_parser_stack(pParser);
  }
  free_proc(pParser);
}

/*
 * Find the appropriate action for a parser given the terminal
 * lookahead token lookahead.
 *
 * If the lookahead token is YYNOCODE, then check to see if the
 * action is independent of the lookahead. It it is, return the
 * action, otherwise return YY_NO_ACTION.
 */
static int yy_find_shift_action(yyParser *pParser, int lookahead) {
  int state_no = pParser->yystack[pParser->yyidx].state_no;
  int i = yy_shift_ofst[state_no];
  if (i == YY_SHIFT_USE_DFLT) {
    return yy_default[state_no];
  }
  if (lookahead == YYNOCODE) {
    return YY_NO_ACTION;
  }
  i += lookahead;
  if (i < 0 || i >= YY_SZ_ACTTAB || yy_lookahead[i] != lookahead) {
#ifdef YYFALLBACK
    int fallback;
    if (lookahead < sizeof(yyFallback) / sizeof(yyFallback[0]) &&
        (fallback = yyFallback[lookahead]) != 0) {
#ifndef NDEBUG
      if (yyTraceFILE != NULL) {
        fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
            yyTracePrompt, yyTokenName[lookahead], yyTokenName[fallback]);
      }
#endif
      return yy_find_shift_action(pParser, fallback);
    }
#endif
    return yy_default[state_no];
  } else {
    return yy_action[i];
  }
}

/*
 * Find the appropriate action for a parser given the non-terminal
 * lookahead token lookahead.
 *
 * If the lookahead token is YYNOCODE, then check to see if the
 * action is independent of the lookahead. If it is, return the
 * action, otherwise return YY_NO_ACTION.
 */
static int yy_find_reduce_action(yyParser *pParser, int lookahead) {
  int state_no = pParser->yystack[pParser->yyidx].state_no;
  int i = yy_reduce_ofst[state_no];

  if (i == YY_REDUCE_USE_DFLT) {
    return yy_default[state_no];
  }
  if (lookahead == YYNOCODE) {
    return YY_NO_ACTION;
  }
  i += lookahead;
  if (i < 0 || i >= YY_SZ_ACTTAB || yy_lookahead[i] != lookahead) {
    return yy_default[state_no];
  } else {
    return yy_action[i];
  }
}

/*
 * Preform a shift action.
 */
static void yy_shift(yyParser *yypParser, int new_state, int major,
                     YYMINORTYPE *minor) {
  yyStackEntry *yytos;
  yypParser->yyidx++;
  if (yypParser->yyidx >= YYSTACKDEPTH) {
    ParseARG_FETCH;
    yypParser->yyidx--;
#ifndef NDEBUG
    if (yyTraceFILE != NULL) {
      fprintf(yyTraceFILE, "%sStack Overflow!\n", yyTracePrompt);
    }
#endif
    while (yypParser->yyidx >= 0) {
      yy_pop_parser_stack(yypParser);
    }
    /* Here code is inserted which will execute if the parser\
     * stack every overflows */
%%
    ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
    return;
  }

  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->state_no = new_state;
  yytos->major = major;
  yytos->minor = *minor;
#ifndef NDEBUG
  if (yyTraceFILE != NULL && yypParser->yyidx > 0) {
    int i;
    fprintf(yyTraceFILE, "%sShift %d\n", yyTracePrompt, new_state);
    fprintf(yyTraceFILE, "%sStack:", yyTracePrompt);
    for (i = 1; i <= yypParser->yyidx; i++) {
      fprintf(yyTraceFILE, " %s", yyTokenName[yypParser->yystack[i].major]);
    }
    fprintf(yyTraceFILE, "\n");
  }
#endif
}

/*
 * The following table contains information about every rule that
 * is used during the reduce.
 */
static struct {
  YYCODETYPE lhs;       /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;   /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
%%
};

static void yy_accept(yyParser*);  /* Forward declaration */

/*
 * Perform a reduce action and the shift that must immediately
 * follow the reduce.
 *
 *    + yypParser is the parser.
 *    + yyruleno is the number of the rule by which to reduce.
 */
static void yy_reduce(yyParser *yypParser, int yyruleno) {
  int yygoto;               /* The next state */
  int yyact;                /* The next action */
  YYMINORTYPE yygotominor;  /* The LHS of the rule reduced */
  yyStackEntry *yymsp;      /* The top of the parser's stack */
  int yysize;               /* Amount to pop the stack */

  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];

#ifndef NDEBUG
  if (yyTraceFILE != NULL &&
      yyruleno < sizeof(yyRuleName) / sizeof(yyRuleName[0])) {
    fprintf(yyTraceFILE, "%sReduce [%s].\n",
            yyTracePrompt, yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  switch (yyruleno) {
  /* Beginning here are the reduction cases. A typical example
   * follows:
   *  case 0:
   *  #line <lineno> <grammmarfile>
   *    { ... }   // User supplied code
   *  #line <lineno> <thisfile>
   *  break;
   *
   */
%%
  }
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yypParser, yygoto);
  if (yyact < YYNSTATE) {
    yy_shift(yypParser, yyact, yygoto, &yygotominor);
  } else if (yyact == YYNSTATE + YYNRULE + 1) {
    yy_accept(yypParser);
  }
}

/*
 * The following code executes when the parse fails.
 */
static void yy_parse_failed(yyParser *yypParser) {
  ParseARG_FETCH;
#ifndef NDEBUG
  if (yyTraceFILE != NULL) {
    fprintf(yyTraceFILE, "%sFail!\n", yyTracePrompt);
  }
#endif
  while (yypParser->yyidx >= 0) {
    yy_pop_parser_stack(yypParser);
  }
  /*
   * Here code is inserted which be executed whenever the
   * parser fails.
   */
%%
  ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
 * The following code executes when a syntax error first occors.
 */
static void yy_syntax_error(yyParser *yypParser, int yymajor,
    YYMINORTYPE yyminor) {
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
%%
  ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
 * The following code executes when the parser accepts.
 */
static void yy_accept(yyParser *yypParser) {
  ParseARG_FETCH;
#ifndef NDEBUG
  if (yyTraceFILE != NULL) {
    fprintf(yyTraceFILE, "%sAccept!\n", yyTracePrompt);
  }
#endif
  while (yypParser->yyidx >= 0) {
    yy_pop_parser_stack(yypParser);
  }
  /*
   * Here code is inserted which will be executed whenever the parser
   * accepts.
   */
%%
  ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
 * The main parser program.
 *
 *    + The first argument is a pointer to a structure obtained from
 *      "ParseAlloc" which describes the current state of the parser.
 *    + The second argument is the major token number.
 *    + The third argument is the minor token.
 *    + The fourth optional argument is whatever the user wants (and
 *      specified in the grammar) and is available for use by the
 *      action routine.
 */
void Parse(void *yyp, int yymajor, ParseTOKENTYPE yyminor ParseARG_PDECL) {
  YYMINORTYPE minor;
  int yyact;            /* The parser actions */
  int yyendofinput;     /* True if we are at the end of input */
  int yyerrorhit = 0;   /* Ture if yymajor has invoked an error */
  yyParser *yypParser;  /* THe parser */

  /* (re)initialiee the parser, if necessary */
  yypParser = (yyParser *) yyp;
  if (yypParser->yyidx < 0) {
    if (yymajor == 0) {
      return;
    }
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].state_no = 0;
    yypParser->yystack[0].major = 0;
  }
  minor.yy0 = yyminor;
  yyendofinput = (yymajor == 0);
  ParseARG_STORE;

#ifndef NDEBUG
  if (yyTraceFILE != NULL) {
    fprintf(yyTraceFILE, "%sInput %s\n", yyTracePrompt, yyTokenName[yymajor]);
  }
#endif

  do {
    yyact = yy_find_shift_action(yypParser, yymajor);
    if (yyact < YYNSTATE) {
      yy_shift(yypParser, yyact, yymajor, &minor);
      yypParser->yyerrcnt--;
      if (yyendofinput && yypParser->yyidx >= 0) {
        yymajor = 0;
      } else {
        yymajor = YYNOCODE;
      }
    } else if (yyact < YYNSTATE + YYNRULE) {
      yy_reduce(yypParser, yyact - YYNSTATE);
    } else if (yyact == YY_ERROR_ACTION) {
      int yymx;
#ifndef NDEBUG
      if (yyTraceFILE != NULL) {
        fprintf(yyTraceFILE, "%sSyntax Error!\n", yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /*
       * A syntax error has occurred.
       * The response to an error depends upon whether or not the
       * grammar defines an error token "ERROR".
       *
       * THis is what we do if the grammar does define ERROR:
       *
       *    * Call the %syntax_error function.
       *
       *    * Begin popping the stack until we enter a state where
       *      it is legal to shift the error symbol, then shift
       *      the error symbol.
       *
       *    * Set the error count to three.
       *
       *    * Begin accepting and shifting new tokens. No new error
       *      processing will occur until thress tokens have been
       *      shifted successfully.
       */
      if (yypParser->yyerrcnt < 0) {
        yy_syntax_error(yypParser, yymajor, minor);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if (yymx == YYERRORSYMBOL || yyerrorhit) {
#ifndef NDEBUG
        if (yyTraceFILE != NULL) {
          fprintf(yyTraceFILE, "%s Discard input token %s\n",
              yyTracePrompt, yyTokenName[yymajor]);
        }
#endif /* NDEBUG */
        yy_destructor(yymajor, &minor);
        yymajor = YYNOCODE;
      } else {
        while (yypParser->yyidx >= 0 && yymx != YYERRORSYMBOL &&
            (yyact = yy_find_shift_action(yypParser, YYERRORSYMBOL)) >=
            YYNSTATE) {
          yy_pop_parser_stack(yypParser);
        }
        if (yypParser->yyidx < 0 || yymajor == 0) {
          yy_destructor(yymajor, &minor);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        } else if (yymx != YYERRORSYMBOL) {
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser, yyact, YYERRORSYMBOL, &u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else /* YYERRORSYMBOL is not defined */
      /*
       * This is what we do if the grammar does not define ERROR.
       *
       *    * Report an error message, and throw away the input token.
       *
       *    * If the input token is $, then fail the parse.
       *
       * As before, subsequent error message are suppressed until
       * three input tokens have been successfully shifted.
       */
      if (yypParser->yyerrcnt <= 0) {
        yy_syntax_error(yypParser, yymajor, minor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor, &yyminor);
      if (yyendofinput) {
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif /* YYERRORSYMBOL */
    } else {
      yy_accept(yypParser);
      yymajor = YYNOCODE;
    }
  } while (yymajor != YYNOCODE && yypParser->yyidx >= 0);
}


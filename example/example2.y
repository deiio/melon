%include {
#include <stdio.h>
#include <stdlib.h>
#include "example2.h"

struct Token {
  int value;
  unsigned n;
};
}

%token_type {struct Token}
%default_type {struct Token}

%left PLUS MINUS.
%left DIVIDE TIMES.

%syntax_error {
  printf("Syntax error!\n");
}

program ::= expr(A). {
        printf("Result.value = %d\nResult.n = %u\n", A.value, A.n);
      }

expr(A) ::= expr(B) MINUS expr(C). {
        A.value = B.value - C.value;
        A.n = B.n + 1 + C.n + 1;
      }

expr(A) ::= expr(B) PLUS expr(C). {
        A.value = B.value + C.value;
        A.n = B.n + 1 + C.n + 1;
      }

expr(A) ::= expr(B) TIMES expr(C). {
        A.value = B.value * C.value;
        A.n = B.n + 1 + C.n + 1;
      }

expr(A) ::= expr(B) DIVIDE expr(C). {
        if (C.value != 0) {
          A.value = B.value / C.value;
          A.n = B.n + 1+ C.n + 1;
        } else {
          printf("divide by zero\n");
        }
      }

expr(A) ::= NUM(B). { A.value = B.value; A.n = B.n + 1; }


%code {
int main() {
  FILE *f = fopen("record.txt", "w");
  ParseTrace(f, "");
  void *parser = ParseAlloc(malloc);
  struct Token t0 = {4, 0}, t1 = {13, 0};

  printf("\t t0.value(4) PLUS t1.value(13)\n");
  Parse(parser, NUM, t0);
  Parse(parser, PLUS, t0);
  Parse(parser, NUM, t1);
  Parse(parser, 0, t0);

  ParseTrace(NULL, "");
  fclose(f);

  return 0;
}
}


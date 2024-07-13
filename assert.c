/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "assert.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * A more efficient way of handling assertions.
 */
void MlnAssert(const char *filename, int line) {
  fprintf(stderr, "Assertion failed on line %d of file \"%s\"\n", line,
          filename);
  exit(1);
}

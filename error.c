/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 *
 * Code for printing error message.
 */

#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
 * Find a good place to break "msg" so that its length is at least "min"
 * but no more than "max". Make the point as close to mas as possible.
 */
static int FindBreak(char *msg, int min, int max) {
  int spot = min;
  int i;
  char c;

  for (i = spot; i <= max; i++) {
    c = msg[i];
    if (c == '\t') {
      msg[i] = ' ';
    }
    if (c == '\n') {
      msg[i] = ' ';
      spot = i;
      break;
    }
    if (c == '\0') {
      spot = i;
      break;
    }
    if (c == '-' && i < max - 1) {
      spot = i + 1;
    }
    if (c == ' ') {
      spot = i;
    }
  }
  return spot;
}

/* Hope this is big enough. No way to error check */
const int kErrMsgSize = 10000;
/* Max width of any output line */
const int kLineWidth = 79;
/* Max width of the prefix on each line */
const int kPrefixLimit = 30;
/*
 * The error message is split across multiple lines if necessary. The
 * splits occur at a space, if there is a space available near the end
 * of the line.
 */
void MlnErrorMsg(const char *filename, int line, const char *fmt, ...) {
  char err_msg[kErrMsgSize];
  char prefix[kPrefixLimit + 10];
  int prefix_size;
  int available_width;
  int err_msg_size;
  int base, end, restart;
  va_list ap;

  va_start(ap, fmt);
  /* Prepare a prefix to be prepended to every output line. */
  if (line > 0) {
    sprintf(prefix, "%.*s:%d: ", kPrefixLimit, filename, line);
  } else {
    sprintf(prefix, "%.*s: ", kPrefixLimit, filename);
  }
  prefix_size = strlen(prefix);
  available_width = kLineWidth - prefix_size;

  /* Generate the error message. */
  vsprintf(err_msg, fmt, ap);
  va_end(ap);
  err_msg_size = strlen(err_msg);
  /* Remove trailing '\n's from the error message. */
  while (err_msg_size > 0 && err_msg[err_msg_size - 1] == '\n') {
    err_msg[--err_msg_size] = '\0';
  }

  /* Print the error message. */
  base = 0;
  while (err_msg[base] != '\0') {
    end = restart = FindBreak(&err_msg[base], 0, available_width);
    restart += base;
    while (err_msg[restart] == ' ') {
      restart++;
    }
    fprintf(stdout, "%s%.*s\n", prefix, end, &err_msg[base]);
    base = restart;
  }
}

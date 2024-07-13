/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#ifndef MELON_ASSERT_H_
#define MELON_ASSERT_H_

void MlnAssert(const char *filename, int line);

#ifndef NDEBUG
#undef assert
#define assert(X)                                                              \
  if (!(X))                                                                    \
  MlnAssert(__FILE__, __LINE__)
#else
#define assert(X)
#endif

#endif

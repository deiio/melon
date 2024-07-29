/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

/*
 * This file defines following macros about version:
 *
 *    - MLN_VERSION_MAJOR
 *    - MLN_VERSION_MINOR
 *    - MLN_VERSION_PATCH
 *    - MLN_VERSION
 *    - MLN_VERSION_NUMBER
 */

#ifndef MELON_VERSION_H_
#define MELON_VERSION_H_

#define MLN_VERSION_MAJOR 0
#define MLN_VERSION_MINOR 1
#define MLN_VERSION_PATCH 1

/* Helper macros */
#define MLN_CON(A, B) MLN_CON_(A, B)
#define MLN_CON_(A, B) A##.##B
#define MLN_STR(A) #A
#define MLN_AS_IT(A) MLN_STR(A)
#define MLN_VER_TMP                                                            \
  MLN_CON(MLN_CON(MLN_VERSION_MAJOR, MLN_VERSION_MINOR), MLN_VERSION_PATCH)

#define MLN_VERSION MLN_AS_IT(MLN_VER_TMP)

#define MLN_VERSION_NUMBER                                                     \
  ((MLN_VERSION_MAJON << 24) | (MLN_VERSION_MINOR << 16) | (MLN_VERSION_PATCH))

#endif

// Copyright (c) 2022 furzoom.com, All rights reserved.
// Author: Furzoom, mn@furzoom.com
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//     PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//     CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//     LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef CUTIO_CTEST_H_
#define CUTIO_CTEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Maximum length of last message
#define CUTIO_TEST_MESSAGE_LEN 1024
// Nanoseconds per second
#define CUTIO_TEST_NSECS 1E9
// Equivalence error
#define CUTIO_TEST_EPSILON 1E-12

// Counters
extern int cutio_unit_run;
extern int cutio_unit_assert;
extern int cutio_unit_fail;
// Status
extern int cutio_unit_status;

// Timers
typedef struct timespec timespec_t;
extern timespec_t cutio_unit_real_timer;
extern timespec_t cutio_unit_proc_timer;

// Last message
extern char cutio_last_message[CUTIO_TEST_MESSAGE_LEN];

// Test setup and teardown function pointers
extern void (*cutio_setup)();
extern void (*cutio_teardown)();

// Definitions of test and test suite
#define CU_TEST(method_name) static void method_name()
#define CU_TEST_SUITE(suite_name) static void suite_name()
#define CU_RUN_SUIT(suite_name)                                                \
  do {                                                                         \
    suite_name();                                                              \
    cutio_setup = NULL;                                                        \
    cutio_teardown = NULL;                                                     \
  } while (0)

// Configure setup and teardown functions
#define CU_SUITE_CONFIGURE(setup_func, teardown_func)                          \
  do {                                                                         \
    cutio_setup = setup_func;                                                  \
    cutio_teardown = teardown_func;                                            \
  } while (0)

// Assertions
#define CU_CHECK(test)                                                         \
  do {                                                                         \
    cutio_unit_assert++;                                                       \
    if (!(test)) {                                                             \
      snprintf(cutio_last_message, CUTIO_TEST_MESSAGE_LEN,                     \
               "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__,        \
               #test);                                                         \
      cutio_unit_status = 1;                                                   \
      return;                                                                  \
    } else {                                                                   \
      printf(".");                                                             \
    }                                                                          \
  } while (0)

#define CU_FAIL(message)                                                       \
  do {                                                                         \
    cutio_unit_assert++;                                                       \
    snprintf(cutio_last_message, CUTIO_TEST_MESSAGE_LEN,                       \
             "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__,          \
             message);                                                         \
    cutio_unit_status = 1;                                                     \
    return;                                                                    \
  } while (0)

#define CU_ASSERT(test, message)                                               \
  do {                                                                         \
    cutio_unit_assert++;                                                       \
    if (!(test)) {                                                             \
      snprintf(cutio_last_message, CUTIO_TEST_MESSAGE_LEN,                     \
               "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__,        \
               message);                                                       \
      cutio_unit_status = 1;                                                   \
      return;                                                                  \
    } else {                                                                   \
      printf(".");                                                             \
    }                                                                          \
  } while (0)

#define CU_ASSERT_EQ(expected, result)                                         \
  do {                                                                         \
    cutio_unit_assert++;                                                       \
    int e = (expected);                                                        \
    int r = (result);                                                          \
    if (e != r) {                                                              \
      snprintf(cutio_last_message, CUTIO_TEST_MESSAGE_LEN,                     \
               "%s failed:\n\t%s:%d: %d expected but was %d", __func__,        \
               __FILE__, __LINE__, e, r);                                      \
      cutio_unit_status = 1;                                                   \
      return;                                                                  \
    } else {                                                                   \
      printf(".");                                                             \
    }                                                                          \
  } while (0)

#define CU_ASSERT_DOUBLE_EQ(expected, result)                                  \
  do {                                                                         \
    cutio_unit_assert++;                                                       \
    double e = (expected);                                                     \
    double r = (result);                                                       \
    if (fabs(e - r) > CUTIO_TEST_EPSILON) {                                    \
      int s = 1 - log10(CUTIO_TEST_EPSILON);                                   \
      snprintf(cutio_last_message, CUTIO_TEST_MESSAGE_LEN,                     \
               "%s failed:\n\t%s:%d: %.*g expected but was %.*g", __func__,    \
               __FILE__, __LINE__, s, e, s, r);                                \
      cutio_unit_status = 1;                                                   \
      return;                                                                  \
    } else {                                                                   \
      printf(".");                                                             \
    }                                                                          \
  } while (0)

#define CU_ASSERT_STRING_EQ(expected, result)                                  \
  do {                                                                         \
    cutio_unit_assert++;                                                       \
    const char *e = "<null pointer>";                                          \
    const char *r = "<null pointer>";                                          \
    if (expected != NULL) {                                                    \
      e = (expected);                                                          \
    }                                                                          \
    if (result != NULL) {                                                      \
      r = (result);                                                            \
    }                                                                          \
    if (strcmp(e, r) != 0) {                                                   \
      snprintf(cutio_last_message, CUTIO_TEST_MESSAGE_LEN,                     \
               "%s failed:\n\t%s:%d: '%s' expected but was '%s'", __func__,    \
               __FILE__, __LINE__, e, r);                                      \
      cutio_unit_status = 1;                                                   \
      return;                                                                  \
    } else {                                                                   \
      printf(".");                                                             \
    }                                                                          \
  } while (0)

#define CU_RUN_TEST(test)                                                      \
  do {                                                                         \
    if (cutio_unit_real_timer.tv_sec == 0 &&                                   \
        cutio_unit_real_timer.tv_nsec == 0) {                                  \
      clock_gettime(CLOCK_MONOTONIC, &cutio_unit_real_timer);                  \
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cutio_unit_proc_timer);         \
    }                                                                          \
    if (cutio_setup) {                                                         \
      (*cutio_setup)();                                                        \
    }                                                                          \
    cutio_unit_status = 0;                                                     \
    test();                                                                    \
    cutio_unit_run++;                                                          \
    if (cutio_unit_status) {                                                   \
      cutio_unit_fail++;                                                       \
      printf("F");                                                             \
      printf("\n%s\n", cutio_last_message);                                    \
    }                                                                          \
    fflush(stdout);                                                            \
    if (cutio_teardown) {                                                      \
      (*cutio_teardown)();                                                     \
    }                                                                          \
  } while (0)

// 1 tests, 1 assertions, 0 failures
#define CU_REPORT()                                                            \
  do {                                                                         \
    printf("\n\n%d tests, %d assertions, %d failures\n", cutio_unit_run,       \
           cutio_unit_assert, cutio_unit_fail);                                \
    timespec_t end_real_timer;                                                 \
    timespec_t end_proc_timer;                                                 \
    clock_gettime(CLOCK_MONOTONIC, &end_real_timer);                           \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_proc_timer);                  \
    printf("\nFininshed in %.8f seconds (real) %.8f seconds (proc)\n\n",       \
           cutio_timer_diff(&end_real_timer, &cutio_unit_real_timer),          \
           cutio_timer_diff(&end_proc_timer, &cutio_unit_proc_timer));         \
  } while (0)

// Exit code
#define CU_EXIT_CODE cutio_unit_fail

// Utilities
static double cutio_timer_diff(timespec_t *end, timespec_t *start) {
  return ((end->tv_sec - start->tv_sec) * CUTIO_TEST_NSECS +
          (end->tv_nsec - start->tv_nsec)) /
         CUTIO_TEST_NSECS;
}

#ifdef __cplusplus
};
#endif

#endif // CUTIO_CTEST_H_

/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include <stdlib.h>

#include "option.h"
#include "test/melon_test.h"

CU_TEST(option_test_flag) {
  int version = 0;
  int rpflag = 0;
  int basis_flag = 0;
  int compress = 0;
  int quiet = 0;
  int statistics = 0;
  int mhflag = 0;

  char argv0[] = "melon_test";
  char argv1[] = "-x";
  char argv2[] = "-b";
  char *argv[] = {argv0, argv1, argv2, NULL};

  MlnOption options[] = {
      {MLN_OPT_FLAG, "b", (char *)&basis_flag,
       "Print only the basis in report."},
      {MLN_OPT_FLAG, "c", (char *)&compress,
       "Don't compress the action table."},
      {MLN_OPT_FLAG, "g", (char *)&rpflag, "Print grammer without actions."},
      {MLN_OPT_FLAG, "m", (char *)&mhflag,
       "Output a makeheaders compatible file."},
      {MLN_OPT_FLAG, "q", (char *)&quiet,
       "(Quiet) Don't print the report file."},
      {MLN_OPT_FLAG, "s", (char *)&statistics,
       "Print parser stats to standard output."},
      {MLN_OPT_FLAG, "x", (char *)&version, "Print the version number."},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };

  CU_CHECK(MlnOptInit(argv, options, stderr) == 0);
  CU_CHECK(version == 1);
  CU_CHECK(basis_flag == 1);
}

CU_TEST(option_test_switch) {
  int i = 0;
  double d = 0.0;
  char *s;

  char argv0[] = "melon_test";
  char argv1[] = "i=100";
  char argv2[] = "d=12.34";
  char argv3[] = "s=melon";

  char *argv[] = {argv0, argv1, argv2, argv3, NULL};

  MlnOption options[] = {
      {MLN_OPT_INT, "i", (char *)&i, "integer"},
      {MLN_OPT_DBL, "d", (char *)&d, "double"},
      {MLN_OPT_STR, "s", (char *)&s, "string"},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };

  CU_CHECK(MlnOptInit(argv, options, stderr) == 0);
  CU_ASSERT_EQ(100, i);
  CU_ASSERT_DOUBLE_EQ(12.34, d);
  CU_ASSERT_STRING_EQ("melon", s);
}

static int f_flag = 0;
static int f_int = 0;
static double f_double = 0.0;
static char f_string[20];

static void CheckFlag(int f) { f_flag = f; }
static void CheckInt(int n) { f_int = n; }
static void CheckDouble(double d) { f_double = d; }
static void CheckString(char *s) { strncpy(f_string, s, sizeof(f_string)); }

CU_TEST(option_test_function) {
  char argv0[] = "melon_test";
  char argv1[] = "-f";
  char argv2[] = "i=10";
  char argv3[] = "d=12.23";
  char argv4[] = "string=melon";
  char *argv[] = {argv0, argv1, argv2, argv3, argv4, NULL};

  MlnOption options[] = {
      {MLN_OPT_FFLAG, "f", (char *)&CheckFlag, "flag function"},
      {MLN_OPT_FINT, "i", (char *)&CheckInt, "int function"},
      {MLN_OPT_FDBL, "d", (char *)&CheckDouble, "double function"},
      {MLN_OPT_FSTR, "string", (char *)&CheckString, "string function"},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };

  CU_CHECK(MlnOptInit(argv, options, stderr) == 0);
  CU_ASSERT_EQ(1, f_flag);
  CU_ASSERT_EQ(10, f_int);
  CU_ASSERT_DOUBLE_EQ(12.23, f_double);
  CU_ASSERT_STRING_EQ("melon", f_string);
}

CU_TEST(option_test_print) {
  int flag = 0;
  int i = 0;
  char argv0[] = "melon_test";
  char argv1[] = "-f";
  char argv2[] = "i=10";
  char *argv[] = {argv0, argv1, argv2, NULL};
  char *filename = "text.txt";
  FILE *file = fopen(filename, "w+");
  long size = 0;
  char *buf;
  char *p;
  int lines = 0;

  CU_CHECK(file != NULL);
  remove(filename);

  MlnOption options[] = {
      {MLN_OPT_FLAG, "f", (char *)&flag, "flag"},
      {MLN_OPT_INT, "i", (char *)&i, "integer"},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };
  CU_ASSERT_EQ(0, MlnOptInit(argv, options, file));

  MlnOptPrint();

  size = ftell(file);
  buf = calloc(1, size + 1);

  CU_CHECK(buf != NULL);
  CU_ASSERT_EQ(0, fseek(file, 0, SEEK_SET));

  CU_ASSERT_EQ(size, fread(buf, 1, size, file));
  p = buf;
  while (p != NULL) {
    if (*p == '\n') {
      lines++;
    }
    p++;
  }
  CU_ASSERT_EQ(2, lines);

  free(buf);
  fclose(file);
}

CU_TEST(option_test_args) {
  int flag = 0;
  int i = 0;
  char argv0[] = "melon_test";
  char argv1[] = "-f";
  char argv2[] = "i=10";
  char argv3[] = "filename1";
  char argv4[] = "filename2";
  char *argv[] = {argv0, argv1, argv2, argv3, argv4, NULL};

  MlnOption options[] = {
      {MLN_OPT_FLAG, "f", (char *)&flag, "flag"},
      {MLN_OPT_INT, "i", (char *)&i, "integer"},
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };
  CU_ASSERT_EQ(0, MlnOptInit(argv, options, stderr));
  CU_ASSERT_EQ(2, MlnOptNArgs());
  CU_ASSERT_STRING_EQ("filename1", MlnOptArg(1));
  CU_ASSERT_STRING_EQ("filename2", MlnOptArg(2));
  CU_CHECK(NULL == MlnOptArg(3));
}

CU_TEST(option_test_error) {
  char argv0[] = "melon_test";
  char argv1[] = "-f";
  char argv2[] = "i=10";
  char *argv[] = {argv0, argv1, argv2, NULL};
  char *filename = "text.txt";
  FILE *file = fopen(filename, "w+");

  CU_CHECK(file != NULL);
  remove(filename);

  MlnOption options[] = {
      {MLN_OPT_FLAG, NULL, NULL, NULL},
  };
  CU_ASSERT_EQ(1, MlnOptInit(argv, options, file));

  fclose(file);
}

void MlnInitOptionTest() {
  CU_RUN_TEST(option_test_flag);
  CU_RUN_TEST(option_test_switch);
  CU_RUN_TEST(option_test_function);
  CU_RUN_TEST(option_test_print);
  CU_RUN_TEST(option_test_error);
}

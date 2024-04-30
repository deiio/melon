/*
 * Copyright (c) 2024 furzoom.com, All rights reserved.
 * Author: mn, mn@furzoom.com
 */

#include "test/cutio-ctest.h"
#include "test/melon_test.h"

void test_setup() {
  // Nothing
}

void test_teardown() {
  // Nothing
}

CU_TEST_SUITE(test_suite) {
  CU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MlnInitTest();
}

int main(int argc, char *argv[]) {
  CU_RUN_SUIT(test_suite);
  CU_REPORT();
  return CU_EXIT_CODE;
}

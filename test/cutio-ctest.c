
#include "test/cutio-ctest.h"

// Counters
int cutio_unit_run = 0;
int cutio_unit_assert = 0;
int cutio_unit_fail = 0;
// Status
int cutio_unit_status = 0;

// Timers
timespec_t cutio_unit_real_timer = {.tv_sec = 0, .tv_nsec = 0};
timespec_t cutio_unit_proc_timer = {.tv_sec = 0, .tv_nsec = 0};

// Last message
char cutio_last_message[CUTIO_TEST_MESSAGE_LEN];

// Test setup and teardown function pointers
void (*cutio_setup)() = NULL;
void (*cutio_teardown)() = NULL;

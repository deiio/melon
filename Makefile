# Melon Makefile
# Copyright (c) 2024 furzoom.com, All rights reserved.
# Author: mn, mn@furzoom.com

CFLAGS ?= -Wall -Werror
CCOPT = $(CFLAGS)
INCLUDES ?= -I.

PRGNAME = melon
TEST_BIN = melon_main

OBJ = action.o 		 	\
			acttab.o			\
			assert.o 			\
			build.o 			\
			configlist.o 	\
			error.o 			\
			msort.o 			\
			option.o 			\
			parse.o 			\
			plink.o 			\
			report.o 			\
			set.o 				\
			table.o

MAIN = main.o

TEST_OBJ = test/melon_main.o  \
					 test/melon_test.o  \
					 test/cutio-ctest/cutio-ctest.o \
					 test/option_test.o

all: CFLAGS += -O2 -DNDEBUG
all: $(PRGNAME)

debug: CFLAGS += -g
debug: $(PRGNAME) test

$(PRGNAME): $(OBJ) $(MAIN)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CCOPT) -o $@ $< $(INCLUDES)

$(TEST_OBJ): CFLAGS += -Wno-unused-function

%.o: 					struct.h
action.o: 		action.c action.h
acttab.o:		  acttab.c acttab.h
assert.o: 		assert.c assert.h
build.o:  		build.c build.h
configlist.o: configlist.c configlist.h
error.o:			error.c error.h
main.o:				main.c version.h
msort.o:			msort.c msort.h
option.o:			option.c option.h
parse.o:			parse.c parse.h
plink.o:			plink.c plink.h
report.o:			report.c report.h
set.o:				set.c set.h
table.o:			table.c table.h

test: $(TEST_OBJ) $(OBJ)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $^

.PHONY: clean test
clean:
	rm -rf $(PRGNAME) $(OBJ) $(MAIN) $(TEST_BIN) $(TEST_OBJ) *.o test/*.o *.dSYM


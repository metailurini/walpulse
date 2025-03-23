CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lsqlite3

SRC = main.c utils.c wal_parser.c page_analyzer.c db_utils.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/main.c tests/test_wal_parser.c tests/test_utils.c tests/test_page_analyzer.c tests/test_harness.c tests/test_db_utils.c
TEST_OBJ = $(TEST_SRC:.c=.o)
EXEC = walpulse
TEST_EXEC = run_tests

all: $(EXEC)

$(EXEC): $(OBJ)
	@$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_EXEC)
	@./$(TEST_EXEC)
	@$(MAKE) clean

$(TEST_EXEC): $(TEST_OBJ) utils.o wal_parser.o page_analyzer.o db_utils.o
	@$(CC) $(TEST_OBJ) utils.o wal_parser.o page_analyzer.o db_utils.o -o $(TEST_EXEC) $(LDFLAGS)

run: $(EXEC)
	@./$(EXEC) tests/testdata/test.db
	@$(MAKE) clean

clean:
	@rm -f *.o tests/*.o $(EXEC) $(TEST_EXEC)

.PHONY: all test run clean

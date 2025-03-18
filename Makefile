CC = gcc
CFLAGS = -Wall -g

SRC = main.c utils.c wal_parser.c page_analyzer.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/main.c tests/test_wal_parser.c tests/test_utils.c tests/test_page_analyzer.c tests/test_harness.c
TEST_OBJ = $(TEST_SRC:.c=.o)
EXEC = walpulse
TEST_EXEC = run_tests

all: $(EXEC)

$(EXEC): $(OBJ)
	@$(CC) $(OBJ) -o $(EXEC)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_EXEC)
	@./$(TEST_EXEC)
	@$(MAKE) clean

$(TEST_EXEC): $(TEST_OBJ) utils.o wal_parser.o page_analyzer.o
	@$(CC) $(TEST_OBJ) utils.o wal_parser.o page_analyzer.o -o $(TEST_EXEC)

run: $(EXEC)
	@./$(EXEC) test.db-wal
	@$(MAKE) clean

clean:
	@rm -f *.o tests/*.o $(EXEC) $(TEST_EXEC)

.PHONY: all test run clean

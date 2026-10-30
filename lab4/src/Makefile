# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -pthread

# Итоговые программы
TARGETS = par_min_max process_memory psn

all: $(TARGETS)

# Задание 1
par_min_max: par_min_max.c find_min_max.c utils.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -o $@ par_min_max.c find_min_max.c utils.c

# Задание 3
process_memory: process_memory.c
	$(CC) $(CFLAGS) -o $@ $<

# Задание с потоками (psn)
psn: psn.c parallel_sum_lib.c parallel_sum_lib.h utils.c utils.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ psn.c parallel_sum_lib.c utils.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean
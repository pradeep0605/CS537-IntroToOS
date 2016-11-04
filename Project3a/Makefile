CURR_DIR=$(PWD)
SRC=$(CURR_DIR)
SOURCES=$(SRC)/stats_client.c $(SRC)/stats.c $(SRC)/stats_server.c

GCC=gcc
FLAGS=-O2 -Wall -gdwarf-2 -Wall -Werror -lpthread

all: stats_client stats_server

stats_server: stats_server.c libstats.so
	@echo "$(notdir $^) --> $(notdir $@)"
	@$(GCC) $^ -o $@ $(FLAGS) -lstats -L.

stats_client: stats_client.c libstats.so
	@echo "$(notdir $^) --> $(notdir $@)"
	@$(GCC) $^ -o $@ $(FLAGS) -lrt -lstats -L.

libstats.so: stats.o
	@echo "$(notdir $^) --> $(notdir $@)"
	@$(GCC) -shared -o libstats.so stats.o

stats.o: stats.c stats.h
	@echo "$(notdir $^) --> $(notdir $@)"
	@$(GCC) -c -fpic $(FLAGS) stats.c

lint:
	@echo "Testing linter"
	@cp $(CURR_DIR)/../lint/CPPLINT.cfg $(SRC)/
	@$(CURR_DIR)/../lint/cpplint.py --extensions c,h $(SOURCES)
	@rm $(SRC)/CPPLINT.cfg

clean:
	@rm -rf stats_client stats_server stats.o libstats.so $(SRC)/CPPLINT.cfg

.PHONY:test

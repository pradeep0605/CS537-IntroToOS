CURR_DIR=$(PWD)
SRC=$(CURR_DIR)
SOURCES=$(SRC)/main.c

GCC=gcc
FLAGS=-O2 -Wall -gdwarf-2 -Wall -Werror -lpthread

all: fscheck 

fscheck: main.c 
	@echo "$(notdir $^) --> $(notdir $@)"
	@$(GCC) $^ -o $@ $(FLAGS) 

clean:
	@rm -rf fscheck

.PHONY:test

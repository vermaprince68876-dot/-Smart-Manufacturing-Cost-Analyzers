# Smart Manufacturing Cost Analyzer
#
#   make        build ./costanalyzer
#   make run    build and print the report
#   make clean  remove build output and the database
#
# SQLite is bundled in database/sqlite3.c, so there is nothing to install.

CXX      ?= g++
CC       ?= gcc
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -I. -Idatabase
CFLAGS   ?= -O2 -w -Idatabase
LDLIBS   ?= -lpthread -lxlsxwriter

BIN      := costanalyzer
CPP_SRC  := main.cpp $(wildcard services/*.cpp) $(wildcard database/*.cpp) $(wildcard reports/*.cpp)
CPP_OBJ  := $(CPP_SRC:.cpp=.o)
OBJ      := $(CPP_OBJ) database/sqlite3.o

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

database/sqlite3.o: database/sqlite3.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) report

clean:
	rm -f $(OBJ) $(BIN) factory.db

# Global variables.
PRJ_ROOT             := ../..
BASE_MODULES         := sys util
SUB_MAKEFILES        := $(addsuffix /module.mk,$(addprefix $(PRJ_ROOT)/base/,$(BASE_MODULES)))

# Functions
subdirectory = $(patsubst %/module.mk,%, \
	$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# Targets
SOURCES              := $(wildcard ./src/*.cpp)
OBJECTS              := $(subst /src/,/obj/,$(subst .cpp,.o,$(SOURCES)))
LIBRARIES            := 
BINARIES             := ./bin/testLogger
DEPEND_INC           := $(addprefix -I$(PRJ_ROOT)/base/,$(addsuffix /inc,$(BASE_MODULES))) -I./inc
LINK_LIBS            := -lsys -lutil -lpthread

# System variables.
ARCH                 := $(shell arch)
OS                   := $(shell uname)

# Global variables.
NOTICE_COLOR         := 2
COMMAND_COLOR        := 4

# Complier and linker
COMPLIER             := g++ -std=c++11
CPPFLAGS             := -g -Wall -Wextra -Werror -DDEBUG
LINKER               := g++
LDFLAGS              := -Wl,-rpath,$(PRJ_ROOT)/libs -L$(PRJ_ROOT)/libs

# Commands
RM                   := rm -rf
SED                  := sed
MV                   := mv
AR                   := ar

.PHONY: all
all:

include $(SUB_MAKEFILES)

all: $(OBJECTS) $(LIBRARIES) $(BINARIES)

./bin/testLogger: ./obj/testLogger.o $(LIBRARIES)
	@tput setaf $(NOTICE_COLOR); echo "Building binary $@"; tput setaf $(COMMAND_COLOR)
	$(LINKER) $(LDFLAGS) $< -o $@ $(LINK_LIBS)
	@echo; tput setaf 0

./obj/%.o: ./src/%.cpp
	@tput setaf $(NOTICE_COLOR); echo "Building source $^"; tput setaf $(COMMAND_COLOR)
	$(COMPLIER) $(CPPFLAGS) $(DEPEND_INC) -c $^ -o $@
	@echo; tput setaf 0

.PHONY: clean
clean:
	@rm -f $(OBJECTS) $(LIBRARIES) $(BINARIES)

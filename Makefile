# Functions
subdirectory = $(patsubst %/module.mk,%, \
	$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# Targets
OBJECTS              :=
LIBRARIES            :=
BINARIES             :=

# System variables.
ARCH                 := $(shell arch)
OS                   := $(shell uname)

# Global variables.
PRJ_ROOT             := .
SUB_MAKEFILES        := $(shell find . -name module.mk)
NOTICE_COLOR         := 2
COMMAND_COLOR        := 4

# Complier and linker
CCACHE               := ccache 
ifeq (, $(shell command -v ccache))
CCACHE               :=
endif

COMPLIER             := $(CCACHE) g++ -std=c++11
ifdef RELEASE
    CPPFLAGS         := -Wall -Wextra -Werror -O3 -Wnon-virtual-dtor -fno-strict-aliasing -DRELEASE
else
    CPPFLAGS         := -g -Wall -Wextra -Werror -Wnon-virtual-dtor -fno-strict-aliasing -DDEBUG
endif
LINKER               := g++
LDFLAGS              := -Wl,-rpath,$(PRJ_ROOT)/lib -L$(PRJ_ROOT)/lib

# Commands
RM                   := rm -rf
SED                  := sed
MV                   := mv
AR                   := ar

# Set the default goal, or the included makefile will change it.
.DEFAULT_GOAL        := all

include Platform.mk $(SUB_MAKEFILES)

.PHONY: all
all: $(OBJECTS) $(LIBRARIES) $(BINARIES)

.PHONY: clean
clean:
	@find $(PRJ_ROOT) -name '*.o' -exec $(RM) {} \;
	@find $(PRJ_ROOT) -name '*~' -exec $(RM) {} \;
	@$(RM) $(PRJ_ROOT)/lib/* $(PRJ_ROOT)/bin/* $(PRJ_ROOT)/project/*/lib/* $(PRJ_ROOT)/project/*/bin/*

.PHONY: dist
dist:
	@echo Currently not supported.

.PHONY: help
help:
	@echo "make all       - Build all projects."
	@echo "make clean     - Remove object files, generated libraries, binaries and emacs tmp files."
	@echo "make dist      - To create a distribution file."

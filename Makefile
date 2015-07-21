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
COMPLIER             := g++ -std=c++11
ifdef RELEASE
    COMPLIER_OPTIONS := -Wall -Wextra -Werror -O3 -DRELEASE
else
    COMPLIER_OPTIONS := -g -Wall -Wextra -Werror -DDEBUG
endif
LINKER               := g++ -std=c++11
LINKER_OPTIONS       := -Wl,-rpath,$(PROJECT_ROOT)/libs

# Commands
RM                   := rm -rf
SED                  := sed
MV                   := mv
AR                   := ar

help:
	@echo
	@echo "make all       - Build all projects."
	@echo "make <proj>    - Build project 'proj' which is the directory name in projects directory."
	@echo "make clean     - Remove object files, generated libraries, binaries and emacs tmp files."
	@echo "make tarball   - To create a release tarball."

include $(SUB_MAKEFILES)

.PHONY: all
all: $(OBJECTS) $(LIBRARIES) $(BINARIES)

.PHONY: clean
clean:
	@find $(PRJ_ROOT) -name '*.o' -exec $(RM) {} \;
	@find $(PRJ_ROOT) -name '*~' -exec $(RM) {} \;
	@$(RM) $(PRJ_ROOT)/libs/* $(PRJ_ROOT)/bin/*

.PHONY: tarball
tarball:
	@echo tarball

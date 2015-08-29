MODULE := $(shell basename $(subdirectory))

$(MODULE)_DIR         := $(subdirectory)
$(MODULE)_TARGET      := $(PRJ_ROOT)/libs/lib$(MODULE).a
$(MODULE)_DEP_COMP    :=
$(MODULE)_DEP_BASE    := util
$(MODULE)_3RD_PARTY   := openssl

$(MODULE)_SRC         := $(wildcard $($(MODULE)_DIR)/src/*.cpp)
$(MODULE)_INC         := $($(MODULE)_DIR)/inc
$(MODULE)_OBJ         := $(subst /src/,/obj/,$(subst .cpp,.o,$($(MODULE)_SRC)))

# Get the inc directory of modules in $Project/base directory.
BASE_INC              := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/base/,$($(MODULE)_DEP_BASE)))

# Get the inc directory of modules in $Project/component directory.
COMP_INC              := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/component/,$($(MODULE)_DEP_COMP)))

THIRD_PARTY_INC       := $($(addsuffix _INC, \
							$(shell echo $($(MODULE)_3RD_PARTY) | tr a-z A-Z)))

# Join depend include directory.
$(MODULE)_INC         += $(COMP_INC) $(BASE_INC) $(THIRD_PARTY_INC)

# Add '-I ' for complier.
$(MODULE)_INC         := $(addprefix -I ,$($(MODULE)_INC))

# Add this module to targets.
OBJECTS               += $($(MODULE)_OBJ)
LIBRARIES             += $($(MODULE)_TARGET)

$($(MODULE)_DIR)/obj/%.o: LOCAL_CPPFLAGS := $($(MODULE)_CPPFLAGS)
$($(MODULE)_DIR)/obj/%.o: LOCAL_INCLUDE  := $($(MODULE)_INC)
$($(MODULE)_DIR)/obj/%.o: $($(MODULE)_DIR)/src/%.cpp
	@tput setaf $(NOTICE_COLOR); echo "Building source $^"; tput setaf $(COMMAND_COLOR)
	$(COMPLIER) $(CPPFLAGS) $(LOCAL_INCLUDE) -c $^ -o $@
	@echo; tput setaf 0

$($(MODULE)_TARGET): LOCAL_TARGET := $($(MODULE)_TARGET)
$($(MODULE)_TARGET): $($(MODULE)_OBJ)
	@tput setaf $(NOTICE_COLOR); echo "Building library $(LOCAL_TARGET)"; tput setaf $(COMMAND_COLOR)
	$(AR) -rcv $@ $^
	@echo; tput setaf 0

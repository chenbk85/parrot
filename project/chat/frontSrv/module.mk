MODULE := $(shell basename $(subdirectory))

$(MODULE)_DIR         := $(subdirectory)
$(MODULE)_TARGET      := $(PRJ_ROOT)/bin/$(MODULE)
$(MODULE)_DEP_PRJ     :=
$(MODULE)_DEP_COMP    := servergears websocket json 
$(MODULE)_DEP_BASE    := sys util
$(MODULE)_3RD_PARTY   := openssl rapidjson
$(MODULE)_OTHER_LIB   := -lpthread -ldl

$(MODULE)_SRC         := $(wildcard $($(MODULE)_DIR)/src/*.cpp)
$(MODULE)_INC         := $($(MODULE)_DIR)/inc
$(MODULE)_OBJ         := $(subst /src/,/obj/,$(subst .cpp,.o,$($(MODULE)_SRC)))

# Get the inc directory of modules in $Project/base directory.
BASE_INC              := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/base/,$($(MODULE)_DEP_BASE)))

# Get the inc directory of modules in $Project/component directory.
COMP_INC              := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/component/,$($(MODULE)_DEP_COMP)))

# Get the inc directory of modules in $Project/yourPrj directory.
THIS_PRJ_INC          := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/chat/,$($(MODULE)_DEP_PRJ)))

THIRD_PARTY_DEP       := $(shell echo $($(MODULE)_3RD_PARTY) | tr a-z A-Z)
THIRD_PARTY_INC       := $(addsuffix _INC, THIRD_PARTY_DEP)
THIRD_PARTY_INC       := $(foreach VA,$(THIRD_PARTY_INC),$($(VA)))
THIRD_PARTY_LIB       := $(addsuffix _LIB, $(THIRD_PARTY_DEP))
THIRD_PARTY_LIB       := $(foreach VA,$(THIRD_PARTY_LIB),$($(VA)))

# Join depend include directory.
$(MODULE)_INC         += $(THIS_PRJ_INC) $(COMP_INC) $(BASE_INC) $(THIRD_PARTY_INC)

# Add '-I ' for complier.
$(MODULE)_INC         := $(addprefix -I ,$($(MODULE)_INC))

# Get all libraries.
$(MODULE)_DEP_LIB     := $($(MODULE)_DEP_PRJ) $($(MODULE)_DEP_COMP) $($(MODULE)_DEP_BASE)
$(MODULE)_LINK_LIB    := $(addprefix -l, $($(MODULE)_DEP_LIB))
$(MODULE)_DEP_LIB     := $(addsuffix .a, $(addprefix $(PRJ_ROOT)/lib/lib,$($(MODULE)_DEP_LIB)))

# Add this module to targets.
OBJECTS               += $($(MODULE)_OBJ)
BINARIES              += $($(MODULE)_TARGET)

$($(MODULE)_DIR)/obj/%.o: LOCAL_INCLUDE := $($(MODULE)_INC)
$($(MODULE)_DIR)/obj/%.o: $($(MODULE)_DIR)/src/%.cpp
	@tput setaf $(NOTICE_COLOR); echo "Building source $<"; tput setaf $(COMMAND_COLOR)
	$(COMPLIER) $(CPPFLAGS) $(LOCAL_INCLUDE) -c $< -o $@
	@echo; tput setaf 0

$($(MODULE)_TARGET): LOCAL_TARGET    := $($(MODULE)_TARGET)
$($(MODULE)_TARGET): LOCAL_OTHER_LIB := $($(MODULE)_OTHER_LIB)
$($(MODULE)_TARGET): LOCAL_3RD_LIB   := $(THIRD_PARTY_LIB)
$($(MODULE)_TARGET): $($(MODULE)_DEP_LIB)
$($(MODULE)_TARGET): $($(MODULE)_OBJ)
	@tput setaf $(NOTICE_COLOR); echo "Building binary $(LOCAL_TARGET)"; tput setaf $(COMMAND_COLOR)
	$(LINKER) $(LDFLAGS) $^ -o $@ $(LOCAL_3RD_LIB) $(LOCAL_OTHER_LIB)
	@echo; tput setaf 0

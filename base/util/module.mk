MODULE := $(shell basename $(subdirectory))

$(MODULE)_DIR         := $(subdirectory)
$(MODULE)_TARGET      := $(PRJ_ROOT)/libs/lib$(MODULE).a
$(MODULE)_CPPFLAGS    :=
$(MODULE)_LDFLAGS     :=
$(MODULE)_DEP_COMP    :=
$(MODULE)_DEP_BASE    := util sys

$(MODULE)_SRC         := $(wildcard $($(MODULE)_DIR)/src/*.cpp)
$(MODULE)_INC         := $($(MODULE)_DIR)/inc
$(MODULE)_OBJ         := $(subst /src/,/obj/,$(subst .cpp,.o,$($(MODULE)_SRC)))

# Get the inc directory of modules in $Project/base directory.
BASE_INC              := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/base/,$($(MODULE)_DEP_BASE)))

# Get the inc directory of modules in $Project/component directory.
COMP_INC              := $(addsuffix /inc,\
							$(addprefix $(PRJ_ROOT)/component/,$($(MODULE)_DEP_COMP)))

# Join depend include directory.
$(MODULE)_INC         += $(COMP_INC) $(BASE_INC)

# Add '-I ' for complier.
$(MODULE)_INC         := $(addprefix -I ,$($(MODULE)_INC))

# Add this module to targets.
TARGETS               += $($(MODULE)_TARGET)

$($(MODULE)_DIR)/obj/%.o: $($(MODULE)_DIR)/src/%.cpp
	$(COMPLIER) $(COMPLIER_OPTIONS) $($(MODULE)_CPPFLAGS) $($(MODULE)_INC) -c $^ -o $@

$($(MODULE)_TARGET): $($(MODULE)_OBJ)
	$(AR) -rcv $@ $^

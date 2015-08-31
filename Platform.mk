uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')

THIRD_PARTY_DIR := $(PRJ_ROOT)/third-party

ifeq ($(uname_S), mingw64_win)

else
OPENSSL_INC := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/openssl/include
OPENSSL_LIB := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/openssl/lib

RAPIDJSON_INC := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/rapidjson/include
endif

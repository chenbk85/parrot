uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')

THIRD_PARTY_DIR   := $(PRJ_ROOT)/third-party

ifeq ($(uname_S), mingw64_win)

# Rapidjson
CPPFLAGS          += -DRAPIDJSON_HAS_STDSTRING -DRAPIDJSON_SSE2 -DRAPIDJSON_SSE42
endif

ifeq ($(uname_S), Linux)

# Openssl
OPENSSL_INC       := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/openssl/include
OPENSSL_LIB       := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/openssl/lib
OPENSSL_LIB       := -Wl,-rpath,$(OPENSSL_LIB) -L$(OPENSSL_LIB) -lssl -lcrypto

# Rapidjson
RAPIDJSON_INC     := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/rapidjson/include
CPPFLAGS          += -DRAPIDJSON_HAS_STDSTRING -DRAPIDJSON_SSE2
endif

ifeq ($(uname_S), Darwin)

COMPLIER          := /Users/leopold/Workspace/tools/llvm/bin/clang++ -stdlib=libc++ -std=c++11 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk

LINKER            := /Users/leopold/Workspace/tools/llvm/bin/clang++

# Openssl
OPENSSL_INC       := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/openssl/include
OPENSSL_LIB       := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/openssl/lib
OPENSSL_LIB       := -Wl,-rpath,$(OPENSSL_LIB) -L$(OPENSSL_LIB) -lssl -lcrypto

# Rapidjson
RAPIDJSON_INC     := $(THIRD_PARTY_DIR)/$(uname_S).$(uname_M)/rapidjson/include
CPPFLAGS          += -DRAPIDJSON_HAS_STDSTRING -DRAPIDJSON_SSE2

# For clang
ifndef RELEASE
#CPPFLAGS          += -fsanitize=address -fno-omit-frame-pointer
#LDFLAGS           += -fsanitize=address -fno-omit-frame-pointer
endif

endif

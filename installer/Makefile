# Standalone ELF payload. Run fetch-deps.sh first.
ROOT := ..
DEPS ?= $(ROOT)/.deps/installer
MBED := $(DEPS)/mbedtls-3.6.7
PS5_PAYLOAD_SDK ?= $(if $(wildcard $(ROOT)/.deps/native/ps5-payload-sdk/bin/prospero-clang),$(ROOT)/.deps/native/ps5-payload-sdk,/opt/ps5-payload-sdk)
CC := $(PS5_PAYLOAD_SDK)/bin/prospero-clang
CXX := $(PS5_PAYLOAD_SDK)/bin/prospero-clang++
AR := $(PS5_PAYLOAD_SDK)/bin/prospero-ar
OUT := $(ROOT)/build/installer
OBJ := $(OUT)/obj
INCS := -Isrc -Iconfig -I$(MBED)/include -I$(ROOT)/third_party -I$(ROOT)/examples/update-check -I$(ROOT)/examples/self-update
DEFS := -DMBEDTLS_USER_CONFIG_FILE='"mbedtls_user_config.h"'
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections $(DEFS) $(INCS)
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections $(DEFS) $(INCS)
MINIZ_CFLAGS := -std=c11 -O2 -w -DMINIZ_NO_STDIO -DMINIZ_NO_TIME -DMINIZ_NO_ARCHIVE_WRITING_APIS -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES

C_SOURCES := util net tls http json github fsx log plat_ps5
C_OBJECTS := $(addprefix $(OBJ)/,$(addsuffix .o,$(C_SOURCES)))
CXX_SOURCES := install main
CXX_OBJECTS := $(addprefix $(OBJ)/,$(addsuffix .o,$(CXX_SOURCES)))
SHARED_CXX := $(OBJ)/archive.o $(OBJ)/files.o $(OBJ)/updater.o
SHARED_C := $(OBJ)/update_check.o $(OBJ)/self_update_sha256.o
MINIZ_NAMES := miniz miniz_tinfl miniz_tdef miniz_zip
MINIZ_OBJECTS := $(addprefix $(OBJ)/,$(addsuffix .o,$(MINIZ_NAMES)))
MBED_SRC := $(wildcard $(MBED)/library/*.c)
MBED_OBJECTS := $(patsubst $(MBED)/library/%.c,$(OBJ)/mbedtls/%.o,$(MBED_SRC))
OUTPUT := $(OUT)/PSX1Installer.elf

.PHONY: ps5
ps5: $(OUTPUT)

$(OBJ) $(OBJ)/mbedtls:
	mkdir -p $@

$(OBJ)/%.o: src/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/%.o: src/%.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/archive.o: $(ROOT)/examples/self-update-helper/archive.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/files.o: $(ROOT)/examples/self-update-helper/files.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/updater.o: $(ROOT)/examples/self-update-helper/updater.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/update_check.o: $(ROOT)/examples/update-check/update_check.c | $(OBJ)
	$(CC) $(CFLAGS) -DUPDATE_CHECK_NO_NETWORK -c -o $@ $<

$(OBJ)/self_update_sha256.o: $(ROOT)/examples/self-update/self_update_sha256.c | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/miniz%.o: $(ROOT)/third_party/miniz/miniz%.c | $(OBJ)
	$(CC) $(MINIZ_CFLAGS) -c -o $@ $<

$(OBJ)/miniz.o: $(ROOT)/third_party/miniz/miniz.c | $(OBJ)
	$(CC) $(MINIZ_CFLAGS) -c -o $@ $<

$(OBJ)/mbedtls/%.o: $(MBED)/library/%.c | $(OBJ)/mbedtls
	$(CC) $(CFLAGS) -I$(MBED)/library -c -o $@ $<

$(OBJ)/cacert_data.c: ca/cacert.pem embed-ca.py | $(OBJ)
	python3 embed-ca.py $< $@

$(OBJ)/cacert_data.o: $(OBJ)/cacert_data.c | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/libmbedtls.a: $(MBED_OBJECTS)
	$(AR) rcs $@ $^

$(OUTPUT): $(C_OBJECTS) $(CXX_OBJECTS) $(SHARED_CXX) $(SHARED_C) $(MINIZ_OBJECTS) $(OBJ)/cacert_data.o $(OBJ)/libmbedtls.a
	$(CXX) -Wl,--build-id=none -o $@ $^ -lSceNet

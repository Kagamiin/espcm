CC := cc
SRC_DIR := ./src
BUILD_DIR := ./build

CFLAGS_DBG := \
	-Wall \
	-Wextra \
	-Werror \
	-O0 \
	-g \
	-fsanitize=address \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \

CFLAGS_DEV := \
	-Wall \
	-Wextra \
	-Werror \
	-O3 \


DEFINES_DEV := \
	-D_DEBUG \
	-I$(SRC_DIR)/include

CFLAGS := $(CFLAGS_DEV) $(DEFINES_DEV)

objects_enc := \
	bit_pack_unpack.o \
	espcm_encoder.o \
	error_strs.o \
	sample_conv.o \
	wav_file.o \

vpath %.c $(SRC_DIR)
vpath %.o $(BUILD_DIR)

.PHONY: build_dirs all clean

all: build_dirs $(BUILD_DIR)/espcm_encoder

$(BUILD_DIR)/espcm_encoder: $(objects_enc)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/espcm_encoder -lm $(patsubst %,$(BUILD_DIR)/%,$(objects_enc))

%.o: %.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/$@ $<

build_dirs:
	@mkdir -p $(BUILD_DIR) 2>/dev/null

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/espcm_encoder

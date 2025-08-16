CC = gcc
CFLAGS = -O2 -Wall -fPIC -std=c11 -mavx512f -mavx512vl -I./include -I./utils
LDFLAGS = -shared -lz

SRC_DIR = src
UTILS_DIR = utils
OBJ_DIR = obj
LIB_DIR = lib
INCLUDE_DIR = include

SOURCES = $(SRC_DIR)/dot_add.c \
          $(SRC_DIR)/dot_sub.c \
          $(SRC_DIR)/dot_add_approx.c \
          $(SRC_DIR)/dot_sub_approx.c \
          $(UTILS_DIR)/dot_utils.c \

OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter $(SRC_DIR)/%,$(SOURCES))) \
          $(patsubst $(UTILS_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter $(UTILS_DIR)/%,$(SOURCES)))

LIBRARY = $(LIB_DIR)/libdot.so

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(UTILS_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)

.PHONY: all clean
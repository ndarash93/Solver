# Compiler and flags
SRC_DIR = ./src
BUILD_DIR = ./bld
#LDLIBS     = -lm -L/usr/local/Cellar/glfw/3.3.1/lib -lglfw
#LDFLAGS    = -framework OpenGL
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

ifeq ($(shell uname), Darwin) # macOS
  CC = clang
  CFLAGS = -Wall -g -arch arm64 -v -DDEBUG
  #LDLIBS = -L/usr/local/Cellar/glfw/3.3.1/lib -lglfw
  LDLIBS = -L/opt/homebrew/lib -lglfw
  LDFLAGS = -framework OpenGL -F /System/Library/Frameworks
  #LDFLAGS = -framework OpenGL
else # Linux
  CFLAGS = -Wall -g -DDEBUG
  CC = gcc
  LDLIBS = -lm -lGL -lglfw
endif

# Target executable
TARGET = $(BUILD_DIR)/Solver

# Build rules
all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*

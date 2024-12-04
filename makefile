# Define compilers
CC := gcc
CXX := g++

# Common compilation flags
CFLAGS_COMMON := -Wall
CFLAGS_C := $(CFLAGS_COMMON)
CXXFLAGS_C := $(CFLAGS_COMMON) -std=c++17

# Include and library directories
INCLUDE_DIRS := -I./include -I./deps/VulkanMemoryAllocator/include -I./deps/SPIRV-Reflect
LIB_DIRS := -Llib
LIBS := -lvulkan -lSDL2 -lshaderc 

# Source files
SRC_FILES := $(shell find src -name '*.c' -o -name '*.cpp') deps/SPIRV-Reflect/spirv_reflect.c

# Object files
OBJ_FILES := $(patsubst %.c,obj/%.o,$(patsubst %.cpp,obj/%.o,$(SRC_FILES)))


# Dependency files
DEP_FILES := $(OBJ_FILES:.o=.d)

.PHONY: all
all: bin/main

bin/main: $(OBJ_FILES)
	@mkdir -p bin
	$(CXX) $(OBJ_FILES) $(LIB_DIRS) $(LIBS) -o $@

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) $(INCLUDE_DIRS) -MMD -MF $(@:.o=.d) -c $< -o $@

obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_C) $(INCLUDE_DIRS) -MMD -MF $(@:.o=.d) -c $< -o $@

-include $(DEP_FILES)

.PHONY: clean
clean:
	rm -rf obj bin

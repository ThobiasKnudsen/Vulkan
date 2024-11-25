CC := gcc
CXX := g++

CFLAGS_COMMON := -Wall
CFLAGS_C := $(CFLAGS_COMMON)
CXXFLAGS_C := $(CFLAGS_COMMON) -std=c++17

INCLUDE_DIRS := -Iinclude -Ideps/VulkanMemoryAllocator/include
LIB_DIRS := -Llib
LIBS := -lvulkan -lSDL2 -lshaderc 

SRC_FILES := $(shell find src -name '*.c' -o -name '*.cpp')
OBJ_FILES := $(patsubst %.c,obj/%.o,$(patsubst %.cpp,obj/%.o,$(notdir $(SRC_FILES))))
DEP_FILES := $(OBJ_FILES:.o=.d)

.PHONY: all
all: bin/main

bin/main: $(OBJ_FILES)
	@mkdir -p bin
	$(CXX) $(OBJ_FILES) $(LIB_DIRS) $(LIBS) -o $@

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS_C) $(INCLUDE_DIRS) -MMD -MF obj/$*.d -c $< -o $@

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS_C) $(INCLUDE_DIRS) -MMD -MF obj/$*.d -c $< -o $@

-include $(DEP_FILES)

.PHONY: clean
clean:
	rm -rf obj bin

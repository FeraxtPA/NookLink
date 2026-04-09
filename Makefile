CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wno-reorder -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers -I./include
CFLAGS = -Wall -Wextra -I./include
DEPFLAGS = -MMD -MP
LDFLAGS = -lraylib
OBJ_DIR = obj

# Updated to find .cpp files in src and src/UI
CPP_SOURCES = $(wildcard src/*.cpp) $(wildcard src/UI/*.cpp)
# Include tinyfiledialogs if required (it is likely used by your UI)
C_SOURCES = include/tinyfiledialogs/tinyfiledialogs.c

# Create object lists
CPP_OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CPP_SOURCES))
C_OBJECTS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)
DEPS = $(OBJECTS:.o=.d)

EXECUTABLE = NookLink_Linux

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile C++ files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile C files (using CXX to keep it simple, or use gcc)
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) $(DEPFLAGS) -x c -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE)
	@mkdir -p $(OBJ_DIR)

.PHONY: all clean

-include $(DEPS)
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-reorder -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers -I./include
LDFLAGS = -lraylib

# Updated to find .cpp files in src and src/UI
CPP_SOURCES = $(wildcard src/*.cpp) $(wildcard src/UI/*.cpp)
# Include tinyfiledialogs if required (it is likely used by your UI)
C_SOURCES = include/tinyfiledialogs/tinyfiledialogs.c

# Create object lists
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

EXECUTABLE = NookLink_Linux

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	rm -f $(OBJECTS)

# Compile C++ files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C files (using CXX to keep it simple, or use gcc)
%.o: %.c
	$(CXX) $(CXXFLAGS) -x c -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
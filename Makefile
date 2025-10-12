# Compile all .cpp files and link into 'app', removing .o files after linking

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -I./include
LDFLAGS = -lraylib
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = NookLink

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	rm -f $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean

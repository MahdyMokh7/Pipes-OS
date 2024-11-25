CXX = g++

CXXFLAGS = -std=c++17

SOURCES = main.cpp store.cpp part.cpp

OBJECTS = $(SOURCES:.cpp=.o)

EXECUTABLE = main.out

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean

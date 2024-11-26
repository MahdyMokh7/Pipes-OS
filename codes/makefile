CXX = g++
CXXFLAGS = -std=c++17

EXECUTABLE = main.out
STORE_EXEC = store.out
PART_EXEC = part.out

all: $(EXECUTABLE) $(STORE_EXEC) $(PART_EXEC)
	./$(EXECUTABLE) # Automatically run the main executable after building

$(EXECUTABLE): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(STORE_EXEC): store.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(PART_EXEC): part.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(EXECUTABLE) $(STORE_EXEC) $(PART_EXEC)
	rm -f main.o store.o part.o
	rm -rf ../PipedFolder

.PHONY: all clean

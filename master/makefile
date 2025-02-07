CXX = clang++
CXXFLAGS = -std=c++11
INCLUDES = -I/opt/homebrew/include
LIBS = -L/opt/homebrew/lib -lboost_system
SRC = main.cpp
OBJ = main.o
OUT = main

# Default target
all: $(OUT)

# Link object files to create the final executable
$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJ) $(LIBS) -o $(OUT)
	./$(OUT)

# Compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OUT) $(OBJ)

.PHONY: all clean

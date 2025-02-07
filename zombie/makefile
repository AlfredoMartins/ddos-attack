# Compiler
CXX = g++

# Compiler flags for C++17, debugging, and enabling threads
CXXFLAGS = -Wall -g -std=c++17 -pthread

# Target executable
TARGET = main

# For deleting the target
TARGET_DEL = main

# Source files
SRCS = main.cpp TaskScheduler.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule to build and run the executable
all: $(TARGET) run

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) -lcurl

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to run the executable
run: $(TARGET)
	./$(TARGET)

# Clean rule to remove generated files
clean:
	rm -f $(TARGET_DEL) $(OBJS)
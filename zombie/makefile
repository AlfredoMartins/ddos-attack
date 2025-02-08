# Compiler
CXX = g++

# Compiler flags for C++17, debugging, and enabling threads
CXXFLAGS = -Wall -g -std=c++17

# Detect OS and adjust settings
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)  # macOS
    LDFLAGS = -lcurl
    RM = rm -f
    EXEEXT =
else ifeq ($(OS), Windows_NT)  # Windows (MinGW or Cygwin)
    LDFLAGS = -lcurl -lws2_32
    RM = del /F /Q
    EXEEXT = .exe
else  # Linux
    LDFLAGS = -lcurl
    RM = rm -f
    EXEEXT =
endif

# Target executable
TARGET = main$(EXEEXT)

# Source files
SRCS = main.cpp TaskScheduler.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule to build and run the executable
all: $(TARGET) run

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to run the executable
run: $(TARGET)
	./$(TARGET)

# Clean rule to remove generated files
clean:
	$(RM) $(TARGET) $(OBJS)

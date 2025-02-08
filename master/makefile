CXX = clang++
CXXFLAGS = -std=c++11
INCLUDES = -I/opt/homebrew/include
LIBS = -L/opt/homebrew/lib -lboost_system
SRC = main.cpp
OBJ = main.o
OUT = main

ifeq ($(UNAME_S), Darwin)  # macOS
    LDFLAGS = -lcurl
    RM = rm -f
    EXEEXT =
else ifeq ($(OS), Windows_NT)  # Windows (MinGW or Cygwin)
    LDFLAGS = -lcurl -lws2_32
    RM = del /F /Q
    EXEEXT = .exe
else  # Linux
	CXX = g++
    RM = rm -f
    EXEEXT =
endif

# Default target
all: $(OUT) run

# Link object files to create the final executable
$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJ) $(LIBS) -o $(OUT)

# Compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

TARGET = main$(EXEEXT)

# Clean up generated files
clean:
	rm -f $(OUT) $(OBJ)

.PHONY: all clean

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -I Source/include -Wall -g -MMD -MP

# Linker flags
LDFLAGS = -L Source/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

# Source directory
SRC_DIR = Source

# Object directory
OBJ_DIR = obj

# Target executable
TARGET = main

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))

# Dependency files
DEPS = $(OBJECTS:.o=.d)

# Default target
all: create_obj_dir $(TARGET)

# Create object directory if it doesn't exist
create_obj_dir:
	if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

# Linking target
$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Include dependency files
-include $(DEPS)

# Clean up
clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d $(TARGET)

# Phony targets
.PHONY: all clean create_obj_dir

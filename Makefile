APP_NAME = game

BUILD_DIR = build
BIN_DIR = bin
CXX = g++
CXXFLAGS = -fno-rtti -fno-exceptions -std=c++17 -MMD #-MP
LDFLAGS = -lSDL2 -lGL -ldl -lpthread 
SOURCES =                  \
	src/main.cpp           \
	src/vorbis_decode.cpp  \
	src/imgui.cpp          \
	external/glad/glad.cpp
INCLUDE_DIRS =                      \
	external                        \
	external/sdl2                   \
	external/sdl2/SDL2              \
	external/physx/physx/include    \
	external/physx/pxshared/include \
	external/stb                    \
	external/imgui
PHYSX_LIBS =                                     \
	libPhysXCooking_static_64.a            \
	libPhysXVehicle_static_64.a            \
	libPhysXExtensions_static_64.a         \
	libPhysX_static_64.a                   \
	libPhysXPvdSDK_static_64.a             \
	libPhysXCharacterKinematic_static_64.a \
	libPhysXFoundation_static_64.a         \
	libPhysXCommon_static_64.a

OBJECTS = $(SOURCES:.cpp=.o)
INCLUDE_FLAGS = $(addprefix -I, $(INCLUDE_DIRS))

DEBUG_DIR = $(BUILD_DIR)/debug
DEBUG_EXE = $(DEBUG_DIR)/$(APP_NAME)
DEBUG_OBJECTS = $(addprefix $(DEBUG_DIR)/, $(OBJECTS))
DEBUG_CXXFLAGS = -D_DEBUG -D_GLIBCXX_DEBUG -g
DEBUG_LDFLAGS = -Lexternal/physx/physx/bin/linux.clang/checked/ $(addprefix -l:, $(PHYSX_LIBS))
DEBUG_DEPS = $(addprefix $(DEBUG_DIR)/, $(SOURCES:.cpp=.d))

RELEASE_DIR = $(BUILD_DIR)/release
RELEASE_EXE = $(RELEASE_DIR)/$(APP_NAME)
RELEASE_OBJECTS = $(addprefix $(RELEASE_DIR)/, $(OBJECTS))
RELEASE_CXXFLAGS = -DNDEBUG -ofast
RELEASE_LDFLAGS = -Lexternal/physx/physx/bin/linux.clang/release/ $(addprefix -l:, $(PHYSX_LIBS))
RELEASE_DEPS = $(addprefix $(RELEASE_DIR)/, $(SOURCES:.cpp=.d))

# Rules

.PHONY: all clean debug prep release

all: release

## DEBUG

debug: prep $(DEBUG_EXE)
	@cp $(DEBUG_EXE) $(BIN_DIR)/$(APP_NAME)
	@echo "Debug build successful"

### LINK
$(DEBUG_EXE): $(DEBUG_OBJECTS)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -o $(DEBUG_EXE) $^ $(LDFLAGS) $(DEBUG_LDFLAGS)
	
### COMPILE
$(DEBUG_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $(DEBUG_CXXFLAGS) $(INCLUDE_FLAGS) -o $@ $<

## RELEASE

release: prep $(RELEASE_EXE)
	@strip $(RELEASE_EXE)
	@cp $(RELEASE_EXE) $(BIN_DIR)/$(APP_NAME)
	@echo "Release build successful"

### LINK
$(RELEASE_EXE): $(RELEASE_OBJECTS)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -o $(RELEASE_EXE) $^ $(LDFLAGS) $(RELEASE_LDFLAGS)
	
### COMPILE
$(RELEASE_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $(RELEASE_CXXFLAGS) $(INCLUDE_FLAGS) -o $@ $<

## GENERATE compile_commands.json

$(BUILD_DIR)/compile_commands/%.compile_command: %.cpp
	@mkdir -p $(dir $@)
	@echo "  {" > $@
	@echo "    \"command\": \"$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $<\"," >> $@
	@echo "    \"directory\": \"$(CURDIR)\"," >> $@
	@echo "    \"file\": \"$<\"" >> $@
	@echo "  }," >> $@

COMPILE_COMMAND_ENTRIES = $(addprefix $(BUILD_DIR)/compile_commands/, $(SOURCES:.cpp=.compile_command))

compile_commands.json: $(COMPILE_COMMAND_ENTRIES)
	@echo "[" > $@.tmp
	@cat $^ >> $@.tmp
	@sed '$$d' < $@.tmp > $@
	@echo "  }" >> $@
	@echo "]" >> $@
	@rm $@.tmp

## OTHER

prep: compile_commands.json shaders

shaders:
	@cp -r shaders $(BIN_DIR)/

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf compile_commands.json

-include $(DEBUG_DEPS)
-include $(RELEASE_DEPS)

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
PKG_CONFIG ?= pkg-config
CXXFLAGS += $(shell $(PKG_CONFIG) --cflags glfw3 glew glm)
LDLIBS   := $(shell $(PKG_CONFIG) --libs glfw3 glew glm)
ifeq ($(shell uname -s),Darwin)
LDLIBS += -framework OpenGL
else
LDLIBS += -lGL
endif
CPPFLAGS += -DLANE_RUNNER_ASSET_DIR='"$(CURDIR)"'
CXXFLAGS += -MMD -MP

TARGET  := laneRunner
SOURCES := engine/window.cpp engine/shader.cpp engine/mesh.cpp engine/renderer.cpp \
           engine/texgen.cpp engine/texture.cpp engine/hud.cpp engine/objloader.cpp \
           game/game.cpp game/main.cpp
OBJECTS := $(SOURCES:%.cpp=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)

-include $(OBJECTS:.o=.d)

.PHONY: all run clean

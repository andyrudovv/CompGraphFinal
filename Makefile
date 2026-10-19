CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDLIBS   := -lglfw -lGLEW -lGL

TARGET  := laneRunner
SOURCES := engine/window.cpp engine/shader.cpp engine/mesh.cpp engine/renderer.cpp \
           engine/texgen.cpp engine/texture.cpp engine/hud.cpp game/game.cpp game/main.cpp
OBJECTS := $(SOURCES:%.cpp=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)

.PHONY: all run clean

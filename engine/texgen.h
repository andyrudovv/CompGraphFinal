// Procedural texture generators for asphalt, grass, concrete and building facades.

#ifndef ENGINE_TEXGEN_H
#define ENGINE_TEXGEN_H

#include <cstdint>
#include <vector>

namespace engine {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

Image generateAsphalt(int size = 256);
Image generateGrass(int size = 256);
Image generateConcrete(int size = 128);
Image generateFacade(int size = 256);

}

#endif

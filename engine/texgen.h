#ifndef ENGINE_TEXGEN_H
#define ENGINE_TEXGEN_H

// Procedural textures generated on the CPU at start-up. No image files are
// loaded, so there is nothing to license or credit: every pixel comes from the
// hash-based value noise in texgen.cpp. All images are RGBA8 and tile
// seamlessly. The alpha channel is an "emissive mask" (only the facade uses it:
// lit windows), everything else is fully opaque.
//
// Colour values are centred around 0.8 and the scene shader multiplies them by
// TEX_GAIN (1.25), so a textured surface keeps the brightness of its vertex
// colour and the texture only adds detail.

#include <cstdint>
#include <vector>

namespace engine {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;  // width * height * 4, row 0 at the bottom (v = 0)
};

Image generateAsphalt(int size = 256);   // fine grain, pebbles, dark patches
Image generateGrass(int size = 256);     // multi-octave noise with blade streaks
Image generateConcrete(int size = 128);  // curb stone: soft noise and pores
Image generateFacade(int size = 256);    // 4 x 4 window grid; alpha = lit window mask

}  // namespace engine

#endif

#include "texgen.h"

#include <algorithm>
#include <cmath>

namespace engine {

namespace {

// ---- hash-based, tileable value noise --------------------------------------

std::uint32_t hash2(int x, int y, std::uint32_t seed) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u +
                      static_cast<std::uint32_t>(y) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Uniform value in [0, 1) for an integer lattice point.
float rnd(int x, int y, std::uint32_t seed) {
    return static_cast<float>(hash2(x, y, seed) & 0xFFFFFFu) / 16777216.0f;
}

int wrap(int v, int period) { return ((v % period) + period) % period; }

float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

// Value noise whose lattice repeats every `period` cells, so (u, v) in [0, 1)
// tiles without a seam.
float valueNoise(float u, float v, int period, std::uint32_t seed) {
    float x = u * period, y = v * period;
    int xi = static_cast<int>(std::floor(x)), yi = static_cast<int>(std::floor(y));
    float fx = smooth(x - xi), fy = smooth(y - yi);
    auto at = [&](int ix, int iy) { return rnd(wrap(ix, period), wrap(iy, period), seed); };
    float a = at(xi, yi), b = at(xi + 1, yi), c = at(xi, yi + 1), d = at(xi + 1, yi + 1);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

// Fractal sum of valueNoise, result in [0, 1].
float fbm(float u, float v, int basePeriod, int octaves, std::uint32_t seed) {
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    int period = basePeriod;
    for (int o = 0; o < octaves; ++o) {
        sum += amp * valueNoise(u, v, period, seed + static_cast<std::uint32_t>(o) * 101u);
        norm += amp;
        amp *= 0.5f;
        period *= 2;
    }
    return sum / norm;
}

std::uint8_t toByte(float v) {
    return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
}

void put(Image& img, int x, int y, float r, float g, float b, float a = 1.0f) {
    std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
    img.rgba[i + 0] = toByte(r);
    img.rgba[i + 1] = toByte(g);
    img.rgba[i + 2] = toByte(b);
    img.rgba[i + 3] = toByte(a);
}

Image makeImage(int size) {
    Image img;
    img.width = img.height = size;
    img.rgba.assign(static_cast<std::size_t>(size) * size * 4, 255);
    return img;
}

}  // namespace

// ---- asphalt ---------------------------------------------------------------

Image generateAsphalt(int size) {
    Image img = makeImage(size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = (x + 0.5f) / size, v = (y + 0.5f) / size;

            float large = fbm(u, v, 3, 3, 11u);        // worn and patched areas
            float grain = fbm(u, v, 48, 2, 23u);       // fine aggregate
            float value = 0.74f + (large - 0.5f) * 0.30f + (grain - 0.5f) * 0.34f;

            // Single bright pebbles and dark pits.
            float pebble = rnd(x, y, 5u);
            if (pebble > 0.985f) value += 0.20f;
            else if (pebble < 0.012f) value -= 0.22f;

            // Faint cool/warm tint variation keeps it from looking grey-flat.
            float tint = (fbm(u, v, 5, 2, 77u) - 0.5f) * 0.05f;
            put(img, x, y, value + tint, value, value - tint);
        }
    }
    return img;
}

// ---- grass -----------------------------------------------------------------

Image generateGrass(int size) {
    Image img = makeImage(size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = (x + 0.5f) / size, v = (y + 0.5f) / size;

            float patch = fbm(u, v, 4, 4, 31u);        // lighter and darker lawn patches
            float blades = fbm(u, v, 64, 2, 47u);     // blade-sized speckle
            float streak = valueNoise(u, v, 96, 59u) * 0.6f + valueNoise(u, v, 24, 61u) * 0.4f;

            float value = 0.80f + (patch - 0.5f) * 0.38f + (blades - 0.5f) * 0.22f +
                          (streak - 0.5f) * 0.18f;
            float dry = std::clamp((patch - 0.62f) * 2.5f, 0.0f, 1.0f);  // yellowish dry patches

            put(img, x, y, value * (1.0f + 0.10f * dry), value, value * (1.0f - 0.18f * dry));
        }
    }
    return img;
}

// ---- concrete --------------------------------------------------------------

Image generateConcrete(int size) {
    Image img = makeImage(size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = (x + 0.5f) / size, v = (y + 0.5f) / size;

            float soft = fbm(u, v, 4, 4, 83u);
            float pores = rnd(x, y, 9u) < 0.02f ? -0.16f : 0.0f;
            float value = 0.80f + (soft - 0.5f) * 0.30f + pores;

            // Expansion joint every half tile: a thin dark line across the stone.
            float joint = (y % (size / 2) == 0) ? -0.20f : 0.0f;
            put(img, x, y, value + joint, value + joint, value + joint * 0.9f);
        }
    }
    return img;
}

// ---- building facade ---------------------------------------------------------

Image generateFacade(int size) {
    Image img = makeImage(size);
    const int cols = 4, rows = 4;
    const float cellW = static_cast<float>(size) / cols;
    const float cellH = static_cast<float>(size) / rows;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = (x + 0.5f) / size, v = (y + 0.5f) / size;

            int cx = static_cast<int>(x / cellW), cy = static_cast<int>(y / cellH);
            float lx = (x - cx * cellW) / cellW;   // 0..1 inside the window cell
            float ly = (y - cy * cellH) / cellH;

            // Concrete wall with mild stains.
            float wall = 0.84f + (fbm(u, v, 6, 3, 91u) - 0.5f) * 0.22f +
                         (rnd(x, y, 13u) - 0.5f) * 0.05f;
            // Dark band under every floor, like a cornice / spandrel.
            if (ly < 0.10f) wall *= 0.80f;

            float r = wall, g = wall, b = wall * 0.98f, emissive = 0.0f;

            // Window opening with a frame.
            bool inWindow = lx > 0.20f && lx < 0.80f && ly > 0.24f && ly < 0.86f;
            bool inGlass = lx > 0.25f && lx < 0.75f && ly > 0.29f && ly < 0.81f;
            if (inWindow && !inGlass) {
                r = g = b = 0.32f;  // frame
            } else if (inGlass) {
                bool lit = rnd(cx, cy, 17u) < 0.42f;  // fixed per window, repeats with the tile
                bool mullion = std::fabs(lx - 0.5f) < 0.018f || std::fabs(ly - 0.55f) < 0.018f;
                float sky = 0.30f + 0.25f * ly;       // cold reflection of the sky by day
                if (lit) {
                    float warm = 0.75f + 0.25f * rnd(cx, cy, 19u);
                    r = 0.98f * warm;
                    g = 0.88f * warm;
                    b = 0.58f * warm;
                    emissive = 1.0f;
                } else {
                    r = sky * 0.70f;
                    g = sky * 0.85f;
                    b = sky * 1.05f;
                }
                if (mullion) {
                    r = g = b = 0.32f;
                    emissive = 0.0f;
                }
            }
            put(img, x, y, r, g, b, emissive);
        }
    }
    return img;
}

}  // namespace engine

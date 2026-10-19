#ifndef ENGINE_OBJLOADER_H
#define ENGINE_OBJLOADER_H

// Minimal Wavefront OBJ loader for downloaded car models.
//
//   * reads v / vt / vn / f (triangles, quads and n-gons, negative indices too)
//     and the diffuse colour Kd of every material from the .mtl next to it;
//   * textures (map_Kd) are NOT loaded, surfaces get their Kd colour;
//   * the model is turned, centred and scaled so that it fits the game's frame:
//     origin on the ground under the middle of the car, forward = -Z, the length
//     along Z equal to ObjOptions::length metres;
//   * faces are split into two meshes: "paint" (the car body, stored white so the
//     game can tint every car with its own colour) and "rest" (glass, tyres,
//     lights... drawn with their .mtl colours).

#include <string>
#include <vector>

#include "mesh.h"

namespace engine {

    struct ObjOptions {
        float length = 4.2f;      // metres from bumper to bumper after scaling
        float yawDegrees = 0.0f;  // extra turn around Y if the car faces backwards (180) or sideways (90)
        bool autoAlign = true;    // if the model is longer along X than along Z, turn it 90 degrees first

        // Materials whose name contains one of these words (case-insensitive) are paint.
        // If none matches, the material with the most triangles is taken as the paint.
        std::vector<std::string> paintKeywords{ "paint", "body", "carpaint", "car_paint", "main" };
    };

    // Returns false (and leaves the meshes empty) if the file is missing or has no faces.
    bool loadObjModel(const std::string& path, const ObjOptions& options, Mesh& paint, Mesh& rest);

}  // namespace engine

#endif

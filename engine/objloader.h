// Optional OBJ model loader for car and truck models.

#ifndef ENGINE_OBJLOADER_H
#define ENGINE_OBJLOADER_H

#include <string>
#include <vector>

#include "mesh.h"

namespace engine {

    struct ObjOptions {
        float length = 4.2f;
        float yawDegrees = 0.0f;
        bool autoAlign = true;

        std::vector<std::string> paintKeywords{ "paint", "body", "carpaint", "car_paint", "main" };
    };

    bool loadObjModel(const std::string& path, const ObjOptions& options, Mesh& paint, Mesh& rest);

}

#endif

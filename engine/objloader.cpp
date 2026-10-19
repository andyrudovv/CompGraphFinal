#include "objloader.h"

#include "shader.h"  // assetPath

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace engine {

    namespace {

        struct Corner {
            int v = -1, vt = -1, vn = -1;
        };

        struct Tri {
            glm::vec3 p[3];
            glm::vec3 n[3];
            glm::vec2 uv[3];
            bool hasNormals = false;
            int material = 0;
        };

        std::string lower(std::string s) {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        std::string trim(const std::string& s) {
            std::size_t a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos) return "";
            std::size_t b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }

        // Everything after the keyword: material and file names may contain spaces.
        std::string restOfLine(std::istringstream& ss) {
            std::string rest;
            std::getline(ss, rest);
            return trim(rest);
        }

        std::string directoryOf(const std::string& path) {
            std::size_t pos = path.find_last_of("/\\");
            return pos == std::string::npos ? std::string() : path.substr(0, pos + 1);
        }

        // OBJ indices are 1-based, negative ones count from the end; 0 / empty = missing.
        int fixIndex(const std::string& token, std::size_t count) {
            if (token.empty()) return -1;
            int i = std::atoi(token.c_str());
            if (i > 0) return i - 1;
            if (i < 0) return static_cast<int>(count) + i;
            return -1;
        }

        Corner parseCorner(const std::string& token, std::size_t nv, std::size_t nt, std::size_t nn) {
            std::string parts[3];
            int k = 0;
            for (char c : token) {
                if (c == '/') {
                    if (++k > 2) break;
                }
                else {
                    parts[k] += c;
                }
            }
            Corner corner;
            corner.v = fixIndex(parts[0], nv);
            corner.vt = fixIndex(parts[1], nt);
            corner.vn = fixIndex(parts[2], nn);
            return corner;
        }

        void readMtl(const std::string& path, std::map<std::string, glm::vec3>& colors) {
            std::ifstream file(path);
            if (!file) {
                std::fprintf(stderr, "objloader: no material file %s, using grey\n", path.c_str());
                return;
            }
            std::string line, current;
            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string key;
                ss >> key;
                if (key == "newmtl") {
                    current = restOfLine(ss);
                    colors[current] = glm::vec3(0.7f);
                }
                else if (key == "Kd" && !current.empty()) {
                    glm::vec3 c(0.7f);
                    ss >> c.x >> c.y >> c.z;
                    colors[current] = c;
                }
            }
        }

        glm::vec3 rotateY(const glm::vec3& v, float c, float s) {
            return glm::vec3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
        }

    }  // namespace

    bool loadObjModel(const std::string& path, const ObjOptions& options, Mesh& paint, Mesh& rest) {
        std::string full = assetPath(path);
        std::ifstream file(full);
        if (!file) {
            std::fprintf(stderr, "objloader: %s not found, using the built-in model\n", path.c_str());
            return false;
        }

        std::vector<glm::vec3> positions, normals;
        std::vector<glm::vec2> uvs;
        std::map<std::string, glm::vec3> mtlColors;
        std::vector<std::string> materials{ "" };  // index 0: faces before any usemtl
        std::map<std::string, int> materialIndex{ {"", 0} };
        int currentMaterial = 0;
        std::vector<Tri> tris;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string key;
            ss >> key;
            if (key == "v") {
                glm::vec3 p(0.0f);
                ss >> p.x >> p.y >> p.z;
                positions.push_back(p);
            }
            else if (key == "vn") {
                glm::vec3 n(0.0f);
                ss >> n.x >> n.y >> n.z;
                normals.push_back(n);
            }
            else if (key == "vt") {
                glm::vec2 t(0.0f);
                ss >> t.x >> t.y;
                uvs.push_back(t);
            }
            else if (key == "mtllib") {
                readMtl(directoryOf(full) + restOfLine(ss), mtlColors);
            }
            else if (key == "usemtl") {
                std::string name = restOfLine(ss);
                auto it = materialIndex.find(name);
                if (it == materialIndex.end()) {
                    it = materialIndex.emplace(name, static_cast<int>(materials.size())).first;
                    materials.push_back(name);
                }
                currentMaterial = it->second;
            }
            else if (key == "f") {
                std::vector<Corner> corners;
                std::string token;
                while (ss >> token) {
                    corners.push_back(parseCorner(token, positions.size(), uvs.size(), normals.size()));
                }
                // Fan triangulation; faces with broken indices are skipped.
                for (std::size_t k = 1; k + 1 < corners.size(); ++k) {
                    const Corner c[3] = { corners[0], corners[k], corners[k + 1] };
                    Tri t;
                    t.material = currentMaterial;
                    t.hasNormals = true;
                    bool ok = true;
                    for (int q = 0; q < 3; ++q) {
                        if (c[q].v < 0 || c[q].v >= static_cast<int>(positions.size())) {
                            ok = false;
                            break;
                        }
                        t.p[q] = positions[c[q].v];
                        bool hasUv = c[q].vt >= 0 && c[q].vt < static_cast<int>(uvs.size());
                        t.uv[q] = hasUv ? uvs[c[q].vt] : glm::vec2(0.0f);
                        if (c[q].vn >= 0 && c[q].vn < static_cast<int>(normals.size())) {
                            t.n[q] = normals[c[q].vn];
                        }
                        else {
                            t.hasNormals = false;
                        }
                    }
                    if (ok) tris.push_back(t);
                }
            }
        }

        if (tris.empty()) {
            std::fprintf(stderr, "objloader: %s has no faces\n", path.c_str());
            return false;
        }

        // ---- orientation: turn the model so its length runs along Z
        glm::vec3 lo(1e30f), hi(-1e30f);
        for (const Tri& t : tris) {
            for (const glm::vec3& p : t.p) {
                lo = glm::min(lo, p);
                hi = glm::max(hi, p);
            }
        }
        float yaw = glm::radians(options.yawDegrees);
        if (options.autoAlign && (hi.x - lo.x) > (hi.z - lo.z) * 1.2f) yaw += glm::radians(90.0f);
        float cy = std::cos(yaw), sy = std::sin(yaw);

        lo = glm::vec3(1e30f);
        hi = glm::vec3(-1e30f);
        for (Tri& t : tris) {
            for (int q = 0; q < 3; ++q) {
                t.p[q] = rotateY(t.p[q], cy, sy);
                if (t.hasNormals) t.n[q] = rotateY(t.n[q], cy, sy);
                lo = glm::min(lo, t.p[q]);
                hi = glm::max(hi, t.p[q]);
            }
        }

        // ---- centre on the ground and scale to the wanted length
        float lengthZ = hi.z - lo.z;
        float scale = lengthZ > 1e-6f ? options.length / lengthZ : 1.0f;
        glm::vec3 anchor((lo.x + hi.x) * 0.5f, lo.y, (lo.z + hi.z) * 0.5f);

        // ---- which materials are paint
        std::vector<int> triCount(materials.size(), 0);
        for (const Tri& t : tris) ++triCount[t.material];

        std::set<int> paintSet;
        for (std::size_t m = 0; m < materials.size(); ++m) {
            std::string name = lower(materials[m]);
            for (const std::string& word : options.paintKeywords) {
                if (!word.empty() && name.find(lower(word)) != std::string::npos) {
                    paintSet.insert(static_cast<int>(m));
                    break;
                }
            }
        }
        if (paintSet.empty()) {
            int biggest = static_cast<int>(std::max_element(triCount.begin(), triCount.end()) -
                triCount.begin());
            paintSet.insert(biggest);
        }

        // ---- build the two meshes
        std::vector<Vertex> pv, rv;
        std::vector<GLuint> pi, ri;
        for (const Tri& t : tris) {
            glm::vec3 p[3];
            for (int q = 0; q < 3; ++q) p[q] = (t.p[q] - anchor) * scale;
            glm::vec3 face = glm::cross(p[1] - p[0], p[2] - p[0]);
            if (glm::length(face) < 1e-12f) continue;  // degenerate
            face = glm::normalize(face);

            bool isPaint = paintSet.count(t.material) > 0;
            glm::vec3 color(1.0f);
            if (!isPaint) {
                auto it = mtlColors.find(materials[t.material]);
                color = it != mtlColors.end() ? it->second : glm::vec3(0.7f);
            }

            std::vector<Vertex>& verts = isPaint ? pv : rv;
            std::vector<GLuint>& idx = isPaint ? pi : ri;
            for (int q = 0; q < 3; ++q) {
                glm::vec3 n = face;
                if (t.hasNormals && glm::length(t.n[q]) > 1e-6f) n = glm::normalize(t.n[q]);
                idx.push_back(static_cast<GLuint>(verts.size()));
                verts.push_back({ p[q], n, color, t.uv[q] });
            }
        }

        paint.upload(pv, pi);
        rest.upload(rv, ri);

        std::string paintNames;
        for (int m : paintSet) paintNames += (paintNames.empty() ? "" : ", ") + materials[m];
        std::fprintf(stderr, "objloader: %s: %zu triangles, paint material: %s\n", path.c_str(),
            tris.size(), paintNames.empty() ? "(none)" : paintNames.c_str());
        return true;
    }

}  // namespace engine

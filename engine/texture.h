#ifndef ENGINE_TEXTURE_H
#define ENGINE_TEXTURE_H

#include <GL/glew.h>

#include "texgen.h"

namespace engine {

// A 2D RGBA texture: repeat wrapping, trilinear filtering with a full mip chain
// and, when the driver offers it, 8x anisotropic filtering. The anisotropy is
// what keeps the asphalt crisp instead of smeared at grazing angles.
class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool create(const Image& image);
    void bind(unsigned unit) const;
    void destroy();
    bool valid() const { return id_ != 0; }

private:
    GLuint id_ = 0;
};

}  // namespace engine

#endif

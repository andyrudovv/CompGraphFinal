#ifndef ENGINE_NODE_H
#define ENGINE_NODE_H

#include <glm/glm.hpp>

#include <utility>
#include <vector>

#include "renderer.h"

namespace engine {

// A node of a transform tree: world = parent world * local. The mesh is optional,
// so a node can also be a pure pivot (a wheel axle, a light mount).
struct Node {
    glm::mat4 local{1.0f};
    const Mesh* mesh = nullptr;
    Material material;
    bool visible = true;
    std::vector<Node> children;

    // World matrix cached by updateWorld(); read it after the update (lights, attachments).
    const glm::mat4& world() const { return world_; }

    Node& add(Node child) {
        children.push_back(std::move(child));
        return children.back();
    }

    void updateWorld(const glm::mat4& parentWorld) {
        world_ = parentWorld * local;
        for (Node& c : children) c.updateWorld(world_);
    }

    void draw(Renderer& renderer) const {
        if (!visible) return;
        if (mesh) renderer.draw(*mesh, world_, material);
        for (const Node& c : children) c.draw(renderer);
    }

private:
    glm::mat4 world_{1.0f};
};

}  // namespace engine

#endif

#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <random>
#include <vector>

#include "../engine/engine.h"

namespace game {

const int LANE_COUNT = 3;

struct TrafficCar {
    float x = 0.0f;
    float z = 0.0f;
    float speed = 20.0f;   // own forward speed, always below the player's
    int lane = 1;
    int targetLane = 1;
    float changeCooldown = 3.0f;
    glm::vec3 color{1.0f};
    bool counted = false;  // already scored as an overtake
};

struct Prop {
    enum Type { Lamp, Tree, Building } type = Tree;
    float x = 0.0f;
    float z = 0.0f;
    float scale = 1.0f;
    float height = 1.0f;
    glm::vec3 color{1.0f};
};

class Game {
public:
    bool init();
    void update(engine::Window& window);
    void render(engine::Window& window);
    void setDayTime(float t) { dayTime_ = t; }

private:
    enum class State { Playing, Crashed };

    void reset();
    void buildMeshes();
    void buildProps();
    void updatePlayer(engine::Window& window, float dt);
    void updateTraffic(float dt);
    void updateWorld(float dt);
    void updateDayNight(float dt);
    void spawnTraffic(float z);
    bool laneBlocked(int lane, float z, float margin) const;
    void checkCollision();
    void collectLights();
    void drawCar(const glm::mat4& model, const glm::vec3& color, float headlights, float taillights);
    void drawHud(engine::Window& window);
    float randRange(float a, float b);
    int difficultyLevel() const;

    engine::Renderer renderer_;
    engine::Hud hud_;
    engine::Camera camera_;
    engine::Environment env_;

    engine::Mesh ground_, road_, dash_, curb_;
    engine::Mesh carBody_, carTrim_, carHead_, carTail_;
    engine::Mesh lampPole_, lampHead_, tree_, building_;

    State state_ = State::Playing;
    float elapsed_ = 0.0f;
    float distance_ = 0.0f;
    int overtakes_ = 0;
    int score_ = 0;
    int best_ = 0;
    float crashFlash_ = 0.0f;
    bool paused_ = false;

    // Player
    int playerLane_ = 1;
    float playerX_ = 0.0f;
    float playerRoll_ = 0.0f;
    float speed_ = 24.0f;
    float boost_ = 1.0f;

    // World
    std::vector<TrafficCar> traffic_;
    std::vector<Prop> props_;
    std::vector<float> dashes_;
    float spawnTimer_ = 1.0f;
    float dayTime_ = 0.12f;  // 0..1, 0.25 is noon
    float night_ = 0.0f;

    std::mt19937 rng_{20260916u};
};

}  // namespace game

#endif

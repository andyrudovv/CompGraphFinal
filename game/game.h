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
        float speed = 20.0f;   // current own forward speed (changes when following / wrecked)
        float cruise = 20.0f;  // speed the driver wants to keep
        int lane = 1;
        int targetLane = 1;
        float changeCooldown = 3.0f;
        glm::vec3 color{ 1.0f };
        bool counted = false;  // already scored as an overtake

        int type = 0;          // 0 = sedan, 1 = truck
        float halfX = 0.95f;   // half extents of the collision box
        float halfZ = 2.1f;
        bool braking = false;  // brake lights
        float wheelAngle = 0.0f;

        // Wreck state: after a car-to-car crash the car spins and slides to the shoulder.
        bool wrecked = false;
        float slideDir = 1.0f; // -1 left shoulder, +1 right shoulder
        float vx = 0.0f;       // sideways velocity
        float yaw = 0.0f;      // heading offset (lane-change lean or wreck spin)
        float yawVel = 0.0f;
        float hitTimer = 0.0f; // > 0: a fresh impact, no new sparks/debris yet (stops spam)
    };

    struct Prop {
        enum Type { Lamp, Tree, Building } type = Tree;
        float x = 0.0f;
        float z = 0.0f;
        float scale = 1.0f;
        float height = 1.0f;
        glm::vec3 color{ 1.0f };
    };

    struct Spark {
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };   // world-space velocity (the road is the reference frame)
        float life = 0.0f;
    };

    // A broken-off piece of a car (body panel or glass) that flies, bounces and
    // then lies on the road until it scrolls away behind the camera.
    struct Debris {
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };   // road frame, like Spark
        glm::vec3 axis{ 1.0f, 0.0f, 0.0f };
        float angle = 0.0f;
        float spin = 0.0f;       // rad/s around axis
        glm::vec3 size{ 0.3f };
        glm::vec3 color{ 1.0f };
        float life = 0.0f;
        bool glass = false;
        bool resting = false;
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
        void updateSparks(float dt);
        void updateDebris(float dt);
        void spawnTraffic(float z);
        void spawnSparks(const glm::vec3& p, int count, float power, float worldSpeed);
        void spawnDebris(const glm::vec3& p, const glm::vec3& paint, int count, float power,
            float worldSpeed);
        void resolveTrafficCollisions();
        void wreck(TrafficCar& c, float side);
        bool laneBlocked(int lane, float z, float margin) const;
        void checkCollision();
        void collectLights();
        void drawCar(const glm::mat4& model, const glm::vec3& color, float headlights, float taillights,
            float wheelAngle);
        void drawTruck(const glm::mat4& model, const glm::vec3& color, float headlights,
            float taillights, float wheelAngle);
        void drawWheels(const glm::mat4& model, float angle, float zFront, float zRear, float x);
        void drawHud(engine::Window& window);
        float randRange(float a, float b);
        int difficultyLevel() const;

        engine::Renderer renderer_;
        engine::Hud hud_;
        engine::Camera camera_;
        engine::Environment env_;

        engine::Mesh ground_, road_, dash_, curb_;
        engine::Mesh carBody_, carTrim_, carHead_, carTail_, wheel_, playerKit_;
        engine::Mesh truckCab_, truckCargo_, truckTrim_;
        engine::Mesh spark_, shard_;

        // Optional downloaded models (models/car.obj, models/truck.obj). When a file is
        // missing the procedural box cars are used, so the game always runs.
        engine::Mesh carModelPaint_, carModelRest_, truckModelPaint_, truckModelRest_;
        bool carModel_ = false;
        bool truckModel_ = false;
        engine::Mesh lampPole_, lampHead_, tree_, building_, buildingRoof_;

        // Procedural textures (generated in init, nothing is loaded from disk).
        engine::Texture grassTex_, asphaltTex_, concreteTex_, facadeTex_;

        State state_ = State::Playing;
        float elapsed_ = 0.0f;
        float distance_ = 0.0f;
        int overtakes_ = 0;
        int score_ = 0;
        int best_ = 0;
        float crashFlash_ = 0.0f;
        float shakeTime_ = 0.0f;   // camera shake, 1 -> 0
        bool paused_ = false;

        // Player
        int playerLane_ = 1;
        float playerX_ = 0.0f;
        float playerRoll_ = 0.0f;
        float playerYaw_ = 0.0f;
        float playerYawVel_ = 0.0f;
        float playerVx_ = 0.0f;
        float wheelAngle_ = 0.0f;
        float speed_ = 24.0f;
        float boost_ = 1.0f;

        // World
        std::vector<TrafficCar> traffic_;
        std::vector<Prop> props_;
        std::vector<Spark> sparks_;
        std::vector<Debris> debris_;
        std::vector<float> dashes_;
        float spawnTimer_ = 1.0f;
        float dayTime_ = 0.12f;  // 0..1, 0.25 is noon
        float night_ = 0.0f;

        std::mt19937 rng_{ 20260916u };
    };

}  // namespace game

#endif

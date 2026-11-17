// Game state: player, traffic, props, particles, cameras, day/night cycle and HUD state.

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
        float speed = 20.0f;
        float cruise = 20.0f;
        int lane = 1;
        int targetLane = 1;
        float changeCooldown = 3.0f;
        glm::vec3 color{ 1.0f };
        bool counted = false;

        int type = 0;
        float halfX = 0.95f;
        float halfZ = 2.1f;
        bool braking = false;
        float wheelAngle = 0.0f;

        bool wrecked = false;
        float slideDir = 1.0f;
        float vx = 0.0f;
        float yaw = 0.0f;
        float yawVel = 0.0f;
        float hitTimer = 0.0f;
    };

    struct CarRig {
        engine::Node root;
        engine::Node* cargo = nullptr;
        engine::Node* wheels[4] = {};
        glm::vec3 wheelPos[4];
        engine::Node* lamps[2] = {};
        engine::Node* tail = nullptr;
    };

    struct Prop {
        enum Type { Lamp, Tree, Building, Hill } type = Tree;
        float x = 0.0f;
        float z = 0.0f;
        float scale = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
        glm::vec3 color{ 1.0f };
    };

    struct Spark {
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };
        float life = 0.0f;
    };

    struct Debris {
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };
        glm::vec3 axis{ 1.0f, 0.0f, 0.0f };
        float angle = 0.0f;
        float spin = 0.0f;
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
        void setDayTime(float t);
        void setLightingEnabled(bool enabled) { env_.lightingEnabled = enabled; }
        bool lightingEnabled() const { return env_.lightingEnabled; }
        void setVsync(bool on) { vsync_ = on; }
        float fps() const { return fps_; }

    private:
        enum class State { Playing, Crashed };

        void reset();
        void buildMeshes();
        void buildProps();
        void updatePlayer(engine::Window& window, float dt);
        void updateJump(float dt);
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
        void buildRigs();
        void poseRig(CarRig& rig, const glm::mat4& model, const glm::vec3& color, float headlights,
            float taillights, float wheelAngle);
        glm::mat4 playerMatrix() const;
        void drawHud(engine::Window& window);
        float randRange(float a, float b);
        int difficultyLevel() const;

        engine::Renderer renderer_;
        engine::Hud hud_;
        engine::Camera camera_;
        engine::Environment env_;
        enum class CameraMode { Chase, Free, Orbit };
        CameraMode cameraMode_ = CameraMode::Chase;
        float cameraYaw_ = 0.0f;
        float cameraPitch_ = 0.0f;
        float orbitYaw_ = 0.6f;
        float orbitPitch_ = 0.3f;
        float orbitDist_ = 9.0f;

        engine::Mesh ground_, road_, dash_, curb_;
        engine::Mesh carBody_, carTrim_, headlamp_, carTail_, wheel_, playerKit_;
        engine::Mesh truckCab_, truckCargo_, truckTrim_;
        engine::Mesh spark_, shard_;
        CarRig sedanRig_, truckRig_;

        engine::Mesh carModelPaint_, carModelRest_, truckModelPaint_, truckModelRest_;
        bool carModel_ = false;
        bool truckModel_ = false;
        engine::Mesh lampPole_, lampHead_, tree_, building_, buildingRoof_, hill_;

        engine::Texture grassTex_, asphaltTex_, concreteTex_, facadeTex_;

        State state_ = State::Playing;
        float elapsed_ = 0.0f;
        float distance_ = 0.0f;
        int overtakes_ = 0;
        int score_ = 0;
        int best_ = 0;
        float crashFlash_ = 0.0f;
        float shakeTime_ = 0.0f;
        bool paused_ = false;
        bool showHelp_ = false;
        bool dayCycle_ = true;
        bool hills_ = true;
        bool vsync_ = true;

        float fps_ = 0.0f;
        float frameMs_ = 0.0f;
        int fpsFrames_ = 0;
        float fpsStart_ = -1.0f;
        double accumulator_ = 0.0;

        int playerLane_ = 1;
        float playerX_ = 0.0f;
        float playerRoll_ = 0.0f;
        float playerY_ = 0.0f;
        float playerVy_ = 0.0f;
        float jumpCooldown_ = 0.0f;
        bool jumpRequest_ = false;
        float playerYaw_ = 0.0f;
        float playerYawVel_ = 0.0f;
        float playerVx_ = 0.0f;
        float wheelAngle_ = 0.0f;
        float speed_ = 24.0f;
        float boost_ = 1.0f;

        std::vector<TrafficCar> traffic_;
        std::vector<Prop> props_;
        std::vector<Spark> sparks_;
        std::vector<Debris> debris_;
        std::vector<float> dashes_;
        float spawnTimer_ = 1.0f;
        float dayTime_ = 0.12f;
        float sunYaw_ = 0.0f;
        float night_ = 0.0f;

        std::mt19937 rng_{ 20260916u };
    };

}

#endif

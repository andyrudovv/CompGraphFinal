#include "game.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace game {

    static const float LANE_WIDTH = 3.4f;
    static const float ROAD_HALF = LANE_WIDTH * LANE_COUNT * 0.5f + 1.1f;
    static const float ROAD_LENGTH = 900.0f;
    static const float SPAWN_Z = -185.0f;
    static const float DESPAWN_Z = 30.0f;
    static const float DASH_SPACING = 9.0f;
    static const int DASH_COUNT = 46;
    static const float PROP_SPAN = 560.0f;
    static const float CAR_HALF_X = 0.95f;
    static const float CAR_HALF_Z = 2.1f;
    static const float WHEEL_RADIUS = 0.36f;
    static const float DAY_CYCLE = 90.0f;  // seconds for a full day/night rotation
    static const float TWO_PI = 6.28318530718f;

    // How many metres one repeat of each texture covers. The road, grass and curb
    // textures scroll with the travelled distance; one facade tile is a 4 x 4 block
    // of windows (2.4 m wide, 3.2 m tall each).
    static const float ASPHALT_TILE = 4.0f;
    static const float GRASS_TILE = 8.0f;
    static const float CURB_TILE = 3.0f;
    static const float FACADE_TILE_W = 9.6f;
    static const float FACADE_TILE_H = 12.8f;

    // Texture offset for a surface that slides under the car: the fractional part of
    // distance / tile keeps the number small, the texture repeats anyway.
    static float scrollOffset(float distance, float tileMetres) {
        return std::fmod(distance / tileMetres, 1.0f);
    }

    static float lanePosition(int lane) { return (lane - 1) * LANE_WIDTH; }

    // Axis-aligned half extents of a box that may be rotated around Y (wrecks spin).
    static void boxExtents(float halfX, float halfZ, float yaw, float& hx, float& hz) {
        float s = std::fabs(std::sin(yaw));
        float k = std::fabs(std::cos(yaw));
        hx = k * halfX + s * halfZ;
        hz = s * halfX + k * halfZ;
    }

    static void halfExtents(const TrafficCar& c, float& hx, float& hz) {
        boxExtents(c.halfX, c.halfZ, c.yaw, hx, hz);
    }

    // Trucks are heavier: in a crash they push sedans around, not the other way.
    static float massOf(const TrafficCar& c) { return c.type == 1 ? 3.0f : 1.0f; }

    static const glm::vec3 PLAYER_COLOR(0.90f, 0.15f, 0.12f);

    static const glm::vec3 TRAFFIC_COLORS[] = {
        {0.85f, 0.20f, 0.18f}, {0.20f, 0.42f, 0.85f}, {0.92f, 0.72f, 0.12f},
        {0.85f, 0.85f, 0.88f}, {0.15f, 0.55f, 0.35f}, {0.55f, 0.20f, 0.65f},
        {0.95f, 0.45f, 0.10f}, {0.20f, 0.22f, 0.26f},
    };

    float Game::randRange(float a, float b) {
        std::uniform_real_distribution<float> dist(a, b);
        return dist(rng_);
    }

    int Game::difficultyLevel() const {
        return 1 + static_cast<int>(elapsed_ / 18.0f);
    }

    bool Game::init() {
        if (!renderer_.init()) return false;
        if (!hud_.init()) return false;
        if (!grassTex_.create(engine::generateGrass()) ||
            !asphaltTex_.create(engine::generateAsphalt()) ||
            !concreteTex_.create(engine::generateConcrete()) ||
            !facadeTex_.create(engine::generateFacade())) {
            return false;
        }
        buildMeshes();

        // Downloaded models are optional (see engine/objloader.h): put them in models/.
        // If a file is missing the built-in box car is drawn instead.
        engine::ObjOptions carOpt;
        carOpt.length = CAR_HALF_Z * 2.0f;
        carModel_ = engine::loadObjModel("models/car.obj", carOpt, carModelPaint_, carModelRest_);

        engine::ObjOptions truckOpt;
        truckOpt.length = 6.2f;
        truckModel_ = engine::loadObjModel("models/truck.obj", truckOpt, truckModelPaint_,
            truckModelRest_);

        buildProps();
        reset();
        return true;
    }

    void Game::reset() {
        state_ = State::Playing;
        elapsed_ = 0.0f;
        distance_ = 0.0f;
        overtakes_ = 0;
        score_ = 0;
        crashFlash_ = 0.0f;
        shakeTime_ = 0.0f;
        playerLane_ = 1;
        playerX_ = lanePosition(playerLane_);
        playerRoll_ = 0.0f;
        playerYaw_ = 0.0f;
        playerYawVel_ = 0.0f;
        playerVx_ = 0.0f;
        wheelAngle_ = 0.0f;
        speed_ = 24.0f;
        boost_ = 1.0f;
        spawnTimer_ = 1.2f;
        traffic_.clear();
        sparks_.clear();
        debris_.clear();
        for (int i = 0; i < 4; ++i) spawnTraffic(-55.0f - 38.0f * i);

        dashes_.resize(DASH_COUNT);
        for (int i = 0; i < DASH_COUNT; ++i) dashes_[i] = DESPAWN_Z - i * DASH_SPACING;
    }

    void Game::buildMeshes() {
        engine::MeshBuilder b;
        const glm::mat4 I(1.0f);

        b.uvPerMeter(1.0f / GRASS_TILE);
        b.plane(I, 1600.0f, 1600.0f, glm::vec3(0.12f, 0.25f, 0.11f));
        b.build(ground_);

        // Road surface and edge lines share one mesh: the road never moves, only its
        // texture is scrolled. The worn-paint look of the lines comes from the same
        // asphalt texture.
        b.clear();
        b.uvPerMeter(1.0f / ASPHALT_TILE);
        b.plane(glm::translate(I, glm::vec3(0.0f, 0.02f, 0.0f)), ROAD_HALF * 2.0f, ROAD_LENGTH,
            glm::vec3(0.11f, 0.11f, 0.125f));
        for (int s = -1; s <= 1; s += 2) {
            float x = s * (ROAD_HALF - 0.45f);
            b.box(glm::translate(I, glm::vec3(x, 0.03f, 0.0f)), glm::vec3(0.22f, 0.02f, ROAD_LENGTH),
                glm::vec3(0.86f, 0.86f, 0.80f));
        }
        b.build(road_);

        // Curbs are a separate mesh so they can use the concrete texture.
        b.clear();
        b.uvPerMeter(1.0f / CURB_TILE);
        for (int s = -1; s <= 1; s += 2) {
            b.box(glm::translate(I, glm::vec3(s * (ROAD_HALF + 0.35f), 0.11f, 0.0f)),
                glm::vec3(0.7f, 0.22f, ROAD_LENGTH), glm::vec3(0.33f, 0.33f, 0.35f));
        }
        b.build(curb_);

        // One "dash" holds both lane separators, so a row of markings is one draw call.
        b.clear();
        for (int s = -1; s <= 1; s += 2) {
            b.box(glm::translate(I, glm::vec3(s * LANE_WIDTH * 0.5f, 0.03f, 0.0f)),
                glm::vec3(0.22f, 0.02f, 2.6f), glm::vec3(0.88f, 0.88f, 0.82f));
        }
        b.build(dash_);

        // ---------------------------------------------------------------- sedan
        // Car, local frame: origin on the ground, forward is -Z.
        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 0.55f, 0.0f)), glm::vec3(1.9f, 0.7f, 4.2f),
            glm::vec3(1.0f));
        b.wedge(glm::translate(I, glm::vec3(0.0f, 1.03f, 0.15f)), glm::vec3(1.66f, 0.66f, 2.1f), 0.72f,
            glm::vec3(1.0f));
        b.build(carBody_);

        // Trim: bumpers, glass, grille, mirrors (the wheels are a separate, spinning mesh).
        const glm::vec3 glass(0.08f, 0.12f, 0.16f);
        b.clear();
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(glm::translate(I, glm::vec3(0.0f, 0.45f, sz * 2.12f)),
                glm::vec3(1.94f, 0.34f, 0.12f), glm::vec3(0.18f, 0.18f, 0.20f));
        }
        b.box(glm::translate(I, glm::vec3(0.0f, 1.06f, -0.82f)), glm::vec3(1.42f, 0.5f, 0.06f),
            glass);  // windscreen
        b.box(glm::translate(I, glm::vec3(0.0f, 1.06f, 1.16f)), glm::vec3(1.36f, 0.44f, 0.06f),
            glass);  // rear window
        for (int sx = -1; sx <= 1; sx += 2) {
            // side windows (tune x if they sink into / float off the cabin wedge)
            b.box(glm::translate(I, glm::vec3(sx * 0.80f, 1.06f, 0.15f)),
                glm::vec3(0.05f, 0.40f, 1.55f), glass);
            // mirrors
            b.box(glm::translate(I, glm::vec3(sx * 0.92f, 0.95f, -0.78f)),
                glm::vec3(0.16f, 0.10f, 0.18f), glm::vec3(0.12f));
        }
        b.box(glm::translate(I, glm::vec3(0.0f, 0.52f, -2.20f)), glm::vec3(1.0f, 0.14f, 0.05f),
            glm::vec3(0.05f));  // grille
        b.box(glm::translate(I, glm::vec3(0.0f, 0.28f, 0.0f)), glm::vec3(1.7f, 0.12f, 4.0f),
            glm::vec3(0.06f));  // under-body shadow strip
        b.build(carTrim_);

        b.clear();
        for (int sx = -1; sx <= 1; sx += 2) {
            b.box(glm::translate(I, glm::vec3(sx * 0.62f, 0.62f, -2.22f)),
                glm::vec3(0.42f, 0.22f, 0.08f), glm::vec3(1.0f));
        }
        b.build(carHead_);

        b.clear();
        for (int sx = -1; sx <= 1; sx += 2) {
            b.box(glm::translate(I, glm::vec3(sx * 0.66f, 0.66f, 2.22f)),
                glm::vec3(0.36f, 0.16f, 0.08f), glm::vec3(1.0f));
        }
        b.build(carTail_);

        // One wheel at the origin, rolling around the X axis. The tyre is dark, the rim is
        // light and carries six dark spokes, so the rotation is actually visible.
        // (Assumes MeshBuilder::cylinder has its axis along X, as the old wheels did.)
        b.clear();
        b.cylinder(I, WHEEL_RADIUS, 0.26f, 16, glm::vec3(0.07f, 0.07f, 0.08f));
        b.cylinder(I, 0.24f, 0.28f, 12, glm::vec3(0.62f, 0.64f, 0.68f));
        for (int k = 0; k < 3; ++k) {
            glm::mat4 r = glm::rotate(I, k * 1.0471975f, glm::vec3(1.0f, 0.0f, 0.0f));
            b.box(r, glm::vec3(0.30f, 0.42f, 0.07f), glm::vec3(0.10f, 0.10f, 0.12f));
        }
        b.build(wheel_);

        // Extras that only the player's car has: rear wing and two racing stripes.
        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 1.30f, 1.95f)), glm::vec3(1.7f, 0.06f, 0.5f),
            glm::vec3(0.08f, 0.08f, 0.09f));
        for (int sx = -1; sx <= 1; sx += 2) {
            b.box(glm::translate(I, glm::vec3(sx * 0.55f, 1.10f, 1.95f)),
                glm::vec3(0.06f, 0.40f, 0.12f), glm::vec3(0.08f, 0.08f, 0.09f));
            b.box(glm::translate(I, glm::vec3(sx * 0.18f, 0.908f, -1.45f)),
                glm::vec3(0.22f, 0.012f, 1.3f), glm::vec3(0.95f));  // hood stripe
            b.box(glm::translate(I, glm::vec3(sx * 0.18f, 0.908f, 1.55f)),
                glm::vec3(0.22f, 0.012f, 0.9f), glm::vec3(0.95f));  // trunk stripe
        }
        b.build(playerKit_);

        // ---------------------------------------------------------------- truck
        // 6.2 m long, 2.2 m wide: cab in front (-Z), cargo box behind it.
        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 1.45f, -2.2f)), glm::vec3(2.0f, 1.5f, 1.8f),
            glm::vec3(1.0f));
        b.build(truckCab_);

        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 1.8f, 0.95f)), glm::vec3(2.2f, 2.2f, 4.3f),
            glm::vec3(1.0f));
        b.build(truckCargo_);

        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 0.45f, 0.0f)), glm::vec3(2.0f, 0.5f, 6.2f),
            glm::vec3(0.15f, 0.15f, 0.17f));  // chassis
        b.box(glm::translate(I, glm::vec3(0.0f, 1.75f, -3.1f)), glm::vec3(1.6f, 0.65f, 0.06f),
            glass);  // windscreen
        b.box(glm::translate(I, glm::vec3(0.0f, 0.45f, -3.14f)), glm::vec3(2.04f, 0.3f, 0.1f),
            glm::vec3(0.2f));  // front bumper
        b.box(glm::translate(I, glm::vec3(0.0f, 0.45f, 3.14f)), glm::vec3(2.04f, 0.3f, 0.1f),
            glm::vec3(0.2f));  // rear bumper
        b.build(truckTrim_);

        // Spark: a tiny cube, drawn emissive.
        b.clear();
        b.box(I, glm::vec3(0.12f), glm::vec3(1.0f));
        b.build(spark_);

        // Debris shard: a unit box, every piece scales it to its own size.
        b.clear();
        b.box(I, glm::vec3(1.0f), glm::vec3(1.0f));
        b.build(shard_);

        // ---------------------------------------------------------------- props
        // Street lamp: the arm points towards +X, the model matrix mirrors it per side.
        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 3.0f, 0.0f)), glm::vec3(0.18f, 6.0f, 0.18f),
            glm::vec3(0.30f, 0.31f, 0.33f));
        b.box(glm::translate(I, glm::vec3(0.85f, 5.9f, 0.0f)), glm::vec3(1.8f, 0.14f, 0.14f),
            glm::vec3(0.30f, 0.31f, 0.33f));
        b.build(lampPole_);

        b.clear();
        b.box(glm::translate(I, glm::vec3(1.62f, 5.74f, 0.0f)), glm::vec3(0.52f, 0.2f, 0.34f),
            glm::vec3(1.0f));
        b.build(lampHead_);

        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 0.9f, 0.0f)), glm::vec3(0.34f, 1.8f, 0.34f),
            glm::vec3(0.26f, 0.18f, 0.11f));
        b.wedge(glm::translate(I, glm::vec3(0.0f, 2.1f, 0.0f)), glm::vec3(2.8f, 1.6f, 2.8f), 0.55f,
            glm::vec3(0.14f, 0.33f, 0.14f));
        b.wedge(glm::translate(I, glm::vec3(0.0f, 3.3f, 0.0f)), glm::vec3(1.9f, 1.8f, 1.9f), 0.12f,
            glm::vec3(0.16f, 0.38f, 0.16f));
        b.build(tree_);

        // Unit-cube tower: the UVs span 0..1 per wall, and render() scales them by
        // width / FACADE_TILE_W and height / FACADE_TILE_H, so windows keep their size
        // on towers of any dimension.
        b.clear();
        b.facadeLayout(true);
        b.box(glm::translate(I, glm::vec3(0.0f, 0.5f, 0.0f)), glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f));
        b.build(building_);

        b.clear();
        b.box(glm::translate(I, glm::vec3(0.0f, 1.02f, 0.0f)), glm::vec3(1.08f, 0.05f, 1.08f),
            glm::vec3(0.35f, 0.35f, 0.38f));
        b.build(buildingRoof_);
    }

    void Game::buildProps() {
        props_.clear();

        for (float z = DESPAWN_Z; z > DESPAWN_Z - PROP_SPAN; z -= 34.0f) {
            for (int s = -1; s <= 1; s += 2) {
                Prop lamp;
                lamp.type = Prop::Lamp;
                lamp.x = s * (ROAD_HALF + 1.4f);
                lamp.z = z + (s < 0 ? 17.0f : 0.0f);
                props_.push_back(lamp);
            }
        }

        for (float z = DESPAWN_Z; z > DESPAWN_Z - PROP_SPAN; z -= 11.0f) {
            for (int s = -1; s <= 1; s += 2) {
                Prop p;
                bool building = randRange(0.0f, 1.0f) < 0.35f;
                p.type = building ? Prop::Building : Prop::Tree;
                p.z = z + randRange(-4.0f, 4.0f);
                if (building) {
                    p.x = s * randRange(20.0f, 42.0f);
                    p.scale = randRange(6.0f, 13.0f);
                    p.height = randRange(8.0f, 30.0f);
                    float grey = randRange(0.25f, 0.55f);
                    p.color = glm::vec3(grey, grey * randRange(0.9f, 1.05f), grey * randRange(0.9f, 1.1f));
                }
                else {
                    p.x = s * randRange(10.0f, 18.0f);
                    p.scale = randRange(0.7f, 1.35f);
                    p.height = p.scale;
                    p.color = glm::vec3(randRange(0.8f, 1.15f));
                }
                props_.push_back(p);
            }
        }
    }

    void Game::update(engine::Window& window) {
        float dt = window.dt();

        if (window.keyPressed(GLFW_KEY_ESCAPE)) window.close();
        if (window.keyPressed(GLFW_KEY_P)) paused_ = !paused_;
        if (window.keyPressed(GLFW_KEY_R)) {
            best_ = std::max(best_, score_);
            reset();
        }

        if (paused_) dt = 0.0f;

        if (state_ == State::Playing) {
            elapsed_ += dt;
            updatePlayer(window, dt);
            updateTraffic(dt);
            updateWorld(dt);
            checkCollision();
            distance_ += speed_ * dt;
            score_ = static_cast<int>(distance_ * 0.5f) + overtakes_ * 25;
        }
        else {
            // Roll to a stop, keep the world alive so the scene still reads as 3D.
            speed_ = std::max(speed_ - 26.0f * dt, 0.0f);
            crashFlash_ = std::max(crashFlash_ - dt * 0.9f, 0.0f);
            updateWorld(dt);
            updateTraffic(dt);  // the pile-up keeps developing behind the crash

            // The player's wreck slides sideways and spins, then settles.
            playerX_ += playerVx_ * dt;
            playerVx_ *= std::exp(-2.2f * dt);
            playerX_ = glm::clamp(playerX_, -(ROAD_HALF + 0.8f), ROAD_HALF + 0.8f);
            playerYaw_ += playerYawVel_ * dt;
            playerYawVel_ *= std::exp(-1.1f * dt);
        }

        wheelAngle_ = std::fmod(wheelAngle_ + speed_ * dt / WHEEL_RADIUS, TWO_PI);
        updateSparks(dt);
        updateDebris(dt);
        shakeTime_ = std::max(shakeTime_ - dt * 1.5f, 0.0f);

        updateDayNight(dt);
        collectLights();

        // Chase camera: it leans with the lane change and drifts back when quick.
        float t = static_cast<float>(window.time());
        float shake = (state_ == State::Playing) ? std::sin(elapsed_ * 31.0f) * speed_ * 0.0015f : 0.0f;
        float hit = shakeTime_ * shakeTime_ * 0.45f;  // impact shake
        camera_.aspect = window.aspect();
        camera_.position = glm::vec3(playerX_ * 0.45f + std::sin(t * 47.0f) * hit,
            4.6f + shake + std::cos(t * 59.0f) * hit,
            10.2f + speed_ * 0.045f);
        camera_.target = glm::vec3(playerX_ * 0.75f, 1.35f, -14.0f);
        camera_.fovDegrees = 60.0f + std::min(speed_, 70.0f) * 0.14f;
    }

    void Game::updatePlayer(engine::Window& window, float dt) {
        if (window.keyPressed(GLFW_KEY_LEFT) || window.keyPressed(GLFW_KEY_A)) {
            playerLane_ = std::max(playerLane_ - 1, 0);
        }
        if (window.keyPressed(GLFW_KEY_RIGHT) || window.keyPressed(GLFW_KEY_D)) {
            playerLane_ = std::min(playerLane_ + 1, LANE_COUNT - 1);
        }

        bool accelerating = window.keyDown(GLFW_KEY_UP) || window.keyDown(GLFW_KEY_W);
        bool braking = window.keyDown(GLFW_KEY_DOWN) || window.keyDown(GLFW_KEY_S);
        float wanted = accelerating ? 1.28f : (braking ? 0.68f : 1.0f);
        boost_ += (wanted - boost_) * std::min(dt * 3.0f, 1.0f);

        float base = 24.0f + 2.4f * (difficultyLevel() - 1);
        speed_ = std::min(base, 58.0f) * boost_;

        float target = lanePosition(playerLane_);
        float blend = std::min(dt * 8.5f, 1.0f);
        float previous = playerX_;
        playerX_ += (target - playerX_) * blend;

        float lateral = dt > 0.0f ? (playerX_ - previous) / dt : 0.0f;
        playerRoll_ += (glm::clamp(-lateral * 0.035f, -0.25f, 0.25f) - playerRoll_) *
            std::min(dt * 9.0f, 1.0f);
    }

    void Game::updateTraffic(float dt) {
        int level = difficultyLevel();

        if (state_ == State::Playing) {
            spawnTimer_ -= dt;
            if (spawnTimer_ <= 0.0f) {
                float interval = std::max(1.15f - 0.08f * (level - 1), 0.30f);
                spawnTimer_ = interval * randRange(0.75f, 1.3f);
                spawnTraffic(SPAWN_Z + randRange(-15.0f, 15.0f));
            }
        }

        for (auto& car : traffic_) {
            car.wheelAngle = std::fmod(car.wheelAngle + car.speed * dt / WHEEL_RADIUS, TWO_PI);
            car.hitTimer = std::max(car.hitTimer - dt, 0.0f);

            if (car.wrecked) {
                // Friction kills the forward speed, the car slides towards its shoulder
                // (so a wreck never walls off the whole road for long) and spins.
                car.speed = std::max(car.speed - 14.0f * dt, 0.0f);
                float target = car.slideDir * (ROAD_HALF + 2.2f);
                car.vx += ((target - car.x) * 1.4f - car.vx) * std::min(dt * 4.0f, 1.0f);
                car.x += car.vx * dt;
                car.yaw += car.yawVel * dt;
                car.yawVel *= std::exp(-0.9f * dt);
                car.braking = false;
            }
            else {
                // 1. Find the closest car ahead that overlaps our lane (lane-change aware).
                const TrafficCar* leader = nullptr;
                float leaderGap = 1e9f;
                for (const auto& o : traffic_) {
                    if (&o == &car || o.z >= car.z) continue;
                    // A spinning wreck is wider than its lane, use its rotated box.
                    float ohx, ohz;
                    halfExtents(o, ohx, ohz);
                    if (std::fabs(o.x - car.x) > car.halfX + ohx + 0.1f) continue;
                    float gap = (car.z - car.halfZ) - (o.z + ohz);
                    if (gap < leaderGap) {
                        leaderGap = gap;
                        leader = &o;
                    }
                }

                // 2. Adaptive cruise: keep a safe gap that grows with the closing speed
                //    (stopping distance at 8 m/s^2), otherwise return to the cruise speed.
                float desired = car.cruise;
                if (leader) {
                    float closing = std::max(car.speed - leader->speed, 0.0f);
                    float safe = 4.0f + 0.25f * car.speed + closing * closing / 16.0f;
                    if (leaderGap < safe) {
                        desired = std::min(desired, leader->speed * (leaderGap < safe * 0.5f ? 0.8f : 1.0f));
                    }
                }
                float dv = desired - car.speed;
                car.braking = dv < -0.5f;
                if (dv < 0.0f) car.speed += std::max(dv, -10.0f * dt);
                else car.speed += std::min(dv, 4.0f * dt);

                // 3. Lane switching keeps late levels unpredictable.
                car.changeCooldown -= dt;
                if (level >= 3 && !car.braking && car.lane == car.targetLane &&
                    car.changeCooldown <= 0.0f && car.z < -25.0f) {
                    car.changeCooldown = randRange(2.5f, 6.0f);
                    if (randRange(0.0f, 1.0f) < 0.05f * (level - 2)) {
                        int dir = randRange(0.0f, 1.0f) < 0.5f ? -1 : 1;
                        int next = car.lane + dir;
                        if (next >= 0 && next < LANE_COUNT && !laneBlocked(next, car.z, 22.0f)) {
                            car.targetLane = next;
                        }
                    }
                }
                float targetX = lanePosition(car.targetLane);
                car.x += (targetX - car.x) * std::min(dt * 2.2f, 1.0f);
                if (std::fabs(targetX - car.x) < 0.05f) car.lane = car.targetLane;
                // The car leans into the lane change.
                car.yaw = -glm::clamp((targetX - car.x) * 0.10f, -0.2f, 0.2f);
            }

            car.z += (speed_ - car.speed) * dt;

            if (state_ == State::Playing && !car.counted && car.z > 0.0f) {
                car.counted = true;
                ++overtakes_;
            }
        }

        resolveTrafficCollisions();

        traffic_.erase(std::remove_if(traffic_.begin(), traffic_.end(),
            [](const TrafficCar& c) { return c.z > DESPAWN_Z; }),
            traffic_.end());
    }

    // Puts a car into the wreck state, or just kicks one that is already wrecked.
    // side: -1 pushes it to the left shoulder, +1 to the right one.
    void Game::wreck(TrafficCar& c, float side) {
        if (!c.wrecked) {
            c.wrecked = true;
            c.slideDir = side;
            c.targetLane = c.lane;
        }
        c.yawVel += side * randRange(1.5f, 3.2f);
    }

    void Game::resolveTrafficCollisions() {
        // Cars are solid boxes. Every overlapping pair (wrecks included) is pushed
        // apart and trades speed like an inelastic hit, so nothing drives through
        // anything any more. An intact car that touches anything still becomes a
        // wreck, which keeps the chain pile-ups. Two passes settle a pile-up where
        // one push creates a new overlap; effects only happen in the first pass.
        const float RESTITUTION = 0.3f;

        for (int pass = 0; pass < 2; ++pass) {
            for (std::size_t i = 0; i < traffic_.size(); ++i) {
                for (std::size_t j = i + 1; j < traffic_.size(); ++j) {
                    TrafficCar& a = traffic_[i];
                    TrafficCar& b = traffic_[j];

                    float ahx, ahz, bhx, bhz;
                    halfExtents(a, ahx, ahz);
                    halfExtents(b, bhx, bhz);
                    float dx = b.x - a.x;
                    float dz = b.z - a.z;
                    float ox = ahx + bhx - std::fabs(dx);  // overlap along X
                    float oz = ahz + bhz - std::fabs(dz);  // overlap along Z
                    if (ox <= 0.0f || oz <= 0.0f) continue;

                    float ma = massOf(a), mb = massOf(b);
                    float wa = mb / (ma + mb);  // share of the push: the lighter car moves more
                    float wb = ma / (ma + mb);
                    float impact = 0.0f;

                    if (ox < oz) {
                        // Side contact: separate along X.
                        float s = (dx >= 0.0f) ? 1.0f : -1.0f;  // +1: b is right of a
                        a.x -= s * ox * wa;
                        b.x += s * ox * wb;
                        float closing = (a.vx - b.vx) * s;
                        if (closing > 0.0f) {
                            float jx = closing * (1.0f + RESTITUTION) / (1.0f / ma + 1.0f / mb);
                            a.vx -= s * jx / ma;
                            b.vx += s * jx / mb;
                        }
                        if (pass == 0) {  // small bounce so they do not stay glued together
                            a.vx -= s * 1.5f * wa;
                            b.vx += s * 1.5f * wb;
                        }
                        impact = std::max(closing, 0.0f) + 2.0f;
                    }
                    else {
                        // Front/rear contact: separate along Z (larger z = further behind).
                        float s = (dz >= 0.0f) ? 1.0f : -1.0f;  // +1: b is behind a
                        a.z -= s * oz * wa;
                        b.z += s * oz * wb;
                        TrafficCar& rear = (s > 0.0f) ? b : a;
                        TrafficCar& front = (s > 0.0f) ? a : b;
                        float closing = rear.speed - front.speed;  // own forward speeds
                        if (closing > 0.0f) {
                            float mr = massOf(rear), mf = massOf(front);
                            float jz = closing * (1.0f + RESTITUTION) / (1.0f / mr + 1.0f / mf);
                            rear.speed = std::max(rear.speed - jz / mr, 0.0f);
                            front.speed += jz / mf;
                        }
                        impact = std::max(closing, 0.0f);
                    }

                    if (pass > 0) continue;

                    // Wrecks resting against each other only get pushed; a fresh crash
                    // or a hard knock spins them and throws sparks and debris.
                    bool fresh = !a.wrecked || !b.wrecked;
                    bool hard = impact > 3.0f && a.hitTimer <= 0.0f && b.hitTimer <= 0.0f;
                    if (!fresh && !hard) continue;

                    float side = (dx >= 0.0f) ? -1.0f : 1.0f;  // a is pushed away from b
                    if (std::fabs(dx) < 0.3f) side = randRange(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
                    wreck(a, side);
                    wreck(b, -side);
                    a.hitTimer = b.hitTimer = 0.35f;

                    glm::vec3 contact((a.x + b.x) * 0.5f, 0.6f, (a.z + b.z) * 0.5f);
                    float power = std::min(impact + 4.0f, 12.0f);
                    float carSpeed = (a.speed + b.speed) * 0.5f;
                    if (sparks_.size() < 400) spawnSparks(contact, 22, power, carSpeed);
                    int pieces = fresh ? 7 : 3;
                    spawnDebris(contact, a.color, pieces, power, carSpeed);
                    spawnDebris(contact, b.color, pieces, power, carSpeed);
                    if (std::fabs(a.z) < 45.0f) shakeTime_ = std::max(shakeTime_, 0.3f);
                }
            }
        }

        // After the player has crashed, his wreck is a solid obstacle as well. It is
        // treated as very heavy: the traffic car does all of the moving.
        if (state_ == State::Crashed) {
            float phx, phz;
            boxExtents(CAR_HALF_X, CAR_HALF_Z, playerYaw_, phx, phz);
            for (auto& c : traffic_) {
                float hx, hz;
                halfExtents(c, hx, hz);
                float dx = c.x - playerX_;
                float dz = c.z;  // the player sits at z = 0
                float ox = hx + phx - std::fabs(dx);
                float oz = hz + phz - std::fabs(dz);
                if (ox <= 0.0f || oz <= 0.0f) continue;

                if (ox < oz) {
                    float s = (dx >= 0.0f) ? 1.0f : -1.0f;
                    c.x += s * ox;
                    if (c.vx * s < 0.0f) c.vx = -c.vx * 0.3f;
                }
                else {
                    float s = (dz >= 0.0f) ? 1.0f : -1.0f;  // +1: car is behind the player
                    c.z += s * oz;
                    // Match speeds so the car stops pressing into the wreck.
                    if (s > 0.0f) c.speed = std::min(c.speed, speed_);
                    else c.speed = std::max(c.speed, speed_);
                }

                if (c.wrecked) continue;
                float side = (dx >= 0.0f) ? 1.0f : -1.0f;
                wreck(c, side);
                c.hitTimer = 0.35f;
                glm::vec3 contact(playerX_ + side * 0.9f, 0.6f, c.z * 0.5f);
                if (sparks_.size() < 400) spawnSparks(contact, 16, 8.0f, c.speed);
                spawnDebris(contact, c.color, 6, 8.0f, c.speed);
                spawnDebris(contact, PLAYER_COLOR, 4, 8.0f, c.speed);
            }
        }
    }

    bool Game::laneBlocked(int lane, float z, float margin) const {
        for (const auto& car : traffic_) {
            if (car.lane != lane && car.targetLane != lane) continue;
            if (std::fabs(car.z - z) < margin) return true;
        }
        return false;
    }

    void Game::spawnTraffic(float z) {
        int level = difficultyLevel();
        int lane = static_cast<int>(randRange(0.0f, static_cast<float>(LANE_COUNT) - 0.001f));
        if (laneBlocked(lane, z, 30.0f)) return;

        // Never close every lane at once: the player must always have an escape.
        int freeLanes = 0;
        for (int l = 0; l < LANE_COUNT; ++l) {
            if (l != lane && !laneBlocked(l, z, 24.0f)) ++freeLanes;
        }
        if (freeLanes == 0) return;

        TrafficCar car;
        car.lane = car.targetLane = lane;
        car.x = lanePosition(lane);
        car.z = z;

        bool truck = level >= 2 && randRange(0.0f, 1.0f) < 0.22f;
        car.type = truck ? 1 : 0;
        car.halfX = truck ? 1.1f : CAR_HALF_X;
        car.halfZ = truck ? 3.1f : CAR_HALF_Z;

        float base = 24.0f + 2.4f * (level - 1);
        car.cruise = car.speed = base * (truck ? randRange(0.30f, 0.50f) : randRange(0.38f, 0.74f));
        car.changeCooldown = randRange(1.5f, 5.0f);
        int index = static_cast<int>(randRange(0.0f, 7.999f));
        car.color = TRAFFIC_COLORS[index];
        traffic_.push_back(car);
    }

    void Game::spawnSparks(const glm::vec3& p, int count, float power, float worldSpeed) {
        for (int i = 0; i < count; ++i) {
            Spark s;
            s.pos = p + glm::vec3(randRange(-0.5f, 0.5f), randRange(0.0f, 0.3f), randRange(-0.5f, 0.5f));
            s.vel = glm::vec3(randRange(-1.0f, 1.0f) * power * 0.5f,
                randRange(1.5f, 2.0f + power * 0.35f),
                -worldSpeed + randRange(-1.0f, 1.0f) * power * 0.5f);
            s.life = randRange(0.35f, 1.0f);
            sparks_.push_back(s);
        }
    }

    void Game::updateSparks(float dt) {
        for (Spark& s : sparks_) {
            s.vel.y -= 16.0f * dt;
            // The world scrolls towards the camera at speed_, so add it to z.
            s.pos += glm::vec3(s.vel.x, s.vel.y, s.vel.z + speed_) * dt;
            if (s.pos.y < 0.03f) {  // bounce on the asphalt
                s.pos.y = 0.03f;
                s.vel.y = std::fabs(s.vel.y) * 0.3f;
                s.vel.x *= 0.7f;
                s.vel.z *= 0.7f;
            }
            s.life -= dt;
        }
        sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(),
            [](const Spark& s) { return s.life <= 0.0f || s.pos.z > DESPAWN_Z; }),
            sparks_.end());
    }

    void Game::spawnDebris(const glm::vec3& p, const glm::vec3& paint, int count, float power,
        float worldSpeed) {
        for (int i = 0; i < count && debris_.size() < 300; ++i) {
            Debris d;
            d.glass = randRange(0.0f, 1.0f) < 0.3f;
            if (d.glass) {
                d.size = glm::vec3(randRange(0.06f, 0.16f), randRange(0.02f, 0.04f),
                    randRange(0.06f, 0.16f));
                d.color = glm::vec3(0.55f, 0.72f, 0.82f);
            }
            else {
                d.size = glm::vec3(randRange(0.15f, 0.45f), randRange(0.03f, 0.07f),
                    randRange(0.12f, 0.35f));
                d.color = paint * randRange(0.7f, 1.0f);  // a bit darker: scratched paint
            }
            d.pos = p + glm::vec3(randRange(-0.6f, 0.6f), randRange(0.0f, 0.4f), randRange(-0.6f, 0.6f));
            d.vel = glm::vec3(randRange(-1.0f, 1.0f) * power * 0.45f,
                randRange(2.0f, 3.0f + power * 0.4f),
                -worldSpeed + randRange(-1.0f, 1.0f) * power * 0.4f);
            d.axis = glm::normalize(glm::vec3(randRange(-1.0f, 1.0f), randRange(0.2f, 1.0f),
                randRange(-1.0f, 1.0f)));
            d.angle = randRange(0.0f, TWO_PI);
            d.spin = randRange(5.0f, 15.0f) * (randRange(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f);
            d.life = randRange(4.0f, 7.0f);
            debris_.push_back(d);
        }
    }

    void Game::updateDebris(float dt) {
        for (Debris& d : debris_) {
            d.life -= dt;
            if (d.resting) {  // lying on the road: it scrolls away with the asphalt
                d.pos.z += speed_ * dt;
                continue;
            }
            d.vel.y -= 16.0f * dt;
            d.pos += glm::vec3(d.vel.x, d.vel.y, d.vel.z + speed_) * dt;
            d.angle += d.spin * dt;

            float floor = 0.03f + d.size.y * 0.5f;
            if (d.pos.y < floor) {
                d.pos.y = floor;
                if (d.vel.y < 0.0f) {  // bounce
                    d.vel.y = -d.vel.y * 0.3f;
                    d.vel.x *= 0.6f;
                    d.vel.z *= 0.6f;
                    d.spin *= 0.5f;
                }
                float scrape = std::exp(-5.0f * dt);  // sliding friction on the asphalt
                d.vel.x *= scrape;
                d.vel.z *= scrape;
                if (d.vel.y < 0.6f && std::fabs(d.vel.x) + std::fabs(d.vel.z) < 0.8f) {
                    d.resting = true;
                    d.vel = glm::vec3(0.0f);
                    d.axis = glm::vec3(0.0f, 1.0f, 0.0f);  // lie flat, keep a random heading
                    d.spin = 0.0f;
                }
            }
        }
        debris_.erase(std::remove_if(debris_.begin(), debris_.end(),
            [](const Debris& d) { return d.life <= 0.0f || d.pos.z > DESPAWN_Z; }),
            debris_.end());
    }

    void Game::updateWorld(float dt) {
        float travel = speed_ * dt;

        for (float& z : dashes_) {
            z += travel;
            if (z > DESPAWN_Z) z -= DASH_COUNT * DASH_SPACING;
        }
        for (Prop& p : props_) {
            p.z += travel;
            if (p.z > DESPAWN_Z) p.z -= PROP_SPAN;
        }
    }

    void Game::checkCollision() {
        for (auto& car : traffic_) {
            float hx, hz;
            halfExtents(car, hx, hz);
            if (std::fabs(car.z) < hz + CAR_HALF_Z - 0.15f &&
                std::fabs(car.x - playerX_) < hx + CAR_HALF_X - 0.1f) {
                float side = (car.x >= playerX_) ? 1.0f : -1.0f;
                if (std::fabs(car.x - playerX_) < 0.3f) {
                    side = randRange(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
                }

                state_ = State::Crashed;
                crashFlash_ = 1.0f;
                shakeTime_ = 1.0f;
                best_ = std::max(best_, score_);

                // Player bounces away from the hit car and spins.
                playerVx_ = -side * 3.5f;
                playerYawVel_ = -side * randRange(1.8f, 3.2f);

                // The other car is knocked forward and wrecked as well.
                car.speed += speed_ * 0.25f;
                wreck(car, side);
                glm::vec3 contact(playerX_ + side * 0.9f, 0.6f, -CAR_HALF_Z);
                spawnSparks(contact, 40, 14.0f, car.speed);
                spawnDebris(contact, PLAYER_COLOR, 14, 12.0f, car.speed);
                spawnDebris(contact, car.color, 14, 12.0f, car.speed);
                return;
            }
        }
    }

    void Game::updateDayNight(float dt) {
        dayTime_ += dt / DAY_CYCLE;
        if (dayTime_ >= 1.0f) dayTime_ -= 1.0f;

        float angle = dayTime_ * TWO_PI;
        glm::vec3 sunDir = glm::normalize(glm::vec3(std::cos(angle) * 0.9f, std::sin(angle), -0.35f));
        float elevation = sunDir.y;
        float day = glm::smoothstep(-0.12f, 0.18f, elevation);
        night_ = 1.0f - day;

        float sunset = std::exp(-(elevation * 3.0f) * (elevation * 3.0f));
        glm::vec3 sunTint = glm::mix(glm::vec3(1.0f, 0.96f, 0.88f), glm::vec3(1.0f, 0.52f, 0.22f),
            sunset * 0.85f);

        float sunPower = glm::smoothstep(-0.02f, 0.22f, elevation);
        float moonPower = glm::smoothstep(0.02f, -0.18f, elevation) * 0.22f;

        env_.sunDirection = sunDir;
        env_.sunColor = sunTint * (0.25f + 0.75f * sunPower);
        if (elevation > 0.0f) {
            env_.lightDirection = sunDir;
            env_.lightColor = sunTint * sunPower;
        }
        else {
            env_.lightDirection = -sunDir;
            env_.lightColor = glm::vec3(0.55f, 0.65f, 0.95f) * moonPower;
        }

        glm::vec3 zenith = glm::mix(glm::vec3(0.20f, 0.44f, 0.86f), glm::vec3(0.012f, 0.02f, 0.07f),
            night_);
        glm::vec3 horizon = glm::mix(glm::vec3(0.72f, 0.83f, 0.95f), glm::vec3(0.05f, 0.06f, 0.14f),
            night_);
        horizon = glm::mix(horizon, glm::vec3(0.95f, 0.45f, 0.18f), sunset * 0.7f);
        zenith = glm::mix(zenith, glm::vec3(0.35f, 0.22f, 0.35f), sunset * 0.35f);

        env_.skyZenith = zenith;
        env_.skyHorizon = horizon;
        env_.groundColor = glm::mix(glm::vec3(0.10f, 0.20f, 0.09f), glm::vec3(0.02f, 0.03f, 0.05f),
            night_);
        env_.ambient = glm::mix(glm::vec3(0.30f, 0.33f, 0.38f), glm::vec3(0.05f, 0.06f, 0.11f), night_);
        env_.fogColor = horizon * glm::mix(1.0f, 0.55f, night_);
        env_.fogDensity = glm::mix(0.0055f, 0.0105f, night_);
        env_.night = night_;
    }

    void Game::collectLights() {
        env_.pointLights.clear();
        float lightsOn = glm::smoothstep(0.25f, 0.65f, night_);
        if (lightsOn <= 0.01f) return;

        for (int s = -1; s <= 1; s += 2) {
            engine::PointLight head;
            head.position = glm::vec3(playerX_ + s * 0.62f, 0.7f, -3.2f);
            head.color = glm::vec3(1.0f, 0.93f, 0.78f) * (1.5f * lightsOn);
            head.range = 26.0f;
            env_.pointLights.push_back(head);
        }

        // Only the lamps around the car can matter; the shader takes eight lights total.
        std::vector<const Prop*> lamps;
        for (const Prop& p : props_) {
            if (p.type == Prop::Lamp && p.z > -70.0f && p.z < 12.0f) lamps.push_back(&p);
        }
        std::sort(lamps.begin(), lamps.end(), [](const Prop* a, const Prop* b) {
            return std::fabs(a->z + 18.0f) < std::fabs(b->z + 18.0f);
            });

        int count = std::min<int>(static_cast<int>(lamps.size()), engine::MAX_POINT_LIGHTS - 2);
        for (int i = 0; i < count; ++i) {
            engine::PointLight lamp;
            float side = lamps[i]->x > 0.0f ? 1.0f : -1.0f;
            lamp.position = glm::vec3(lamps[i]->x - side * 1.6f, 5.6f, lamps[i]->z);
            lamp.color = glm::vec3(1.0f, 0.82f, 0.50f) * (1.35f * lightsOn);
            lamp.range = 24.0f;
            env_.pointLights.push_back(lamp);
        }
    }

    // Four wheels that roll around X. Forward is -Z, so a forward roll is a negative angle.
    void Game::drawWheels(const glm::mat4& model, float angle, float zFront, float zRear, float x) {
        engine::Material m;
        m.specular = 0.3f;
        m.shininess = 32.0f;
        const float zs[2] = { zFront, zRear };
        for (int sx = -1; sx <= 1; sx += 2) {
            for (float z : zs) {
                glm::mat4 w = glm::translate(model, glm::vec3(sx * x, WHEEL_RADIUS, z));
                w = glm::rotate(w, -angle, glm::vec3(1.0f, 0.0f, 0.0f));
                renderer_.draw(wheel_, w, m);
            }
        }
    }

    void Game::drawCar(const glm::mat4& model, const glm::vec3& color, float headlights,
        float taillights, float wheelAngle) {
        engine::Material body;
        body.tint = color;
        body.specular = 0.55f;
        body.shininess = 64.0f;
        engine::Material trim;
        trim.specular = 0.2f;

        if (carModel_) {
            renderer_.draw(carModelPaint_, model, body);  // tinted with the car colour
            renderer_.draw(carModelRest_, model, trim);   // keeps the colours from the .mtl
        }
        else {
            renderer_.draw(carBody_, model, body);
            renderer_.draw(carTrim_, model, trim);
        }

        engine::Material head;
        head.tint = glm::vec3(1.0f, 0.95f, 0.82f);
        head.emissive = headlights;
        renderer_.draw(carHead_, model, head);

        engine::Material tail;
        tail.tint = glm::vec3(1.0f, 0.18f, 0.12f);
        tail.emissive = taillights;
        renderer_.draw(carTail_, model, tail);

        // Downloaded models come with their own (non-spinning) wheels.
        if (!carModel_) drawWheels(model, wheelAngle, -1.35f, 1.35f, 0.92f);
    }

    void Game::drawTruck(const glm::mat4& model, const glm::vec3& color, float headlights,
        float taillights, float wheelAngle) {
        engine::Material cab;
        cab.tint = color;
        cab.specular = 0.5f;
        cab.shininess = 48.0f;
        engine::Material trim;
        trim.specular = 0.2f;

        if (truckModel_) {
            renderer_.draw(truckModelPaint_, model, cab);
            renderer_.draw(truckModelRest_, model, trim);
        }
        else {
            renderer_.draw(truckCab_, model, cab);

            engine::Material cargo;
            cargo.tint = glm::mix(glm::vec3(0.82f), color, 0.15f);
            cargo.specular = 0.15f;
            renderer_.draw(truckCargo_, model, cargo);

            renderer_.draw(truckTrim_, model, trim);
        }

        // The sedan's light meshes are reused, moved to the truck's front and rear faces.
        engine::Material head;
        head.tint = glm::vec3(1.0f, 0.95f, 0.82f);
        head.emissive = headlights;
        renderer_.draw(carHead_, glm::translate(model, glm::vec3(0.0f, 0.0f, -0.88f)), head);

        engine::Material tail;
        tail.tint = glm::vec3(1.0f, 0.18f, 0.12f);
        tail.emissive = taillights;
        renderer_.draw(carTail_, glm::translate(model, glm::vec3(0.0f, 0.0f, 0.88f)), tail);

        if (!truckModel_) drawWheels(model, wheelAngle, -2.2f, 2.2f, 1.02f);
    }

    void Game::render(engine::Window& window) {
        renderer_.beginFrame(camera_, env_);
        renderer_.drawSky();

        const glm::mat4 I(1.0f);
        engine::Material flat;
        flat.specular = 0.0f;
        flat.texture = &grassTex_;
        flat.uvOffset = glm::vec2(0.0f, scrollOffset(distance_, GRASS_TILE));
        renderer_.draw(ground_, I, flat);

        engine::Material asphalt;
        asphalt.specular = 0.12f;
        asphalt.shininess = 16.0f;
        asphalt.texture = &asphaltTex_;
        asphalt.uvOffset = glm::vec2(0.0f, scrollOffset(distance_, ASPHALT_TILE));
        renderer_.draw(road_, I, asphalt);

        engine::Material curb;
        curb.specular = 0.08f;
        curb.texture = &concreteTex_;
        curb.uvOffset = glm::vec2(0.0f, scrollOffset(distance_, CURB_TILE));
        renderer_.draw(curb_, I, curb);

        engine::Material marking;
        marking.specular = 0.05f;
        for (float z : dashes_) {
            renderer_.draw(dash_, glm::translate(I, glm::vec3(0.0f, 0.0f, z)), marking);
        }

        float lampGlow = glm::smoothstep(0.25f, 0.65f, night_);
        for (const Prop& p : props_) {
            if (p.z < -260.0f || p.z > DESPAWN_Z + 5.0f) continue;
            glm::mat4 model = glm::translate(I, glm::vec3(p.x, 0.0f, p.z));
            switch (p.type) {
            case Prop::Lamp: {
                if (p.x > 0.0f) model = glm::rotate(model, 3.14159265f, glm::vec3(0, 1, 0));
                engine::Material pole;
                pole.specular = 0.3f;
                renderer_.draw(lampPole_, model, pole);
                engine::Material glow;
                glow.tint = glm::vec3(1.0f, 0.85f, 0.55f);
                glow.emissive = lampGlow;
                renderer_.draw(lampHead_, model, glow);
                break;
            }
            case Prop::Tree: {
                engine::Material m;
                m.tint = p.color;
                m.specular = 0.05f;
                renderer_.draw(tree_, glm::scale(model, glm::vec3(p.scale)), m);
                break;
            }
            case Prop::Building: {
                glm::mat4 tower = glm::scale(model, glm::vec3(p.scale, p.height, p.scale));

                engine::Material wall;
                wall.tint = p.color;
                wall.specular = 0.1f;
                wall.texture = &facadeTex_;
                wall.uvScale = glm::vec2(p.scale / FACADE_TILE_W, p.height / FACADE_TILE_H);
                // Shift the window pattern by whole window columns/rows, so every
                // tower lights a different set of windows without cutting a window.
                std::size_t id = static_cast<std::size_t>(&p - props_.data());
                wall.uvOffset = glm::vec2(0.25f * static_cast<float>(id % 4),
                    0.25f * static_cast<float>((id / 4) % 4));
                wall.textureGlow = lampGlow;
                renderer_.draw(building_, tower, wall);

                engine::Material roof;
                roof.tint = p.color;
                roof.specular = 0.1f;
                renderer_.draw(buildingRoof_, tower, roof);
                break;
            }
            }
        }

        float now = static_cast<float>(window.time());
        float hazard = std::sin(now * 9.0f) > 0.0f ? 1.0f : 0.0f;  // wrecks blink their hazard lights

        for (const auto& car : traffic_) {
            glm::mat4 model = glm::translate(I, glm::vec3(car.x, 0.0f, car.z));
            model = glm::rotate(model, car.yaw, glm::vec3(0.0f, 1.0f, 0.0f));

            float head = car.wrecked ? hazard * 0.6f : 0.15f * night_;
            float tail = car.wrecked ? hazard : (car.braking ? 1.0f : 0.35f + 0.5f * night_);
            if (car.type == 1) drawTruck(model, car.color, head, tail, car.wheelAngle);
            else drawCar(model, car.color, head, tail, car.wheelAngle);
        }

        glm::mat4 player = glm::translate(I, glm::vec3(playerX_, 0.0f, 0.0f));
        player = glm::rotate(player, playerYaw_, glm::vec3(0.0f, 1.0f, 0.0f));
        player = glm::rotate(player, playerRoll_, glm::vec3(0.0f, 0.0f, 1.0f));
        float braking = window.keyDown(GLFW_KEY_DOWN) || window.keyDown(GLFW_KEY_S) ? 1.0f : 0.4f;
        if (state_ == State::Crashed) braking = hazard;
        drawCar(player, PLAYER_COLOR, glm::smoothstep(0.2f, 0.6f, night_), braking, wheelAngle_);
        if (!carModel_) {  // the wing and stripes only fit the built-in body
            engine::Material kit;
            kit.specular = 0.4f;
            kit.shininess = 48.0f;
            renderer_.draw(playerKit_, player, kit);
        }

        // Debris: tumbling body panels and glass, they shrink away at the end of their life.
        engine::Material shardMat;
        for (const Debris& d : debris_) {
            float k = glm::clamp(d.life * 2.0f, 0.05f, 1.0f);
            glm::mat4 m = glm::translate(I, d.pos);
            m = glm::rotate(m, d.angle, d.axis);
            m = glm::scale(m, d.size * k);
            shardMat.tint = d.color;
            shardMat.specular = d.glass ? 0.9f : 0.5f;
            shardMat.shininess = d.glass ? 128.0f : 64.0f;
            renderer_.draw(shard_, m, shardMat);
        }

        // Sparks: tiny emissive cubes that shrink as they die.
        engine::Material sparkMat;
        sparkMat.tint = glm::vec3(1.0f, 0.65f, 0.2f);
        sparkMat.emissive = 1.0f;
        sparkMat.specular = 0.0f;
        for (const Spark& s : sparks_) {
            float k = glm::clamp(s.life * 3.0f, 0.0f, 1.0f);
            renderer_.draw(spark_, glm::scale(glm::translate(I, s.pos), glm::vec3(k)), sparkMat);
        }

        drawHud(window);
    }

    void Game::drawHud(engine::Window& window) {
        const glm::vec4 white(1.0f, 1.0f, 1.0f, 0.92f);
        const glm::vec4 amber(1.0f, 0.78f, 0.25f, 0.95f);
        float w = static_cast<float>(window.width());
        float h = static_cast<float>(window.height());

        hud_.begin(window.width(), window.height());

        hud_.rect(0.0f, 0.0f, w, 74.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.28f));
        hud_.number(22.0f, 18.0f, 40.0f, score_, 7, white);

        // Speed bar with the current difficulty level next to it.
        float speedFill = (speed_ - 18.0f) / 55.0f;
        hud_.bar(22.0f, 72.0f, 220.0f, 14.0f, speedFill, glm::vec4(0.35f, 0.85f, 1.0f, 0.9f));
        hud_.number(252.0f, 62.0f, 26.0f, static_cast<int>(speed_ * 3.6f), 3, glm::vec4(0.7f, 0.85f, 1.0f, 0.85f));

        hud_.number(w - 70.0f, 18.0f, 34.0f, difficultyLevel(), 2, amber);
        hud_.number(w - 150.0f, 80.0f, 22.0f, overtakes_, 3, glm::vec4(0.8f, 1.0f, 0.8f, 0.8f));

        // Day/night indicator: a bar that fills as night falls.
        hud_.bar(w - 250.0f, 22.0f, 150.0f, 14.0f, night_, glm::vec4(0.45f, 0.5f, 0.95f, 0.9f));

        if (crashFlash_ > 0.0f) {
            hud_.rect(0.0f, 0.0f, w, h, glm::vec4(0.8f, 0.1f, 0.1f, 0.45f * crashFlash_));
        }
        if (state_ == State::Crashed) {
            hud_.rect(0.0f, h * 0.32f, w, h * 0.30f, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
            hud_.number(w * 0.5f - 130.0f, h * 0.36f, 72.0f, score_, 7, white);
            hud_.number(w * 0.5f - 60.0f, h * 0.51f, 28.0f, best_, 7, amber);
            // Blinking strip hints that the run can be restarted.
            float blink = 0.5f + 0.5f * std::sin(window.time() * 5.0f);
            hud_.rect(w * 0.5f - 90.0f, h * 0.585f, 180.0f, 6.0f,
                glm::vec4(1.0f, 1.0f, 1.0f, 0.25f + 0.55f * blink));
        }
        if (paused_) {
            hud_.rect(0.0f, 0.0f, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
            hud_.rect(w * 0.5f - 34.0f, h * 0.45f, 22.0f, 70.0f, white);
            hud_.rect(w * 0.5f + 12.0f, h * 0.45f, 22.0f, 70.0f, white);
        }

        hud_.end();
    }

}  // namespace game

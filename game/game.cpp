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
    playerLane_ = 1;
    playerX_ = lanePosition(playerLane_);
    playerRoll_ = 0.0f;
    speed_ = 24.0f;
    boost_ = 1.0f;
    spawnTimer_ = 1.2f;
    traffic_.clear();
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

    // Car, local frame: origin on the ground, forward is -Z.
    b.clear();
    b.box(glm::translate(I, glm::vec3(0.0f, 0.55f, 0.0f)), glm::vec3(1.9f, 0.7f, 4.2f),
          glm::vec3(1.0f));
    b.wedge(glm::translate(I, glm::vec3(0.0f, 1.03f, 0.15f)), glm::vec3(1.66f, 0.66f, 2.1f), 0.72f,
            glm::vec3(1.0f));
    b.build(carBody_);

    b.clear();
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.cylinder(glm::translate(I, glm::vec3(sx * 0.92f, 0.36f, sz * 1.35f)), 0.36f, 0.26f, 14,
                       glm::vec3(0.07f, 0.07f, 0.08f));
        }
    }
    for (int sz = -1; sz <= 1; sz += 2) {
        b.box(glm::translate(I, glm::vec3(0.0f, 0.45f, sz * 2.12f)),
              glm::vec3(1.94f, 0.34f, 0.12f), glm::vec3(0.18f, 0.18f, 0.20f));
    }
    b.box(glm::translate(I, glm::vec3(0.0f, 1.06f, -0.82f)), glm::vec3(1.42f, 0.5f, 0.06f),
          glm::vec3(0.08f, 0.12f, 0.16f));  // windscreen
    b.box(glm::translate(I, glm::vec3(0.0f, 1.06f, 1.16f)), glm::vec3(1.36f, 0.44f, 0.06f),
          glm::vec3(0.08f, 0.12f, 0.16f));  // rear window
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
            } else {
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
    } else {
        // Roll to a stop, keep the world alive so the scene still reads as 3D.
        speed_ = std::max(speed_ - 26.0f * dt, 0.0f);
        crashFlash_ = std::max(crashFlash_ - dt * 0.9f, 0.0f);
        updateWorld(dt);
        for (auto& car : traffic_) car.z += (speed_ - car.speed) * dt;
    }

    updateDayNight(dt);
    collectLights();

    // Chase camera: it leans with the lane change and drifts back when quick.
    float shake = (state_ == State::Playing) ? std::sin(elapsed_ * 31.0f) * speed_ * 0.0015f : 0.0f;
    camera_.aspect = window.aspect();
    camera_.position = glm::vec3(playerX_ * 0.45f, 4.6f + shake, 10.2f + speed_ * 0.045f);
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

    spawnTimer_ -= dt;
    if (spawnTimer_ <= 0.0f) {
        float interval = std::max(1.15f - 0.08f * (level - 1), 0.30f);
        spawnTimer_ = interval * randRange(0.75f, 1.3f);
        spawnTraffic(SPAWN_Z + randRange(-15.0f, 15.0f));
    }

    for (auto& car : traffic_) {
        car.z += (speed_ - car.speed) * dt;

        // Lane switching keeps late levels unpredictable.
        car.changeCooldown -= dt;
        if (level >= 3 && car.lane == car.targetLane && car.changeCooldown <= 0.0f &&
            car.z < -25.0f) {
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

        if (!car.counted && car.z > 0.0f) {
            car.counted = true;
            ++overtakes_;
        }
    }

    traffic_.erase(std::remove_if(traffic_.begin(), traffic_.end(),
                                  [](const TrafficCar& c) { return c.z > DESPAWN_Z; }),
                   traffic_.end());
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
    float base = 24.0f + 2.4f * (level - 1);
    car.speed = base * randRange(0.38f, 0.74f);
    car.changeCooldown = randRange(1.5f, 5.0f);
    int index = static_cast<int>(randRange(0.0f, 7.999f));
    car.color = TRAFFIC_COLORS[index];
    traffic_.push_back(car);
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
    for (const auto& car : traffic_) {
        if (std::fabs(car.z) < CAR_HALF_Z * 2.0f - 0.25f &&
            std::fabs(car.x - playerX_) < CAR_HALF_X * 2.0f - 0.2f) {
            state_ = State::Crashed;
            crashFlash_ = 1.0f;
            best_ = std::max(best_, score_);
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
    } else {
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

void Game::drawCar(const glm::mat4& model, const glm::vec3& color, float headlights,
                   float taillights) {
    engine::Material body;
    body.tint = color;
    body.specular = 0.55f;
    body.shininess = 64.0f;
    renderer_.draw(carBody_, model, body);

    engine::Material trim;
    trim.specular = 0.2f;
    renderer_.draw(carTrim_, model, trim);

    engine::Material head;
    head.tint = glm::vec3(1.0f, 0.95f, 0.82f);
    head.emissive = headlights;
    renderer_.draw(carHead_, model, head);

    engine::Material tail;
    tail.tint = glm::vec3(1.0f, 0.18f, 0.12f);
    tail.emissive = taillights;
    renderer_.draw(carTail_, model, tail);
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

    for (const auto& car : traffic_) {
        glm::mat4 model = glm::translate(I, glm::vec3(car.x, 0.0f, car.z));
        drawCar(model, car.color, 0.15f * night_, 0.35f + 0.5f * night_);
    }

    glm::mat4 player = glm::translate(I, glm::vec3(playerX_, 0.0f, 0.0f));
    player = glm::rotate(player, playerRoll_, glm::vec3(0.0f, 0.0f, 1.0f));
    float braking = window.keyDown(GLFW_KEY_DOWN) || window.keyDown(GLFW_KEY_S) ? 1.0f : 0.4f;
    drawCar(player, glm::vec3(0.90f, 0.15f, 0.12f), glm::smoothstep(0.2f, 0.6f, night_), braking);

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

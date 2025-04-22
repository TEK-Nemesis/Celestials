#include "World.hpp"
#include "DataManager.hpp"
#include <Constants.hpp>
#include "CelestialObjectManager.hpp"

World::World() : totalTime(0.0f), immediateFadeFromNight(false), transitionCompletionDelay(0.0f), world(b2WorldId{}),
terrainMode(TerrainGenerationMode::BOTTOM), regenerationTriggered(false), regenerateDistantTriggered(false),
currentTimeOfDay(TimeOfDay::MID_DAY), targetTimeOfDay(TimeOfDay::MID_DAY),
skyTransitionTime(0.0f), skyTransitionDuration(1.0f), skyTransitioning(false), transitionProgress(0.0f),
lightColor(1.0f, 1.0f, 1.0f), targetLightColor(1.0f, 1.0f, 1.0f) {
    sceneNames = { "Summer", "Fall", "Winter", "Spring", "Alien" };
    scene = Scene::SUMMER;
    defaultSummerLowColor = glm::vec3(0.5f, 0.35f, 0.15f);
    defaultSummerHighColor = glm::vec3(0.2f, 0.5f, 0.2f);
    defaultFallLowColor = glm::vec3(0.472f, 0.431f, 0.345f);
    defaultFallHighColor = glm::vec3(0.618f, 0.413f, 0.193f);
    defaultWinterLowColor = glm::vec3(0.3f, 0.3f, 0.3f);
    defaultWinterHighColor = glm::vec3(0.9f, 0.9f, 1.0f);
    defaultSpringLowColor = glm::vec3(0.4f, 0.348f, 0.242f);
    defaultSpringHighColor = glm::vec3(0.332f, 0.363f, 0.211f);
    defaultAlienLowColor = glm::vec3(0.4f, 0.1f, 0.4f);
    defaultAlienHighColor = glm::vec3(0.1f, 0.5f, 0.5f);
    resetScene();
    resetDistantTerrainParams();
    resetBottomNoiseParameters();
    resetDistantNoiseParameters();
}

World::~World() {
    if (b2World_IsValid(world)) b2DestroyWorld(world);
}

bool World::initialize() {
    srand(static_cast<unsigned int>(time(nullptr)));

    b2Vec2 gravity = { 0.0f, -GRAVITY };
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = gravity;
    world = b2CreateWorld(&worldDef);
    if (!b2World_IsValid(world)) {
        DataManager::LogError("World", "initialize", "Failed to create Box2D world");
        return false;
    }

    // Initialize terrain with DebugTester dimensions and colors
    int terrainWidth = WINDOW_WIDTH + 400;
    int terrainDepth = 400;
    bottomTerrain = std::make_unique<Terrain>(terrainWidth, terrainDepth, glm::vec4(0.5f, 0.0f, 0.5f, 1.0f));
    distantTerrain = std::make_unique<Terrain>(terrainWidth, 200, glm::vec4(0.1f, 0.15f, 0.45f, 1.0f));
    initializeNoiseParameters();

    noise::module::Perlin perlin;
    perlin.SetSeed(static_cast<int>(time(nullptr)));
    perlin.SetFrequency(noiseParamsBottom.frequency);
    perlin.SetPersistence(noiseParamsBottom.persistence);
    perlin.SetLacunarity(noiseParamsBottom.lacunarity);
    perlin.SetOctaveCount(static_cast<int>(noiseParamsBottom.octaves));

    // Generate bottom terrain with DebugTester parameters
    bottomTerrain->generate(perlin, noiseParamsBottom.baseHeight, noiseParamsBottom.minHeight, noiseParamsBottom.maxHeight, terrainLowColor, terrainHighColor, nullptr);

    perlin.SetFrequency(noiseParamsDistant.frequency);
    perlin.SetPersistence(noiseParamsDistant.persistence);
    perlin.SetLacunarity(noiseParamsDistant.lacunarity);
    perlin.SetOctaveCount(static_cast<int>(noiseParamsDistant.octaves));

    // Generate distant terrain with DebugTester parameters
    distantTerrain->generate(perlin, noiseParamsDistant.baseHeight, noiseParamsDistant.minHeight, noiseParamsDistant.maxHeight, terrainLowColor, terrainHighColor, nullptr);

    DataManager::LogDebug(DebugCategory::RENDERING, "World", "initialize",
        "distantTerrain generated: vertices=" + std::to_string(distantTerrain->getVertices().size()) +
        ", indices=" + std::to_string(distantTerrain->getIndices().size()));

    setupPhysicsTerrain();

    celestialObjectManager = std::make_unique<CelestialObjectManager>(scene, this);
    celestialObjectManager->initialize();

    return true;
}

void World::update(float dt) {
    totalTime += dt;

    b2World_Step(world, dt, 8);

    if (skyTransitioning) {
        skyTransitionTime += dt;
        float t = std::min(skyTransitionTime / skyTransitionDuration, 1.0f);
        lightColor = glm::mix(lightColor, targetLightColor, t);
        transitionProgress = t;

        if (t >= 1.0f) {
            skyTransitioning = false;
            skyTransitionTime = 0.0f;
            currentTimeOfDay = targetTimeOfDay;
            transitionProgress = 1.0f;
        }
    }

    // Check if bottom terrain regeneration is triggered
    if (regenerationTriggered) {
        noise::module::Perlin perlin;
        perlin.SetSeed(static_cast<int>(time(nullptr)));
        perlin.SetFrequency(noiseParamsBottom.frequency);
        perlin.SetPersistence(noiseParamsBottom.persistence);
        perlin.SetLacunarity(noiseParamsBottom.lacunarity);
        perlin.SetOctaveCount(static_cast<int>(noiseParamsBottom.octaves));

        bottomTerrain->generate(perlin, noiseParamsBottom.baseHeight, noiseParamsBottom.minHeight, noiseParamsBottom.maxHeight, terrainLowColor, terrainHighColor, nullptr);
        setupPhysicsTerrain();
        regenerationTriggered = false;
    }

    // Check if distant terrain regeneration is triggered
    if (regenerateDistantTriggered) {
        noise::module::Perlin perlin;
        perlin.SetSeed(static_cast<int>(time(nullptr)));
        perlin.SetFrequency(noiseParamsDistant.frequency);
        perlin.SetPersistence(noiseParamsDistant.persistence);
        perlin.SetLacunarity(noiseParamsDistant.lacunarity);
        perlin.SetOctaveCount(static_cast<int>(noiseParamsDistant.octaves));

        distantTerrain->generate(perlin, noiseParamsDistant.baseHeight, noiseParamsDistant.minHeight, noiseParamsDistant.maxHeight, terrainLowColor, terrainHighColor, nullptr);
        regenerateDistantTriggered = false;
    }

    celestialObjectManager->update(dt, currentTimeOfDay);
}

void World::setTimeOfDay(TimeOfDay newTime) {
    skyTransitioning = true;
    skyTransitionTime = 0.0f;
    targetTimeOfDay = newTime;

    if (currentTimeOfDay == TimeOfDay::NIGHT && newTime != TimeOfDay::NIGHT) {
        immediateFadeFromNight = true;
        transitionProgress = 0.0f;
    } else if (newTime == TimeOfDay::NIGHT) {
        immediateFadeFromNight = false;
        transitionProgress = 0.0f;
    } else {
        immediateFadeFromNight = false;
    }

    switch (newTime) {
        case TimeOfDay::DAWN:
            targetLightColor = glm::vec3(0.9f, 0.8f, 0.7f);
            break;
        case TimeOfDay::MID_DAY:
            targetLightColor = glm::vec3(1.0f, 1.0f, 0.95f);
            break;
        case TimeOfDay::DUSK:
            targetLightColor = glm::vec3(0.8f, 0.7f, 0.6f);
            break;
        case TimeOfDay::NIGHT:
            targetLightColor = glm::vec3(0.3f, 0.3f, 0.5f);
            break;
    }
}

void World::initializeNoiseParameters() {
    resetBottomNoiseParameters();
    resetDistantNoiseParameters();
}

void World::resetBottomNoiseParameters() {
    noiseParamsBottom.baseHeight = 0.0f;
    noiseParamsBottom.minHeight = WINDOW_HEIGHT * 0.2f;
    noiseParamsBottom.maxHeight = WINDOW_HEIGHT * 0.4f;
    noiseParamsBottom.frequency = 0.600f;
    noiseParamsBottom.persistence = 0.450f;
    noiseParamsBottom.lacunarity = 1.669f;
    noiseParamsBottom.octaves = 8.0f;
}

void World::resetDistantNoiseParameters() {
    noiseParamsDistant.baseHeight = 0.0f;
    noiseParamsDistant.minHeight = WINDOW_HEIGHT * 0.5f;
    noiseParamsDistant.maxHeight = WINDOW_HEIGHT * 0.6f;
    noiseParamsDistant.frequency = 0.600f;
    noiseParamsDistant.persistence = 0.450f;
    noiseParamsDistant.lacunarity = 2.000f;
    noiseParamsDistant.octaves = 6.0f;
}

void World::resetDistantTerrainParams() {
    distantParams.zPosition = 250.0f;
    distantParams.yOffset = 0.0f;
    distantParams.colorFade = 0.3f;
    distantParams.depthFade = 0.567f;
}

void World::resetScene() {
    switch (scene) {
        case Scene::SUMMER:
            terrainLowColor = defaultSummerLowColor;
            terrainHighColor = defaultSummerHighColor;
            break;
        case Scene::FALL:
            terrainLowColor = defaultFallLowColor;
            terrainHighColor = defaultFallHighColor;
            break;
        case Scene::WINTER:
            terrainLowColor = defaultWinterLowColor;
            terrainHighColor = defaultWinterHighColor;
            break;
        case Scene::SPRING:
            terrainLowColor = defaultSpringLowColor;
            terrainHighColor = defaultSpringHighColor;
            break;
        case Scene::ALIEN:
            terrainLowColor = defaultAlienLowColor;
            terrainHighColor = defaultAlienHighColor;
            break;
    }
}

void World::triggerRegeneration(TerrainGenerationMode mode) {
    terrainMode = mode;
    if (mode == TerrainGenerationMode::BOTTOM) {
        regenerationTriggered = true;
    } else if (mode == TerrainGenerationMode::DISTANT) {
        regenerateDistantTriggered = true;
    }
}

void World::setupPhysicsTerrain() {
    std::vector<float> bottomHeightmap = bottomTerrain->getHeightmap(50);
    std::vector<b2Vec2> bottomPoints;
    for (int x = 0; x < bottomHeightmap.size(); ++x) {
        float y = bottomHeightmap[x];
        if (y == FLT_MAX || y < 0.0f || y > WINDOW_HEIGHT) {
            y = WINDOW_HEIGHT;
        }
        bottomPoints.emplace_back(b2Vec2{ static_cast<float>(x) * 2.0f / PIXELS_PER_METER, y / PIXELS_PER_METER });
    }

    b2BodyDef bottomBodyDef = b2DefaultBodyDef();
    bottomBodyDef.type = b2_staticBody;
    bottomBodyDef.position = b2Vec2{ 0.0f, 0.0f };
    b2BodyId bottomGroundBody = b2CreateBody(world, &bottomBodyDef);

    b2ChainDef bottomChainDef = b2DefaultChainDef();
    bottomChainDef.points = bottomPoints.data();
    bottomChainDef.count = static_cast<int32_t>(bottomPoints.size());
    b2CreateChain(bottomGroundBody, &bottomChainDef);
}
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "box2d/box2d.h"
#include "Terrain.hpp"
#include "Enums.hpp"
#include "CelestialObjectManager.hpp"

// this struct should be moved into the appropriate class
struct NoiseParameters {
    float baseHeight;
    float minHeight;
    float maxHeight;
    float frequency;
    float persistence;
    float lacunarity;
    float octaves;
};

// this struct should be moved into the appropriate class
struct DistantTerrainParameters {
    float zPosition;
    float yOffset;
    float colorFade;
    float depthFade;
};

class World {
public:
    World();
    ~World();

    bool initialize();
    void update(float dt);
    void setTimeOfDay(TimeOfDay newTime);

    Scene getScene() const { return scene; }
    void setScene(Scene newScene) { scene = newScene; resetScene(); }
    TimeOfDay getCurrentTimeOfDay() const { return currentTimeOfDay; }
    float getTransitionProgress() const { return transitionProgress; }
    bool isImmediateFadeFromNight() const { return immediateFadeFromNight; }
    float getTotalTime() const { return totalTime; }
    glm::vec3 getLightColor() const { return lightColor; }
    Terrain* getBottomTerrain() { return bottomTerrain.get(); }
    Terrain* getDistantTerrain() { return distantTerrain.get(); }
    DistantTerrainParameters& getDistantParams() { return distantParams; }
    NoiseParameters& getBottomNoiseParams() { return noiseParamsBottom; }
    NoiseParameters& getDistantNoiseParams() { return noiseParamsDistant; }
    void triggerRegeneration(TerrainGenerationMode mode);
    void resetBottomNoiseParameters();
    void resetDistantNoiseParameters();
    void resetDistantTerrainParams();
    void resetScene();
    CelestialObjectManager* getCelestialObjectManager() { return celestialObjectManager.get(); }

private:
    float totalTime;
    bool immediateFadeFromNight;
    float transitionCompletionDelay;
    b2WorldId world;
    TerrainGenerationMode terrainMode;
    bool regenerationTriggered;
    bool regenerateDistantTriggered;
    TimeOfDay currentTimeOfDay;
    TimeOfDay targetTimeOfDay;
    float skyTransitionTime;
    float skyTransitionDuration;
    bool skyTransitioning;
    float transitionProgress;
    glm::vec3 lightColor;
    glm::vec3 targetLightColor;
    std::vector<std::string> sceneNames;
    Scene scene;
    glm::vec3 defaultSummerLowColor;
    glm::vec3 defaultSummerHighColor;
    glm::vec3 defaultFallLowColor;
    glm::vec3 defaultFallHighColor;
    glm::vec3 defaultWinterLowColor;
    glm::vec3 defaultWinterHighColor;
    glm::vec3 defaultSpringLowColor;
    glm::vec3 defaultSpringHighColor;
    glm::vec3 defaultAlienLowColor;
    glm::vec3 defaultAlienHighColor;
    glm::vec3 terrainLowColor;
    glm::vec3 terrainHighColor;
    std::unique_ptr<Terrain> bottomTerrain;
    std::unique_ptr<Terrain> distantTerrain;
    std::unique_ptr<CelestialObjectManager> celestialObjectManager;
    NoiseParameters noiseParamsBottom;
    NoiseParameters noiseParamsDistant;
    DistantTerrainParameters distantParams;

    void initializeNoiseParameters();
    void setupPhysicsTerrain();
};

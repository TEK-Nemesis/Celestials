#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <random>
#include "Enums.hpp"

// Forward declaration
class World;
class Renderer;

class CelestialObjectManager {
public:
    CelestialObjectManager(Scene scene, World* world);
    ~CelestialObjectManager();

    bool initialize();
    void update(float dt, TimeOfDay timeOfDay);
    void render(float starAlpha, float sunMoonPosition);
    void renderCloseCelestials(float starAlpha, float sunMoonPosition);
    void renderText(Renderer* renderer, float starAlpha);

    void toggleShowConstellationNames() { showConstellationNames = !showConstellationNames; }
    void toggleShowPlanetNames() { showPlanetNames = !showPlanetNames; }
    void toggleShowSatelliteNames() { showSatelliteNames = !showSatelliteNames; }
    bool getShowConstellationNames() { return showConstellationNames; }
    bool getShowPlanetNames() { return showPlanetNames; }
    bool getShowSatelliteNames() { return showSatelliteNames; }

    void setScene(Scene newScene) {
        scene = newScene;
        selectRandomSkyPattern(); // Re-select pattern when scene changes
        initialize(); // Re-initialize to apply new pattern
    }

private:

    enum class SkyPattern {
        EARTH_NORTH,
        EARTH_EAST,
        EARTH_SOUTH,
        EARTH_WEST,
        ALIEN_PATTERN_1,
        ALIEN_PATTERN_2,
        ALIEN_PATTERN_3,
        ALIEN_PATTERN_4
    };

    struct Star {
        glm::vec2 position;
        float brightness;
    };

    struct Constellation {
        std::vector<int> starIndices;
        std::string name;
    };

    struct ShootingStar {
        glm::vec2 position;
        glm::vec2 velocity;
        float brightness;
        float lifetime;
    };

    struct ExhaustSegment {
        glm::vec2 startPos;
        glm::vec2 endPos;
        float lifetime;
        float alpha;
    };

    struct Satellite {
        std::string name;
        float speed;
        glm::vec2 position;
        glm::vec2 heading;
        glm::vec3 textColor;
        float size;
        float brightness;
        bool isISS;
        float lastDisplayedTime;
        std::vector<ExhaustSegment> trail;
    };

    struct StarlinkTrain {
        std::vector<glm::vec2> positions;
        glm::vec2 heading;
        float speed;
        float brightness;
        float size;
        float spacing;
        int count;
        float lastDisplayedTime;
        std::vector<std::vector<ExhaustSegment>> trails;
    };

    struct Planet { // distant and rendered without PNG, like Praxis, Boreth, Mars, Jupiter, etc.
        glm::vec2 position;
        float brightness;
        glm::vec3 color;
        float size;
        std::string name;
    };

    struct CloseCelestial {  // sun, moon, and single alien planet Qo'noS
        std::string name;
        glm::vec2 position;
        float size;
        glm::vec3 tintColor;
        float rotation;
        float brightness;
        std::string texturePath;
        GLuint texture;
        std::string celestialType;
    };

    World* world;
    Scene scene;
    SkyPattern currentPattern;

    // Random number generator
    static std::mt19937 rng; // Static to ensure one instance across all objects

    GLuint starShader;
    GLuint starVAO, starVBO;
    GLuint smokeShader;
    GLuint smokeVAO, smokeVBO;
    std::vector<Star> stars;

    std::vector<Constellation> constellations;
    float constellationScale = 1.0f;  // scale for constellation span (to increase/decrease size)
    float constellationStarScale = 0.75f;  // scale for stars used in a constellation (to increase/decrease size)

    std::vector<ShootingStar> shootingStars;
    std::vector<Satellite> satellites;
    std::vector<StarlinkTrain> starlinkTrains;
    std::vector<Planet> planets;
    float shootingStarTimer;
    float satelliteTimer;
    float starlinkTimer;
    float totalTime;
    float timeFactor = 0.5f;
    GLuint glowTexture; // sun glow texture

    std::vector<CloseCelestial> closeCelestials;
    GLuint closeCelestialShader;
    GLuint closeCelestialVAO, closeCelestialVBO, closeCelestialEBO;

    bool showConstellationNames;
    bool showPlanetNames;
    bool showSatelliteNames;

    std::map<std::string, float> satelliteDisplayHistory;

    void initializeStars();
    void initializeCloseCelestials();
    void updateShootingStars(float dt, TimeOfDay currentTime);
    void updateSatellites(float dt, TimeOfDay currentTime);
    void updateStarlinkTrains(float dt, TimeOfDay currentTime);

    void selectRandomSkyPattern();
    void setupRandomStars();
    void setupEarthSkyPattern();
    void setupEarthNorth();
    void setupEarthEast();
    void setupEarthSouth();
    void setupEarthWest();
    void setupAlienSkyPattern();
    void setupAlienPattern1();
    void setupAlienPattern2();
    void setupAlienPattern3();
    void setupAlienPattern4();
    void initializeStarBuffers();
};

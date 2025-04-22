#include "CelestialObjectManager.hpp"
#include "Renderer.hpp"
#include <iostream>
#include "DataManager.hpp"
#include "Constants.hpp"
#include <tuple>
#include <SDL3_image/SDL_image.h>
#include <algorithm>  // for clamp
#include <random>

// Define the static rng member
std::mt19937 CelestialObjectManager::rng(static_cast<unsigned int>(time(0)));

CelestialObjectManager::CelestialObjectManager(Scene scene, World* world)
    : scene(scene), world(world), starShader(0), starVAO(0), starVBO(0),
    smokeShader(0), smokeVAO(0), smokeVBO(0), totalTime(0.0f),
    shootingStarTimer(0.0f), satelliteTimer(0.0f), starlinkTimer(0.0f),
    showConstellationNames(false), showPlanetNames(false), showSatelliteNames(false),
    closeCelestialShader(0), closeCelestialVAO(0), closeCelestialVBO(0), closeCelestialEBO(0) {
}

CelestialObjectManager::~CelestialObjectManager() {
    if (starShader) glDeleteProgram(starShader);
    if (starVAO) glDeleteVertexArrays(1, &starVAO);
    if (starVBO) glDeleteBuffers(1, &starVBO);
    if (smokeShader) glDeleteProgram(smokeShader);
    if (smokeVAO) glDeleteVertexArrays(1, &smokeVAO);
    if (smokeVBO) glDeleteBuffers(1, &smokeVBO);
    if (closeCelestialShader) glDeleteProgram(closeCelestialShader);
    if (closeCelestialVAO) glDeleteVertexArrays(1, &closeCelestialVAO);
    if (closeCelestialVBO) glDeleteBuffers(1, &closeCelestialVBO);
    if (closeCelestialEBO) glDeleteBuffers(1, &closeCelestialEBO);
    for (auto& celestial : closeCelestials) {
        if (celestial.texture) glDeleteTextures(1, &celestial.texture);
    }
}

bool CelestialObjectManager::initialize() {
    // Clear all celestial objects and reset timers
    stars.clear();
    constellations.clear();
    planets.clear();
    satellites.clear();
    starlinkTrains.clear();
    shootingStars.clear();
    closeCelestials.clear();
    satelliteDisplayHistory.clear();
    shootingStarTimer = 0.0f;
    satelliteTimer = 0.0f;
    starlinkTimer = 0.0f;
    totalTime = 0.0f;

    selectRandomSkyPattern();
    initializeStars();
    initializeCloseCelestials();

    return true;
}

void CelestialObjectManager::selectRandomSkyPattern() {
    std::uniform_int_distribution<int> dist(0, 3);

    if (scene == Scene::ALIEN) {
        int patternIndex = dist(rng);
        switch (patternIndex) {
            case 0: currentPattern = SkyPattern::ALIEN_PATTERN_1; break;
            case 1: currentPattern = SkyPattern::ALIEN_PATTERN_2; break;
            case 2: currentPattern = SkyPattern::ALIEN_PATTERN_3; break;
            case 3: currentPattern = SkyPattern::ALIEN_PATTERN_4; break;
        }
        DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "selectRandomSkyPattern",
            "Selected alien pattern: " + std::to_string(static_cast<int>(currentPattern)));
    } else {
        int patternIndex = dist(rng);
        switch (patternIndex) {
            case 0: currentPattern = SkyPattern::EARTH_NORTH; break;
            case 1: currentPattern = SkyPattern::EARTH_EAST; break;
            case 2: currentPattern = SkyPattern::EARTH_SOUTH; break;
            case 3: currentPattern = SkyPattern::EARTH_WEST; break;
        }
        DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "selectRandomSkyPattern",
            "Selected earth pattern: " + std::to_string(static_cast<int>(currentPattern)));
    }
}

void CelestialObjectManager::initializeStars() {
    stars.clear();
    constellations.clear();
    planets.clear();
    satellites.clear();
    starlinkTrains.clear();
    shootingStars.clear();
    satelliteDisplayHistory.clear();

    setupRandomStars();

    if (scene == Scene::ALIEN) {
        setupAlienSkyPattern();
    } else {
        setupEarthSkyPattern();
    }

    initializeStarBuffers();
}

void CelestialObjectManager::setupRandomStars() {
    const int numStars = 50;
    const float minDist = 0.1f;
    const int k = 30;
    std::vector<glm::vec2> activeList;
    std::vector<glm::vec2> points;

    activeList.push_back(glm::vec2(0.5f, 0.5f));
    points.push_back(activeList[0]);

    std::uniform_int_distribution<int> idxDist(0, 1000); // For selecting activeList index
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> radiusDist(minDist, minDist * 2.0f);
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> brightnessDist(0.0f, 1.0f);

    while (!activeList.empty() && points.size() < numStars) {
        int idx = idxDist(rng) % activeList.size();
        glm::vec2 center = activeList[idx];
        bool placed = false;

        for (int i = 0; i < k; ++i) {
            float angle = angleDist(rng);
            float radius = radiusDist(rng);
            glm::vec2 newPoint = center + glm::vec2(cos(angle), sin(angle)) * radius;

            if (newPoint.y < 0.3f) {
                if (probDist(rng) > 0.02f) {
                    continue;
                }
            }
            float yProbability = newPoint.y * newPoint.y;
            if (probDist(rng) > yProbability) continue;

            if (newPoint.x < 0.0f || newPoint.x > 1.0f || newPoint.y < 0.0f || newPoint.y > 1.0f) continue;

            bool tooClose = false;
            for (const auto& point : points) {
                if (glm::distance(newPoint, point) < minDist) {
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose) {
                activeList.push_back(newPoint);
                points.push_back(newPoint);
                placed = true;
            }
        }

        if (!placed) {
            activeList.erase(activeList.begin() + idx);
        }
    }

    stars.resize(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        Star& star = stars[i];
        star.position = points[i];
        star.brightness = 0.4f + brightnessDist(rng) * 0.3f + star.position.y * 0.2f;
    }

    // Add distant stars
    const int numDistantStars = 100;
    int addedDistantStars = 0;
    while (addedDistantStars < numDistantStars) {
        Star distantStar;
        distantStar.position = glm::vec2(
            probDist(rng),
            probDist(rng)
        );
        if (distantStar.position.y < 0.3f) {
            if (probDist(rng) > 0.02f) {
                continue;
            }
        }
        distantStar.brightness = 0.2f + brightnessDist(rng) * 0.1f;
        stars.push_back(distantStar);
        addedDistantStars++;
    }

    DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "setupRandomStars",
        "Stars initialized: stars.size()=" + std::to_string(stars.size()));
}

void CelestialObjectManager::setupEarthSkyPattern() {
    switch (currentPattern) {
        case SkyPattern::EARTH_NORTH: setupEarthNorth(); break;
        case SkyPattern::EARTH_EAST: setupEarthEast(); break;
        case SkyPattern::EARTH_SOUTH: setupEarthSouth(); break;
        case SkyPattern::EARTH_WEST: setupEarthWest(); break;
        default: setupEarthNorth(); break; // Fallback to North
    }
}

void CelestialObjectManager::setupAlienSkyPattern() {
    switch (currentPattern) {
        case SkyPattern::ALIEN_PATTERN_1: setupAlienPattern1(); break;
        case SkyPattern::ALIEN_PATTERN_2: setupAlienPattern2(); break;
        case SkyPattern::ALIEN_PATTERN_3: setupAlienPattern3(); break;
        case SkyPattern::ALIEN_PATTERN_4: setupAlienPattern4(); break;
        default: setupAlienPattern1(); break; // Fallback to Pattern 1
    }
}

void CelestialObjectManager::setupEarthNorth() {
    // Edmonton, Alberta (latitude ~53.5°N) looking North
    // Prominent constellations: Ursa Major, Cassiopeia, Cepheus, Draco
    // Planets: Typically, planets are near the ecliptic, so few may be visible directly north, but we'll include a possible sighting like Jupiter

    Constellation ursaMajor;
    ursaMajor.name = "URSA MAJOR";
    ursaMajor.starIndices = { 0, 1, 2, 3, 4, 5, 6 };
    stars[0].position = glm::vec2(0.35f, 0.70f); // Dubhe (bowl top-left)
    stars[1].position = glm::vec2(0.40f, 0.65f); // Merak (bowl bottom-left)
    stars[2].position = glm::vec2(0.45f, 0.67f); // Phecda (bowl bottom-right)
    stars[3].position = glm::vec2(0.50f, 0.72f); // Megrez (bowl top-right)
    stars[4].position = glm::vec2(0.55f, 0.69f); // Alioth (handle base)
    stars[5].position = glm::vec2(0.60f, 0.65f); // Mizar (handle middle)
    stars[6].position = glm::vec2(0.65f, 0.61f); // Alkaid (handle end)
    stars[0].brightness = 0.9f;
    stars[1].brightness = 0.85f;
    stars[2].brightness = 0.7f;
    stars[3].brightness = 0.65f;
    stars[4].brightness = 0.8f;
    stars[5].brightness = 0.8f;
    stars[6].brightness = 0.9f;
    constellations.push_back(ursaMajor);

    Constellation cassiopeia;
    cassiopeia.name = "CASSIOPEIA";
    cassiopeia.starIndices = { 7, 8, 9, 10, 11 };
    stars[7].position = glm::vec2(0.70f, 0.88f); // Schedar
    stars[8].position = glm::vec2(0.73f, 0.84f); // Caph
    stars[9].position = glm::vec2(0.76f, 0.86f); // Gamma Cas
    stars[10].position = glm::vec2(0.79f, 0.82f); // Ruchbah
    stars[11].position = glm::vec2(0.82f, 0.84f); // Epsilon Cas
    stars[7].brightness = 0.9f;
    stars[8].brightness = 0.85f;
    stars[9].brightness = 0.95f;
    stars[10].brightness = 0.8f;
    stars[11].brightness = 0.7f;
    constellations.push_back(cassiopeia);

    Constellation cepheus;
    cepheus.name = "CEPHEUS";
    cepheus.starIndices = { 12, 13, 14, 15, 16 };
    stars[12].position = glm::vec2(0.60f, 0.90f); // Alderamin
    stars[13].position = glm::vec2(0.65f, 0.87f); // Alfirk
    stars[14].position = glm::vec2(0.68f, 0.92f); // Gamma Cep
    stars[15].position = glm::vec2(0.55f, 0.85f); // Delta Cep
    stars[16].position = glm::vec2(0.58f, 0.80f); // Epsilon Cep
    stars[12].brightness = 0.9f;
    stars[13].brightness = 0.8f;
    stars[14].brightness = 0.85f;
    stars[15].brightness = 0.7f;
    stars[16].brightness = 0.75f;
    constellations.push_back(cepheus);

    Constellation draco;
    draco.name = "DRACO";
    draco.starIndices = { 17, 18, 19, 20, 21 };
    stars[17].position = glm::vec2(0.30f, 0.85f); // Thuban
    stars[18].position = glm::vec2(0.35f, 0.90f); // Rastaban
    stars[19].position = glm::vec2(0.40f, 0.87f); // Eltanin
    stars[20].position = glm::vec2(0.45f, 0.82f); // Delta Dra
    stars[21].position = glm::vec2(0.50f, 0.78f); // Epsilon Dra
    stars[17].brightness = 0.7f;
    stars[18].brightness = 0.85f;
    stars[19].brightness = 0.9f;
    stars[20].brightness = 0.75f;
    stars[21].brightness = 0.7f;
    constellations.push_back(draco);

    // Add a planet: Jupiter might be visible low on the northern horizon if near the ecliptic
    Planet jupiter;
    jupiter.position = glm::vec2(0.90f, 0.60f);
    jupiter.brightness = 1.0f;
    jupiter.color = glm::vec3(1.0f, 0.9f, 0.8f);
    jupiter.size = 6.0f;
    jupiter.name = "JUPITER";
    planets.push_back(jupiter);
}

void CelestialObjectManager::setupEarthEast() {
    // Edmonton, Alberta looking East
    // Prominent constellations: Taurus, Auriga, Perseus, Andromeda
    // Planets: Venus (morning star), Mars, Jupiter may be visible

    Constellation taurus;
    taurus.name = "TAURUS";
    taurus.starIndices = { 0, 1, 2, 3, 4, 5, 6 };
    stars[0].position = glm::vec2(0.80f, 0.70f); // Aldebaran
    stars[1].position = glm::vec2(0.82f, 0.72f); // Hyades tip
    stars[2].position = glm::vec2(0.78f, 0.72f); // Hyades tip
    stars[3].position = glm::vec2(0.83f, 0.67f); // Hyades base
    stars[4].position = glm::vec2(0.77f, 0.67f); // Hyades base
    stars[5].position = glm::vec2(0.85f, 0.73f); // Elnath
    stars[6].position = glm::vec2(0.75f, 0.73f); // Zeta Tau
    stars[0].brightness = 1.0f;
    stars[1].brightness = 0.7f;
    stars[2].brightness = 0.7f;
    stars[3].brightness = 0.65f;
    stars[4].brightness = 0.65f;
    stars[5].brightness = 0.8f;
    stars[6].brightness = 0.75f;
    constellations.push_back(taurus);

    Constellation auriga;
    auriga.name = "AURIGA";
    auriga.starIndices = { 7, 8, 9, 10, 11 };
    stars[7].position = glm::vec2(0.60f, 0.80f); // Capella
    stars[8].position = glm::vec2(0.65f, 0.78f); // Menkalinan
    stars[9].position = glm::vec2(0.62f, 0.75f); // Theta Aur
    stars[10].position = glm::vec2(0.58f, 0.72f); // Delta Aur
    stars[11].position = glm::vec2(0.55f, 0.76f); // Epsilon Aur
    stars[7].brightness = 1.0f;
    stars[8].brightness = 0.8f;
    stars[9].brightness = 0.7f;
    stars[10].brightness = 0.75f;
    stars[11].brightness = 0.7f;
    constellations.push_back(auriga);

    Constellation perseus;
    perseus.name = "PERSEUS";
    perseus.starIndices = { 12, 13, 14, 15, 16 };
    stars[12].position = glm::vec2(0.45f, 0.85f); // Mirfak
    stars[13].position = glm::vec2(0.50f, 0.82f); // Algol
    stars[14].position = glm::vec2(0.48f, 0.87f); // Delta Per
    stars[15].position = glm::vec2(0.40f, 0.80f); // Epsilon Per
    stars[16].position = glm::vec2(0.42f, 0.75f); // Zeta Per
    stars[12].brightness = 0.9f;
    stars[13].brightness = 0.85f;
    stars[14].brightness = 0.7f;
    stars[15].brightness = 0.75f;
    stars[16].brightness = 0.8f;
    constellations.push_back(perseus);

    Constellation andromeda;
    andromeda.name = "ANDROMEDA";
    andromeda.starIndices = { 17, 18, 19, 20 };
    stars[17].position = glm::vec2(0.30f, 0.70f); // Alpheratz
    stars[18].position = glm::vec2(0.35f, 0.68f); // Mirach
    stars[19].position = glm::vec2(0.40f, 0.66f); // Almach
    stars[20].position = glm::vec2(0.37f, 0.63f); // Delta And
    stars[17].brightness = 0.9f;
    stars[18].brightness = 0.85f;
    stars[19].brightness = 0.8f;
    stars[20].brightness = 0.7f;
    constellations.push_back(andromeda);

    // Add planets: Venus, Mars, Jupiter
    Planet venus;
    venus.position = glm::vec2(0.90f, 0.65f);
    venus.brightness = 1.2f;
    venus.color = glm::vec3(1.0f, 1.0f, 0.9f);
    venus.size = 4.0f;
    venus.name = "VENUS";
    planets.push_back(venus);

    Planet mars;
    mars.position = glm::vec2(0.85f, 0.60f);
    mars.brightness = 0.9f;
    mars.color = glm::vec3(1.0f, 0.5f, 0.5f);
    mars.size = 4.0f;
    mars.name = "MARS";
    planets.push_back(mars);

    Planet jupiter;
    jupiter.position = glm::vec2(0.80f, 0.60f); 
    jupiter.brightness = 1.0f;
    jupiter.color = glm::vec3(1.0f, 0.9f, 0.8f);
    jupiter.size = 6.0f;
    jupiter.name = "JUPITER";
    planets.push_back(jupiter);
}

void CelestialObjectManager::setupEarthSouth() {
    // Edmonton, Alberta looking South
    // Prominent constellations: Sagittarius, Scorpius, Capricornus, Aquarius
    // Planets: Jupiter, Saturn may be visible along the ecliptic

    Constellation sagittarius;
    sagittarius.name = "SAGITTARIUS";
    sagittarius.starIndices = { 0, 1, 2, 3, 4, 5, 6 };
    stars[0].position = glm::vec2(0.50f, 0.60f); // Nunki
    stars[1].position = glm::vec2(0.55f, 0.62f); // Kaus Australis
    stars[2].position = glm::vec2(0.52f, 0.65f); // Kaus Media
    stars[3].position = glm::vec2(0.48f, 0.67f); // Kaus Borealis
    stars[4].position = glm::vec2(0.45f, 0.63f); // Ascella
    stars[5].position = glm::vec2(0.40f, 0.61f); // Tau Sgr
    stars[6].position = glm::vec2(0.43f, 0.58f); // Phi Sgr
    stars[0].brightness = 0.9f;
    stars[1].brightness = 0.85f;
    stars[2].brightness = 0.8f;
    stars[3].brightness = 0.75f;
    stars[4].brightness = 0.8f;
    stars[5].brightness = 0.7f;
    stars[6].brightness = 0.7f;
    constellations.push_back(sagittarius);

    Constellation scorpius;
    scorpius.name = "SCORPIUS";
    scorpius.starIndices = { 7, 8, 9, 10, 11, 12, 13 };
    stars[7].position = glm::vec2(0.20f, 0.65f); // Antares
    stars[8].position = glm::vec2(0.22f, 0.67f); // Dschubba
    stars[9].position = glm::vec2(0.24f, 0.69f); // Pi Sco
    stars[10].position = glm::vec2(0.18f, 0.63f); // Sigma Sco
    stars[11].position = glm::vec2(0.16f, 0.61f); // Tau Sco
    stars[12].position = glm::vec2(0.14f, 0.59f); // Shaula
    stars[13].position = glm::vec2(0.17f, 0.57f); // Lesath
    stars[7].brightness = 1.0f;
    stars[8].brightness = 0.8f;
    stars[9].brightness = 0.7f;
    stars[10].brightness = 0.75f;
    stars[11].brightness = 0.8f;
    stars[12].brightness = 0.9f;
    stars[13].brightness = 0.85f;
    constellations.push_back(scorpius);

    Constellation capricornus;
    capricornus.name = "CAPRICORNUS";
    capricornus.starIndices = { 14, 15, 16, 17, 18 };
    stars[14].position = glm::vec2(0.70f, 0.55f); // Deneb Algedi
    stars[15].position = glm::vec2(0.65f, 0.57f); // Dabih
    stars[16].position = glm::vec2(0.60f, 0.54f); // Algedi
    stars[17].position = glm::vec2(0.67f, 0.52f); // Delta Cap
    stars[18].position = glm::vec2(0.63f, 0.50f); // Gamma Cap
    stars[14].brightness = 0.8f;
    stars[15].brightness = 0.85f;
    stars[16].brightness = 0.7f;
    stars[17].brightness = 0.75f;
    stars[18].brightness = 0.7f;
    constellations.push_back(capricornus);

    Constellation aquarius;
    aquarius.name = "AQUARIUS";
    aquarius.starIndices = { 19, 20, 21, 22 };
    stars[19].position = glm::vec2(0.80f, 0.60f); // Sadalmelik
    stars[20].position = glm::vec2(0.85f, 0.58f); // Sadalsuud
    stars[21].position = glm::vec2(0.82f, 0.55f); // Albali
    stars[22].position = glm::vec2(0.78f, 0.53f); // Delta Aqr
    stars[19].brightness = 0.85f;
    stars[20].brightness = 0.8f;
    stars[21].brightness = 0.75f;
    stars[22].brightness = 0.7f;
    constellations.push_back(aquarius);

    // Add planets: Jupiter, Saturn
    Planet jupiter;
    jupiter.position = glm::vec2(0.55f, 0.55f);
    jupiter.brightness = 1.0f;
    jupiter.color = glm::vec3(1.0f, 0.9f, 0.8f);
    jupiter.size = 6.0f;
    jupiter.name = "JUPITER";
    planets.push_back(jupiter);

    Planet saturn;
    saturn.position = glm::vec2(0.60f, 0.60f); 
    saturn.brightness = 0.8f;
    saturn.color = glm::vec3(1.0f, 0.9f, 0.7f);
    saturn.size = 4.0f;
    saturn.name = "SATURN";
    planets.push_back(saturn);
}

void CelestialObjectManager::setupEarthWest() {
    // Edmonton, Alberta looking West
    // Prominent constellations: Cygnus, Lyra, Aquila, Delphinus
    // Planets: Venus (evening star), Saturn may be visible

    Constellation cygnus;
    cygnus.name = "CYGNUS";
    cygnus.starIndices = { 0, 1, 2, 3, 4 };
    stars[0].position = glm::vec2(0.50f, 0.85f); // Deneb
    stars[1].position = glm::vec2(0.45f, 0.80f); // Sadr
    stars[2].position = glm::vec2(0.48f, 0.75f); // Gienah
    stars[3].position = glm::vec2(0.52f, 0.78f); // Delta Cyg
    stars[4].position = glm::vec2(0.47f, 0.70f); // Albireo
    stars[0].brightness = 0.95f;
    stars[1].brightness = 0.85f;
    stars[2].brightness = 0.8f;
    stars[3].brightness = 0.7f;
    stars[4].brightness = 0.75f;
    constellations.push_back(cygnus);

    Constellation lyra;
    lyra.name = "LYRA";
    lyra.starIndices = { 5, 6, 7, 8 };
    stars[5].position = glm::vec2(0.35f, 0.80f); // Vega
    stars[6].position = glm::vec2(0.38f, 0.77f); // Sheliak
    stars[7].position = glm::vec2(0.37f, 0.74f); // Sulafat
    stars[8].position = glm::vec2(0.34f, 0.76f); // Delta Lyr
    stars[5].brightness = 1.0f;
    stars[6].brightness = 0.8f;
    stars[7].brightness = 0.75f;
    stars[8].brightness = 0.7f;
    constellations.push_back(lyra);

    Constellation aquila;
    aquila.name = "AQUILA";
    aquila.starIndices = { 9, 10, 11, 12 };
    stars[9].position = glm::vec2(0.65f, 0.70f); // Altair
    stars[10].position = glm::vec2(0.60f, 0.68f); // Alshain
    stars[11].position = glm::vec2(0.70f, 0.67f); // Tarazed
    stars[12].position = glm::vec2(0.63f, 0.65f); // Delta Aql
    stars[9].brightness = 0.95f;
    stars[10].brightness = 0.7f;
    stars[11].brightness = 0.85f;
    stars[12].brightness = 0.7f;
    constellations.push_back(aquila);

    Constellation delphinus;
    delphinus.name = "DELPHINUS";
    delphinus.starIndices = { 13, 14, 15, 16 };
    stars[13].position = glm::vec2(0.80f, 0.75f); // Sualocin
    stars[14].position = glm::vec2(0.83f, 0.73f); // Rotanev
    stars[15].position = glm::vec2(0.81f, 0.70f); // Delta Del
    stars[16].position = glm::vec2(0.78f, 0.72f); // Epsilon Del
    stars[13].brightness = 0.8f;
    stars[14].brightness = 0.85f;
    stars[15].brightness = 0.7f;
    stars[16].brightness = 0.75f;
    constellations.push_back(delphinus);

    // Add planets: Venus (evening star), Saturn
    Planet venus;
    venus.position = glm::vec2(0.20f, 0.65f);
    venus.brightness = 1.2f;
    venus.color = glm::vec3(1.0f, 1.0f, 0.9f);
    venus.size = 4.0f;
    venus.name = "VENUS";
    planets.push_back(venus);

    Planet saturn;
    saturn.position = glm::vec2(0.25f, 0.60f);
    saturn.brightness = 0.8f;
    saturn.color = glm::vec3(1.0f, 0.9f, 0.7f);
    saturn.size = 4.0f;
    saturn.name = "SATURN";
    planets.push_back(saturn);
}

void CelestialObjectManager::setupAlienPattern1() {
    // Pattern 1: A sky with a prominent central constellation and smaller ones around the edges
    Constellation batlh; // "Honor" - A large, central sword-like shape
    batlh.name = "BATLH";
    batlh.starIndices = { 0, 1, 2, 3, 4, 5 };
    stars[0].position = glm::vec2(0.50f, 0.85f); // Tip of the sword
    stars[1].position = glm::vec2(0.52f, 0.80f); // Blade
    stars[2].position = glm::vec2(0.48f, 0.80f); // Blade
    stars[3].position = glm::vec2(0.50f, 0.75f); // Base of blade
    stars[4].position = glm::vec2(0.53f, 0.70f); // Hilt
    stars[5].position = glm::vec2(0.47f, 0.70f); // Hilt
    stars[0].brightness = 0.95f;
    stars[1].brightness = 0.8f;
    stars[2].brightness = 0.8f;
    stars[3].brightness = 0.9f;
    stars[4].brightness = 0.7f;
    stars[5].brightness = 0.7f;
    constellations.push_back(batlh);

    Constellation qapla; // "Success" - A small, triangular formation in the lower-left
    qapla.name = "QAPLA'";
    qapla.starIndices = { 6, 7, 8 };
    stars[6].position = glm::vec2(0.20f, 0.55f);
    stars[7].position = glm::vec2(0.25f, 0.50f);
    stars[8].position = glm::vec2(0.15f, 0.50f);
    stars[6].brightness = 0.9f;
    stars[7].brightness = 0.8f;
    stars[8].brightness = 0.7f;
    constellations.push_back(qapla);

    Constellation tlhingan; // "Klingon" - A crescent shape in the upper-right
    tlhingan.name = "TLHINGAN";
    tlhingan.starIndices = { 9, 10, 11, 12 };
    stars[9].position = glm::vec2(0.80f, 0.80f);
    stars[10].position = glm::vec2(0.83f, 0.77f);
    stars[11].position = glm::vec2(0.85f, 0.73f);
    stars[12].position = glm::vec2(0.82f, 0.70f);
    stars[9].brightness = 0.9f;
    stars[10].brightness = 0.85f;
    stars[11].brightness = 0.8f;
    stars[12].brightness = 0.7f;
    constellations.push_back(tlhingan);

    // Unique Klingon Planets for Pattern 1
    Planet ruraPenthe;
    ruraPenthe.position = glm::vec2(0.20f, 0.65f); 
    ruraPenthe.brightness = 1.0f;
    ruraPenthe.color = glm::vec3(0.7f, 0.3f, 0.5f); // Purplish
    ruraPenthe.size = 5.0f;
    ruraPenthe.name = "RURA PENTHE";
    planets.push_back(ruraPenthe);

    Planet kronos;
    kronos.position = glm::vec2(0.85f, 0.65f);
    kronos.brightness = 0.9f;
    kronos.color = glm::vec3(0.6f, 0.4f, 0.8f); // Bluish-purple
    kronos.size = 4.5f;
    kronos.name = "KRONOS";
    planets.push_back(kronos);

    Planet khitomer;
    khitomer.position = glm::vec2(0.30f, 0.55f);
    khitomer.brightness = 0.8f;
    khitomer.color = glm::vec3(0.9f, 0.6f, 0.3f); // Orange
    khitomer.size = 4.0f;
    khitomer.name = "KHITOMER";
    planets.push_back(khitomer);
}

void CelestialObjectManager::setupAlienPattern2() {
    // Pattern 2: A sky with a sprawling constellation across the middle and two smaller ones at the top and bottom
    Constellation moktah; // "Challenge" - A sprawling, zigzag pattern across the middle
    moktah.name = "MOK'TAH";
    moktah.starIndices = { 0, 1, 2, 3, 4, 5, 6 };
    stars[0].position = glm::vec2(0.20f, 0.65f);
    stars[1].position = glm::vec2(0.30f, 0.70f);
    stars[2].position = glm::vec2(0.40f, 0.60f);
    stars[3].position = glm::vec2(0.50f, 0.65f);
    stars[4].position = glm::vec2(0.60f, 0.60f);
    stars[5].position = glm::vec2(0.70f, 0.65f);
    stars[6].position = glm::vec2(0.80f, 0.60f);
    stars[0].brightness = 0.9f;
    stars[1].brightness = 0.8f;
    stars[2].brightness = 0.7f;
    stars[3].brightness = 0.9f;
    stars[4].brightness = 0.8f;
    stars[5].brightness = 0.7f;
    stars[6].brightness = 0.85f;
    constellations.push_back(moktah);

    Constellation vengor; // "Vengeance" - A small cluster at the top-left
    vengor.name = "VENGOR";
    vengor.starIndices = { 7, 8, 9, 10 };
    stars[7].position = glm::vec2(0.30f, 0.80f); 
    stars[8].position = glm::vec2(0.33f, 0.78f); 
    stars[9].position = glm::vec2(0.35f, 0.76f); 
    stars[10].position = glm::vec2(0.32f, 0.74f); 
    stars[7].brightness = 0.9f;
    stars[8].brightness = 0.8f;
    stars[9].brightness = 0.7f;
    stars[10].brightness = 0.75f;
    constellations.push_back(vengor);

    Constellation khorvok; // "Wrath" - A small, diamond shape at the bottom-right
    khorvok.name = "KHORVOK";
    khorvok.starIndices = { 11, 12, 13, 14 };
    stars[11].position = glm::vec2(0.65f, 0.65f);
    stars[12].position = glm::vec2(0.68f, 0.63f);
    stars[13].position = glm::vec2(0.65f, 0.61f);
    stars[14].position = glm::vec2(0.62f, 0.63f);
    stars[11].brightness = 0.9f;
    stars[12].brightness = 0.8f;
    stars[13].brightness = 0.7f;
    stars[14].brightness = 0.85f;
    constellations.push_back(khorvok);

    // Unique Klingon Planets for Pattern 2
    Planet praxis;
    praxis.position = glm::vec2(0.82f, 0.60f);
    praxis.brightness = 0.9f;
    praxis.color = glm::vec3(0.5f, 0.7f, 0.3f); // Greenish
    praxis.size = 4.0f;
    praxis.name = "PRAXIS";
    planets.push_back(praxis);

    Planet klinzhai;
    klinzhai.position = glm::vec2(0.30f, 0.70f);
    klinzhai.brightness = 1.0f;
    klinzhai.color = glm::vec3(0.8f, 0.4f, 0.4f); // Reddish
    klinzhai.size = 5.0f;
    klinzhai.name = "KLINZHAI";
    planets.push_back(klinzhai);

    Planet tyghokor;
    tyghokor.position = glm::vec2(0.45f, 0.65f);
    tyghokor.brightness = 0.8f;
    tyghokor.color = glm::vec3(0.3f, 0.6f, 0.9f); // Bluish
    tyghokor.size = 4.5f;
    tyghokor.name = "TY'GOKOR";
    planets.push_back(tyghokor);
}

void CelestialObjectManager::setupAlienPattern3() {
    // Pattern 3: A sky with a large arc-shaped constellation in the upper half and two smaller ones below
    Constellation torvak; // "Glory" - A large arc across the upper half
    torvak.name = "TORVAK";
    torvak.starIndices = { 0, 1, 2, 3, 4, 5 };
    stars[0].position = glm::vec2(0.30f, 0.80f); 
    stars[1].position = glm::vec2(0.40f, 0.85f); 
    stars[2].position = glm::vec2(0.50f, 0.87f); 
    stars[3].position = glm::vec2(0.60f, 0.85f); 
    stars[4].position = glm::vec2(0.70f, 0.82f);
    stars[5].position = glm::vec2(0.80f, 0.78f); 
    stars[0].brightness = 0.9f;
    stars[1].brightness = 0.8f;
    stars[2].brightness = 0.85f;
    stars[3].brightness = 0.9f;
    stars[4].brightness = 0.8f;
    stars[5].brightness = 0.7f;
    constellations.push_back(torvak);

    Constellation gharnok; // "Fury" - A small, cross-shaped pattern in the lower-left
    gharnok.name = "GHARNOK";
    gharnok.starIndices = { 6, 7, 8, 9 };
    stars[6].position = glm::vec2(0.25f, 0.55f);
    stars[7].position = glm::vec2(0.25f, 0.60f);
    stars[8].position = glm::vec2(0.22f, 0.57f);
    stars[9].position = glm::vec2(0.28f, 0.57f);
    stars[6].brightness = 0.9f;
    stars[7].brightness = 0.9f;
    stars[8].brightness = 0.7f;
    stars[9].brightness = 0.7f;
    constellations.push_back(gharnok);

    Constellation zeltar; // "Dominion" - A small, square pattern in the lower-right
    zeltar.name = "ZELTAR";
    zeltar.starIndices = { 10, 11, 12, 13 };
    stars[10].position = glm::vec2(0.75f, 0.60f);
    stars[11].position = glm::vec2(0.68f, 0.60f);
    stars[12].position = glm::vec2(0.78f, 0.57f);
    stars[13].position = glm::vec2(0.75f, 0.57f);
    stars[10].brightness = 0.5f;
    stars[11].brightness = 0.8f;
    stars[12].brightness = 0.6f;
    stars[13].brightness = 0.3f;
    constellations.push_back(zeltar);

    // Unique Klingon Planets for Pattern 3
    Planet boreth;
    boreth.position = glm::vec2(0.25f, 0.65f);
    boreth.brightness = 0.8f;
    boreth.color = glm::vec3(0.8f, 0.5f, 0.4f); // Reddish
    boreth.size = 4.0f;
    boreth.name = "BORETH";
    planets.push_back(boreth);

    Planet morska;
    morska.position = glm::vec2(0.80f, 0.75f);
    morska.brightness = 0.95f;
    morska.color = glm::vec3(0.4f, 0.7f, 0.5f); // Greenish-blue
    morska.size = 5.0f;
    morska.name = "MORSKA";
    planets.push_back(morska);

    Planet krios;
    krios.position = glm::vec2(0.50f, 0.55f);
    krios.brightness = 0.85f;
    krios.color = glm::vec3(0.9f, 0.5f, 0.7f); // Pinkish
    krios.size = 4.5f;
    krios.name = "KRIOS";
    planets.push_back(krios);
}

void CelestialObjectManager::setupAlienPattern4() {
    // Pattern 4: A sky with a vertical constellation on the left, a circular one on the right, and a small cluster in the center
    Constellation drakthar; // "Valor" - A vertical line on the left
    drakthar.name = "DRAKTHAR";
    drakthar.starIndices = { 0, 1, 2, 3 };
    stars[0].position = glm::vec2(0.35f, 0.1000f);
    stars[1].position = glm::vec2(0.35f, 0.90f);
    stars[2].position = glm::vec2(0.35f, 0.80f);
    stars[3].position = glm::vec2(0.35f, 0.80f);
    stars[0].brightness = 0.9f;
    stars[1].brightness = 0.85f;
    stars[2].brightness = 0.8f;
    stars[3].brightness = 0.7f;
    constellations.push_back(drakthar);

    Constellation syrvex; // "Might" - A small cluster in the center
    syrvex.name = "SYRVEX";
    syrvex.starIndices = { 4, 5, 6, 7 };
    stars[4].position = glm::vec2(0.50f, 0.65f);
    stars[5].position = glm::vec2(0.53f, 0.67f);
    stars[6].position = glm::vec2(0.47f, 0.67f);
    stars[7].position = glm::vec2(0.50f, 0.70f);
    stars[4].brightness = 0.9f;
    stars[5].brightness = 0.8f;
    stars[6].brightness = 0.7f;
    stars[7].brightness = 0.75f;
    constellations.push_back(syrvex);

    Constellation qolthar; // "Power" - A circular pattern on the right
    qolthar.name = "QOLTHAR";
    qolthar.starIndices = { 8, 9, 10, 11, 12 };
    stars[8].position = glm::vec2(0.80f, 0.75f); // Center
    stars[9].position = glm::vec2(0.83f, 0.77f);
    stars[10].position = glm::vec2(0.82f, 0.72f);
    stars[11].position = glm::vec2(0.77f, 0.73f);
    stars[12].position = glm::vec2(0.78f, 0.78f);
    stars[8].brightness = 0.95f;
    stars[9].brightness = 0.8f;
    stars[10].brightness = 0.7f;
    stars[11].brightness = 0.85f;
    stars[12].brightness = 0.75f;
    constellations.push_back(qolthar);

    // Unique Klingon Planets for Pattern 4
    Planet kolarus;
    kolarus.position = glm::vec2(0.9f, 0.9f); 
    kolarus.brightness = 0.9f;
    kolarus.color = glm::vec3(0.5f, 0.8f, 0.6f); // Light green
    kolarus.size = 4.5f;
    kolarus.name = "KOLARUS";
    planets.push_back(kolarus);

    Planet rakhar;
    rakhar.position = glm::vec2(0.75f, 0.60f);
    rakhar.brightness = 0.85f;
    rakhar.color = glm::vec3(0.7f, 0.3f, 0.7f); // Magenta
    rakhar.size = 4.0f;
    rakhar.name = "RAKHAR";
    planets.push_back(rakhar);

    Planet betaThoridor;
    betaThoridor.position = glm::vec2(0.35f, 0.55f);
    betaThoridor.brightness = 0.95f;
    betaThoridor.color = glm::vec3(0.9f, 0.7f, 0.3f); // Golden
    betaThoridor.size = 5.0f;
    betaThoridor.name = "BETA THORIDOR";
    planets.push_back(betaThoridor);
}

void CelestialObjectManager::initializeStarBuffers() {
    satellites.clear();
    satelliteTimer = 0.0f;

    std::vector<float> starVertices;
    for (size_t i = 0; i < stars.size(); ++i) {
        const auto& star = stars[i];
        starVertices.push_back(star.position.x);
        starVertices.push_back(star.position.y);
        starVertices.push_back(star.brightness);
        float size = (i >= 50) ? 3.0f : 4.0f;
        starVertices.push_back(size);
        starVertices.push_back(1.0f);
        starVertices.push_back(1.0f);
        starVertices.push_back(1.0f);
    }

    DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "initializeStarBuffers",
        "Star vertices: starVertices.size()=" + std::to_string(starVertices.size()));

    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);
    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starVertices.size() * sizeof(float), starVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in float aBrightness;
        layout(location = 2) in float aSize;
        layout(location = 3) in vec3 aColor;
        out float Brightness;
        out vec3 Color;
        out vec2 Pos;
        void main() {
            vec2 ndcPos = aPos * 2.0 - 1.0;
            gl_Position = vec4(ndcPos, 0.999, 1.0);
            gl_PointSize = aSize;
            Brightness = aBrightness;
            Color = aColor;
            Pos = aPos;
        }
    )";
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in float Brightness;
        in vec3 Color;
        in vec2 Pos;
        uniform float alpha;
        uniform float sunMoonPosition;
        uniform float aspectRatio;
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            float dist = length(coord);
            if (dist > 0.5) discard;

            vec2 sunMoonPos = vec2(sunMoonPosition, 0.8 - sunMoonPosition * 0.4);
            vec2 adjustedPos = Pos;
            adjustedPos.x *= aspectRatio;
            sunMoonPos.x *= aspectRatio;
            float sunMoonDist = length(adjustedPos - sunMoonPos);
            float sunMoonRadius = 0.03;
            float edge = smoothstep(0.5, 0.3, dist);
            float finalBrightness = Brightness * alpha;
            vec3 finalColor = Color * finalBrightness;
            float finalAlpha = edge * alpha;
            if (finalAlpha < 0.01) discard;
            FragColor = vec4(finalColor, finalAlpha);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Star vertex shader compilation failed: " << infoLog << std::endl;
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Star fragment shader compilation failed: " << infoLog << std::endl;
        return;
    }

    starShader = glCreateProgram();
    glAttachShader(starShader, vertexShader);
    glAttachShader(starShader, fragmentShader);
    glLinkProgram(starShader);
    glGetProgramiv(starShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(starShader, 512, nullptr, infoLog);
        std::cerr << "Star shader program linking failed: " << infoLog << std::endl;
        return;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up VAO/VBO for exhaust trails (line strip)
    glGenVertexArrays(1, &smokeVAO);
    glGenBuffers(1, &smokeVBO);

    glBindVertexArray(smokeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, smokeVBO);
    glBufferData(GL_ARRAY_BUFFER, 1000 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create shader for exhaust trails
    const char* exhaustVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in float aAlpha;
        out float Alpha;
        void main() {
            vec2 ndcPos = aPos * 2.0 - 1.0;
            gl_Position = vec4(ndcPos, 0.998, 1.0);
            Alpha = aAlpha;
        }
    )";

    const char* exhaustFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in float Alpha;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, Alpha);
        }
    )";

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &exhaustVertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Exhaust vertex shader compilation failed: " << infoLog << std::endl;
        return;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &exhaustFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Exhaust fragment shader compilation failed: " << infoLog << std::endl;
        return;
    }

    smokeShader = glCreateProgram();
    glAttachShader(smokeShader, vertexShader);
    glAttachShader(smokeShader, fragmentShader);
    glLinkProgram(smokeShader);
    glGetProgramiv(smokeShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(smokeShader, 512, nullptr, infoLog);
        std::cerr << "Exhaust shader program linking failed: " << infoLog << std::endl;
        return;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void CelestialObjectManager::render(float starAlpha, float sunMoonPosition) {
    if (starAlpha <= 0.0f) return;

    // Ensure depth testing is enabled for stars
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Update star vertices with current brightness (distant celestials)
    std::vector<float> starVertices;
    for (const auto& star : stars) {
        starVertices.push_back(star.position.x);
        starVertices.push_back(star.position.y);
        starVertices.push_back(star.brightness * starAlpha);
        float size = (stars.size() > 50 && &star >= &stars[50]) ? 3.0f : 4.0f; // Distant stars are smaller
        starVertices.push_back(size);
        starVertices.push_back(1.0f); // Color (white)
        starVertices.push_back(1.0f);
        starVertices.push_back(1.0f);
    }

    for (const auto& planet : planets) {
        starVertices.push_back(planet.position.x);
        starVertices.push_back(planet.position.y);
        starVertices.push_back(planet.brightness * starAlpha);
        starVertices.push_back(planet.size);
        starVertices.push_back(planet.color.r);
        starVertices.push_back(planet.color.g);
        starVertices.push_back(planet.color.b);
    }

    for (const auto& satellite : satellites) {
        starVertices.push_back(satellite.position.x);
        starVertices.push_back(satellite.position.y);
        starVertices.push_back(satellite.brightness * starAlpha);
        starVertices.push_back(satellite.size);
        starVertices.push_back(satellite.isISS ? 1.0f : 0.5f); // Color (yellow for ISS, dim white for others)
        starVertices.push_back(satellite.isISS ? 1.0f : 0.5f);
        starVertices.push_back(satellite.isISS ? 0.0f : 0.5f);
    }

    for (const auto& train : starlinkTrains) {
        for (size_t i = 0; i < train.positions.size(); ++i) {
            starVertices.push_back(train.positions[i].x);
            starVertices.push_back(train.positions[i].y);
            starVertices.push_back(train.brightness * starAlpha);
            starVertices.push_back(train.size);
            starVertices.push_back(0.5f); // Dim white
            starVertices.push_back(0.5f);
            starVertices.push_back(0.5f);
        }
    }

    for (const auto& shootingStar : shootingStars) {
        starVertices.push_back(shootingStar.position.x);
        starVertices.push_back(shootingStar.position.y);
        starVertices.push_back(shootingStar.brightness * starAlpha);
        starVertices.push_back(2.0f + (static_cast<float>(rand()) / RAND_MAX) * 0.5f);  // Set point size for rendering between 1.5 and 2.0
        starVertices.push_back(1.0f); // White
        starVertices.push_back(1.0f);
        starVertices.push_back(1.0f);
    }

    // Update VBO
    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starVertices.size() * sizeof(float), starVertices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);

    // Render distant celestials
    glUseProgram(starShader);
    float aspectRatio = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    glUniform1f(glGetUniformLocation(starShader, "alpha"), starAlpha);
    glUniform1f(glGetUniformLocation(starShader, "sunMoonPosition"), sunMoonPosition);
    glUniform1f(glGetUniformLocation(starShader, "aspectRatio"), aspectRatio);

    glBindVertexArray(starVAO);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(starVertices.size() / 7));
    glDisable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(0);

    // Render exhaust trails for satellites and Starlink trains in alien scene
    if (scene == Scene::ALIEN) {

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        glUseProgram(smokeShader);
        glBindVertexArray(smokeVAO);
        glLineWidth(2.0f); // Set the line width for the exhaust trail

        for (const auto& satellite : satellites) {
            if (satellite.trail.empty()) continue;

            std::vector<float> trailVertices;
            for (const auto& segment : satellite.trail) {
                trailVertices.push_back(segment.startPos.x);
                trailVertices.push_back(segment.startPos.y);
                trailVertices.push_back(segment.alpha);
                trailVertices.push_back(0.0f); // Padding
                trailVertices.push_back(segment.endPos.x);
                trailVertices.push_back(segment.endPos.y);
                trailVertices.push_back(segment.alpha);
                trailVertices.push_back(0.0f); // Padding
            }

            glBindBuffer(GL_ARRAY_BUFFER, smokeVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, trailVertices.size() * sizeof(float), trailVertices.data());
            // Blend the spaceship's textColor with the base glow color
            glm::vec3 baseColor(0.8f, 0.8f, 1.0f); // Light blue-purple glow
            glm::vec3 blendedColor = glm::mix(baseColor, satellite.textColor, 0.3f); // 30% spaceship color
            glUniform3f(glGetUniformLocation(smokeShader, "color"), blendedColor.r, blendedColor.g, blendedColor.b);
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(trailVertices.size() / 4));
        }

        // Render exhaust trails for satellites and Starlink trains in alien scene
        for (const auto& train : starlinkTrains) {
            for (size_t i = 0; i < train.trails.size(); ++i) {
                const auto& trail = train.trails[i];
                if (trail.empty()) continue;

                // Prepare vertex data for satellite exhaust trails
                std::vector<float> trailVertices;
                for (const auto& segment : trail) {
                    trailVertices.push_back(segment.startPos.x);  // Start X position
                    trailVertices.push_back(segment.startPos.y);  // Start Y position
                    trailVertices.push_back(segment.alpha);  // Alpha value for fading
                    trailVertices.push_back(0.0f);  // Padding for alignment
                    trailVertices.push_back(segment.endPos.x);  // End X position
                    trailVertices.push_back(segment.endPos.y);  // End Y position
                    trailVertices.push_back(segment.alpha);  // Alpha value for fading
                    trailVertices.push_back(0.0f);  // Padding for alignment
                }

                // Render satellite exhaust trails
                glBindBuffer(GL_ARRAY_BUFFER, smokeVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, trailVertices.size() * sizeof(float), trailVertices.data());
                // Starlink trains use a yellow color (1.0f, 1.0f, 0.0f) in dynamicVertices
                glm::vec3 baseColor(0.8f, 0.8f, 1.0f); // Light blue-purple glow
                glm::vec3 shipColor(1.0f, 1.0f, 0.0f); // Yellow from dynamicVertices
                glm::vec3 blendedColor = glm::mix(baseColor, shipColor, 0.3f); // 30% spaceship color
                glUniform3f(glGetUniformLocation(smokeShader, "color"), blendedColor.r, blendedColor.g, blendedColor.b);
                glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(trailVertices.size() / 4));

            }
        }
    }
}

void CelestialObjectManager::renderText(Renderer* renderer, float starAlpha) {
    if (starAlpha <= 0.0f) return;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // Render constellation names
    if (showConstellationNames) {
        for (const auto& constellation : constellations) {
            glm::vec2 center(0.0f);
            int count = 0;
            for (int idx : constellation.starIndices) {
                center += stars[idx].position;
                count++;
            }
            if (count > 0) {
                center /= static_cast<float>(count);
                float x = center.x * WINDOW_WIDTH;
                float y = center.y * WINDOW_HEIGHT + 20.0f;
                renderer->renderText(constellation.name, x, y, CELESTIAL_TEXT_SCALE_CONSTELLATIONS, glm::vec3(0.5f, 0.5f, 0.5f), true);
            }
        }
    }

    // Render planet names
    if (showPlanetNames) {
        for (const auto& planet : planets) {
            float x = planet.position.x * WINDOW_WIDTH;
            float y = planet.position.y * WINDOW_HEIGHT + 20.0f;
            renderer->renderText(planet.name, x, y, CELESTIAL_TEXT_SCALE_PLANETS, glm::vec3(planet.color.r * 0.5f, planet.color.g * 0.5f, planet.color.b * 0.5f) * glm::vec3(0.8f), true);
        }
    }

    // Render satellite names
    if (showSatelliteNames) {
        for (const auto& sat : satellites) {
            float x = sat.position.x * WINDOW_WIDTH;
            float y = sat.position.y * WINDOW_HEIGHT + 20.0f;
            renderer->renderText(sat.name, x, y, CELESTIAL_TEXT_SCALE_SATELLITES_AND_SPACESHIPS, sat.textColor * glm::vec3(0.5f), true);
        }
        for (const auto& train : starlinkTrains) {
            if (!train.positions.empty()) {
                float x = train.positions[0].x * WINDOW_WIDTH;
                float y = train.positions[0].y * WINDOW_HEIGHT + 20.0f;
                renderer->renderText(scene == Scene::ALIEN ? "KLINGON DEFENSE FORCE" : "STARLINK TRAIN", x, y, CELESTIAL_TEXT_SCALE_SATELLITES_AND_SPACESHIPS, glm::vec3(1.0f, 1.0f, 0.0f) * glm::vec3(0.5f) * glm::vec3(0.8f), true);
            }
        }
    }

    // Render close celestial names (planet)
    if (showPlanetNames) {
        for (const auto& celestial : closeCelestials) {
            if (celestial.celestialType != "Planet") continue;

            float x = celestial.position.x * WINDOW_WIDTH;
            float y = celestial.position.y * WINDOW_HEIGHT + 30.0f;

            bool isVisible = true;
            int terrainX = static_cast<int>(celestial.position.x * (world->getDistantTerrain()->getWidth() - 1));
            std::vector<float> heightmap = world->getDistantTerrain()->getHeightmap(world->getDistantTerrain()->getDepth() - 1);
            if (terrainX >= 0 && terrainX < static_cast<int>(heightmap.size())) {
                float terrainHeight = heightmap[terrainX];
                if (terrainHeight != FLT_MAX) {
                    float terrainScreenY = terrainHeight + world->getDistantParams().yOffset;
                    if (y < terrainScreenY) {
                        isVisible = false;
                    }
                }
            }

            if (isVisible) {
                renderer->renderText(celestial.name, x, y, CELESTIAL_TEXT_SCALE_PLANETS, celestial.tintColor, true);
            }
        }
    }
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void CelestialObjectManager::updateShootingStars(float dt, TimeOfDay currentTime) {
    if (currentTime != TimeOfDay::NIGHT) {
        shootingStars.clear();
        return;
    }

    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(30.0f, 60.0f);
    std::uniform_real_distribution<float> brightnessDist(0.3f, 1.2f);
    std::uniform_real_distribution<float> lifetimeVariation(-0.5f, 0.5f);
    std::uniform_real_distribution<float> timerDist(5.0f, 15.0f);

    shootingStarTimer -= dt;
    if (shootingStarTimer <= 0.0f) {
        ShootingStar star;
        star.position = glm::vec2(probDist(rng), 1.0f);
        float angle = glm::radians(angleDist(rng));
        star.velocity = glm::vec2(-cos(angle), -sin(angle)) * 0.5f;

        star.brightness = brightnessDist(rng);

        float brightnessFactor = (star.brightness - 0.3f) / 0.9f;
        star.lifetime = 0.5f + brightnessFactor * 1.5f;
        star.lifetime += lifetimeVariation(rng) * 0.2f;
        star.lifetime = glm::clamp(star.lifetime, 0.5f, 2.0f);

        shootingStars.push_back(star);
        shootingStarTimer = timerDist(rng);
    }

    for (auto it = shootingStars.begin(); it != shootingStars.end();) {
        it->position += it->velocity * dt;
        it->lifetime -= dt;
        it->brightness = (it->brightness / it->lifetime) * (it->lifetime - dt);
        if (it->lifetime <= 0.0f || it->position.x < 0.0f || it->position.y < 0.0f) {
            it = shootingStars.erase(it);
        } else {
            ++it;
        }
    }
}

void CelestialObjectManager::updateSatellites(float dt, TimeOfDay currentTime) {
    if (currentTime != TimeOfDay::NIGHT) {
        satellites.clear();
        return;
    }

    std::uniform_int_distribution<int> binaryDist(0, 1);
    std::uniform_real_distribution<float> posDist(0.3f, 0.7f);
    std::uniform_int_distribution<int> idxDist(0, 1000);
    std::uniform_real_distribution<float> timerDist(60.0f, 120.0f);

    satelliteTimer -= dt;
    if (satelliteTimer <= 0.0f && satellites.size() < 2) {
        Satellite sat;
        bool startLeft = binaryDist(rng) == 0;
        // Start one pixel inside the screen edge in normalized coordinates
        float onePixelNormalized = 1.0f / static_cast<float>(WINDOW_WIDTH);
        float startX = startLeft ? onePixelNormalized : 1.0f - onePixelNormalized;
        sat.position = glm::vec2(startX, posDist(rng));

        const std::vector<std::tuple<std::string, float, float, glm::vec3, float, float>> satelliteData =
            (scene == Scene::ALIEN) ?
            std::vector<std::tuple<std::string, float, float, glm::vec3, float, float>> {
                {"IKS D'GAVAH (BIRD-OF-PREY)", 0.012f, 45.0f, glm::vec3(1.0f, 0.0f, 0.0f), 3.0f, 1.2f},
                { "IKS KORINAR (K'VORT-CLASS)", 0.010f, 40.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.5f, 0.9f },
                { "IKS MAUK (VOR'CHA-CLASS)", 0.011f, 35.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.8f, 1.0f },
                { "IKS TONG (D7-CLASS)", 0.009f, 50.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.0f, 0.8f },
                { "IKS BURUK (B'REL-CLASS)", 0.010f, 42.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.2f, 0.9f },
                { "IKS RAKTAR (NEGH'VAR-CLASS)", 0.012f, 38.0f, glm::vec3(0.7f, 0.2f, 0.2f), 3.0f, 1.1f },
                { "IKS QEH'TAK (K'T'INGA-CLASS)", 0.011f, 47.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.7f, 1.0f },
                { "IKS VOR'NAL (RAPTOR-CLASS)", 0.0095f, 41.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.3f, 0.9f },
                { "IKS JIH'VEK (D5-CLASS)", 0.0105f, 39.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.4f, 0.9f },
                { "IKS NUQ'TAR (KELDON-CLASS)", 0.011f, 43.0f, glm::vec3(0.7f, 0.2f, 0.2f), 2.6f, 1.0f },
                { "IKS TAL'SHIAR (D'DERIDEX WARBIRD)", 0.012f, 44.0f, glm::vec3(0.2f, 0.8f, 0.2f), 3.0f, 1.1f },
                { "IKS VREX'TAL (VALDORE-CLASS)", 0.011f, 40.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.8f, 1.0f },
                { "IKS SOT'HAR (KERCHAN-CLASS)", 0.010f, 42.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.5f, 0.9f },
                { "IKS DUK'TAL (SCORPION-CLASS)", 0.009f, 38.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.2f, 0.8f },
                { "IKS REMAN'VEK (SHRIKE-CLASS)", 0.0105f, 41.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.4f, 0.9f },
                { "IKS NEX'TOR (T'LISS WARBIRD)", 0.011f, 39.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.6f, 1.0f },
                { "IKS VOR'CHA (NERADA-CLASS)", 0.012f, 43.0f, glm::vec3(0.3f, 0.6f, 0.3f), 3.0f, 1.1f },
                { "IKS KEL'TAK (D7 WARBIRD)", 0.010f, 40.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.5f, 0.9f },
                { "IKS TAL'VEK (HAWK-CLASS)", 0.0095f, 42.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.3f, 0.8f },
                { "IKS SAREK'TAL (FALCON-CLASS)", 0.011f, 41.0f, glm::vec3(0.3f, 0.6f, 0.3f), 2.7f, 1.0f },
                { "IKS TAJ'VEK (GALAXY-CLASS)", 0.012f, 45.0f, glm::vec3(0.8f, 0.8f, 1.0f), 3.0f, 1.1f },
                { "IKS QONOS'TAR (CONSTITUTION-CLASS)", 0.011f, 40.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.8f, 1.0f },
                { "IKS VOR'TAK (INTREPID-CLASS)", 0.010f, 42.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.5f, 0.9f },
                { "IKS NEX'VEK (DEFIANT-CLASS)", 0.009f, 38.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.2f, 0.8f },
                { "IKS KEL'CHA (SOVEREIGN-CLASS)", 0.011f, 41.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.7f, 1.0f },
                { "IKS DUK'VEK (NEBULA-CLASS)", 0.0105f, 39.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.4f, 0.9f },
                { "IKS SOT'VEK (EXCELSIOR-CLASS)", 0.012f, 43.0f, glm::vec3(0.5f, 0.5f, 0.8f), 3.0f, 1.1f },
                { "IKS TAL'TAR (AKIRA-CLASS)", 0.010f, 40.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.5f, 0.9f },
                { "IKS VREX'TOR (MIRANDA-CLASS)", 0.0095f, 42.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.3f, 0.8f },
                { "IKS NEX'TAK (PROMETHEUS-CLASS)", 0.011f, 41.0f, glm::vec3(0.5f, 0.5f, 0.8f), 2.7f, 1.0f },
                { "IKS ZOR'TAL (CYLON BASISTAR)", 0.012f, 44.0f, glm::vec3(0.5f, 0.5f, 0.5f), 3.0f, 1.1f },
                { "IKS VEX'CHA (BORG CUBE)", 0.011f, 40.0f, glm::vec3(0.4f, 0.1f, 0.4f), 2.8f, 1.0f },
                { "IKS DOR'TAK (IMPERIAL STAR DESTROYER)", 0.010f, 42.0f, glm::vec3(0.3f, 0.5f, 0.7f), 2.5f, 0.9f },
                { "IKS KOR'VEK (MILLENNIUM FALCON)", 0.009f, 38.0f, glm::vec3(0.8f, 0.6f, 0.4f), 2.2f, 0.8f },
                { "IKS TAL'CHA (FIREFLY-CLASS)", 0.0105f, 41.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.4f, 0.9f },
                { "IKS SOT'TAR (SERENITY)", 0.011f, 39.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.6f, 1.0f },
                { "IKS VOR'TAK (REAVER SHIP)", 0.012f, 43.0f, glm::vec3(0.6f, 0.6f, 0.6f), 3.0f, 1.1f },
                { "IKS NEX'CHA (GALACTICA)", 0.010f, 40.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.5f, 0.9f },
                { "IKS DUK'TOR (VIPER MK II)", 0.0095f, 42.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.3f, 0.8f },
                { "IKS KEL'TAR (RAIDER)", 0.011f, 41.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.7f, 1.0f },
                { "IKS ZOR'VEK (NORMANDY SR-2)", 0.012f, 44.0f, glm::vec3(0.6f, 0.6f, 0.6f), 3.0f, 1.1f },
                { "IKS VEX'TAK (REAPER DESTROYER)", 0.011f, 40.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.8f, 1.0f },
                { "IKS DOR'CHA (DESTINY)", 0.010f, 42.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.5f, 0.9f },
                { "IKS KOR'VEK (ATLANTIS)", 0.009f, 38.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.2f, 0.8f },
                { "IKS TAL'TOR (HIVE SHIP)", 0.0105f, 41.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.4f, 0.9f },
                { "IKS SOT'CHA (WRAITH CRUISER)", 0.011f, 39.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.6f, 1.0f },
                { "IKS VOR'TAK (DART)", 0.012f, 43.0f, glm::vec3(0.6f, 0.6f, 0.6f), 3.0f, 1.1f },
                { "IKS NEX'TOR (ORION DESTROYER)", 0.010f, 40.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.5f, 0.9f },
                { "IKS DUK'CHA (NAQUADAH MINER)", 0.0095f, 42.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.3f, 0.8f },
                { "IKS KEL'TOR (TEL'TAK)", 0.011f, 41.0f, glm::vec3(0.6f, 0.6f, 0.6f), 2.7f, 1.0f }
        } :
        std::vector<std::tuple<std::string, float, float, glm::vec3, float, float>>{
            {"INTERNATIONAL SPACE STATION", 0.01f, 51.6f, glm::vec3(1.0f, 1.0f, 0.0f), 2.0f, 1.0f},
            {"HUBBLE SPACE TELESCOPE", 0.0098f, 28.5f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SPUTNIK 1", 0.0096f, 65.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"LANDSAT 8", 0.0096f, 98.2f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"GOES-16", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"IRIDIUM 33", 0.0095f, 86.4f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"TIANGONG-1", 0.0101f, 42.8f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"NOAA-19", 0.0094f, 98.7f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"AQUA", 0.0096f, 98.2f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"TERRA", 0.0096f, 98.2f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"ENVISAT", 0.0095f, 98.4f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"JASON-3", 0.0092f, 66.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"CRYOSAT-2", 0.0096f, 92.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SENTINEL-1A", 0.0097f, 98.18f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"METOP-A", 0.0094f, 98.7f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"KOSMOS 2251", 0.0095f, 74.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"GALILEO G1", 0.0048f, 56.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"GPS IIF-12", 0.0049f, 55.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"INMARSAT-4 F3", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SIRIUS FM-6", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"CHANDRA X-RAY OBSERVATORY", 0.0035f, 28.5f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"KEPLER SPACE TELESCOPE", 0.0094f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SPOT-7", 0.0097f, 98.2f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SWIFT GAMMA-RAY BURST MISSION", 0.0098f, 20.6f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"FENGYUN-2D", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"RADARSAT-2", 0.0095f, 98.6f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"ALOS-2", 0.0098f, 97.9f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"WORLDVIEW-3", 0.0098f, 97.2f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"GCOM-W1", 0.0096f, 98.2f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"OFEQ-10", 0.0098f, 141.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"YAMAL-402", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"ASTROSAT", 0.0098f, 6.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"CARTOSAT-2", 0.0098f, 97.9f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"RESURS-P1", 0.0099f, 97.3f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"KOMPSAT-3", 0.0097f, 98.1f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"GONETS-M1", 0.0091f, 82.5f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"YAOGAN-30", 0.0098f, 35.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"BEIDOU G7", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"INSAT-4CR", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"EUTELSAT 8 WEST B", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"THAICOM 8", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"NUSANTARA SATU", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"GSAT-31", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"AMOS-17", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"INTELSAT 39", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SES-12", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"TELSTAR 19V", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"ABS-3A", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"BRISAT", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"ECHOSTAR 23", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f},
            {"SKYNET 5D", 0.0039f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 0.7f}
        };

        std::vector<std::string> activeSatellites;
        for (std::vector<Satellite>::const_iterator s = satellites.begin(); s != satellites.end(); ++s) {
            activeSatellites.push_back(s->name);
        }

        std::vector<int> availableIndices;
        for (size_t i = 0; i < satelliteData.size(); ++i) {
            const std::string& name = std::get<0>(satelliteData[i]);
            if (std::find(activeSatellites.begin(), activeSatellites.end(), name) != activeSatellites.end()) {
                continue;
            }
            std::map<std::string, float>::iterator it = satelliteDisplayHistory.find(name);
            bool canDisplay = true;
            if (it != satelliteDisplayHistory.end()) {
                float lastTime = it->second;
                float cooldown = (name == "IKS D'GAVAH (BIRD-OF-PREY)" || name == "INTERNATIONAL SPACE STATION") ? 240.0f : 60.0f;
                if (totalTime - lastTime < cooldown) {
                    canDisplay = false;
                }
            }
            if (canDisplay) {
                availableIndices.push_back(static_cast<int>(i));
            }
        }

        if (availableIndices.empty()) {
            satelliteTimer = 10.0f;
            return;
        }

        std::uniform_int_distribution<int> nameDist(0, availableIndices.size() - 1);
        int nameIndex = availableIndices[nameDist(rng)];
        sat.name = std::get<0>(satelliteData[nameIndex]);
        sat.speed = std::get<1>(satelliteData[nameIndex]);
        float inclination = std::get<2>(satelliteData[nameIndex]);
        sat.textColor = std::get<3>(satelliteData[nameIndex]);
        sat.size = std::get<4>(satelliteData[nameIndex]);
        sat.brightness = std::get<5>(satelliteData[nameIndex]);
        sat.isISS = (sat.name == "IKS D'GAVAH (BIRD-OF-PREY)" || sat.name == "INTERNATIONAL SPACE STATION");
        sat.lastDisplayedTime = totalTime;

        satelliteDisplayHistory[sat.name] = totalTime;

        float angle = glm::radians(90.0f - inclination);
        float direction = startLeft ? 1.0f : -1.0f;
        float xComponent = direction * cos(angle);
        float yComponent = sin(angle);
        // Ensure a minimum X movement to avoid vertical trajectories
        const float minXComponent = 0.5f;
        if (fabs(xComponent) < minXComponent) {
            xComponent = direction * minXComponent;
            // Adjust Y component to maintain unit vector
            float magnitude = sqrt(xComponent * xComponent + yComponent * yComponent);
            xComponent /= magnitude;
            yComponent /= magnitude;
        }
        sat.heading = glm::vec2(xComponent, yComponent);

        satellites.push_back(sat);
        satelliteTimer = timerDist(rng);
    }

    // Update satellites and their exhaust trails
    for (std::vector<Satellite>::iterator it = satellites.begin(); it != satellites.end();) {
        // Store the previous position before updating
        glm::vec2 prevPos = it->position;

        // Update satellite position
        it->position += it->heading * it->speed * dt;

        // Add exhaust trail in alien scene only
        if (scene == Scene::ALIEN) {
            static float exhaustTimer = 0.0f;
            exhaustTimer += dt;
            if (exhaustTimer >= 0.05f) { // Add a new segment every 0.05 seconds
                ExhaustSegment segment;
                segment.startPos = prevPos;
                segment.endPos = it->position;
                segment.lifetime = 1.0f + (it->size - 2.0f) * 0.5f;
                segment.alpha = 0.5f;
                it->trail.push_back(segment);
                exhaustTimer = 0.0f;
            }

            // Update exhaust segments
            for (std::vector<ExhaustSegment>::iterator trailIt = it->trail.begin(); trailIt != it->trail.end();) {
                trailIt->lifetime -= dt;
                trailIt->alpha = (trailIt->lifetime / (1.0f + (it->size - 2.0f) * 0.5f)) * 0.5f;
                if (trailIt->lifetime <= 0.0f || trailIt->alpha <= 0.0f) {
                    trailIt = it->trail.erase(trailIt);
                } else {
                    ++trailIt;
                }
            }
        }

        // Remove satellite if off-screen
        if (it->position.x > 1.0f || it->position.x < 0.0f || it->position.y > 1.0f || it->position.y < 0.0f) {
            it = satellites.erase(it);
        } else {
            ++it;
        }
    }
}

void CelestialObjectManager::updateStarlinkTrains(float dt, TimeOfDay currentTime) {
    if (currentTime != TimeOfDay::NIGHT) {
        starlinkTrains.clear();
        return;
    }

    std::uniform_int_distribution<int> binaryDist(0, 1);
    std::uniform_real_distribution<float> posDist(0.3f, 0.7f);
    std::uniform_real_distribution<float> timerDist(60.0f, 300.0f);

    starlinkTimer -= dt;
    if (starlinkTimer <= 0.0f && starlinkTrains.empty()) {
        const std::string fleetName = (scene == Scene::ALIEN)
            ? "KLINGON DEFENSE FORCE TACTICAL FLEET"
            : "STARLINK TRAIN FORMATION";

        auto it = satelliteDisplayHistory.find(fleetName);
        if (it != satelliteDisplayHistory.end()) {
            float lastTime = it->second;
            float cooldown = 300.0f; 
            if (totalTime - lastTime < cooldown) {
                starlinkTimer = 60.0f; 
                return;
            }
        }

        StarlinkTrain train;
        train.count = 7;
        train.speed = 0.0096f;
        train.brightness = 0.8f;
        train.size = 2.0f;
        train.lastDisplayedTime = totalTime;

        bool startLeft = binaryDist(rng) == 0;
        float startY = posDist(rng);
        float startX = startLeft ? 0.0f : 1.0f;

        float inclination = 53.0f;
        float angle = glm::radians(90.0f - inclination);
        float direction = startLeft ? 1.0f : -1.0f;
        train.heading = glm::vec2(direction * cos(angle), sin(angle));

        float spacing = 0.02f;
        glm::vec2 leaderPos = glm::vec2(startX, startY);
        train.positions.push_back(leaderPos);

        if (scene == Scene::ALIEN) {
            // Alien scene: V-shaped tactical formation
            constexpr float angleSpread = glm::radians(30.0f);
            for (int i = 1; i < train.count; ++i) {
                int side = (i % 2 == 0) ? 1 : -1;
                int rank = (i + 1) / 2;
                float offsetX = rank * spacing * cos(angle + side * angleSpread);
                float offsetY = rank * spacing * sin(angle + side * angleSpread);
                glm::vec2 pos = leaderPos - train.heading * (rank * spacing) + glm::vec2(offsetX, offsetY);
                train.positions.push_back(pos);
            }
        } else {
            // Earth scene: Single-file line formation
            for (int i = 1; i < train.count; ++i) {
                glm::vec2 pos = leaderPos - train.heading * (i * spacing);
                train.positions.push_back(pos);
            }
        }

        // Initialize a trail for each ship in the formation
        train.trails.resize(train.count);

        starlinkTrains.push_back(train);
        satelliteDisplayHistory[fleetName] = totalTime;
        starlinkTimer = timerDist(rng);
    }

    for (auto it = starlinkTrains.begin(); it != starlinkTrains.end();) {
        bool offScreen = true;
        std::vector<glm::vec2> prevPositions(it->positions.size());
        for (size_t i = 0; i < it->positions.size(); ++i) {
            prevPositions[i] = it->positions[i];
            it->positions[i] += it->heading * it->speed * dt;
            if (it->positions[i].x >= 0.0f && it->positions[i].x <= 1.0f &&
                it->positions[i].y >= 0.0f && it->positions[i].y <= 1.0f) {
                offScreen = false;
            }

            // Add exhaust trail for each ship in the alien scene
            if (scene == Scene::ALIEN) {
                static float exhaustTimer = 0.0f;
                exhaustTimer += dt;
                if (exhaustTimer >= 0.05f) { // Add a new segment every 0.05 seconds
                    ExhaustSegment segment;
                    segment.startPos = prevPositions[i]; // Start at the previous position
                    segment.endPos = it->positions[i]; // End at the current position
                    float formationSize = it->size * 1.5f;
                    segment.lifetime = 1.0f + (formationSize - 2.0f) * 0.5f; // Lifetime scales with size
                    segment.alpha = 0.5f;  // Reduced initial alpha for less brightness
                    it->trails[i].push_back(segment);
                    exhaustTimer = 0.0f;
                }
            }
        }

        // Update exhaust segments for each ship's trail
        if (scene == Scene::ALIEN) {
            for (size_t i = 0; i < it->trails.size(); ++i) {
                for (auto trailIt = it->trails[i].begin(); trailIt != it->trails[i].end();) {
                    trailIt->lifetime -= dt;
                    // Fade alpha based on lifetime
                    trailIt->alpha = (trailIt->lifetime / (1.0f + (it->size * 1.5f - 2.0f) * 0.5f)) * 0.5f;
                    if (trailIt->lifetime <= 0.0f || trailIt->alpha <= 0.0f) {
                        trailIt = it->trails[i].erase(trailIt);
                    } else {
                        ++trailIt;
                    }
                }
            }
        }

        // Remove satellite if off-screen
        if (offScreen) {
            it = starlinkTrains.erase(it);
        } else {
            ++it;
        }
    }
}

void CelestialObjectManager::update(float dt, TimeOfDay currentTime) {
    totalTime += dt;
    updateShootingStars(dt, currentTime);
    updateSatellites(dt, currentTime);
    updateStarlinkTrains(dt, currentTime);

    // Calculate timeFactor for position updates based on TimeOfDay
    switch (currentTime) {
        case TimeOfDay::DAWN:
            timeFactor = 0.0f; // Left side of horizon: x = 0.2, y = 0.5
            break;
        case TimeOfDay::MID_DAY:
            timeFactor = 0.5f; // Centered at the top: x = 0.5, y = 0.9
            break;
        case TimeOfDay::DUSK:
            timeFactor = 1.0f; // Right side of horizon: x = 0.8, y = 0.5
            break;
        case TimeOfDay::NIGHT:
            timeFactor = 0.5f; // Moon at Mid-Day position: x = 0.5, y = 0.9
            break;
        default:
            timeFactor = 0.5f;
    }

    // Log timeFactor every 60 frames (approx. once per second at 60 FPS)
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 60 == 0) {
        DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "update",
            "TimeOfDay=" + std::to_string(static_cast<int>(currentTime)) +
            ", timeFactor=" + std::to_string(timeFactor));
        frameCounter = 0;
    }
}

void CelestialObjectManager::initializeCloseCelestials() {
    closeCelestials.clear();

    // Initialize Sun
    CloseCelestial sun;
    sun.name = "SUN";
    sun.position = glm::vec2(0.5f, 0.8f);
    sun.size = 100.0f;
    sun.tintColor = glm::vec3(1.0f, 0.9f, 0.7f); // Yellowish tint
    sun.rotation = 0.0f;
    sun.brightness = 1.0f;
    sun.celestialType = "Sun";
#ifdef _WIN32
    sun.texturePath = "resources\\textures\\sun.png";
#else
    sun.texturePath = "./resources/textures/sun.png";
#endif
    sun.texture = 0;

    // Initialize Moon
    CloseCelestial moon;
    moon.name = "MOON";
    moon.position = glm::vec2(0.5f, 0.8f);
    moon.size = 100.0f;
    moon.tintColor = (scene == Scene::ALIEN) ? glm::vec3(0.8f, 0.7f, 0.9f) : glm::vec3(1.0f, 1.0f, 1.0f);
    moon.rotation = (scene == Scene::ALIEN) ? glm::radians(90.0f) : 0.0f;
    moon.brightness = 1.0f;
    moon.celestialType = "Moon";
#ifdef _WIN32
    moon.texturePath = "resources\\textures\\moon.png";
#else
    moon.texturePath = "./resources/textures/moon.png";
#endif
    moon.texture = 0;

    // Initialize Planet (only for alien scene)
    CloseCelestial planet;
    if (scene == Scene::ALIEN) {
        planet.name = "QO\'NOS";
        planet.position = glm::vec2(0.85f, 0.80f); 
        planet.size = 60.0f;
        planet.tintColor = glm::vec3(0.8f, 0.7f, 0.9f);
        planet.rotation = 0.0f;
        planet.brightness = 0.5f;
        planet.celestialType = "Planet";
#ifdef _WIN32
        planet.texturePath = "resources\\textures\\alien_planet.png";
#else
        planet.texturePath = "./resources/textures/alien_planet.png";
#endif
        planet.texture = 0;
    }

    // Add celestials to the vector
    closeCelestials.push_back(sun);
    closeCelestials.push_back(moon);
    if (scene == Scene::ALIEN) {
        closeCelestials.push_back(planet);
    }

    // Debug: Log the number of celestials initialized
    std::cout << "CelestialObjectManager::initializeCloseCelestials: Initialized " << closeCelestials.size()
        << " celestial objects (Sun, Moon" << (scene == Scene::ALIEN ? ", Planet)" : ")")
        << std::endl;

    // Load textures and set up OpenGL resources for each celestial
    for (auto& celestial : closeCelestials) {
        SDL_Surface* surface = IMG_Load(celestial.texturePath.c_str());
        if (!surface) {
            std::cout << "CelestialObjectManager::initializeCloseCelestials: Failed to load texture for "
                << celestial.name << " at " << celestial.texturePath << ": " << SDL_GetError() << std::endl;
            continue;
        }

        GLenum format;
        if (surface->format == SDL_PIXELFORMAT_RGBA32) {
            format = GL_RGBA;
        } else if (surface->format == SDL_PIXELFORMAT_RGB24) {
            format = GL_RGB;
        } else {
            std::cout << "CelestialObjectManager::initializeCloseCelestials: Unsupported image format for "
                << celestial.name << " at " << celestial.texturePath << ": "
                << SDL_GetPixelFormatName(surface->format) << std::endl;
            SDL_DestroySurface(surface);
            continue;
        }

        glGenTextures(1, &celestial.texture);
        glBindTexture(GL_TEXTURE_2D, celestial.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        SDL_DestroySurface(surface);

        std::cout << "CelestialObjectManager::initializeCloseCelestials: Successfully loaded texture for "
            << celestial.name << " at " << celestial.texturePath << std::endl;
    }

    // Define a quad for close celestials with explicit indices, reverting z to 0.0f
    float vertices[] = {
        // Positions (x, y, z)   // TexCoords
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f,  // Bottom-left (0)
         0.5f, -0.5f, 0.0f,     1.0f, 0.0f,  // Bottom-right (1)
         0.5f,  0.5f, 0.0f,     1.0f, 1.0f,  // Top-right (2)
        -0.5f,  0.5f, 0.0f,     0.0f, 1.0f   // Top-left (3)
    };

    unsigned int indices[] = {
        0, 1, 2,  // First triangle (bottom-left, bottom-right, top-right)
        0, 2, 3   // Second triangle (bottom-left, top-right, top-left)
    };

    glGenVertexArrays(1, &closeCelestialVAO);
    glGenBuffers(1, &closeCelestialVBO);
    glGenBuffers(1, &closeCelestialEBO);

    glBindVertexArray(closeCelestialVAO);

    glBindBuffer(GL_ARRAY_BUFFER, closeCelestialVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, closeCelestialEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Create shader for close celestials
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;

        uniform mat4 model;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * model * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D celestialTexture;
        uniform vec3 tintColor;  // Color tint
        uniform float opacity;   // Opacity multiplier
        uniform int isGlowPass;  // Flag for glow pass (1 for glow, 0 for texture)

        void main() {
            if (isGlowPass == 1) {
                // Glow pass for sun
                vec2 center = vec2(0.5);
                vec2 normalizedTexCoord = TexCoord * 2.0 - 1.0; // Normalize to [-1, 1]
                float dist = length(normalizedTexCoord);
                // Power function for bright core with smooth fade
                float intensity = pow(max(0.0, 1.0 - dist), 3.5);
                // Scale alpha for gentle fade
                float alpha = intensity * opacity * 17.0;  // adjust last value for sun glow brightness
                if (alpha < 0.01) discard;
                FragColor = vec4(tintColor * intensity, alpha);
            } else {
                // Texture pass
                vec4 texColor = texture(celestialTexture, TexCoord);
                if (texColor.a < 0.1) discard; // Adjusted threshold for transparency
                // Apply color tint
                texColor.rgb *= tintColor;
                // Apply opacity
                texColor.a *= opacity;
                FragColor = texColor;
            }
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Close celestial vertex shader compilation failed: " << infoLog << std::endl;
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Close celestial fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }

    closeCelestialShader = glCreateProgram();
    glAttachShader(closeCelestialShader, vertexShader);
    glAttachShader(closeCelestialShader, fragmentShader);
    glLinkProgram(closeCelestialShader);
    glGetProgramiv(closeCelestialShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(closeCelestialShader, 512, nullptr, infoLog);
        std::cerr << "Close celestial shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        closeCelestialShader = 0;
        return;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void CelestialObjectManager::renderCloseCelestials(float starAlpha, float sunMoonPosition) {
    glUseProgram(closeCelestialShader);
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(WINDOW_WIDTH), 0.0f, static_cast<float>(WINDOW_HEIGHT), -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(closeCelestialShader, "projection"), 1, GL_FALSE, &orthoProjection[0][0]);

    // Log timeFactor to confirm its value during rendering
    static int timeFactorFrameCounter = 0;
    timeFactorFrameCounter++;
    if (timeFactorFrameCounter % 60 == 0) {
        DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "renderCloseCelestials",
            "Using timeFactor=" + std::to_string(timeFactor));
        timeFactorFrameCounter = 0;
    }

    int renderedCelestials = 0;
    for (auto& celestial : closeCelestials) {
        // Calculate position based on timeFactor
        float calcX = 0.0f;
        float calcY = 0.0f;
        if (celestial.celestialType == "Sun") {
            calcX = 0.2f + timeFactor * 0.6f; // Moves from 0.2 to 0.8
            calcY = 0.5f + 0.4f * sin(glm::radians(timeFactor * 180.0f)); // Arc from 0.5 to 0.9 to 0.5
            celestial.position.x = calcX;
            celestial.position.y = calcY;
        } else if (celestial.celestialType == "Moon") {
            float moonTimeFactor = (world->getCurrentTimeOfDay() == TimeOfDay::NIGHT) ? 0.5f : timeFactor;
            calcX = 0.1f + moonTimeFactor * 0.2f; // Shift to left: starts at 0.1, at night x = 0.2
            calcY = 0.5f + 0.4f * sin(glm::radians(moonTimeFactor * 180.0f)); // At night y = 0.9
            celestial.position.x = calcX;
            celestial.position.y = calcY;
        } else if (celestial.celestialType == "Planet") {
            calcX = 0.2f + timeFactor * 0.1f; // Slight movement
            calcY = 0.8f - timeFactor * 0.1f; // Slight vertical adjustment
            celestial.position.x = calcX;
            celestial.position.y = calcY;
        }

        // Determine if the celestial should be rendered based on time of day
        bool shouldRender = false;
        if (celestial.celestialType == "Sun") {
            if (world->getCurrentTimeOfDay() == TimeOfDay::DAWN ||
                world->getCurrentTimeOfDay() == TimeOfDay::MID_DAY ||
                world->getCurrentTimeOfDay() == TimeOfDay::DUSK) {
                shouldRender = true;
            }
        } else if (celestial.celestialType == "Moon") {
            if (world->getCurrentTimeOfDay() == TimeOfDay::NIGHT) {
                shouldRender = true;
            }
        } else if (celestial.celestialType == "Planet" && scene == Scene::ALIEN) {
            if (world->getCurrentTimeOfDay() == TimeOfDay::NIGHT) {
                shouldRender = true;
            }
        }

        if (!shouldRender) {
            continue;
        }

        // Adjust opacity based on time of day transitions
        float opacity = 1.0f;
        if (celestial.celestialType == "Sun") {
            // Consistent opacity for sun across DAWN, MID_DAY, DUSK
            opacity = 0.04f; // Reduced for translucency
        } else if (celestial.celestialType == "Moon" || celestial.celestialType == "Planet") {
            if (world->getCurrentTimeOfDay() == TimeOfDay::NIGHT) {
                opacity = starAlpha;
            } else if (world->getCurrentTimeOfDay() == TimeOfDay::DUSK) {
                opacity = starAlpha * 0.5f;
            }
        }

        if (opacity < 0.01f) {
            continue;
        }

        // Log position every 60 frames (approx. once per second at 60 FPS)
        static int renderFrameCounter = 0;
        renderFrameCounter++;
        if (renderFrameCounter % 60 == 0) {
            DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "renderCloseCelestials",
                "Calculated position for " + celestial.name + ": (" + std::to_string(calcX) + ", " + std::to_string(calcY) + ")");
            DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "renderCloseCelestials",
                "Rendering " + celestial.name + " at position (" + std::to_string(celestial.position.x) + ", " +
                std::to_string(celestial.position.y) + ") with opacity=" + std::to_string(opacity) +
                ", screen position (" + std::to_string(celestial.position.x * WINDOW_WIDTH) + ", " +
                std::to_string(celestial.position.y * WINDOW_HEIGHT) + ")");
            renderFrameCounter = 0;
        }

        // Enable blending and depth testing
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE); // Prevent affecting depth buffer

        if (celestial.celestialType == "Sun") {
            // Pass 1: Render glow
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for glow
            glUniform1i(glGetUniformLocation(closeCelestialShader, "isGlowPass"), 1);
            glUniform3fv(glGetUniformLocation(closeCelestialShader, "tintColor"), 1, &celestial.tintColor[0]);
            glUniform1f(glGetUniformLocation(closeCelestialShader, "opacity"), opacity);

            glm::mat4 glowModel = glm::mat4(1.0f);
            glowModel = glm::translate(glowModel, glm::vec3(celestial.position.x * WINDOW_WIDTH, celestial.position.y * WINDOW_HEIGHT, 0.0f));
            glowModel = glm::rotate(glowModel, celestial.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            float glowScale = celestial.size * 80.0f; // Larger scale for glow
            glowModel = glm::scale(glowModel, glm::vec3(glowScale));
            glUniformMatrix4fv(glGetUniformLocation(closeCelestialShader, "model"), 1, GL_FALSE, &glowModel[0][0]);

            glBindVertexArray(closeCelestialVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Pass 2: Render texture
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard blending for texture
            glUniform1i(glGetUniformLocation(closeCelestialShader, "isGlowPass"), 0);
            glUniform3fv(glGetUniformLocation(closeCelestialShader, "tintColor"), 1, &celestial.tintColor[0]);
            glUniform1f(glGetUniformLocation(closeCelestialShader, "opacity"), opacity);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(celestial.position.x * WINDOW_WIDTH, celestial.position.y * WINDOW_HEIGHT, 0.0f));
            model = glm::rotate(model, celestial.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            float scale = celestial.size;
            model = glm::scale(model, glm::vec3(scale));
            glUniformMatrix4fv(glGetUniformLocation(closeCelestialShader, "model"), 1, GL_FALSE, &model[0][0]);

            glBindVertexArray(closeCelestialVAO);
            glBindTexture(GL_TEXTURE_2D, celestial.texture);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        } else {
            // Moon or Planet (texture only)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glUniform1i(glGetUniformLocation(closeCelestialShader, "isGlowPass"), 0);
            glUniform3fv(glGetUniformLocation(closeCelestialShader, "tintColor"), 1, &celestial.tintColor[0]);
            glUniform1f(glGetUniformLocation(closeCelestialShader, "opacity"), opacity);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(celestial.position.x * WINDOW_WIDTH, celestial.position.y * WINDOW_HEIGHT, 0.0f));
            model = glm::rotate(model, celestial.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            float scale = celestial.celestialType == "Planet" ? celestial.size * 0.5f : celestial.size;
            model = glm::scale(model, glm::vec3(scale));
            glUniformMatrix4fv(glGetUniformLocation(closeCelestialShader, "model"), 1, GL_FALSE, &model[0][0]);

            glBindVertexArray(closeCelestialVAO);
            glBindTexture(GL_TEXTURE_2D, celestial.texture);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Restore depth state
        glDepthMask(GL_TRUE);

        renderedCelestials++;
    }

    // Log the number of celestials rendered
    DataManager::LogDebug(DebugCategory::RENDERING, "CelestialObjectManager", "renderCloseCelestials",
        "Rendered " + std::to_string(renderedCelestials) + " close celestials");
}
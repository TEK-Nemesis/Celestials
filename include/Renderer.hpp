#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <vector>
#include "World.hpp"
#include "Sky.hpp"
#include "CelestialObjectManager.hpp"
#include "imgui.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(World* w);
    void render(float dt);

    void toggleShowConstellationNames() { celestialObjectManager->toggleShowConstellationNames(); }
    void toggleShowPlanetNames() { celestialObjectManager->toggleShowPlanetNames(); }
    void toggleShowSatelliteNames() { celestialObjectManager->toggleShowSatelliteNames(); }
    void setCurrentTimeOfDayIndex(int index) { currentTimeOfDayIndex = index; }
    void toggleUseKlingonNames() { useKlingonNames = !useKlingonNames; }
    void setScene(Scene newScene);  // public so InputManager can access it
    void renderBottomTerrain();
    void renderDistantTerrain();

private:

    enum class RenderStage {
        SKY,
        DISTANT_CELESTIALS,
        CLOSE_CELESTIALS,
        CELESTIAL_TEXT,
        DISTANT_TERRAIN,
        CLOUDS,
        BOTTOM_TERRAIN,
        HOTKEYS_TEXT,
        IMGUI,
        COUNT
    };

    friend class CelestialObjectManager;

    World* world;
    TTF_Font* font;
    TTF_Font* klingonFont;
    bool useKlingonFont;
    bool useKlingonNames;
    std::unique_ptr<Sky> sky;
    CelestialObjectManager* celestialObjectManager;

    GLuint textShader, textVAO, textVBO;
    GLuint terrainShader;
    GLuint smokeShader, smokeVAO, smokeVBO, smokeEBO, smokeTexture;

    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 cameraPos;
    glm::vec3 cameraTarget;
    glm::vec3 cameraUp;
    float cameraZoom;
    float cameraYaw;
    float cameraPitch;
    float terrainHardness;

    int currentTimeOfDayIndex;
    int sceneNamesIndex;

    std::vector<std::string> sceneNames;
    bool regenerationTriggered;
    bool regenerateDistantTriggered;

    void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool celestial = false);
    void renderHotKeys();
    void renderCloseCelestials();
    void renderDistantCelestials();
    void renderCelestialText();
    void displayTest_GUI();

    void resetCameraControls();
    bool initializeTerrainShader();
    bool initializeTextRendering();
    void cleanupOpenGLResources();
    void cleanupSmokeResources();
    void cleanupSkyResources();
    void reinitializeCelestialResources();
};

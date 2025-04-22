#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Enums.hpp"

class Sky {
public:
    Sky();
    ~Sky();

    bool initialize();
    void render(TimeOfDay timeOfDay, float transitionProgress);
    void renderClouds(TimeOfDay timeOfDay, float transitionProgress, float totalTime);
    void cleanup();

    float getSunMoonPosition() const { return sunMoonPosition; }

private:
    GLuint skyShader;
    GLuint skyVAO, skyVBO, skyEBO;
    glm::vec3 skyColorTop;
    glm::vec3 skyColorBottom;
    glm::vec3 targetSkyColorTop;
    glm::vec3 targetSkyColorBottom;
    float skyTransitionTime;
    float skyTransitionDuration;
    bool skyTransitioning;
    TimeOfDay currentTimeOfDay;
    TimeOfDay targetTimeOfDay;
    float sunMoonPosition;
    float targetSunMoonPosition;
    float transitionProgress;
    float aspectRatio;
    bool immediateFadeFromNight;

    // Cloud rendering
    GLuint cloudShader;
    GLuint cloudVAO, cloudVBO, cloudEBO;

    bool initializeClouds();
    bool initializeSky();
};

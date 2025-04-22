#include "Renderer.hpp"
#include "World.hpp"
#include "DataManager.hpp"
#include <SDL3_image/SDL_image.h>
#include <numeric>
#include <string>
#include <Constants.hpp>

Renderer::Renderer() : world(nullptr), font(nullptr), klingonFont(nullptr), useKlingonFont(false), useKlingonNames(true),
textShader(0), textVAO(0), textVBO(0), terrainShader(0), smokeShader(0), smokeVAO(0), smokeVBO(0),
smokeEBO(0), smokeTexture(0), cameraZoom(1713.225f), cameraYaw(0.0f), cameraPitch(11.690f),
terrainHardness(0.5f), currentTimeOfDayIndex(1), sceneNamesIndex(0), regenerationTriggered(false), regenerateDistantTriggered(false) {
    sceneNames = { "Summer", "Fall", "Winter", "Spring", "Alien" };
}

Renderer::~Renderer() {
    cleanupOpenGLResources();
    cleanupSmokeResources();
    cleanupSkyResources();
    if (font) TTF_CloseFont(font);
    if (klingonFont) TTF_CloseFont(klingonFont);
    TTF_Quit();
}

bool Renderer::initializeTextRendering() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec4 vertex;
        out vec2 TexCoords;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        uniform sampler2D text;
        uniform vec3 textColor;
        void main() {
            vec4 sampled = texture(text, TexCoords);
            if (sampled.a < 0.1) discard;  // Discard pixels with low alpha to ensure transparency
            color = vec4(textColor, sampled.a);  // Use the sampled alpha for anti-aliased edges
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
        DataManager::LogError("Renderer", "initializeTextRendering", "Text vertex shader compilation failed: " + std::string(infoLog));
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        DataManager::LogError("Renderer", "initializeTextRendering", "Text fragment shader compilation failed: " + std::string(infoLog));
        glDeleteShader(vertexShader);
        return false;
    }

    textShader = glCreateProgram();
    glAttachShader(textShader, vertexShader);
    glAttachShader(textShader, fragmentShader);
    glLinkProgram(textShader);
    glGetProgramiv(textShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(textShader, 512, nullptr, infoLog);
        DataManager::LogError("Renderer", "initializeTextRendering", "Text shader program linking failed: " + std::string(infoLog));
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glValidateProgram(textShader);
    glGetProgramiv(textShader, GL_VALIDATE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(textShader, 512, nullptr, infoLog);
        DataManager::LogError("Renderer", "initializeTextRendering", "Text shader program validation failed: " + std::string(infoLog));
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Renderer::renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool celestial) {
    TTF_Font* activeFont = (celestial && useKlingonFont && useKlingonNames) ? klingonFont : font;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SDL_Color sdlColor = { 255, 255, 255, 255 };
    SDL_Surface* surface = TTF_RenderText_Solid(activeFont, text.c_str(), static_cast<int>(text.length()), sdlColor);
    if (!surface) {
        DataManager::LogError("Renderer", "renderText", "Failed to render text: " + std::string(SDL_GetError()));
        return;
    }

    SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!rgbaSurface) {
        DataManager::LogError("Renderer", "renderText", "Failed to convert surface to RGBA: " + std::string(SDL_GetError()));
        SDL_DestroySurface(surface);
        return;
    }

    if (SDL_MUSTLOCK(rgbaSurface)) {
        if (!SDL_LockSurface(rgbaSurface)) {
            std::string errorMsg = SDL_GetError();
            DataManager::LogError("Renderer", "renderText",
                "Failed to lock surface for directly accessing the pixels. " + (errorMsg.length() > 0 ? errorMsg : ""));
            SDL_DestroySurface(surface);
            SDL_DestroySurface(rgbaSurface);
            return;
        }
    }

    Uint32* pixels = (Uint32*)rgbaSurface->pixels;
    int pitch = rgbaSurface->pitch / sizeof(Uint32);
    for (int py = 0; py < rgbaSurface->h; ++py) {
        for (int px = 0; px < rgbaSurface->w; ++px) {
            Uint32 pixel = pixels[py * pitch + px];
            Uint8 alpha = (pixel >> 24) & 0xFF;
            if (alpha == 0) {
                pixels[py * pitch + px] = 0x00000000;
            } else {
                pixels[py * pitch + px] = (pixel & 0x00FFFFFF) | (255 << 24);
            }
        }
    }

    if (SDL_MUSTLOCK(rgbaSurface)) SDL_UnlockSurface(rgbaSurface);

    SDL_SetSurfaceBlendMode(rgbaSurface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceAlphaMod(rgbaSurface, 255);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);

    glUseProgram(textShader);
    glUniform3fv(glGetUniformLocation(textShader, "textColor"), 1, &color[0]);
    glUniform1i(glGetUniformLocation(textShader, "text"), 0);

    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(WINDOW_WIDTH), 0.0f, static_cast<float>(WINDOW_HEIGHT));
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, &orthoProjection[0][0]);

    float width = rgbaSurface->w * scale;
    float height = rgbaSurface->h * scale;

    float xPos = celestial ? x - (width / 2.0f) : x;
    float yPos = y;

    float vertices[] = {
        xPos,         yPos + height, 0.0f, 0.0f,
        xPos,         yPos,          0.0f, 1.0f,
        xPos + width, yPos,          1.0f, 1.0f,

        xPos,         yPos + height, 0.0f, 0.0f,
        xPos + width, yPos,          1.0f, 1.0f,
        xPos + width, yPos + height, 1.0f, 0.0f
    };

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteTextures(1, &texture);
    SDL_DestroySurface(surface);
    SDL_DestroySurface(rgbaSurface);
}

void Renderer::setScene(Scene newScene) {
    if (newScene != world->getScene()) {
        sceneNamesIndex = static_cast<int>(newScene);
        world->setScene(newScene);
        useKlingonFont = (newScene == Scene::ALIEN);
        useKlingonNames = useKlingonFont;
        celestialObjectManager->setScene(newScene);
        world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
        world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        reinitializeCelestialResources();

        DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "setScene",
            "Switching to scene: " + std::to_string(static_cast<int>(newScene)));
    }
}

void Renderer::displayTest_GUI() {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500.0f, 1100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 400.0f), ImVec2(FLT_MAX, FLT_MAX));

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.7f);

    if (ImGui::Begin("Debug Controls")) {
        ImGui::Text("Terrain Scene:");
        std::vector<const char*> sceneNamesCStr;
        for (const auto& name : sceneNames) {
            sceneNamesCStr.push_back(name.c_str());
        }
        if (ImGui::Combo("Scene", &sceneNamesIndex, sceneNamesCStr.data(), static_cast<int>(sceneNamesCStr.size()))) {
            setScene(static_cast<Scene>(sceneNamesIndex));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Select a terrain scene.");
        }

        ImGui::Text("Low Height Color (RGB):");
        glm::vec3& terrainLowColor = world->getBottomTerrain()->getLowColor();
        if (ImGui::SliderFloat("Low R", &terrainLowColor.r, 0.0f, 1.0f)) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        }
        if (ImGui::SliderFloat("Low G", &terrainLowColor.g, 0.0f, 1.0f)) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        }
        if (ImGui::SliderFloat("Low B", &terrainLowColor.b, 0.0f, 1.0f)) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        }
        ImGui::Text("Low Color Value: (%.3f, %.3f, %.3f)", terrainLowColor.r, terrainLowColor.g, terrainLowColor.b);

        ImGui::Text("High Height Color (RGB):");
        glm::vec3& terrainHighColor = world->getBottomTerrain()->getHighColor();
        if (ImGui::SliderFloat("High R", &terrainHighColor.r, 0.0f, 1.0f)) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        }
        if (ImGui::SliderFloat("High G", &terrainHighColor.g, 0.0f, 1.0f)) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        }
        if (ImGui::SliderFloat("High B", &terrainHighColor.b, 0.0f, 1.0f)) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
        }
        ImGui::Text("High Color Value: (%.3f, %.3f, %.3f)", terrainHighColor.r, terrainHighColor.g, terrainHighColor.b);

        ImGui::Text("Distant Terrain Noise Parameters:");
        ImGui::PushItemWidth(300.0f);
        NoiseParameters& noiseParamsDistant = world->getDistantNoiseParams();
        ImGui::SliderFloat("Distant Base Height", &noiseParamsDistant.baseHeight, -WINDOW_HEIGHT, WINDOW_HEIGHT);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the base height of the distant terrain in pixels (-%d to %d).\nLower values shift the terrain downward, higher values shift it upward.", WINDOW_HEIGHT, WINDOW_HEIGHT);
        }
        ImGui::SliderFloat("Distant Min Height", &noiseParamsDistant.minHeight, 0.0f, WINDOW_HEIGHT / 2.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the minimum height offset for distant terrain valleys in pixels (0 to %d).\nHigher values create deeper valleys below the base height.", WINDOW_HEIGHT / 2);
        }
        ImGui::SliderFloat("Distant Max Height", &noiseParamsDistant.maxHeight, 0.0f, WINDOW_HEIGHT / 2.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the maximum height offset for distant terrain peaks in pixels (0 to %d).\nHigher values create taller peaks above the base height.", WINDOW_HEIGHT / 2);
        }
        ImGui::SliderFloat("Distant Frequency", &noiseParamsDistant.frequency, 0.001f, 2.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the frequency of distant terrain variations (0.001 to 2.0).\nLower values create broader, smoother hills; higher values create more frequent, jagged features.");
        }
        ImGui::SliderFloat("Distant Persistence", &noiseParamsDistant.persistence, 0.1f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the amplitude scaling of distant noise layers (0.1 to 1.0).\nHigher values create more pronounced peaks and valleys; lower values create flatter terrain.");
        }
        ImGui::SliderFloat("Distant Lacunarity", &noiseParamsDistant.lacunarity, 1.0f, 3.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the frequency scaling of distant noise layers (1.0 to 3.0).\nHigher values increase the detail in terrain features; lower values create smoother transitions.");
        }
        ImGui::SliderFloat("Distant Octaves", &noiseParamsDistant.octaves, 1.0f, 10.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the number of noise layers for distant terrain (1 to 10).\nHigher values add more fine details to the terrain; lower values create simpler, broader shapes.");
        }

        ImGui::Text("Distant Terrain Visual Parameters:");
        DistantTerrainParameters& distantParams = world->getDistantParams();
        ImGui::SliderFloat("Distant Z Position", &distantParams.zPosition, 200.0f, 2000.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the z-position of the distant terrain (200 to 2000).\nHigher values move the terrain farther away.");
        }
        ImGui::SliderFloat("Distant Y Offset", &distantParams.yOffset, -WINDOW_HEIGHT, WINDOW_HEIGHT);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the vertical position of the distant terrain (-%d to %d).\nHigher values move the terrain upward.", WINDOW_HEIGHT, WINDOW_HEIGHT);
        }
        ImGui::SliderFloat("Distant Color Fade", &distantParams.colorFade, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the color fading of the distant terrain (0.0 to 1.0).\nHigher values make the terrain colors more faded (grayer).");
        }
        ImGui::SliderFloat("Distant Depth Fade", &distantParams.depthFade, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the fade effect from the closest to the farthest part of the distant terrain (0.0 to 1.0).\nHigher values increase the fade effect towards the back.");
        }

        if (ImGui::Button("Generate Distant Terrain")) {
            world->triggerRegeneration(TerrainGenerationMode::DISTANT);
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "displayTest_GUI", "Generate Distant Terrain button clicked");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Regenerate the distant terrain with the current noise parameters.");
        }

        if (ImGui::Button("Reset Distant Defaults")) {
            world->resetDistantTerrainParams();
            world->resetDistantNoiseParameters();
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "displayTest_GUI", "Reset Distant Defaults button clicked");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Reset all distant terrain parameters to their default values.");
        }

        ImGui::Text("Bottom Terrain Noise Parameters:");
        NoiseParameters& noiseParamsBottom = world->getBottomNoiseParams();
        ImGui::SliderFloat("Bottom Base Height", &noiseParamsBottom.baseHeight, -WINDOW_HEIGHT, WINDOW_HEIGHT);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the base height of the terrain in pixels (-%d to %d).\nLower values shift the terrain downward, higher values shift it upward.", WINDOW_HEIGHT, WINDOW_HEIGHT);
        }
        ImGui::SliderFloat("Bottom Min Height", &noiseParamsBottom.minHeight, 0.0f, WINDOW_HEIGHT / 2.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the minimum height offset for terrain valleys in pixels (0 to %d).\nHigher values create deeper valleys below the base height.", WINDOW_HEIGHT / 2);
        }
        ImGui::SliderFloat("Bottom Max Height", &noiseParamsBottom.maxHeight, 0.0f, WINDOW_HEIGHT / 2.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the maximum height offset for terrain peaks in pixels (0 to %d).\nHigher values create taller peaks above the base height.", WINDOW_HEIGHT / 2);
        }
        ImGui::SliderFloat("Bottom Frequency", &noiseParamsBottom.frequency, 0.001f, 2.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the frequency of terrain variations (0.001 to 2.0).\nLower values create broader, smoother hills; higher values create more frequent, jagged features.");
        }
        ImGui::SliderFloat("Bottom Persistence", &noiseParamsBottom.persistence, 0.1f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the amplitude scaling of noise layers (0.1 to 1.0).\nHigher values create more pronounced peaks and valleys; lower values create flatter terrain.");
        }
        ImGui::SliderFloat("Bottom Lacunarity", &noiseParamsBottom.lacunarity, 1.0f, 3.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the frequency scaling of noise layers (1.0 to 3.0).\nHigher values increase the detail in terrain features; lower values create smoother transitions.");
        }
        ImGui::SliderFloat("Bottom Octaves", &noiseParamsBottom.octaves, 1.0f, 10.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the number of noise layers (1 to 10).\nHigher values add more fine details to the terrain; lower values create simpler, broader shapes.");
        }

        if (ImGui::Button("Generate Bottom Terrain")) {
            world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "displayTest_GUI", "Generate Bottom Terrain button clicked");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Regenerate the bottom terrain with the current noise parameters.");
        }

        if (ImGui::Button("Reset Bottom Defaults")) {
            world->resetBottomNoiseParameters();
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "displayTest_GUI", "Reset Bottom Defaults button clicked");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Reset all bottom terrain parameters to their default values.");
        }

        ImGui::Text("Time of Day and Projectile Settings:");
        const char* timeOfDayModes[] = { "Dawn", "Mid-Day", "Dusk", "Night" };
        if (ImGui::Combo("Time of Day", &currentTimeOfDayIndex, timeOfDayModes, IM_ARRAYSIZE(timeOfDayModes))) {
            world->setTimeOfDay(static_cast<TimeOfDay>(currentTimeOfDayIndex));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Select the time of day to transition the sky appearance.\nHotkeys: F4 (Dawn), F5 (Mid-Day), F6 (Dusk), F7 (Night)");
        }

        static int numPlayers = 1;
        if (ImGui::SliderInt("Number of Players", &numPlayers, 1, 10)) {
            // Placeholder; player spawning logic not implemented
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the number of players to place on the bottom terrain (1 to 10).");
        }

        static int currentProjectile = 0;
        const char* projectileTypes[] = { "Disintegrate", "Create Terrain", "Disturb" };
        if (ImGui::Combo("Projectile Type", &currentProjectile, projectileTypes, IM_ARRAYSIZE(projectileTypes))) {
            // Placeholder; projectile logic not implemented
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Select the type of projectile to fire:\n- Disintegrate: Removes terrain and damages units.\n- Create Terrain: Adds terrain and covers units in mud.\n- Disturb: Disturbs ceiling terrain, causing particles or chunks to fall.");
        }

        ImGui::SliderFloat("Terrain Hardness", &terrainHardness, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the terrain hardness (0.0 to 1.0):\n- 0.0 (soft): Particles fall gently, covering units.\n- 1.0 (hard): Chunks fall, damaging units.");
        }

        ImGui::Text("Camera Controls:");
        ImGui::SliderFloat("Camera Zoom", &cameraZoom, 200.0f, 3000.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust the camera zoom (distance from terrain, 200 to 3000).\nLower values zoom in closer to the terrain; higher values zoom out farther.");
        }
        ImGui::SliderFloat("Camera Rotation (Yaw)", &cameraYaw, -180.0f, 180.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Rotate the camera around the Y-axis (horizontal rotation, -180 to 180).\nAdjusts the left-right viewing angle.");
        }
        ImGui::SliderFloat("Camera Tilt (Pitch)", &cameraPitch, -80.0f, 80.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Tilt the camera up or down (vertical rotation, -80 to 80).\nNegative values look downward; positive values look upward.");
        }

        ImGui::PopItemWidth();

        ImGui::Text("Click on the screen to fire the selected projectile at that position.");
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void Renderer::resetCameraControls() {
    cameraZoom = 1713.225f;
    cameraYaw = 0.0f;
    cameraPitch = 11.690f;
    cameraPos = glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 800.0f, 800.0f);
    cameraTarget = glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);
    float camX = cameraTarget.x + cameraZoom * cos(pitchRad) * sin(yawRad);
    float camY = cameraTarget.y + cameraZoom * sin(pitchRad);
    float camZ = cameraTarget.z + cameraZoom * cos(pitchRad) * cos(yawRad);
    cameraPos = glm::vec3(camX, camY, camZ);

    view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "resetCameraControls",
        "Camera controls reset: Zoom=" + std::to_string(cameraZoom) +
        ", Yaw=" + std::to_string(cameraYaw) +
        ", Pitch=" + std::to_string(cameraPitch));
}

bool Renderer::initializeTerrainShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 Normal;
        out vec3 FragPos;
        out vec3 Color;
        out float ZCoord;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            Color = aColor;
            ZCoord = aPos.z;
        }
    )";
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 Normal;
        in vec3 FragPos;
        in vec3 Color;
        in float ZCoord;
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform float depthFade;
        uniform float terrainDepth;
        void main() {
            float ambientStrength = 0.5;
            vec3 ambient = ambientStrength * lightColor * Color;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor * Color;

            vec3 result = (ambient + diffuse) * Color;

            if (depthFade > 0.0) {
                float zNormalized = (ZCoord + terrainDepth / 2.0) / terrainDepth;
                float fadeFactor = mix(1.0, 1.0 - zNormalized, depthFade);
                result *= fadeFactor;
            }

            FragColor = vec4(result, 1.0);
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
        DataManager::LogError("Renderer", "initializeTerrainShader", "Vertex shader compilation failed: " + std::string(infoLog));
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        DataManager::LogError("Renderer", "initializeTerrainShader", "Fragment shader compilation failed: " + std::string(infoLog));
        return false;
    }

    terrainShader = glCreateProgram();
    glAttachShader(terrainShader, vertexShader);
    glAttachShader(terrainShader, fragmentShader);
    glLinkProgram(terrainShader);
    glGetProgramiv(terrainShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(terrainShader, 512, nullptr, infoLog);
        DataManager::LogError("Renderer", "initializeTerrainShader", "Shader program linking failed: " + std::string(infoLog));
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void Renderer::cleanupOpenGLResources() {
    if (terrainShader) glDeleteProgram(terrainShader);
    if (textShader) glDeleteProgram(textShader);
    if (textVAO) glDeleteVertexArrays(1, &textVAO);
    if (textVBO) glDeleteBuffers(1, &textVBO);
}

void Renderer::cleanupSmokeResources() {
    if (smokeShader) glDeleteProgram(smokeShader);
    if (smokeVAO) glDeleteVertexArrays(1, &smokeVAO);
    if (smokeVBO) glDeleteBuffers(1, &smokeVBO);
    if (smokeEBO) glDeleteBuffers(1, &smokeEBO);
    if (smokeTexture) glDeleteTextures(1, &smokeTexture);
}

void Renderer::cleanupSkyResources() {
    sky->cleanup();
}

bool Renderer::initialize(World* w) {
    world = w;

    sky = std::make_unique<Sky>();
    if (!sky->initialize()) {
        DataManager::LogError("Renderer", "initialize", "Sky initialization failed");
        return false;
    }

    celestialObjectManager = world->getCelestialObjectManager();

    std::string fontPath = SDL_GetBasePath() ? std::string(SDL_GetBasePath()) + "resources/fonts/arial.ttf" : "resources/fonts/arial.ttf";
    std::string klingonFontPath = SDL_GetBasePath() ? std::string(SDL_GetBasePath()) + "resources/fonts/klingon_piqad.ttf" : "resources/fonts/klingon_piqad.ttf";

    font = TTF_OpenFont(fontPath.c_str(), 24);
    if (!font) {
        DataManager::LogError("Renderer", "initialize", "Failed to load font at " + fontPath + ": " + std::string(SDL_GetError()));
        return false;
    }

    klingonFont = TTF_OpenFont(klingonFontPath.c_str(), 24);
    if (!klingonFont) {
        DataManager::LogError("Renderer", "initialize", "Failed to load Klingon font at " + klingonFontPath + ": " + std::string(SDL_GetError()));
        TTF_CloseFont(font);
        return false;
    }

    useKlingonFont = (world->getScene() == Scene::ALIEN);
    useKlingonNames = useKlingonFont;

    if (!initializeTextRendering()) {
        TTF_CloseFont(font);
        TTF_CloseFont(klingonFont);
        return false;
    }
    if (!initializeTerrainShader()) {
        TTF_CloseFont(font);
        TTF_CloseFont(klingonFont);
        return false;
    }

    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 2000.0f);
    resetCameraControls();

    return true;
}

void Renderer::reinitializeCelestialResources() {
    celestialObjectManager->initialize();
}

void Renderer::render(float dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);
    float camX = cameraTarget.x + cameraZoom * cos(pitchRad) * sin(yawRad);
    float camY = cameraTarget.y + cameraZoom * sin(pitchRad);
    float camZ = cameraTarget.z + cameraZoom * cos(pitchRad) * cos(yawRad);
    cameraPos = glm::vec3(camX, camY, camZ);
    view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    for (int stage = static_cast<int>(RenderStage::SKY); stage < static_cast<int>(RenderStage::COUNT); ++stage) {
        switch (static_cast<RenderStage>(stage)) {
            case RenderStage::SKY:
                sky->render(world->getCurrentTimeOfDay(), world->getTransitionProgress());
                break;
            case RenderStage::DISTANT_CELESTIALS:
                renderDistantCelestials();
                break;
            case RenderStage::CELESTIAL_TEXT:
                renderCelestialText();
                break;
            case RenderStage::CLOUDS:
                sky->renderClouds(world->getCurrentTimeOfDay(), world->getTransitionProgress(), world->getTotalTime());
                break;
            case RenderStage::DISTANT_TERRAIN:
                renderDistantTerrain();
                break;
            case RenderStage::BOTTOM_TERRAIN:
                renderBottomTerrain();
                break;
            case RenderStage::CLOSE_CELESTIALS:
                renderCloseCelestials();
                break;
            case RenderStage::HOTKEYS_TEXT:
                renderHotKeys();
                break;
            case RenderStage::IMGUI:
                displayTest_GUI();
                break;
            default:
                break;
        }
    }
}

void Renderer::renderHotKeys() {
    glDisable(GL_DEPTH_TEST);

    glm::vec3 enabledColor(0.0f, 1.0f, 0.0f);
    glm::vec3 disabledColor(1.0f, 1.0f, 1.0f);

    float baseYPos = 30.0f;
    float scale = 0.5f;
    float spacing = 20.0f;
    float rowHeight = 20.0f;
    float maxWidth = WINDOW_WIDTH * 0.5f;

    struct HotkeyEntry {
        std::string text;
        bool enabled;
        float width;
    };
    std::vector<HotkeyEntry> hotkeys = {
        {"D: REGENERATE DISTANT TERRAIN", regenerateDistantTriggered, 0.0f},
        {"B: REGENERATE BOTTOM TERRAIN", regenerationTriggered, 0.0f},
        {"F1: CONSTELLATIONS", celestialObjectManager->getShowConstellationNames(), 0.0f},
        {"F2: PLANETS", celestialObjectManager->getShowPlanetNames(), 0.0f},
        {"F3: SATELLITES", celestialObjectManager->getShowSatelliteNames(), 0.0f},
        {"F4: DAWN", world->getCurrentTimeOfDay() == TimeOfDay::DAWN, 0.0f},
        {"F5: MID DAY", world->getCurrentTimeOfDay() == TimeOfDay::MID_DAY, 0.0f},
        {"F6: DUSK", world->getCurrentTimeOfDay() == TimeOfDay::DUSK, 0.0f},
        {"F7: NIGHT", world->getCurrentTimeOfDay() == TimeOfDay::NIGHT, 0.0f},
        {"F8: FALL", world->getScene() == Scene::FALL, 0.0f},
        {"F9: SPRING", world->getScene() == Scene::SPRING, 0.0f},
        {"F10: SUMMER", world->getScene() == Scene::SUMMER, 0.0f},
        {"F11: WINTER", world->getScene() == Scene::WINTER, 0.0f},
        {"F12: TOGGLE KLINGON / ENGLISH NAMES", useKlingonNames, 0.0f},
        {"A: ALIEN PLANET", world->getScene() == Scene::ALIEN, 0.0f}
    };

    for (auto& hotkey : hotkeys) {
        TTF_Font* activeFont = useKlingonNames ? klingonFont : font;
        int w;
        size_t measured_length;
        if (TTF_MeasureString(activeFont, hotkey.text.c_str(), hotkey.text.length(), 0, &w, &measured_length) == true) {
            hotkey.width = static_cast<float>(w) * scale;
        } else {
            hotkey.width = 0.0f;
            DataManager::LogError("Renderer", "renderHotKeys", "Failed to calculate text width for: " + hotkey.text + " - SDL Error:" + SDL_GetError());
        }
        hotkey.width = std::max(hotkey.width, 10.0f);
    }

    std::vector<std::vector<HotkeyEntry>> rows;
    std::vector<HotkeyEntry> currentRow;
    float currentRowWidth = 0.0f;

    for (const auto& hotkey : hotkeys) {
        float entryWidth = hotkey.width + (currentRow.empty() ? 0.0f : spacing);
        if (currentRowWidth + entryWidth > maxWidth && !currentRow.empty()) {
            rows.push_back(currentRow);
            currentRow.clear();
            currentRowWidth = 0.0f;
        }
        currentRow.push_back(hotkey);
        currentRowWidth += entryWidth;
    }
    if (!currentRow.empty()) {
        rows.push_back(currentRow);
    }

    for (size_t rowIdx = 0; rowIdx < rows.size(); ++rowIdx) {
        const auto& row = rows[rowIdx];
        float rowWidth = 0.0f;
        for (size_t i = 0; i < row.size(); ++i) {
            rowWidth += row[i].width + spacing;
        }
        if (!row.empty()) {
            rowWidth -= spacing;
        }
        float xPos = (WINDOW_WIDTH - rowWidth) / 2.0f;
        float yPos = baseYPos + static_cast<float>(rowIdx) * rowHeight;

        for (const auto& hotkey : row) {
            renderText(hotkey.text, xPos, yPos, scale,
                hotkey.enabled ? enabledColor : disabledColor);
            xPos += hotkey.width + spacing;
        }
    }

    glEnable(GL_DEPTH_TEST);
}

void Renderer::renderCloseCelestials() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    celestialObjectManager->renderCloseCelestials(world->getTransitionProgress(), sky->getSunMoonPosition());

    glEnable(GL_DEPTH_TEST);
}

void Renderer::renderDistantCelestials() {
    static int distantFrameCounter = 0;
    distantFrameCounter++;
    float starAlpha = 0.0f;
    if (world->getCurrentTimeOfDay() == TimeOfDay::NIGHT) {
        starAlpha = world->isImmediateFadeFromNight() ? 0.0f : world->getTransitionProgress();
        starAlpha = std::max(starAlpha, 0.5f);
    }
    if (starAlpha > 0.0f) {
        glDepthRange(0.9f, 1.0f);
        if (distantFrameCounter % 60 == 0) {
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "render",
                "Rendering distant celestial objects with starAlpha=" + std::to_string(starAlpha));
        }
        celestialObjectManager->render(starAlpha, sky->getSunMoonPosition());
        glDepthRange(0.0f, 1.0f);
    } else {
        if (distantFrameCounter % 60 == 0) {
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "render",
                "Skipping celestial objects rendering (starAlpha <= 0)");
        }
    }
    if (distantFrameCounter % 60 == 0) {
        distantFrameCounter = 0;
    }
}

void Renderer::renderBottomTerrain() {
    glDepthFunc(GL_LESS);
    glUseProgram(terrainShader);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(terrainShader, "lightPos"), 1, &glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 800.0f, 800.0f)[0]);
    glUniform3fv(glGetUniformLocation(terrainShader, "viewPos"), 1, &cameraPos[0]);
    glUniform3fv(glGetUniformLocation(terrainShader, "lightColor"), 1, &world->getLightColor()[0]);

    glDepthRange(0.0f, 0.25f);
    float extraWidth = 400.0f;
    float scaleX = (WINDOW_WIDTH + extraWidth) / WINDOW_WIDTH;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-WINDOW_WIDTH / 2.0f - extraWidth / 2.0f, 0.0f, -200.0f));
    model = glm::scale(model, glm::vec3(scaleX, 1.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform1f(glGetUniformLocation(terrainShader, "depthFade"), 0.0f);
    glUniform1f(glGetUniformLocation(terrainShader, "terrainDepth"), 1.0f);
    glUniform1f(glGetUniformLocation(terrainShader, "colorFade"), 0.0f);
    world->getBottomTerrain()->render(terrainShader);
    glDepthRange(0.0f, 1.0f);
}

void Renderer::renderDistantTerrain() {
    glDepthFunc(GL_LESS);

    glUseProgram(terrainShader);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(terrainShader, "lightPos"), 1, &glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 800.0f, 800.0f)[0]);
    glUniform3fv(glGetUniformLocation(terrainShader, "viewPos"), 1, &cameraPos[0]);
    glUniform3fv(glGetUniformLocation(terrainShader, "lightColor"), 1, &world->getLightColor()[0]);

    const auto& params = world->getDistantParams();

    glDepthRange(0.5f, 0.75f);
    glm::mat4 distantModel = glm::translate(glm::mat4(1.0f), glm::vec3(-WINDOW_WIDTH / 2.0f, params.yOffset, params.zPosition));
    distantModel = glm::scale(distantModel, glm::vec3(2.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, GL_FALSE, &distantModel[0][0]);
    glUniform1f(glGetUniformLocation(terrainShader, "depthFade"), params.depthFade);
    glUniform1f(glGetUniformLocation(terrainShader, "terrainDepth"), static_cast<float>(world->getDistantTerrain()->getDepth() * 5.0f));
    glUniform1f(glGetUniformLocation(terrainShader, "colorFade"), params.colorFade);

    world->getDistantTerrain()->render(terrainShader);
}

void Renderer::renderCelestialText() {
    static int textFrameCounter = 0;
    textFrameCounter++;
    float starAlpha = 0.0f;
    if (world->getCurrentTimeOfDay() == TimeOfDay::NIGHT) {
        starAlpha = world->isImmediateFadeFromNight() ? 0.0f : world->getTransitionProgress();
        starAlpha = std::max(starAlpha, 0.5f);
    }
    if (starAlpha > 0.0f) {
        if (textFrameCounter % 60 == 0) {
            DataManager::LogDebug(DebugCategory::RENDERING, "Renderer", "render",
                "Rendering celestial text with starAlpha=" + std::to_string(starAlpha));
        }
        celestialObjectManager->renderText(this, starAlpha);
    }
    if (textFrameCounter % 60 == 0) {
        textFrameCounter = 0;
    }
}
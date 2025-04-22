#include "Sky.hpp"
#include "Constants.hpp"
#include "DataManager.hpp"

Sky::Sky() : skyShader(0), skyVAO(0), skyVBO(0), skyEBO(0),
cloudShader(0), cloudVAO(0), cloudVBO(0), cloudEBO(0),
immediateFadeFromNight(false) {
    skyColorTop = glm::vec3(0.2f, 0.4f, 0.8f);
    skyColorBottom = glm::vec3(0.5f, 0.7f, 1.0f);
    skyTransitionTime = 0.0f;
    skyTransitionDuration = 1.0f;
    skyTransitioning = false;
    currentTimeOfDay = TimeOfDay::MID_DAY;
    sunMoonPosition = 0.5f;
    transitionProgress = 0.0f;
    aspectRatio = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
}

Sky::~Sky() {
    cleanup();
}

bool Sky::initialize() {
    if (!initializeSky()) return false;
    if (!initializeClouds()) return false;
    return true;
}

bool Sky::initializeSky() {
    float skyVertices[] = {
        // Positions
        -1.0f, -1.0f, 0.0f,  // Bottom-left
         1.0f, -1.0f, 0.0f,  // Bottom-right
         1.0f,  1.0f, 0.0f,  // Top-right
        -1.0f,  1.0f, 0.0f   // Top-left
    };

    unsigned int skyIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glGenBuffers(1, &skyEBO);

    glBindVertexArray(skyVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVertices), skyVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyIndices), skyIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            TexCoord = (aPos.xy + 1.0) / 2.0; // Map to [0, 1]
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform vec3 topColor;
        uniform vec3 bottomColor;
        uniform float sunMoonPosition;
        uniform int sourceTimeOfDay;
        uniform int targetTimeOfDay;
        uniform float transitionProgress;
        uniform float aspectRatio;

        float random(vec2 st) {
            return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
        }

        void main() {
            float t = pow(TexCoord.y, 2.2);
            vec3 skyColor = mix(bottomColor, topColor, t);

            float noise = random(TexCoord * 1000.0);
            float ditherAmount = 0.015;
            if (sourceTimeOfDay == 3 || targetTimeOfDay == 3) {
                ditherAmount = 0.005;
            }
            skyColor += (noise - 0.5) * ditherAmount;

            skyColor = clamp(skyColor, 0.0, 1.0);
            FragColor = vec4(skyColor, 1.0);
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
        DataManager::LogError("Sky", "initializeSky", "Sky vertex shader compilation failed: " + std::string(infoLog));
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        DataManager::LogError("Sky", "initializeSky", "Sky fragment shader compilation failed: " + std::string(infoLog));
        return false;
    }

    skyShader = glCreateProgram();
    glAttachShader(skyShader, vertexShader);
    glAttachShader(skyShader, fragmentShader);
    glLinkProgram(skyShader);
    glGetProgramiv(skyShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(skyShader, 512, nullptr, infoLog);
        DataManager::LogError("Sky", "initializeSky", "Sky shader program linking failed: " + std::string(infoLog));
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

void Sky::renderClouds(TimeOfDay timeOfDay, float transitionProgress, float totalTime) {
    float density;
    glm::vec3 cloudColor;
    switch (timeOfDay) {
        case TimeOfDay::DAWN:
            density = 0.4f; // Slightly denser clouds for dawn
            cloudColor = glm::vec3(1.0f, 0.9f, 0.8f);
            break;
        case TimeOfDay::MID_DAY:
            density = 0.6f; // More clouds during mid-day
            cloudColor = glm::vec3(1.0f);
            break;
        case TimeOfDay::DUSK:
            density = 0.5f; // Moderate clouds at dusk
            cloudColor = glm::vec3(0.9f, 0.7f, 0.6f);
            break;
        case TimeOfDay::NIGHT:
            density = 0.1f; // Sparse clouds at night
            cloudColor = glm::vec3(0.5f, 0.5f, 0.6f);
            break;
        default:
            density = 0.6f;
            cloudColor = glm::vec3(1.0f);
            break;
    }

    glDepthMask(GL_FALSE);
    glUseProgram(cloudShader);
    glUniform1f(glGetUniformLocation(cloudShader, "time"), totalTime);
    glUniform1i(glGetUniformLocation(cloudShader, "sourceTimeOfDay"), static_cast<int>(timeOfDay));
    glUniform1i(glGetUniformLocation(cloudShader, "targetTimeOfDay"), static_cast<int>(targetTimeOfDay));
    glUniform1f(glGetUniformLocation(cloudShader, "transitionProgress"), transitionProgress);
    glUniform1f(glGetUniformLocation(cloudShader, "aspectRatio"), aspectRatio);
    glBindVertexArray(cloudVAO);
    glDisable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

bool Sky::initializeClouds() {
    // Define vertices for a full-screen quad (same as sky)
    float cloudVertices[] = {
        // Positions
        -1.0f, -1.0f, 0.0f,  // Bottom-left
         1.0f, -1.0f, 0.0f,  // Bottom-right
         1.0f,  1.0f, 0.0f,  // Top-right
        -1.0f,  1.0f, 0.0f   // Top-left
    };

    unsigned int cloudIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &cloudVAO);
    glGenBuffers(1, &cloudVBO);
    glGenBuffers(1, &cloudEBO);

    glBindVertexArray(cloudVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cloudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloudEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cloudIndices), cloudIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            TexCoord = (aPos.xy + 1.0) / 2.0; // Map to [0, 1]
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform float time;
        uniform int sourceTimeOfDay;
        uniform int targetTimeOfDay;
        uniform float transitionProgress;
        uniform float aspectRatio;

        float random(vec2 st) {
            return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
        }

        float noise(vec2 st) {
            vec2 i = floor(st);
            vec2 f = fract(st);
            vec2 u = f * f * (3.0 - 2.0 * f);
            return mix(mix(random(i + vec2(0.0, 0.0)), random(i + vec2(1.0, 0.0)), u.x),
                       mix(random(i + vec2(0.0, 1.0)), random(i + vec2(1.0, 1.0)), u.x), u.y);
        }

        float fbm(vec2 st) {
            float value = 0.0;
            float amplitude = 0.5;
            float frequency = 1.0;
            const int octaves = 6; // Reduced octaves for smoother clouds
            for (int i = 0; i < octaves; i++) {
                value += amplitude * noise(st * frequency);
                st *= 1.8; // Reduced lacunarity for smoother transitions
                amplitude *= 0.5;
            }
            return value;
        }

        void main() {
            // Slight reduction in cloud density at the top
            float topFade = 1.0 - smoothstep(0.8, 1.0, TexCoord.y); // Fade out slightly at the very top

            // Layer 1: Base layer of clouds (slower, larger)
            vec2 cloudPos1 = TexCoord * 4.0; // Increased scale for smaller, more distinct clouds
            cloudPos1.x *= aspectRatio;
            float cloudNoise1 = fbm(cloudPos1 + vec2(time * 0.03, 0.0)); // Slower movement
            float cloudAmount1 = smoothstep(0.5, 0.8, cloudNoise1) * 0.4; // Adjusted thresholds to make sparser, reduced opacity

            // Layer 2: Detail layer of clouds (faster, smaller)
            vec2 cloudPos2 = TexCoord * 8.0; // Increased scale for smaller, more detailed clouds
            cloudPos2.x *= aspectRatio;
            float cloudNoise2 = fbm(cloudPos2 + vec2(time * 0.06, 0.0)); // Faster movement
            float cloudAmount2 = smoothstep(0.6, 0.9, cloudNoise2) * 0.25; // Adjusted thresholds to make sparser, reduced opacity

            // Combine layers
            float cloudAmount = max(cloudAmount1, cloudAmount2) * topFade;

            // Adjust cloud visibility and color based on time of day
            vec3 cloudColor = vec3(0.8, 0.8, 0.9); // Default cloud color
            if (sourceTimeOfDay == 0 || targetTimeOfDay == 0) cloudColor = vec3(0.9, 0.7, 0.6); // Dawn
            if (sourceTimeOfDay == 2 || targetTimeOfDay == 2) cloudColor = vec3(0.9, 0.6, 0.5); // Dusk
            if (sourceTimeOfDay == 3 || targetTimeOfDay == 3) cloudAmount *= 0.2; // Night

            // Output the cloud color with transparency
            FragColor = vec4(cloudColor, cloudAmount);
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
        DataManager::LogError("Sky", "initializeClouds", "Cloud vertex shader compilation failed: " + std::string(infoLog));
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        DataManager::LogError("Sky", "initializeClouds", "Cloud fragment shader compilation failed: " + std::string(infoLog));
        return false;
    }

    cloudShader = glCreateProgram();
    glAttachShader(cloudShader, vertexShader);
    glAttachShader(cloudShader, fragmentShader);
    glLinkProgram(cloudShader);
    glGetProgramiv(cloudShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(cloudShader, 512, nullptr, infoLog);
        DataManager::LogError("Sky", "initializeClouds", "Cloud shader program linking failed: " + std::string(infoLog));
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void Sky::render(TimeOfDay timeOfDay, float transitionProgress) {
    if (timeOfDay != currentTimeOfDay) {
        skyTransitioning = true;
        skyTransitionTime = 0.0f;
        targetTimeOfDay = timeOfDay;

        if (currentTimeOfDay == TimeOfDay::NIGHT && timeOfDay != TimeOfDay::NIGHT) {
            immediateFadeFromNight = true;
            transitionProgress = 0.0f;
        } else if (timeOfDay == TimeOfDay::NIGHT) {
            immediateFadeFromNight = false;
            transitionProgress = 0.0f;
        } else {
            immediateFadeFromNight = false;
        }

        switch (timeOfDay) {
            case TimeOfDay::DAWN:
                targetSkyColorTop = glm::vec3(0.8f, 0.5f, 0.4f);
                targetSkyColorBottom = glm::vec3(1.0f, 0.8f, 0.7f);
                targetSunMoonPosition = 0.1f;
                break;
            case TimeOfDay::MID_DAY:
                targetSkyColorTop = glm::vec3(0.2f, 0.4f, 0.8f);
                targetSkyColorBottom = glm::vec3(0.5f, 0.7f, 1.0f);
                targetSunMoonPosition = 0.5f;
                break;
            case TimeOfDay::DUSK:
                targetSkyColorTop = glm::vec3(0.7f, 0.4f, 0.3f);
                targetSkyColorBottom = glm::vec3(0.9f, 0.6f, 0.5f);
                targetSunMoonPosition = 0.1f;
                break;
            case TimeOfDay::NIGHT:
                targetSkyColorTop = glm::vec3(0.0f, 0.0f, 0.1f);
                targetSkyColorBottom = glm::vec3(0.0f, 0.0f, 0.2f);
                targetSunMoonPosition = 0.5f;
                break;
        }
    }

    if (skyTransitioning) {
        skyTransitionTime += 0.016f;
        float t = std::min(skyTransitionTime / skyTransitionDuration, 1.0f);
        skyColorTop = glm::mix(skyColorTop, targetSkyColorTop, t);
        skyColorBottom = glm::mix(skyColorBottom, targetSkyColorBottom, t);
        sunMoonPosition = glm::mix(sunMoonPosition, targetSunMoonPosition, t);
        transitionProgress = t;

        if (t >= 1.0f) {
            skyTransitioning = false;
            skyTransitionTime = 0.0f;
            currentTimeOfDay = targetTimeOfDay;
            transitionProgress = 1.0f;
        }
    }

    glDepthMask(GL_FALSE);
    glUseProgram(skyShader);
    glUniform3fv(glGetUniformLocation(skyShader, "topColor"), 1, &skyColorTop[0]);
    glUniform3fv(glGetUniformLocation(skyShader, "bottomColor"), 1, &skyColorBottom[0]);
    glUniform1f(glGetUniformLocation(skyShader, "sunMoonPosition"), sunMoonPosition);
    glUniform1i(glGetUniformLocation(skyShader, "sourceTimeOfDay"), static_cast<int>(currentTimeOfDay));
    glUniform1i(glGetUniformLocation(skyShader, "targetTimeOfDay"), static_cast<int>(targetTimeOfDay));
    glUniform1f(glGetUniformLocation(skyShader, "transitionProgress"), transitionProgress);
    glUniform1f(glGetUniformLocation(skyShader, "aspectRatio"), aspectRatio);
    glBindVertexArray(skyVAO);
    glDisable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

void Sky::cleanup() {
    if (skyShader) glDeleteProgram(skyShader);
    if (skyVAO) glDeleteVertexArrays(1, &skyVAO);
    if (skyVBO) glDeleteBuffers(1, &skyVBO);
    if (skyEBO) glDeleteBuffers(1, &skyEBO);
    if (cloudShader) glDeleteProgram(cloudShader);
    if (cloudVAO) glDeleteVertexArrays(1, &cloudVAO);
    if (cloudVBO) glDeleteBuffers(1, &cloudVBO);
    if (cloudEBO) glDeleteBuffers(1, &cloudEBO);
}
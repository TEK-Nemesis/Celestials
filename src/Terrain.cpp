#include "Terrain.hpp"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <DataManager.hpp>

Terrain::Terrain(int width, int depth, const glm::vec4& color)
    : width(width), depth(depth), color(color), vao(0), vbo(0), ebo(0),
    lowColor(0.0f), highColor(0.0f) { // Initialize new members
    heights.resize(width * depth, 0.0f);
}

Terrain::~Terrain() {
    cleanup();
}

void Terrain::generate(noise::module::Perlin& perlin, float baseHeight, float minHeight, float maxHeight, const glm::vec3& lowColor, const glm::vec3& highColor, const std::vector<float>* heightmap) {
    this->lowColor = lowColor;
    this->highColor = highColor;

    // Generate heights with finer noise sampling
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            float noiseValue = static_cast<float>(perlin.GetValue(
                static_cast<double>(x) * 0.015,
                0.0,
                static_cast<double>(z) * 0.015
            ));
            // Normalize noiseValue from [-1, 1] to [0, 1]
            float normalizedNoise = (noiseValue + 1.0f) / 2.0f;
            // Map the normalized noise to the range [minHeight, maxHeight]
            float heightOffset = minHeight + normalizedNoise * (maxHeight - minHeight);
            heights[x + z * width] = baseHeight + heightOffset;
        }
    }

    vertices.clear();
    normals.clear();
    colors.clear();
    indices.clear();
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            float y = heights[x + z * width];
            vertices.push_back(static_cast<float>(x) * 2.0f); // Scale X
            vertices.push_back(y);
            vertices.push_back(static_cast<float>(z) * 5.0f); // Scale Z for visible depth

            // Compute color based on height using the passed terrain colors
            float heightRange = maxHeight - minHeight;
            float normalizedHeight = (y - (baseHeight + minHeight)) / heightRange; // 0.0 at min, 1.0 at max
            glm::vec3 vertexColor = glm::mix(lowColor, highColor, normalizedHeight);
            colors.push_back(vertexColor.r);
            colors.push_back(vertexColor.g);
            colors.push_back(vertexColor.b);
        }
    }

    // Generate indices for a triangle mesh
    for (int z = 0; z < depth - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = x + z * width;
            int topRight = (x + 1) + z * width;
            int bottomLeft = x + (z + 1) * width;
            int bottomRight = (x + 1) + (z + 1) * width;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    computeNormals();
    setupMesh();
}

void Terrain::render(GLuint shader) {
    glUseProgram(shader);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

std::vector<float> Terrain::getHeightmap(int resolution) const {
    // Ensure resolution is at least 1 to avoid division by zero
    if (resolution <= 0) {
        return std::vector<float>(1, FLT_MAX);
    }

    std::vector<float> result(resolution, FLT_MAX);
    float step = static_cast<float>(width - 1) / (resolution - 1);

    for (int i = 0; i < resolution; ++i) {
        int x = static_cast<int>(i * step);
        if (x >= width) x = width - 1;
        int index = x;
        float minHeight = FLT_MAX;
        for (int z = 0; z < depth; ++z) {
            // Bounds check for the index into heights
            size_t heightIndex = z * width + x;
            if (heightIndex >= heights.size()) {
                continue;  // Skip invalid indices
            }
            float h = heights[heightIndex];
            if (h < minHeight) minHeight = h;
        }
        result[i] = minHeight;
    }

    return result;
}

void Terrain::deform(float x, float radius, float intensity, bool addTerrain) {
    x /= 2.0f;
    int xCenter = static_cast<int>(x);
    int r = static_cast<int>(radius);
    float intensityFactor = addTerrain ? intensity : -intensity;

    for (int z = 0; z < depth; ++z) {
        for (int i = xCenter - r; i <= xCenter + r; ++i) {
            if (i < 0 || i >= width) continue;
            float distance = sqrt(static_cast<float>((i - xCenter) * (i - xCenter)));
            if (distance <= radius) {
                float effect = 1.0f - (distance / radius);
                heights[z * width + i] += intensityFactor * effect;
                if (heights[z * width + i] < 0) heights[z * width + i] = 0;
            }
        }
    }

    // Update vertices
    int vertexIndex = 0;
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            int idx = x + z * width;
            if (heights[idx] == FLT_MAX) continue;
            int vertexIdx = vertexIndex * 3;
            if (vertexIdx + 1 < vertices.size()) {
                vertices[vertexIdx + 1] = heights[idx];
            }
            vertexIndex++;
        }
    }

    computeNormals();

    // Update VBO
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), normals.size() * sizeof(float), normals.data());
    glBufferSubData(GL_ARRAY_BUFFER, (vertices.size() + normals.size()) * sizeof(float), colors.size() * sizeof(float), colors.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Terrain::computeNormals() {
    normals.clear();
    normals.resize(vertices.size(), 0.0f); // Initialize with zeros

    // Compute normals for each triangle and accumulate them at vertices
    for (size_t i = 0; i < indices.size(); i += 3) {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        glm::vec3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
        glm::vec3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
        glm::vec3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Accumulate the normal at each vertex
        for (int j = 0; j < 3; ++j) {
            normals[indices[i + j] * 3] += normal.x;
            normals[indices[i + j] * 3 + 1] += normal.y;
            normals[indices[i + j] * 3 + 2] += normal.z;
        }
    }

    // Normalize the accumulated normals
    for (size_t i = 0; i < normals.size(); i += 3) {
        glm::vec3 normal(normals[i], normals[i + 1], normals[i + 2]);
        normal = glm::normalize(normal);
        normals[i] = normal.x;
        normals[i + 1] = normal.y;
        normals[i + 2] = normal.z;
    }
}

void Terrain::setupMesh() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float) + normals.size() * sizeof(float) + colors.size() * sizeof(float),
        nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), normals.size() * sizeof(float), normals.data());
    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + normals.size() * sizeof(float), colors.size() * sizeof(float), colors.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(vertices.size() * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(vertices.size() * sizeof(float) + normals.size() * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Terrain::cleanup() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
}
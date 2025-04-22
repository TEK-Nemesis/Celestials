#pragma once

#include <vector>
#include <noise/noise.h>
#include <glm/glm.hpp>
#include <gl/glew.h>

class Terrain {
public:
    Terrain(int width, int depth, const glm::vec4& color);
    ~Terrain();

    void generate(noise::module::Perlin& perlin, float baseHeight, float minHeight, float maxHeight, const glm::vec3& lowColor, const glm::vec3& highColor, const std::vector<float>* heightmap);
    void render(GLuint shader);
    std::vector<float> getHeightmap(int resolution) const;
    void deform(float x, float radius, float intensity, bool addTerrain);

    int getWidth() const { return width; }
    int getDepth() const { return depth; }
    const std::vector<float>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }
    glm::vec3& getLowColor() { return lowColor; }
    glm::vec3& getHighColor() { return highColor; }
    // Add setter for color
    void setColor(const glm::vec4& newColor) { color = newColor; }

private:
    int width, depth;
    glm::vec4 color;
    std::vector<float> heights;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    std::vector<unsigned int> indices;
    GLuint vao, vbo, ebo;
    glm::vec3 lowColor;
    glm::vec3 highColor;

    void computeNormals();
    void setupMesh();
    void cleanup();
};

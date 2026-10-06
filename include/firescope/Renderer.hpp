#pragma once
#include "SimulationTypes.hpp"
#include "TerrainMesh.hpp"
namespace firescope {
class OpenGLRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer();
    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
    void upload(const RenderState&);
    void setTerrain(const SimulationConfig&, const InitialGrid&, float cellSizeMeters);
    const TerrainMesh& terrain() const { return terrain_; }
    void draw(int x, int y, int width, int height, const Camera&, bool gridLines,
              const std::optional<TerrainHit>& hover, int radius, int brushTool, const EnvironmentParams&, bool vegetation, bool particles);
private:
    unsigned program_ = 0, vao_ = 0, vbo_ = 0, ebo_ = 0, texture_ = 0;
    uint32_t width_ = 0, height_ = 0;
    TerrainMesh terrain_;
    struct Instance { glm::vec4 positionSize, detail; };
    struct Smoke { glm::vec3 position, velocity; float age, lifetime, size; };
    unsigned treeProgram_ = 0, particleProgram_ = 0;
    unsigned treeVao_ = 0, treeMesh_ = 0, treeInstances_ = 0;
    unsigned particleVao_ = 0, particleMesh_ = 0, particleInstances_ = 0;
    int treeVertexCount_ = 0, treeCount_ = 0;
    std::vector<glm::vec3> cellPositions_;
    std::vector<size_t> burning_;
    std::vector<uint8_t> intensity_;
    std::vector<Smoke> smoke_;
    double lastVisualTime_ = 0;
    float spawnRemainder_ = 0;
    uint32_t emission_ = 0;
    void drawEffects(const Camera&, const glm::mat4&, const EnvironmentParams&, bool, bool);
};
}

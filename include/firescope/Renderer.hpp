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
    void refreshProperties(const SimulationConfig&, const GridData&, bool rebuildForest);
    const TerrainMesh& terrain() const { return terrain_; }
    bool cinematic = true;
    float exposure = 1.15f, bloomStrength = 0.22f, atmosphere = 0.35f;
    void draw(int x, int y, int width, int height, const Camera&, bool gridLines,
              const std::optional<TerrainHit>& hover, int radius, int brushTool, const EnvironmentParams&, bool vegetation, bool particles, bool moistureOverlay);
private:
    unsigned program_ = 0, vao_ = 0, vbo_ = 0, ebo_ = 0, texture_ = 0, propertiesTexture_ = 0;
    uint32_t width_ = 0, height_ = 0;
    TerrainMesh terrain_;
    struct Instance { glm::vec4 positionSize, detail; };
    struct Smoke { glm::vec3 position, velocity; float age, lifetime, size; };
    unsigned treeProgram_ = 0, particleProgram_ = 0, skyProgram_ = 0;
    unsigned shadowFbo_ = 0, shadowTexture_ = 0;
    unsigned sceneFbo_ = 0, sceneTexture_ = 0, sceneDepth_ = 0;
    unsigned bloomFbo_[2]{}, bloomTexture_[2]{}, postProgram_ = 0, bloomProgram_ = 0;
    int targetWidth_ = 0, targetHeight_ = 0;
    void resizeTargets(int, int);
    void composite(int, int, int, int, const Camera&, const glm::mat4&);
    glm::mat4 lightMatrix_{1};
    float sceneScale_ = 128;
    bool moistureOverlay_ = false;
    void drawTrees(const glm::mat4&, const Camera&, bool shadow);
    void lightingUniforms(unsigned, const Camera&, bool shadow);
    unsigned treeVao_ = 0, treeMesh_ = 0, treeInstances_ = 0;
    unsigned particleVao_ = 0, particleMesh_ = 0, particleInstances_ = 0;
    int treeVertexCount_ = 0, treeCount_ = 0;
    std::vector<glm::vec3> cellPositions_;
    std::vector<size_t> burning_;
    std::vector<uint8_t> intensity_;
    std::vector<Smoke> smoke_;
    std::vector<Smoke> embers_;
    double lastVisualTime_ = 0;
    float spawnRemainder_ = 0;
    uint32_t emission_ = 0;
    void drawEffects(const Camera&, const glm::mat4&, const EnvironmentParams&, bool, bool);
};
}

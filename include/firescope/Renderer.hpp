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
              const std::optional<TerrainHit>& hover, int radius, int brushTool);
private:
    unsigned program_ = 0, vao_ = 0, vbo_ = 0, ebo_ = 0, texture_ = 0;
    uint32_t width_ = 0, height_ = 0;
    TerrainMesh terrain_;
};
}

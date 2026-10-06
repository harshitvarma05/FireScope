#pragma once
#include "Camera.hpp"
#include "SimulationTypes.hpp"
#include <optional>
namespace firescope {
struct TerrainVertex { glm::vec3 position, normal; glm::vec2 uv; };
struct TerrainHit { glm::vec3 position; uint32_t x, y; float distance; };
class TerrainMesh {
public:
    void build(const SimulationConfig&, const InitialGrid&, float cellSizeMeters);
    std::optional<TerrainHit> intersect(const Ray&) const;
    const std::vector<TerrainVertex>& vertices() const { return vertices_; }
    const std::vector<uint32_t>& indices() const { return indices_; }
    float centerHeight() const { return (minimumHeight_ + maximumHeight_)*0.5f; }
private:
    SimulationConfig config_{};
    std::vector<TerrainVertex> vertices_;
    std::vector<uint32_t> indices_;
    float minimumHeight_ = 0, maximumHeight_ = 0;
};
}

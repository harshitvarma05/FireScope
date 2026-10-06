#include "firescope/TerrainMesh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace firescope {
void TerrainMesh::build(const SimulationConfig& config, const InitialGrid& grid, float cellSizeMeters) {
    if (!config.width || !config.height || config.width > 2048 || config.height > 2048 ||
        grid.elevation.size() != static_cast<size_t>(config.width)*config.height ||
        !std::isfinite(cellSizeMeters) || cellSizeMeters <= 0)
        throw std::invalid_argument("Invalid terrain dimensions or cell size");
    for (float value : grid.elevation) if (!std::isfinite(value))
        throw std::invalid_argument("Terrain elevations must be finite");
    config_ = config;
    const uint32_t stride = config.width+1;
    vertices_.resize(static_cast<size_t>(stride)*(config.height+1));
    indices_.clear(); indices_.reserve(static_cast<size_t>(config.width)*config.height*6);
    minimumHeight_ = std::numeric_limits<float>::infinity(); maximumHeight_ = -minimumHeight_;
    for (uint32_t y = 0; y <= config.height; ++y) {
        for (uint32_t x = 0; x <= config.width; ++x) {
            float elevation = 0; int count = 0;
            // Simulation elevations are cell centers; share averaged corner vertices.
            for (int dy = -1; dy <= 0; ++dy) for (int dx = -1; dx <= 0; ++dx) {
                int cx = static_cast<int>(x)+dx, cy = static_cast<int>(y)+dy;
                if (cx >= 0 && cy >= 0 && cx < static_cast<int>(config.width) && cy < static_cast<int>(config.height)) {
                    elevation += grid.elevation[static_cast<size_t>(cy)*config.width+cx]; ++count;
                }
            }
            elevation /= count*cellSizeMeters;
            auto& vertex = vertices_[static_cast<size_t>(y)*stride+x];
            vertex.position = {static_cast<float>(x)-config.width*0.5f,elevation,config.height*0.5f-static_cast<float>(y)};
            vertex.uv = {static_cast<float>(x)/config.width,static_cast<float>(y)/config.height};
            minimumHeight_ = std::min(minimumHeight_,elevation); maximumHeight_ = std::max(maximumHeight_,elevation);
        }
    }
    for (uint32_t y = 0; y <= config.height; ++y) for (uint32_t x = 0; x <= config.width; ++x) {
        const auto& left = vertices_[static_cast<size_t>(y)*stride+(x ? x-1 : x)].position;
        const auto& right = vertices_[static_cast<size_t>(y)*stride+std::min(x+1,config.width)].position;
        const auto& south = vertices_[static_cast<size_t>(y ? y-1 : y)*stride+x].position;
        const auto& north = vertices_[static_cast<size_t>(std::min(y+1,config.height))*stride+x].position;
        vertices_[static_cast<size_t>(y)*stride+x].normal = glm::normalize(glm::cross(right-left,north-south));
        if (x < config.width && y < config.height) {
            const uint32_t a = y*stride+x, b = a+1, c = a+stride, d = c+1;
            indices_.insert(indices_.end(),{a,b,c,b,d,c});
        }
    }
}
namespace {
std::optional<float> triangle(const Ray& ray, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 e1 = b-a, e2 = c-a, h = glm::cross(ray.direction,e2);
    const float determinant = glm::dot(e1,h);
    if (std::abs(determinant) < 1e-8f) return std::nullopt;
    const float inverse = 1/determinant;
    const glm::vec3 s = ray.origin-a;
    const float u = inverse*glm::dot(s,h);
    if (u < -1e-5f || u > 1.00001f) return std::nullopt;
    const glm::vec3 q = glm::cross(s,e1);
    const float v = inverse*glm::dot(ray.direction,q);
    if (v < -1e-5f || u+v > 1.00001f) return std::nullopt;
    float t = inverse*glm::dot(e2,q);
    return t >= 0 ? std::optional<float>(t) : std::nullopt;
}
}
std::optional<TerrainHit> TerrainMesh::intersect(const Ray& ray) const {
    if (vertices_.empty()) return std::nullopt;
    for (int axis = 0; axis < 3; ++axis)
        if (!std::isfinite(ray.origin[axis]) || !std::isfinite(ray.direction[axis])) return std::nullopt;
    if (glm::length(ray.direction) < 1e-8f) return std::nullopt;
    const glm::vec3 minimum(-static_cast<float>(config_.width)*0.5f,minimumHeight_-0.001f,-static_cast<float>(config_.height)*0.5f);
    const glm::vec3 maximum(config_.width*0.5f,maximumHeight_+0.001f,config_.height*0.5f);
    float enter = 0, exit = std::numeric_limits<float>::infinity();
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(ray.direction[axis]) < 1e-8f) {
            if (ray.origin[axis] < minimum[axis] || ray.origin[axis] > maximum[axis]) return std::nullopt;
        } else {
            float a = (minimum[axis]-ray.origin[axis])/ray.direction[axis];
            float b = (maximum[axis]-ray.origin[axis])/ray.direction[axis];
            if (a > b) std::swap(a,b);
            enter = std::max(enter,a); exit = std::min(exit,b);
            if (enter > exit) return std::nullopt;
        }
    }
    const glm::vec3 start = ray.origin+ray.direction*enter;
    const float gx = start.x+config_.width*0.5f, gy = config_.height*0.5f-start.z;
    const float dx = ray.direction.x, dy = -ray.direction.z;
    int x = std::clamp(static_cast<int>(std::floor(gx)),0,static_cast<int>(config_.width)-1);
    int y = std::clamp(static_cast<int>(std::floor(gy)),0,static_cast<int>(config_.height)-1);
    const int sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1;
    const float infinity = std::numeric_limits<float>::infinity();
    const float deltaX = std::abs(dx) < 1e-8f ? infinity : 1/std::abs(dx);
    const float deltaY = std::abs(dy) < 1e-8f ? infinity : 1/std::abs(dy);
    float nextX = std::isinf(deltaX) ? infinity : enter+((dx > 0 ? x+1.0f : static_cast<float>(x))-gx)/dx;
    float nextY = std::isinf(deltaY) ? infinity : enter+((dy > 0 ? y+1.0f : static_cast<float>(y))-gy)/dy;
    float cellEnter = enter;
    for (uint32_t visited = 0; visited <= config_.width+config_.height; ++visited) {
        const uint32_t a = y*(config_.width+1)+x, b = a+1, c = a+config_.width+1, d = c+1;
        const float cellExit = std::min(exit,std::min(nextX,nextY));
        auto first = triangle(ray,vertices_[a].position,vertices_[b].position,vertices_[c].position);
        auto second = triangle(ray,vertices_[b].position,vertices_[d].position,vertices_[c].position);
        float nearest = infinity;
        for (auto candidate : {first,second})
            if (candidate && *candidate >= cellEnter-0.001f && *candidate <= cellExit+0.001f)
                nearest = std::min(nearest,*candidate);
        if (std::isfinite(nearest)) {
            const glm::vec3 point = ray.origin+ray.direction*nearest;
            // Shader texture coordinates and brush mapping use the same world-to-cell rule.
            const uint32_t cx = static_cast<uint32_t>(std::clamp(static_cast<int>(std::floor(point.x+config_.width*0.5f)),0,static_cast<int>(config_.width)-1));
            const uint32_t cy = static_cast<uint32_t>(std::clamp(static_cast<int>(std::floor(config_.height*0.5f-point.z)),0,static_cast<int>(config_.height)-1));
            return TerrainHit{point,cx,cy,nearest};
        }
        if (cellExit >= exit) break;
        cellEnter = cellExit;
        if (nextX < nextY) { x += sx; nextX += deltaX; }
        else if (nextY < nextX) { y += sy; nextY += deltaY; }
        else { x += sx; y += sy; nextX += deltaX; nextY += deltaY; }
        if (x < 0 || y < 0 || x >= static_cast<int>(config_.width) || y >= static_cast<int>(config_.height)) break;
    }
    return std::nullopt;
}
}

#pragma once
#include <glm/glm.hpp>
namespace firescope {
enum class CameraMode { Orbit, FreeFly };
struct Ray { glm::vec3 origin, direction; };
class Camera {
public:
    void frame(float width, float height, float centerHeight = 0);
    void setMode(CameraMode);
    CameraMode mode() const { return mode_; }
    void rotate(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float wheel);
    void move(float forward, float right, float up, float seconds);
    glm::vec3 position() const;
    glm::vec3 forward() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    Ray screenRay(float x, float y, float width, float height) const;
private:
    CameraMode mode_ = CameraMode::Orbit;
    glm::vec3 target_{0}, eye_{0};
    float yaw_ = -0.45f, pitch_ = -0.8f, distance_ = 160, scale_ = 128;
};
}

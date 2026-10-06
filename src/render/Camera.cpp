#include "firescope/Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace firescope {
void Camera::frame(float width, float height, float centerHeight) {
    scale_ = std::max(width,height);
    target_ = {0,centerHeight,0}; yaw_ = -0.45f; pitch_ = -0.8f;
    distance_ = scale_ * 1.8f; eye_ = target_ - forward() * distance_;
}
glm::vec3 Camera::forward() const {
    return {std::cos(pitch_) * std::sin(yaw_), std::sin(pitch_), -std::cos(pitch_) * std::cos(yaw_)};
}
glm::vec3 Camera::position() const { return mode_ == CameraMode::Orbit ? target_ - forward()*distance_ : eye_; }
void Camera::setMode(CameraMode mode) {
    if (mode == mode_) return;
    if (mode == CameraMode::FreeFly) eye_ = position();
    else target_ = eye_ + forward()*distance_;
    mode_ = mode;
}
void Camera::rotate(float dx, float dy) {
    yaw_ = std::remainder(yaw_ + dx * 0.005f, 6.283185307f);
    pitch_ = std::clamp(pitch_ - dy * 0.005f, -1.52f, 1.52f);
}
void Camera::pan(float dx, float dy) {
    const glm::vec3 right = glm::normalize(glm::cross(forward(),glm::vec3(0,1,0)));
    const glm::vec3 up = glm::cross(right,forward());
    const glm::vec3 delta = (-right*dx + up*dy) * distance_*0.0015f;
    if (mode_ == CameraMode::Orbit) target_ += delta; else eye_ += delta;
}
void Camera::zoom(float wheel) {
    if (mode_ == CameraMode::Orbit) distance_ = std::clamp(distance_*std::exp(-wheel*0.12f),2.0f,scale_*8.0f);
    else eye_ += forward()*wheel*scale_*0.06f;
}
void Camera::move(float f, float r, float up, float seconds) {
    const glm::vec3 right = glm::normalize(glm::cross(forward(),glm::vec3(0,1,0)));
    glm::vec3 delta = forward()*f + right*r + glm::vec3(0,up,0);
    const float magnitude = glm::length(delta);
    if (magnitude > 0) delta /= magnitude;
    delta *= scale_*0.5f*std::min(seconds,0.1f);
    if (mode_ == CameraMode::FreeFly) eye_ += delta; else target_ += delta;
}
glm::mat4 Camera::view() const { return glm::lookAt(position(),position()+forward(),glm::vec3(0,1,0)); }
glm::mat4 Camera::projection(float aspect) const {
    return glm::perspective(glm::radians(45.0f),aspect,0.1f,std::max(100.0f,scale_*20.0f));
}
Ray Camera::screenRay(float x, float y, float width, float height) const {
    if (width <= 0 || height <= 0) throw std::invalid_argument("Ray viewport must have positive dimensions");
    const glm::vec2 ndc(2*x/width-1,1-2*y/height);
    const auto inverse = glm::inverse(projection(width/height)*view());
    glm::vec4 near = inverse*glm::vec4(ndc,-1,1), far = inverse*glm::vec4(ndc,1,1);
    near /= near.w; far /= far.w;
    return {glm::vec3(near),glm::normalize(glm::vec3(far-near))};
}
}

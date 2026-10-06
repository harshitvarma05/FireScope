#pragma once
#include "SimulationTypes.hpp"
namespace firescope {
class OpenGLRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer();
    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
    void upload(const RenderState&);
    void draw(int x, int y, int width, int height);
private:
    unsigned program_ = 0, vao_ = 0, vbo_ = 0, texture_ = 0;
    uint32_t width_ = 0, height_ = 0;
};
}

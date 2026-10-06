#include "firescope/Renderer.hpp"
#include "firescope/GL.hpp"
#include <string>

namespace firescope {
namespace {
unsigned shader(unsigned type, const char* source) {
    unsigned handle = gl::CreateShader(type);
    gl::ShaderSource(handle, 1, &source, nullptr); gl::CompileShader(handle);
    int ok = 0; gl::GetShaderiv(handle, 0x8B81, &ok);
    if (!ok) {
        char log[2048]{}; gl::GetShaderInfoLog(handle, sizeof(log), nullptr, log);
        gl::DeleteShader(handle); throw std::runtime_error(std::string("Shader compilation failed: ") + log);
    }
    return handle;
}
}
OpenGLRenderer::OpenGLRenderer() {
    const char* vertex = R"(#version 410 core
layout(location=0) in vec2 position;
out vec2 uv;
void main() { uv = position * 0.5 + 0.5; gl_Position = vec4(position,0,1); }
)";
    const char* fragment = R"(#version 410 core
in vec2 uv;
uniform sampler2D grid;
out vec4 color;
void main() {
    vec2 data = texture(grid,uv).rg;
    int state = int(round(data.r * 255.0));
    vec3 c = vec3(0.21,0.19,0.16);
    if (state == 1) c = vec3(0.16,0.42,0.23);
    if (state == 2) c = mix(vec3(0.82,0.15,0.03),vec3(1.0,0.73,0.12),data.g);
    if (state == 3) c = vec3(0.055,0.065,0.07);
    color = vec4(c,1);
}
)";
    auto vs = shader(0x8B31, vertex), fs = shader(0x8B30, fragment);
    program_ = gl::CreateProgram(); gl::AttachShader(program_, vs); gl::AttachShader(program_, fs);
    gl::LinkProgram(program_); gl::DeleteShader(vs); gl::DeleteShader(fs);
    int ok = 0; gl::GetProgramiv(program_, 0x8B82, &ok);
    if (!ok) {
        char log[2048]{}; gl::GetProgramInfoLog(program_, sizeof(log), nullptr, log);
        throw std::runtime_error(std::string("Shader linking failed: ") + log);
    }
    const float vertices[] = {-1,-1, 1,-1, 1,1, -1,-1, 1,1, -1,1};
    gl::GenVertexArrays(1,&vao_); gl::BindVertexArray(vao_);
    gl::GenBuffers(1,&vbo_); gl::BindBuffer(0x8892,vbo_);
    gl::BufferData(0x8892,sizeof(vertices),vertices,0x88E4);
    gl::VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);
    gl::EnableVertexAttribArray(0);
    glGenTextures(1,&texture_); glBindTexture(GL_TEXTURE_2D,texture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
}
OpenGLRenderer::~OpenGLRenderer() {
    glDeleteTextures(1,&texture_); gl::DeleteBuffers(1,&vbo_);
    gl::DeleteVertexArrays(1,&vao_); gl::DeleteProgram(program_);
}
void OpenGLRenderer::upload(const RenderState& state) {
    std::vector<uint8_t> bytes(state.cellState.size() * 2);
    for (size_t i = 0; i < state.cellState.size(); ++i) {
        bytes[i*2] = state.cellState[i]; bytes[i*2+1] = state.fireIntensity[i];
    }
    gl::ActiveTexture(0x84C0); glBindTexture(GL_TEXTURE_2D,texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    if (width_ != state.width || height_ != state.height) {
        glTexImage2D(GL_TEXTURE_2D,0,0x822B,state.width,state.height,0,0x8227,GL_UNSIGNED_BYTE,bytes.data());
        width_ = state.width; height_ = state.height;
    } else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,state.width,state.height,0x8227,GL_UNSIGNED_BYTE,bytes.data());
}
void OpenGLRenderer::draw(int x, int y, int width, int height) {
    glViewport(x,y,width,height); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    gl::UseProgram(program_); gl::ActiveTexture(0x84C0); glBindTexture(GL_TEXTURE_2D,texture_);
    gl::Uniform1i(gl::GetUniformLocation(program_,"grid"),0);
    gl::BindVertexArray(vao_); glDrawArrays(GL_TRIANGLES,0,6);
}
}

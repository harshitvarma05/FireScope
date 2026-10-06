#include "firescope/Renderer.hpp"
#include "firescope/GL.hpp"
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include "ShaderSources.hpp"

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
    const char* vertex = shaders::terrainVertex;
    const char* fragment = shaders::terrainFragment;
    auto vs = shader(0x8B31, vertex), fs = shader(0x8B30, fragment);
    program_ = gl::CreateProgram(); gl::AttachShader(program_, vs); gl::AttachShader(program_, fs);
    gl::LinkProgram(program_); gl::DeleteShader(vs); gl::DeleteShader(fs);
    int ok = 0; gl::GetProgramiv(program_, 0x8B82, &ok);
    if (!ok) {
        char log[2048]{}; gl::GetProgramInfoLog(program_, sizeof(log), nullptr, log);
        throw std::runtime_error(std::string("Shader linking failed: ") + log);
    }
    gl::GenVertexArrays(1,&vao_); gl::BindVertexArray(vao_);
    gl::GenBuffers(1,&vbo_); gl::BindBuffer(0x8892,vbo_);
    gl::GenBuffers(1,&ebo_); gl::BindBuffer(0x8893,ebo_);
    gl::VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),reinterpret_cast<void*>(offsetof(TerrainVertex,position)));
    gl::VertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),reinterpret_cast<void*>(offsetof(TerrainVertex,normal)));
    gl::VertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),reinterpret_cast<void*>(offsetof(TerrainVertex,uv)));
    gl::EnableVertexAttribArray(0); gl::EnableVertexAttribArray(1); gl::EnableVertexAttribArray(2);
    glGenTextures(1,&texture_); glBindTexture(GL_TEXTURE_2D,texture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
}
OpenGLRenderer::~OpenGLRenderer() {
    glDeleteTextures(1,&texture_); gl::DeleteBuffers(1,&vbo_);
    gl::DeleteBuffers(1,&ebo_); gl::DeleteVertexArrays(1,&vao_); gl::DeleteProgram(program_);
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
void OpenGLRenderer::setTerrain(const SimulationConfig& config, const InitialGrid& grid, float cellSizeMeters) {
    terrain_.build(config,grid,cellSizeMeters);
    gl::BindVertexArray(vao_);
    gl::BindBuffer(0x8892,vbo_);
    gl::BufferData(0x8892,terrain_.vertices().size()*sizeof(TerrainVertex),terrain_.vertices().data(),0x88E4);
    gl::BindBuffer(0x8893,ebo_);
    gl::BufferData(0x8893,terrain_.indices().size()*sizeof(uint32_t),terrain_.indices().data(),0x88E4);
}
void OpenGLRenderer::draw(int x, int y, int width, int height, const Camera& camera, bool gridLines,
                          const std::optional<TerrainHit>& hover, int radius, int brushTool) {
    glViewport(x,y,width,height); glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    gl::UseProgram(program_); gl::ActiveTexture(0x84C0); glBindTexture(GL_TEXTURE_2D,texture_);
    gl::Uniform1i(gl::GetUniformLocation(program_,"grid"),0);
    const auto model = glm::mat4(1), view = camera.view(), projection = camera.projection(static_cast<float>(width)/height);
    gl::UniformMatrix4fv(gl::GetUniformLocation(program_,"model"),1,GL_FALSE,glm::value_ptr(model));
    gl::UniformMatrix4fv(gl::GetUniformLocation(program_,"view"),1,GL_FALSE,glm::value_ptr(view));
    gl::UniformMatrix4fv(gl::GetUniformLocation(program_,"projection"),1,GL_FALSE,glm::value_ptr(projection));
    gl::Uniform2f(gl::GetUniformLocation(program_,"gridSize"),static_cast<float>(width_),static_cast<float>(height_));
    gl::Uniform1i(gl::GetUniformLocation(program_,"showGrid"),gridLines ? 1 : 0);
    gl::Uniform1i(gl::GetUniformLocation(program_,"brushVisible"),hover ? 1 : 0);
    gl::Uniform1i(gl::GetUniformLocation(program_,"brushTool"),brushTool);
    if (hover) gl::Uniform3f(gl::GetUniformLocation(program_,"brush"),static_cast<float>(hover->x),static_cast<float>(hover->y),static_cast<float>(radius));
    gl::BindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,static_cast<int>(terrain_.indices().size()),GL_UNSIGNED_INT,nullptr);
}
}

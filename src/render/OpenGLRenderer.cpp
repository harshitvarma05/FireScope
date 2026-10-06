#include "firescope/Renderer.hpp"
#include "firescope/GL.hpp"
#include <string>
#include <algorithm>
#include <cmath>
#include "firescope/DeterministicRNG.hpp"
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
unsigned program(const char* vertex, const char* fragment) {
    unsigned vs = shader(0x8B31,vertex), fs = shader(0x8B30,fragment);
    unsigned result = gl::CreateProgram(); gl::AttachShader(result,vs); gl::AttachShader(result,fs);
    gl::LinkProgram(result); gl::DeleteShader(vs); gl::DeleteShader(fs);
    int ok = 0; gl::GetProgramiv(result,0x8B82,&ok);
    if (!ok) {
        char log[2048]{}; gl::GetProgramInfoLog(result,sizeof(log),nullptr,log);
        gl::DeleteProgram(result); throw std::runtime_error(std::string("Effect shader linking failed: ")+log);
    }
    return result;
}
struct TreeVertex { glm::vec3 position, normal; };
std::vector<TreeVertex> treeMesh() {
    std::vector<TreeVertex> mesh;
    auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        const auto normal = glm::normalize(glm::cross(b-a,c-a));
        for (auto point : {a,b,c}) mesh.push_back({point,normal});
    };
    // Six-sided trunk and two cone canopies; flat face normals keep the low-poly style.
    for (int i = 0; i < 6; ++i) {
        float a = i*6.2831853f/6, b = (i+1)*6.2831853f/6;
        glm::vec3 p{std::cos(a)*0.07f,0,std::sin(a)*0.07f};
        glm::vec3 q{std::cos(b)*0.07f,0,std::sin(b)*0.07f};
        triangle(p,q,p+glm::vec3(0,0.65f,0));
        triangle(q,q+glm::vec3(0,0.65f,0),p+glm::vec3(0,0.65f,0));
        for (auto level : {glm::vec3(0.45f,0.36f,1.25f),glm::vec3(0.8f,0.28f,1.65f)}) {
            triangle({std::cos(a)*level.y,level.x,std::sin(a)*level.y},
                     {std::cos(b)*level.y,level.x,std::sin(b)*level.y},{0,level.z,0});
        }
    }
    return mesh;
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
    treeProgram_ = program(shaders::vegetationVertex,shaders::vegetationFragment);
    particleProgram_ = program(shaders::billboardVertex,shaders::billboardFragment);
    gl::GenVertexArrays(1,&treeVao_); gl::BindVertexArray(treeVao_);
    gl::GenBuffers(1,&treeMesh_); gl::BindBuffer(0x8892,treeMesh_);
    const auto mesh = treeMesh(); treeVertexCount_ = static_cast<int>(mesh.size());
    gl::BufferData(0x8892,mesh.size()*sizeof(TreeVertex),mesh.data(),0x88E4);
    gl::VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TreeVertex),nullptr);
    gl::VertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(TreeVertex),reinterpret_cast<void*>(offsetof(TreeVertex,normal)));
    gl::EnableVertexAttribArray(0); gl::EnableVertexAttribArray(1);
    gl::GenBuffers(1,&treeInstances_); gl::BindBuffer(0x8892,treeInstances_);
    for (unsigned slot = 2; slot <= 3; ++slot) {
        gl::VertexAttribPointer(slot,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>((slot-2)*sizeof(glm::vec4)));
        gl::EnableVertexAttribArray(slot); gl::VertexAttribDivisor(slot,1);
    }
    gl::GenVertexArrays(1,&particleVao_); gl::BindVertexArray(particleVao_);
    gl::GenBuffers(1,&particleMesh_); gl::BindBuffer(0x8892,particleMesh_);
    const float quad[] = {-1,-1, 1,-1, -1,1, -1,1, 1,-1, 1,1};
    gl::BufferData(0x8892,sizeof(quad),quad,0x88E4);
    gl::VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr); gl::EnableVertexAttribArray(0);
    gl::GenBuffers(1,&particleInstances_); gl::BindBuffer(0x8892,particleInstances_);
    for (unsigned slot = 2; slot <= 3; ++slot) {
        gl::VertexAttribPointer(slot,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>((slot-2)*sizeof(glm::vec4)));
        gl::EnableVertexAttribArray(slot); gl::VertexAttribDivisor(slot,1);
    }
    lastVisualTime_ = glfwGetTime();
}
OpenGLRenderer::~OpenGLRenderer() {
    for (auto buffer : {treeMesh_,treeInstances_,particleMesh_,particleInstances_}) gl::DeleteBuffers(1,&buffer);
    gl::DeleteVertexArrays(1,&treeVao_); gl::DeleteVertexArrays(1,&particleVao_);
    gl::DeleteProgram(treeProgram_); gl::DeleteProgram(particleProgram_);
    glDeleteTextures(1,&texture_); gl::DeleteBuffers(1,&vbo_);
    gl::DeleteBuffers(1,&ebo_); gl::DeleteVertexArrays(1,&vao_); gl::DeleteProgram(program_);
}
void OpenGLRenderer::upload(const RenderState& state) {
    burning_.clear(); intensity_ = state.fireIntensity;
    for (size_t i = 0; i < state.cellState.size(); ++i)
        if (state.cellState[i] == static_cast<uint8_t>(CellState::Burning)) burning_.push_back(i);
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
    smoke_.clear(); burning_.clear(); emission_ = 0; spawnRemainder_ = 0;
    lastVisualTime_ = glfwGetTime();
    cellPositions_.resize(static_cast<size_t>(config.width)*config.height);
    std::vector<Instance> trees;
    const uint32_t stride = std::max(1u,static_cast<uint32_t>(std::ceil(std::sqrt(cellPositions_.size()/20000.0))));
    for (uint32_t y = 0; y < config.height; ++y) for (uint32_t x = 0; x < config.width; ++x) {
        const size_t index = static_cast<size_t>(y)*config.width+x;
        const auto& vertices = terrain_.vertices();
        // Center lies on the shared diagonal of the same triangles used by picking.
        const auto center = (vertices[static_cast<size_t>(y)*(config.width+1)+x+1].position+
                             vertices[static_cast<size_t>(y+1)*(config.width+1)+x].position)*0.5f;
        cellPositions_[index] = center;
        if (x%stride || y%stride || grid.state[index] == 0 || grid.density[index] <= 0) continue;
        const uint32_t h = hashCell(71,x,y,0);
        const float fraction = static_cast<float>(h&65535u)/65535.0f;
        if (fraction > std::clamp(grid.density[index],0.0f,1.0f)) continue;
        trees.push_back({glm::vec4(center,0.7f+fraction*0.55f),
            {fraction*6.2831853f,(x+0.5f)/config.width,(y+0.5f)/config.height,0}});
    }
    treeCount_ = static_cast<int>(trees.size());
    gl::BindBuffer(0x8892,treeInstances_);
    gl::BufferData(0x8892,trees.size()*sizeof(Instance),trees.data(),0x88E4);
    gl::BindVertexArray(vao_);
    gl::BindBuffer(0x8892,vbo_);
    gl::BufferData(0x8892,terrain_.vertices().size()*sizeof(TerrainVertex),terrain_.vertices().data(),0x88E4);
    gl::BindBuffer(0x8893,ebo_);
    gl::BufferData(0x8893,terrain_.indices().size()*sizeof(uint32_t),terrain_.indices().data(),0x88E4);
}
void OpenGLRenderer::draw(int x, int y, int width, int height, const Camera& camera, bool gridLines,
                          const std::optional<TerrainHit>& hover, int radius, int brushTool, const EnvironmentParams& params, bool vegetation, bool particles) {
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
    drawEffects(camera,projection*view,params,vegetation,particles);
}
void OpenGLRenderer::drawEffects(const Camera& camera, const glm::mat4& viewProjection,
                                 const EnvironmentParams& params, bool vegetation, bool particles) {
    if (vegetation) {
        gl::UseProgram(treeProgram_);
        gl::UniformMatrix4fv(gl::GetUniformLocation(treeProgram_,"viewProjection"),1,GL_FALSE,glm::value_ptr(viewProjection));
        gl::Uniform1i(gl::GetUniformLocation(treeProgram_,"grid"),0);
        gl::BindVertexArray(treeVao_);
        gl::DrawArraysInstanced(GL_TRIANGLES,0,treeVertexCount_,treeCount_);
    }
    const double now = glfwGetTime();
    const float dt = static_cast<float>(std::clamp(now-lastVisualTime_,0.0,0.1)); lastVisualTime_ = now;
    if (!particles) { smoke_.clear(); spawnRemainder_ = 0; return; }
    for (auto& particle : smoke_) {
        particle.age += dt;
        const float turbulence = std::sin(particle.age*2.5f+particle.lifetime)*0.12f;
        particle.position += (particle.velocity+glm::vec3(turbulence,0,-turbulence))*dt;
    }
    smoke_.erase(std::remove_if(smoke_.begin(),smoke_.end(),[](const Smoke& p) { return p.age >= p.lifetime; }),smoke_.end());
    // Global emission and live-particle limits keep graphics cost independent of grid area.
    if (!burning_.empty()) {
        spawnRemainder_ += dt*std::min(500.0f,static_cast<float>(burning_.size())*3.0f);
        while (spawnRemainder_ >= 1 && smoke_.size() < 4096) {
            spawnRemainder_ -= 1;
            const uint32_t h = mixBits(++emission_);
            const size_t cell = burning_[h%burning_.size()];
            const float r = static_cast<float>(h&65535u)/65535.0f;
            glm::vec3 velocity{std::cos(params.windDirectionRadians)*params.windSpeed/params.cellSizeMeters,
                               0.6f+r*0.5f,-std::sin(params.windDirectionRadians)*params.windSpeed/params.cellSizeMeters};
            smoke_.push_back({cellPositions_[cell]+glm::vec3(0,0.7f,0),velocity,0,3+r*2,0.4f+r*0.3f});
        }
        spawnRemainder_ = std::min(spawnRemainder_,1.0f);
    } else spawnRemainder_ = 0;
    std::vector<Instance> instances; instances.reserve(smoke_.size()+2048);
    const size_t stride = std::max(size_t(1),(burning_.size()+2047)/2048);
    for (size_t i = 0; i < burning_.size(); i += stride) {
        const size_t cell = burning_[i];
        const float phase = static_cast<float>(now*7+cell%31);
        const float intensity = intensity_[cell]/255.0f;
        instances.push_back({glm::vec4(cellPositions_[cell]+glm::vec3(0,0.6f,0),0.35f+0.12f*std::sin(phase)),
                             {0,0,intensity,0.85f}});
    }
    for (const auto& p : smoke_) {
        const float age = p.age/p.lifetime;
        instances.push_back({glm::vec4(p.position,p.size+p.age*0.35f),
                             {1,age,0,std::min(p.age*2,1.0f)*(1-age)*0.38f}});
    }
    const auto eye = camera.position();
    std::sort(instances.begin(),instances.end(),[&](const Instance& a,const Instance& b) {
        auto da = glm::vec3(a.positionSize)-eye, db = glm::vec3(b.positionSize)-eye;
        return glm::dot(da,da) > glm::dot(db,db);
    });
    gl::UseProgram(particleProgram_);
    gl::UniformMatrix4fv(gl::GetUniformLocation(particleProgram_,"viewProjection"),1,GL_FALSE,glm::value_ptr(viewProjection));
    const auto right = glm::normalize(glm::cross(camera.forward(),glm::vec3(0,1,0)));
    const auto up = glm::normalize(glm::cross(right,camera.forward()));
    gl::Uniform3f(gl::GetUniformLocation(particleProgram_,"cameraRight"),right.x,right.y,right.z);
    gl::Uniform3f(gl::GetUniformLocation(particleProgram_,"cameraUp"),up.x,up.y,up.z);
    gl::BindVertexArray(particleVao_); gl::BindBuffer(0x8892,particleInstances_);
    gl::BufferData(0x8892,instances.size()*sizeof(Instance),instances.data(),0x88E8);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    gl::DrawArraysInstanced(GL_TRIANGLES,0,6,static_cast<int>(instances.size()));
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}

}

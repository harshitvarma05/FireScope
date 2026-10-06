#include "firescope/TerrainMesh.hpp"
#include "firescope/Scenario.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace firescope;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool close(float a, float b, float tolerance = 0.002f) { return std::abs(a-b) < tolerance; }
// An independent exhaustive triangle scan verifies the accelerated traversal.
std::optional<float> brute(const TerrainMesh& mesh, const Ray& ray) {
    float best = 1e30f;
    for (size_t i = 0; i < mesh.indices().size(); i += 3) {
        auto a = mesh.vertices()[mesh.indices()[i]].position;
        auto b = mesh.vertices()[mesh.indices()[i+1]].position;
        auto c = mesh.vertices()[mesh.indices()[i+2]].position;
        glm::vec3 e1=b-a,e2=c-a,h=glm::cross(ray.direction,e2);
        float det=glm::dot(e1,h); if (std::abs(det)<1e-8f) continue;
        glm::vec3 s=ray.origin-a,q=glm::cross(s,e1);
        float u=glm::dot(s,h)/det,v=glm::dot(ray.direction,q)/det,t=glm::dot(e2,q)/det;
        if (u>=-1e-5f && v>=-1e-5f && u+v<=1.00001f && t>=0) best=std::min(best,t);
    }
    return best < 1e29f ? std::optional<float>(best) : std::nullopt;
}
}
int main() {
    try {
        TerrainMesh mesh;
        auto flat = generateScenario({8,6},42); flat.elevation.assign(48,0);
        mesh.build({8,6},flat,10);
        require(mesh.vertices().size() == 63 && mesh.indices().size() == 288,"Incorrect mesh dimensions");
        for (const auto& v : mesh.vertices()) require(close(v.normal.y,1),"Flat normal not upward");
        auto hit = mesh.intersect({{0.25f,10,-0.25f},{0,-1,0}});
        require(hit && hit->x==4 && hit->y==3 && close(hit->position.y,0) && close(hit->distance,10),"Vertical ray mapping failed");
        require(!mesh.intersect({{0,10,0},{0,1,0}}),"Ray pointing away incorrectly hits");
        require(!mesh.intersect({{5,10,0},{0,-1,0}}),"Ray outside terrain incorrectly hits");
        require(!mesh.intersect({{0,10,0},{1,0,0}}),"Parallel ray above terrain incorrectly hits");
        require(!mesh.intersect({{0,10,0},{0,0,0}}),"Zero direction accepted");
        auto edge = mesh.intersect({{4,10,-3},{0,-1,0}});
        require(edge && edge->x==7 && edge->y==5,"Outer boundary mapping failed");
        auto corner = mesh.intersect({{-4,10,3},{0,-1,0}});
        require(corner && corner->x==0 && corner->y==0,"Minimum corner failed");
        std::cout << "PASS: mesh normals, vertical rays, misses and boundary mapping\n";

        auto hills = generateScenario({16,12},42); mesh.build({16,12},hills,10);
        for (int i=0;i<600;++i) {
            float angle=i*0.123f;
            glm::vec3 origin(std::sin(angle)*22,8+(i%7),std::cos(angle)*18);
            glm::vec3 target((i%19)-9.0f,((i%5)-2)*0.2f,((i%17)-8.0f));
            Ray ray{origin,glm::normalize(target-origin)};
            auto accelerated=mesh.intersect(ray); auto exhaustive=brute(mesh,ray);
            require(accelerated.has_value()==exhaustive.has_value(),"DDA hit/miss differs from exhaustive scan");
            if (accelerated) require(close(accelerated->distance,*exhaustive,0.005f),"DDA did not find nearest triangle");
        }
        std::cout << "PASS: 600 oblique rays agree with exhaustive nearest-triangle scan\n";

        Camera camera; camera.frame(16,12,mesh.centerHeight());
        Ray center = camera.screenRay(450,350,900,700);
        require(glm::dot(center.direction,camera.forward())>0.9999f,"Center screen ray does not follow camera");
        require(mesh.intersect(center).has_value(),"Framed camera misses terrain center");
        glm::vec3 point(2,mesh.centerHeight(),-1);
        glm::vec4 clip=camera.projection(900.0f/700)*camera.view()*glm::vec4(point,1);
        clip/=clip.w;
        Ray projected=camera.screenRay((clip.x+1)*450,(1-clip.y)*350,900,700);
        require(glm::length(glm::cross(point-projected.origin,projected.direction))<0.003f,"Projection/unprojection mismatch");
        auto eye = camera.position(); camera.setMode(CameraMode::FreeFly);
        require(glm::length(camera.position()-eye)<0.001f,"Free-fly mode jumps position");
        camera.move(1,0,0,0.1f); require(glm::length(camera.position()-eye)>0.1f,"Free-fly movement failed");
        eye=camera.position(); camera.setMode(CameraMode::Orbit);
        require(glm::length(camera.position()-eye)<0.001f,"Orbit mode jumps position");
        camera.rotate(100000,100000); require(std::isfinite(camera.view()[0][0]),"Camera rotation singularity");
        std::cout << "PASS: camera projection, unprojection, mode continuity and movement\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}

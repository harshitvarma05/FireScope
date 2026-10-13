#version 410 core
layout(location=0) in vec2 corner;
layout(location=2) in vec4 originSize;
layout(location=3) in vec4 detail; // kind (0 flame, 1 smoke), normalized age, intensity, opacity
uniform mat4 viewProjection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
out vec2 uv;
flat out vec4 particle;
void main() {
    uv = corner*0.5+0.5;
    particle = detail;
    float stretch = detail.x < 0.5 ? 1.8 : (detail.x>2.5 ? 2.8 : 1.0);
    vec3 world = originSize.xyz+(cameraRight*corner.x+cameraUp*corner.y*stretch)*originSize.w;
    gl_Position = viewProjection*vec4(world,1);
}

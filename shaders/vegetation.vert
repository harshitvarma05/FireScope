#version 410 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 originScale;
layout(location=3) in vec4 detail; // rotation, cell UV, unused
uniform mat4 viewProjection;
uniform sampler2D grid;
out vec3 surfaceNormal;
out vec3 treeColor;
void main() {
    int state = int(round(texture(grid,detail.yz).r*255.0));
    float c = cos(detail.x), s = sin(detail.x);
    mat3 rotation = mat3(c,0,-s, 0,1,0, s,0,c);
    vec3 world = originScale.xyz + rotation*position*originScale.w;
    surfaceNormal = rotation*normal;
    treeColor = position.y < 0.5 ? vec3(0.32,0.19,0.09) : vec3(0.12,0.36,0.17);
    if (state == 2) treeColor = vec3(0.44,0.17,0.04);
    if (state == 3) treeColor = vec3(0.065,0.06,0.055);
    gl_Position = state == 0 ? vec4(2,2,2,1) : viewProjection*vec4(world,1);
}

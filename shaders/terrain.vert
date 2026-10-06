#version 410 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 textureCoordinate;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 uv;
out vec3 surfaceNormal;
void main() {
    uv = textureCoordinate;
    surfaceNormal = mat3(transpose(inverse(model))) * normal;
    gl_Position = projection * view * model * vec4(position,1);
}

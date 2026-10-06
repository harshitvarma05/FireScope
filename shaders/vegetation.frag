#version 410 core
in vec3 surfaceNormal;
in vec3 treeColor;
out vec4 color;
void main() {
    float light = 0.42+0.58*max(dot(normalize(surfaceNormal),normalize(vec3(-0.4,1,0.3))),0);
    color = vec4(treeColor*light,1);
}

#version 410 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 originScale;
layout(location=3) in vec4 detail; // yaw, cell UV, variety
layout(location=4) in vec3 materialUV; // leaf UV and material: 0 wood, 1 foliage
uniform mat4 viewProjection;
uniform sampler2D grid;
uniform sampler2D staticProperties;
uniform int moistureOverlay;
uniform float visualTime;
out vec3 surfaceNormal;
out vec3 worldPosition;
out vec3 treeColor;
out vec3 leaf;
void main() {
    vec2 data=texture(grid,detail.yz).rg;
    int state=int(round(data.r*255));
    float c=cos(detail.x),s=sin(detail.x);
    mat3 rotation=mat3(c,0,-s, 0,1,0, s,0,c);
    vec3 local=position;
    local.x += sin(visualTime*1.5+detail.x+position.y)*0.025*position.y*materialUV.z;
    vec3 world=originScale.xyz+rotation*local*originScale.w;
    worldPosition=world;
    surfaceNormal=rotation*normal;
    leaf=materialUV;
    treeColor=materialUV.z<0.5 ? vec3(0.28,0.19,0.105) :
        mix(vec3(0.13,0.23,0.06),vec3(0.29,0.37,0.12),detail.w);
    if (state==2) treeColor=mix(vec3(0.085,0.067,0.05),treeColor,data.g);
    if (state==3) treeColor=vec3(0.09,0.079,0.065);
    if (moistureOverlay!=0 && materialUV.z>0.5)
        treeColor=mix(treeColor,vec3(0.12,0.43,0.75),texture(staticProperties,detail.yz).r*0.75);
    bool removed=state==0 || (state==3 && materialUV.z>0.5) ||
        (state==2 && materialUV.z>0.5 && fract(position.x*13.7+position.z*9.3)>data.g);
    gl_Position=removed ? vec4(2,2,2,1) : viewProjection*vec4(world,1);
}

#version 410 core
in vec2 uv;
in vec3 surfaceNormal;
in vec3 worldPosition;
uniform sampler2D grid;
uniform sampler2D staticProperties;
uniform int moistureOverlay;
uniform sampler2D shadowMap;
uniform mat4 lightMatrix;
uniform vec3 cameraPosition;
uniform float sceneScale;
uniform int shadowPass;
uniform vec2 gridSize;
uniform vec3 brush;
uniform int brushVisible;
uniform int brushTool;
uniform int showGrid;
out vec4 color;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p); f=f*f*(3-2*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+1),f.x),f.y);
}
float shadow(vec3 normal) {
    vec4 light = lightMatrix*vec4(worldPosition,1);
    vec3 p = light.xyz/light.w*0.5+0.5;
    if (p.z>1 || p.z<0 || any(lessThan(p.xy,vec2(0))) || any(greaterThan(p.xy,vec2(1)))) return 1;
    float bias = max(0.00008,0.0004*(1-dot(normal,normalize(vec3(-0.4,0.85,0.3)))));
    float visible = 0;
    vec2 texel = 1.0/vec2(textureSize(shadowMap,0));
    for (int x=-1;x<=1;++x) for (int y=-1;y<=1;++y)
        visible += p.z-bias <= texture(shadowMap,p.xy+vec2(x,y)*texel).r ? 1.0 : 0.0;
    return visible/9.0;
}
void main() {
    if (shadowPass != 0) { color=vec4(1); return; }
    vec2 data=texture(grid,uv).rg;
    int state=int(round(data.r*255));
    vec3 n=normalize(surfaceNormal);
    float mottling=noise(worldPosition.xz*0.43)*0.65+noise(worldPosition.xz*2.3)*0.25+noise(worldPosition.xz*13.0)*0.1;
    vec3 soil=mix(vec3(0.20,0.145,0.085),vec3(0.40,0.31,0.19),mottling);
    vec3 grass=mix(vec3(0.15,0.21,0.085),vec3(0.31,0.35,0.16),mottling);
    vec2 properties=texture(staticProperties,uv).rg;
    vec3 rock=mix(vec3(0.28,0.29,0.26),vec3(0.48,0.47,0.40),mottling);
    vec3 c=soil;
    if (state==1) c=mix(soil,grass,properties.g*(1-smoothstep(0.65,0.84,noise(worldPosition.xz*0.17))));
    c=mix(c,rock,smoothstep(0.12,0.42,1.0-n.y));
    if (state==3) c=mix(vec3(0.075,0.067,0.054),vec3(0.18,0.16,0.13),mottling);
    float visibility=shadow(n);
    float diffuse=max(dot(n,normalize(vec3(-0.4,0.85,0.3))),0);
    c *= vec3(0.58,0.64,0.68)*0.55+vec3(1.0,0.87,0.68)*diffuse*visibility*0.85;
    if (state==2) c=mix(c,vec3(0.58,0.14,0.018),0.7)+vec3(0.45,0.17,0.015)*data.g;
    float fog=1-exp(-max(length(worldPosition-cameraPosition)-sceneScale*0.65,0.0)/sceneScale*0.085);
    c=mix(c,vec3(0.69,0.76,0.78),clamp(fog,0,0.45));
    if (moistureOverlay!=0) c=mix(c,vec3(0.12,0.43,0.75),properties.r*0.75);
    vec2 cell=uv*gridSize;
    if (showGrid!=0) {
        vec2 edge=min(fract(cell),1-fract(cell));
        vec2 line=smoothstep(vec2(0),max(fwidth(cell)*0.8,vec2(0.001)),edge);
        c*=0.72+0.28*min(line.x,line.y);
    }
    if (brushVisible!=0) {
        float d=length(cell-(brush.xy+vec2(0.5)));
        float stroke=1-smoothstep(0.25,0.75,abs(d-brush.z));
        c=mix(c,brushTool==2 ? vec3(0.3,0.75,1) : (brushTool==3 ? vec3(0.55,1,0.35) : vec3(1,0.96,0.72)),stroke*0.9);
    }
    color=vec4(c,1);
}

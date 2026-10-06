#version 410 core
in vec3 surfaceNormal;
in vec3 worldPosition;
in vec3 treeColor;
in vec3 leaf;
uniform sampler2D shadowMap;
uniform mat4 lightMatrix;
uniform vec3 cameraPosition;
uniform float sceneScale;
uniform int shadowPass;
out vec4 color;
float shadow(vec3 n) {
    vec4 light=lightMatrix*vec4(worldPosition,1);
    vec3 p=light.xyz/light.w*0.5+0.5;
    if (p.z>1 || p.z<0 || any(lessThan(p.xy,vec2(0))) || any(greaterThan(p.xy,vec2(1)))) return 1;
    float bias=max(0.00008,0.0004*(1-abs(dot(n,normalize(vec3(-0.4,0.85,0.3))))));
    float visible=0; vec2 texel=1.0/vec2(textureSize(shadowMap,0));
    for (int x=-1;x<=1;++x) for (int y=-1;y<=1;++y)
        visible += p.z-bias <= texture(shadowMap,p.xy+vec2(x,y)*texel).r ? 1.0 : 0.0;
    return visible/9;
}
void main() {
    vec3 c=treeColor;
    if (leaf.z>0.5) {
        vec2 p=leaf.xy*2-1;
        float width=(1-abs(p.y))*0.85+0.1;
        if (abs(p.x)>width || abs(p.y)>0.98) discard;
        float vein=1-smoothstep(0.02,0.07,abs(p.x));
        c*=0.85+0.2*leaf.y+0.12*vein;
    } else {
        c*=0.85+0.15*sin(worldPosition.y*19+worldPosition.x*70);
    }
    if (shadowPass!=0) { color=vec4(1); return; }
    vec3 n=normalize(surfaceNormal);
    float diffuse=leaf.z>0.5 ? abs(dot(n,normalize(vec3(-0.4,0.85,0.3)))) : max(dot(n,normalize(vec3(-0.4,0.85,0.3))),0);
    c*=vec3(0.58,0.64,0.68)*0.65+vec3(1,0.87,0.68)*diffuse*shadow(n)*0.9;
    float fog=1-exp(-max(length(worldPosition-cameraPosition)-sceneScale*0.65,0)/sceneScale*0.085);
    color=vec4(mix(c,vec3(0.69,0.76,0.78),clamp(fog,0,0.45)),1);
}

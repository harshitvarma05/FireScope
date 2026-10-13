#version 410 core
in vec3 surfaceNormal;
in vec3 worldPosition;
in vec3 treeColor;
in vec3 leaf;
in float heat;
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
    float bias=max(0.00008,0.0004*(1-abs(dot(n,normalize(vec3(-0.65,0.48,0.57))))));
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
    c=pow(max(c,vec3(0)),vec3(2.2));
    vec3 n=normalize(surfaceNormal);
    float diffuse=leaf.z>0.5 ? abs(dot(n,normalize(vec3(-0.65,0.48,0.57)))) : max(dot(n,normalize(vec3(-0.65,0.48,0.57))),0);
    float visibility=shadow(n);
    vec3 viewDirection=normalize(cameraPosition-worldPosition);
    vec3 sun=normalize(vec3(-0.65,0.48,0.57));
    float transmission=pow(max(dot(-viewDirection,sun),0),3)*max(0.15,1-diffuse);
    float canopyOcclusion=leaf.z>0.5 ? mix(0.65,1.0,leaf.y) : 0.75;
    c*=vec3(0.40,0.55,0.65)*0.55*canopyOcclusion+vec3(1.35,1.02,0.60)*diffuse*visibility;
    if (leaf.z>0.5) c+=pow(treeColor,vec3(2.2))*vec3(1.3,1.1,0.45)*transmission*visibility*0.8;
    c+=vec3(0.8,0.10,0.008)*heat*(leaf.z>0.5 ? 0.28 : 0.65);
    color=vec4(c,1);
}

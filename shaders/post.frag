#version 410 core
in vec2 screen;
uniform sampler2D scene;
uniform sampler2D bloom;
uniform sampler2D depthMap;
uniform sampler2D shadowMap;
uniform mat4 inverseViewProjection;
uniform mat4 lightMatrix;
uniform vec3 cameraPosition;
uniform float sceneScale;
uniform float terrainHeight;
uniform float exposure;
uniform float bloomStrength;
uniform float atmosphere;
uniform int cinematic;
out vec4 color;
vec3 filmic(vec3 x) {
    return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0,1);
}
void main() {
    vec2 uv=screen*0.5+0.5;
    vec3 c=texture(scene,uv).rgb;
    if (cinematic!=0) {
        if (bloomStrength>0) c+=texture(bloom,uv).rgb*bloomStrength;
        float depth=texture(depthMap,uv).r;
        vec4 point=inverseViewProjection*vec4(screen,depth*2-1,1);
        vec3 delta=point.xyz/point.w-cameraPosition;
        float distanceToSurface=length(delta);
        vec3 ray=delta/max(distanceToSurface,0.0001);
        float lengthToMarch=min(distanceToSurface,sceneScale*1.8);
        // Bounded single-scattering approximation. Shadowed samples create shafts;
        // this is atmospheric rendering, independent of the wildfire model.
        float jitter=fract(sin(dot(gl_FragCoord.xy,vec2(12.9898,78.233)))*43758.5453);
        float transmittance=1; vec3 scatter=vec3(0);
        vec3 sun=normalize(vec3(-0.65,0.48,0.57));
        float phase=0.25+1.8*pow(max(dot(ray,sun),0),8);
        for (int i=0;i<12;++i) {
            vec3 p=cameraPosition+ray*lengthToMarch*(float(i)+jitter)/12;
            float density=exp(-max(p.y-terrainHeight,0)/max(sceneScale*0.09,1));
            vec4 lp=lightMatrix*vec4(p,1);
            vec3 sp=lp.xyz/lp.w*0.5+0.5;
            float lit=1;
            if (all(greaterThan(sp,vec3(0))) && all(lessThan(sp,vec3(1))))
                lit=sp.z-0.0003<texture(shadowMap,sp.xy).r ? 1.0 : 0.12;
            float extinction=1-exp(-density*lengthToMarch/sceneScale*atmosphere*0.23/12);
            scatter+=transmittance*extinction*(vec3(0.38,0.48,0.53)+vec3(1.4,0.88,0.39)*lit*phase);
            transmittance*=1-extinction;
        }
        c=c*transmittance+scatter;
    }
    c=filmic(c*exposure);
    c=pow(c,vec3(1.0/2.2));
    if (cinematic!=0) {
        float vignette=smoothstep(0.35,1.5,length(screen*vec2(0.85,1)));
        c*=1-vignette*0.17;
    }
    color=vec4(c,1);
}

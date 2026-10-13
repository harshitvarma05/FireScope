#version 410 core
in vec2 screen;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPosition;
uniform float visualTime;
out vec4 color;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p),f=fract(p);f=f*f*(3-2*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+1),f.x),f.y);
}
float fbm(vec2 p) { return noise(p)*0.5+noise(p*2.03)*0.28+noise(p*4.1)*0.14+noise(p*8.2)*0.08; }
void main() {
    vec4 farPoint=inverseViewProjection*vec4(screen,1,1);
    vec3 ray=normalize(farPoint.xyz/farPoint.w-cameraPosition);
    vec3 sun=normalize(vec3(-0.65,0.48,0.57));
    float sunDot=max(dot(ray,sun),0);
    float height=max(ray.y,0);
    vec3 sky=mix(vec3(0.60,0.66,0.67),vec3(0.075,0.22,0.39),pow(height,0.48));
    sky+=vec3(0.7,0.32,0.10)*pow(sunDot,12)*0.7;
    sky+=vec3(5.0,3.2,1.4)*smoothstep(0.9993,0.9998,sunDot);
    sky+=vec3(0.5,0.28,0.09)*pow(sunDot,180);
    vec2 cloudUV=ray.xz/max(ray.y+0.22,0.22)*2.5+vec2(visualTime*0.004,0);
    float clouds=fbm(cloudUV);
    float mask=smoothstep(0.50,0.73,clouds)*smoothstep(0.015,0.22,ray.y);
    vec3 cloud=mix(vec3(0.40,0.46,0.51),vec3(1.15,1.03,0.80),smoothstep(0.53,0.72,clouds)+sunDot*0.15);
    sky=mix(sky,cloud,mask*0.85);
    // Distant scenery is an atmospheric backdrop, not simulation cells.
    float angle=atan(ray.z,ray.x);
    for (int layer=0;layer<3;++layer) {
        float l=float(layer);
        float ridge=0.015+noise(vec2(angle*6+l*9,7))*0.075+noise(vec2(angle*19,23+l))*0.024-l*0.036;
        float mountain=1-smoothstep(ridge-0.003,ridge+0.003,ray.y);
        sky=mix(sky,mix(vec3(0.34,0.43,0.46),vec3(0.13,0.23,0.25),l/2),mountain);
    }
    color=vec4(sky,1);
}

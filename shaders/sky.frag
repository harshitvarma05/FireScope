#version 410 core
in vec2 screen;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPosition;
out vec4 color;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+1),f.x),f.y);
}
void main() {
    vec4 farPoint = inverseViewProjection*vec4(screen,1,1);
    vec3 ray = normalize(farPoint.xyz/farPoint.w-cameraPosition);
    float height = max(ray.y,0.0);
    vec3 sky = mix(vec3(0.69,0.76,0.78),vec3(0.25,0.43,0.61),pow(height,0.55));
    vec3 sun = normalize(vec3(-0.4,0.85,0.3));
    sky += vec3(1,0.77,0.46)*pow(max(dot(ray,sun),0.0),128.0)*0.75;
    vec2 cloudUV = ray.xz/max(ray.y+0.2,0.2)*3.0;
    float clouds = noise(cloudUV)*0.6+noise(cloudUV*2.03)*0.3+noise(cloudUV*4.1)*0.1;
    sky = mix(sky,vec3(0.88,0.88,0.82),smoothstep(0.58,0.78,clouds)*smoothstep(0.0,0.18,ray.y)*0.65);
    color = vec4(sky,1);
}

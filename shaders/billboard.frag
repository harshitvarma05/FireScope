#version 410 core
in vec2 uv;
flat in vec4 particle;
out vec4 color;
void main() {
    vec2 p = uv*2.0-1.0;
    float shape;
    vec3 c;
    if (particle.x < 0.5) {
        p.x *= 1.0+max(p.y,0.0)*1.4;
        shape = 1.0-smoothstep(0.2,1.0,length(p));
        c = mix(vec3(1,0.18,0.015),vec3(1,0.88,0.25),shape*particle.z);
    } else {
        shape = 1.0-smoothstep(0.1,1.0,length(p));
        c = mix(vec3(0.22,0.23,0.25),vec3(0.48,0.49,0.51),particle.y);
    }
    float alpha = shape*particle.w;
    if (alpha < 0.01) discard;
    color = vec4(c,alpha);
}

#version 410 core
in vec2 screen;
uniform sampler2D source;
uniform vec2 direction;
uniform int extractBright;
out vec4 color;
vec3 sampleLight(vec2 uv) {
    vec3 c=texture(source,uv).rgb;
    if (extractBright!=0) {
        float brightness=max(c.r,max(c.g,c.b));
        c*=max(brightness-1.0,0.0)/max(brightness,0.0001);
    }
    return c;
}
void main() {
    vec2 uv=screen*0.5+0.5;
    vec3 c=sampleLight(uv)*0.227027;
    c+=(sampleLight(uv+direction*1.384615)+sampleLight(uv-direction*1.384615))*0.316216;
    c+=(sampleLight(uv+direction*3.230769)+sampleLight(uv-direction*3.230769))*0.070270;
    color=vec4(c,1);
}

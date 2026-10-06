#version 410 core
in vec2 uv;
flat in vec4 particle;
uniform float visualTime;
out vec4 color;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p),f=fract(p); f=f*f*(3-2*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+1),f.x),f.y);
}
float fbm(vec2 p) { return noise(p)*0.55+noise(p*2.03+7.1)*0.28+noise(p*4.1+17.3)*0.17; }
void main() {
    vec2 p=uv*2-1;
    float shape; vec3 c;
    float time=visualTime+particle.z*17.0;
    if (particle.x<0.5) {
        float height=uv.y;
        float turbulence=fbm(vec2(p.x*4,height*5-time*3));
        float bend=sin(height*6-time*4)*0.10*height;
        float width=(1-height)*0.75+0.03+(turbulence-0.5)*0.32;
        shape=(1-smoothstep(width*0.45,width,abs(p.x+bend)))*smoothstep(0,0.12,height)*(1-smoothstep(0.72,1,height+turbulence*0.15));
        float core=clamp((1-height)*shape,0,1);
        c=mix(vec3(1,0.115,0.008),vec3(1,0.90,0.44),pow(core,1.3));
        c+=vec3(0.17,0.08,0.01)*shape;
    } else if (particle.x<1.5) {
        vec2 swirl=p*3.2+vec2(time*0.12,-time*0.16);
        float density=fbm(swirl);
        float radius=length(p+vec2((density-0.5)*0.24,0));
        shape=(1-smoothstep(0.38,1,radius))*smoothstep(0.18,0.72,density);
        float light=fbm(swirl+vec2(-0.3,0.4));
        c=mix(vec3(0.16,0.17,0.18),vec3(0.57,0.55,0.49),light*0.85+particle.y*0.15);
    } else {
        shape=pow(max(1-length(p),0),3);
        c=vec3(1,0.32,0.035);
    }
    float alpha=shape*particle.w;
    if (alpha<0.008) discard;
    color=vec4(c,alpha);
}

#version 410 core
in vec2 uv;
in vec3 surfaceNormal;
uniform sampler2D grid;
uniform vec2 gridSize;
uniform vec3 brush; // cell x, cell y, radius
uniform int brushVisible;
uniform int showGrid;
out vec4 color;
void main() {
    vec2 data = texture(grid,uv).rg;
    int state = int(round(data.r * 255.0));
    vec3 c = vec3(0.27,0.22,0.16);
    if (state == 1) c = vec3(0.21,0.46,0.25);
    if (state == 2) c = mix(vec3(0.86,0.19,0.035),vec3(1.0,0.78,0.16),data.g);
    if (state == 3) c = vec3(0.065,0.075,0.08);
    float light = 0.42 + 0.58 * max(dot(normalize(surfaceNormal),normalize(vec3(-0.4,1,0.3))),0);
    if (state != 2) c *= light;
    vec2 cell = uv * gridSize;
    if (showGrid != 0) {
        vec2 edge = min(fract(cell),1.0-fract(cell));
        vec2 line = smoothstep(vec2(0),max(fwidth(cell)*0.8,vec2(0.001)),edge);
        c *= 0.72 + 0.28 * min(line.x,line.y);
    }
    if (brushVisible != 0) {
        float d = length(cell-(brush.xy+vec2(0.5)));
        float stroke = 1.0-smoothstep(0.25,0.75,abs(d-brush.z));
        c = mix(c,vec3(1,0.96,0.72),stroke*0.9);
    }
    color = vec4(c,1);
}

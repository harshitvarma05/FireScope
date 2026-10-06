#include <metal_stdlib>
using namespace metal;
struct EnvironmentParams {
    float baseIgnitionProbability, windSpeed, windDirectionRadians, globalHumidity;
    float moistureStrength, slopeStrength, windStrength, windAlignmentStrength;
    float fuelConsumptionRate, cellSizeMeters;
    uint minimumBurnTicks, seed, timestep;
};
uint mixBits(uint v) {
    v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; return v ^ (v >> 16);
}
uint hashCell(uint seed,uint x,uint y,uint step) {
    return mixBits(seed ^ mixBits(x+0x9e3779b9u) ^ mixBits(y+0x85ebca6bu) ^ mixBits(step+0xc2b2ae35u));
}
kernel void wildfireStep(
    device const uint* state [[buffer(0)]], device const float* fuel [[buffer(1)]], device const uint* ticks [[buffer(2)]],
    device uint* nextState [[buffer(3)]], device float* nextFuel [[buffer(4)]], device uint* nextTicks [[buffer(5)]],
    device const float* moisture [[buffer(6)]], device const float* elevation [[buffer(7)]],
    device const float* vegetation [[buffer(8)]], device const float* density [[buffer(9)]],
    constant EnvironmentParams& p [[buffer(10)]], constant uint2& size [[buffer(11)]],
    uint2 xy [[thread_position_in_grid]]) {
    const uint x=xy.x,y=xy.y,width=size.x,height=size.y;
    if (x>=width || y>=height) return;
    const uint i=y*width+x,s=state[i];
    nextState[i]=s; nextFuel[i]=fuel[i]; nextTicks[i]=ticks[i];
    if (s==2) {
        nextFuel[i]=max(0.0f,fuel[i]-p.fuelConsumptionRate); nextTicks[i]=ticks[i]+1;
        if (nextFuel[i]<=0 && nextTicks[i]>=p.minimumBurnTicks) nextState[i]=3;
    } else if (s==1 && fuel[i]>0) {
        float survival=1.0f;
        const float m=1.0f-(1.0f-moisture[i])*(1.0f-p.globalHumidity);
        for (int dy=-1;dy<=1;++dy) for (int dx=-1;dx<=1;++dx) {
            if (!dx && !dy) continue;
            const int nx=int(x)+dx,ny=int(y)+dy;
            if (nx<0 || ny<0 || nx>=int(width) || ny>=int(height)) continue;
            const uint j=ny*width+nx; if (state[j]!=2) continue;
            const float angle=atan2(float(-dy),float(-dx));
            const float distance=p.cellSizeMeters*((dx && dy)?1.41421356237f:1.0f);
            const float slope=atan((elevation[i]-elevation[j])/distance);
            const float exponent=p.windStrength*p.windSpeed +
                p.windSpeed*p.windAlignmentStrength*(cos(angle-p.windDirectionRadians)-1.0f) +
                p.slopeStrength*slope-p.moistureStrength*m;
            const float probability=clamp(p.baseIgnitionProbability*(0.5f+vegetation[i])*
                (0.5f+density[i])*exp(clamp(exponent,-80.0f,80.0f)),0.0f,1.0f);
            survival*=1.0f-probability;
        }
        const float probability=1.0f-survival;
        // Power-of-two scaling is exact for a float below 1; handle 2^32 separately.
        if (probability>=1.0f || (probability>0.0f && hashCell(p.seed,x,y,p.timestep)<uint(probability*4294967296.0f))) {
            nextState[i]=2; nextTicks[i]=0;
        }
    }
}
kernel void compactRender(device const uint* state [[buffer(0)]],device const float* fuel [[buffer(1)]],
    device uchar2* out [[buffer(2)]],constant uint& count [[buffer(3)]],uint i [[thread_position_in_grid]]) {
    if (i>=count) return;
    out[i]=uchar2(uchar(state[i]),state[i]==2 ? uchar(clamp(fuel[i],0.15f,1.0f)*255.0f) : uchar(0));
}
kernel void paintBrush(device uint* state [[buffer(0)]],device float* fuel [[buffer(1)]],
    device uint* ticks [[buffer(2)]],device float* moisture [[buffer(3)]],
    constant uint2& size [[buffer(4)]],constant int4& brush [[buffer(5)]],
    constant float& strength [[buffer(6)]],device float* density [[buffer(7)]],
    device float* vegetation [[buffer(8)]],uint2 xy [[thread_position_in_grid]]) {
    const int x=int(xy.x)+brush.x-brush.z,y=int(xy.y)+brush.y-brush.z;
    if (x<0 || y<0 || x>=int(size.x) || y>=int(size.y) ||
        (x-brush.x)*(x-brush.x)+(y-brush.y)*(y-brush.y)>brush.z*brush.z) return;
    const uint i=y*size.x+x;
    if (brush.w==0 && state[i]==1 && fuel[i]>0) { state[i]=2; ticks[i]=0; }
    else if (brush.w==1) { state[i]=0; fuel[i]=0; ticks[i]=0; }
    else if (brush.w==2) moisture[i]=min(1.0f,moisture[i]+strength);
    else if (brush.w==3 && state[i]<=1) {
        const float value=min(1.0f,strength);
        density[i]=value; vegetation[i]=value; fuel[i]=value;
        state[i]=value>0 ? 1u : 0u; ticks[i]=0;
    }
}

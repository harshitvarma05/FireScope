#pragma once
#include "firescope/SimulationTypes.hpp"
namespace firescope {
__device__ uint32_t deviceMix(uint32_t v) {
    v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; return v ^ (v >> 16);
}
__device__ uint32_t deviceHash(uint32_t seed, uint32_t x, uint32_t y, uint32_t step) {
    return deviceMix(seed ^ deviceMix(x+0x9e3779b9u) ^ deviceMix(y+0x85ebca6bu) ^ deviceMix(step+0xc2b2ae35u));
}
__global__ void wildfireStep(const uint32_t* state, const float* fuel, const uint32_t* ticks,
    uint32_t* nextState, float* nextFuel, uint32_t* nextTicks,
    const float* moisture, const float* elevation, const float* vegetation, const float* density,
    uint32_t width, uint32_t height, EnvironmentParams p) {
    const uint32_t x=blockIdx.x*blockDim.x+threadIdx.x, y=blockIdx.y*blockDim.y+threadIdx.y;
    if (x>=width || y>=height) return;
    const uint32_t i=y*width+x, s=state[i];
    nextState[i]=s; nextFuel[i]=fuel[i]; nextTicks[i]=ticks[i];
    if (s==2) {
        nextFuel[i]=fmaxf(0.0f,fuel[i]-p.fuelConsumptionRate); nextTicks[i]=ticks[i]+1;
        if (nextFuel[i]<=0 && nextTicks[i]>=p.minimumBurnTicks) nextState[i]=3;
    } else if (s==1 && fuel[i]>0) {
        float survival=1.0f;
        const float m=1.0f-(1.0f-moisture[i])*(1.0f-p.globalHumidity);
        for (int dy=-1;dy<=1;++dy) for (int dx=-1;dx<=1;++dx) {
            if (!dx && !dy) continue;
            const int nx=static_cast<int>(x)+dx, ny=static_cast<int>(y)+dy;
            if (nx<0 || ny<0 || nx>=static_cast<int>(width) || ny>=static_cast<int>(height)) continue;
            const uint32_t j=ny*width+nx; if (state[j]!=2) continue;
            const float angle=atan2f(static_cast<float>(-dy),static_cast<float>(-dx));
            const float distance=p.cellSizeMeters*((dx && dy)?1.41421356237f:1.0f);
            const float slope=atanf((elevation[i]-elevation[j])/distance);
            const float exponent=p.windStrength*p.windSpeed +
                p.windSpeed*p.windAlignmentStrength*(cosf(angle-p.windDirectionRadians)-1.0f) +
                p.slopeStrength*slope-p.moistureStrength*m;
            const float probability=fminf(1.0f,fmaxf(0.0f,p.baseIgnitionProbability*(0.5f+vegetation[i])*
                (0.5f+density[i])*expf(fminf(80.0f,fmaxf(-80.0f,exponent)))));
            survival*=1.0f-probability;
        }
        const float probability=1.0f-survival;
        const uint64_t threshold=static_cast<uint64_t>(static_cast<double>(probability)*4294967296.0);
        if (static_cast<uint64_t>(deviceHash(p.seed,x,y,p.timestep))<threshold) { nextState[i]=2; nextTicks[i]=0; }
    }
}
__global__ void compactRender(const uint32_t* state, const float* fuel, uchar2* out, uint32_t count) {
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x; if (i>=count) return;
    out[i]=make_uchar2(static_cast<unsigned char>(state[i]),state[i]==2 ?
        static_cast<unsigned char>(fminf(1.0f,fmaxf(0.15f,fuel[i]))*255.0f) : 0);
}
__global__ void paintBrush(uint32_t* state, float* fuel, uint32_t* ticks, float* moisture,
    float* density, float* vegetation, uint32_t width, uint32_t height, int cx, int cy, int radius, int tool, float strength) {
    const int x=blockIdx.x*blockDim.x+threadIdx.x+cx-radius;
    const int y=blockIdx.y*blockDim.y+threadIdx.y+cy-radius;
    if (x<0 || y<0 || x>=static_cast<int>(width) || y>=static_cast<int>(height) ||
        (x-cx)*(x-cx)+(y-cy)*(y-cy)>radius*radius) return;
    const uint32_t i=y*width+x;
    if (tool==0 && state[i]==1 && fuel[i]>0) { state[i]=2; ticks[i]=0; }
    else if (tool==1) { state[i]=0; fuel[i]=0; ticks[i]=0; }
    else if (tool==2) moisture[i]=fminf(1.0f,moisture[i]+strength);
    else if (tool==3 && state[i]<=1) {
        const float value=fminf(1.0f,strength);
        density[i]=value; vegetation[i]=value; fuel[i]=value;
        state[i]=value>0 ? 1u : 0u; ticks[i]=0;
    }
}
}

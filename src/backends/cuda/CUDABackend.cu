#include "CUDABackend.hpp"
#include "CPUBackend.hpp"
#include "firescope/Validation.hpp"
#include <cuda_runtime.h>
#include "wildfire_cuda.cuh"
#include <stdexcept>
#include <string>
namespace firescope {
namespace {
void check(cudaError_t result) {
    if (result!=cudaSuccess) throw std::runtime_error(std::string("CUDA: ")+cudaGetErrorString(result));
}
template<class T> struct DeviceBuffer {
    T* data=nullptr;
    ~DeviceBuffer() { if (data) cudaFree(data); }
    void allocate(size_t n) { if (data) { check(cudaFree(data)); data=nullptr; } check(cudaMalloc(reinterpret_cast<void**>(&data),n*sizeof(T))); }
    void upload(const std::vector<T>& values) { check(cudaMemcpy(data,values.data(),values.size()*sizeof(T),cudaMemcpyHostToDevice)); }
    void download(std::vector<T>& values, size_t count) { values.resize(count); check(cudaMemcpy(values.data(),data,count*sizeof(T),cudaMemcpyDeviceToHost)); }
};
}
struct CUDABackend::Impl {
    SimulationConfig config;
    DeviceBuffer<uint32_t> state[2],ticks[2]; DeviceBuffer<float> fuel[2],moisture,elevation,vegetation,density;
    DeviceBuffer<uchar2> compact;
    std::vector<uchar2> staging;
    SimulationStats stats;
    std::string device;
    int current=0; bool initialized=false,dirty=true;
    cudaEvent_t begin=nullptr,end=nullptr;
    ~Impl() { if (begin) cudaEventDestroy(begin); if (end) cudaEventDestroy(end); }
};
CUDABackend::CUDABackend() : impl_(std::make_unique<Impl>()) {
    if (!available()) throw std::runtime_error("No CUDA device available");
    int device=0; check(cudaGetDevice(&device)); cudaDeviceProp properties{};
    check(cudaGetDeviceProperties(&properties,device)); impl_->device=properties.name;
    check(cudaEventCreate(&impl_->begin)); check(cudaEventCreate(&impl_->end));
}
CUDABackend::~CUDABackend() = default;
std::string CUDABackend::deviceName() const { return impl_->device; }
bool CUDABackend::available() { int count=0; return cudaGetDeviceCount(&count)==cudaSuccess && count>0; }
void CUDABackend::initialize(const SimulationConfig& config, const InitialGrid& grid) {
    CPUBackend validate; validate.initialize(config,grid);
    auto& m=*impl_; m.config=config; m.initialized=false;
    const size_t n=grid.state.size();
    for (int i=0;i<2;++i) { m.state[i].allocate(n); m.ticks[i].allocate(n); m.fuel[i].allocate(n); }
    m.moisture.allocate(n); m.elevation.allocate(n); m.vegetation.allocate(n); m.density.allocate(n);
    m.compact.allocate(n); m.staging.resize(n); m.initialized=true; reset(grid);
}
void CUDABackend::reset(const InitialGrid& grid) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize CUDA before reset");
    CPUBackend validate; validate.initialize(m.config,grid);
    m.current=0; m.state[0].upload(grid.state); m.fuel[0].upload(grid.fuel); m.ticks[0].upload(grid.burnTicks);
    m.moisture.upload(grid.moisture); m.elevation.upload(grid.elevation);
    m.vegetation.upload(grid.vegetation); m.density.upload(grid.density); m.stats={}; m.dirty=true;
}
void CUDABackend::step(const EnvironmentParams& params,uint32_t count) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize CUDA before step");
    validateEnvironment(params);
    if (!count) return;
    check(cudaEventRecord(m.begin));
    const dim3 block(16,16), grid((m.config.width+15)/16,(m.config.height+15)/16);
    for (uint32_t k=0;k<count;++k) {
        auto p=params; p.timestep=static_cast<uint32_t>(m.stats.timestep);
        int a=m.current,b=1-a;
        wildfireStep<<<grid,block>>>(m.state[a].data,m.fuel[a].data,m.ticks[a].data,
            m.state[b].data,m.fuel[b].data,m.ticks[b].data,m.moisture.data,m.elevation.data,
            m.vegetation.data,m.density.data,m.config.width,m.config.height,p);
        check(cudaGetLastError()); m.current=b; ++m.stats.timestep;
    }
    check(cudaEventRecord(m.end)); check(cudaEventSynchronize(m.end));
    float ms=0; check(cudaEventElapsedTime(&ms,m.begin,m.end)); m.stats.simulationMs=ms/count; m.dirty=true;
}
void CUDABackend::downloadRenderState(RenderState& out) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize CUDA before download");
    out.width=m.config.width; out.height=m.config.height;
    const uint32_t n=m.config.width*m.config.height;
    compactRender<<<(n+255)/256,256>>>(m.state[m.current].data,m.fuel[m.current].data,m.compact.data,n);
    check(cudaGetLastError()); check(cudaMemcpy(m.staging.data(),m.compact.data,n*sizeof(uchar2),cudaMemcpyDeviceToHost));
    out.cellState.resize(n); out.fireIntensity.resize(n);
    m.stats.noFuel=m.stats.unburned=m.stats.burning=m.stats.burned=0;
    for (uint32_t i=0;i<n;++i) {
        out.cellState[i]=m.staging[i].x; out.fireIntensity[i]=m.staging[i].y;
        switch (m.staging[i].x) { case 0:++m.stats.noFuel;break; case 1:++m.stats.unburned;break; case 2:++m.stats.burning;break; case 3:++m.stats.burned;break; }
    }
    m.dirty=false;
}
SimulationStats CUDABackend::stats() const {
    if (impl_->dirty) { RenderState temporary; const_cast<CUDABackend*>(this)->downloadRenderState(temporary); }
    return impl_->stats;
}
void CUDABackend::downloadGrid(GridData& out) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize CUDA before snapshot");
    const size_t n=static_cast<size_t>(m.config.width)*m.config.height;
    m.state[m.current].download(out.state,n); m.fuel[m.current].download(out.fuel,n);
    m.ticks[m.current].download(out.burnTicks,n); m.moisture.download(out.moisture,n);
    m.elevation.download(out.elevation,n); m.vegetation.download(out.vegetation,n); m.density.download(out.density,n);
}
void CUDABackend::restoreSnapshot(const InitialGrid& grid,uint64_t timestep) { reset(grid); impl_->stats.timestep=timestep; }
void CUDABackend::applyBrush(BrushTool tool,int x,int y,int radius,float strength) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize CUDA before brush");
    if (radius<0 || radius>30 || !std::isfinite(strength) || strength<0) throw std::invalid_argument("Invalid brush parameters");
    const int side=radius*2+1; const dim3 block(16,16),grid((side+15)/16,(side+15)/16);
    paintBrush<<<grid,block>>>(m.state[m.current].data,m.fuel[m.current].data,m.ticks[m.current].data,
        m.moisture.data,m.density.data,m.vegetation.data,m.config.width,m.config.height,x,y,radius,static_cast<int>(tool),strength);
    check(cudaGetLastError()); check(cudaDeviceSynchronize()); m.dirty=true;
}
}

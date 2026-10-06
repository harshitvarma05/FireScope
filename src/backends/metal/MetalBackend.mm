#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "MetalBackend.hpp"
#include "MetalSources.hpp"
#include "CPUBackend.hpp"
#include "firescope/Validation.hpp"
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
namespace firescope {
static_assert(sizeof(EnvironmentParams)==52 && offsetof(EnvironmentParams,minimumBurnTicks)==40 &&
              offsetof(EnvironmentParams,timestep)==48,"C++/Metal environment layout mismatch");
namespace {
void completed(id<MTLCommandBuffer> command) {
    [command commit]; [command waitUntilCompleted];
    if (command.status!=MTLCommandBufferStatusCompleted)
        throw std::runtime_error(std::string("Metal command failed: ")+(command.error.description.UTF8String ?: "unknown error"));
}
}
struct MetalBackend::Impl {
    id<MTLDevice> device=nil; id<MTLCommandQueue> queue=nil;
    id<MTLComputePipelineState> stepPipeline=nil,compactPipeline=nil,brushPipeline=nil;
    id<MTLBuffer> state[2]={nil,nil},fuel[2]={nil,nil},ticks[2]={nil,nil};
    id<MTLBuffer> moisture=nil,elevation=nil,vegetation=nil,density=nil,compact=nil;
    SimulationConfig config; SimulationStats stats;
    int current=0; bool initialized=false,dirty=true;
    id<MTLBuffer> buffer(size_t bytes) {
        auto result=[device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        if (!result) throw std::runtime_error("Metal buffer allocation failed"); return result;
    }
    id<MTLComputePipelineState> pipeline(id<MTLLibrary> library,NSString* name) {
        NSError* error=nil;
        auto function=[library newFunctionWithName:name];
        auto result=[device newComputePipelineStateWithFunction:function error:&error];
        if (!result) throw std::runtime_error(std::string("Metal pipeline: ")+(error.description.UTF8String ?: "missing function"));
        return result;
    }
    id<MTLCommandBuffer> command() {
        auto result=[queue commandBuffer]; if (!result) throw std::runtime_error("Metal command allocation failed"); return result;
    }
};
MetalBackend::MetalBackend() : impl_(std::make_unique<Impl>()) {
    @autoreleasepool {
        auto& m=*impl_; m.device=MTLCreateSystemDefaultDevice();
        if (!m.device) throw std::runtime_error("No Metal device available");
        m.queue=[m.device newCommandQueue]; if (!m.queue) throw std::runtime_error("Metal queue allocation failed");
        NSError* error=nil; MTLCompileOptions* options=[MTLCompileOptions new]; options.fastMathEnabled=NO;
        auto library=[m.device newLibraryWithSource:[NSString stringWithUTF8String:metalSource] options:options error:&error];
        if (!library) throw std::runtime_error(std::string("Metal shader compilation: ")+(error.description.UTF8String ?: "unknown error"));
        m.stepPipeline=m.pipeline(library,@"wildfireStep"); m.compactPipeline=m.pipeline(library,@"compactRender");
        m.brushPipeline=m.pipeline(library,@"paintBrush");
    }
}
MetalBackend::~MetalBackend() = default;
std::string MetalBackend::deviceName() const { return impl_->device.name.UTF8String; }
bool MetalBackend::available() { @autoreleasepool { return MTLCreateSystemDefaultDevice()!=nil; } }
void MetalBackend::initialize(const SimulationConfig& config,const InitialGrid& grid) {
    @autoreleasepool {
        CPUBackend validate; validate.initialize(config,grid);
        auto& m=*impl_; m.config=config; m.initialized=false;
        const size_t bytes=grid.state.size()*4;
        for (int i=0;i<2;++i) { m.state[i]=m.buffer(bytes); m.fuel[i]=m.buffer(bytes); m.ticks[i]=m.buffer(bytes); }
        m.moisture=m.buffer(bytes); m.elevation=m.buffer(bytes); m.vegetation=m.buffer(bytes); m.density=m.buffer(bytes);
        m.compact=m.buffer(grid.state.size()*2); m.initialized=true; reset(grid);
    }
}
void MetalBackend::reset(const InitialGrid& grid) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize Metal before reset");
    CPUBackend validate; validate.initialize(m.config,grid);
    const size_t bytes=grid.state.size()*4; m.current=0;
    std::memcpy(m.state[0].contents,grid.state.data(),bytes); std::memcpy(m.fuel[0].contents,grid.fuel.data(),bytes);
    std::memcpy(m.ticks[0].contents,grid.burnTicks.data(),bytes); std::memcpy(m.moisture.contents,grid.moisture.data(),bytes);
    std::memcpy(m.elevation.contents,grid.elevation.data(),bytes); std::memcpy(m.vegetation.contents,grid.vegetation.data(),bytes);
    std::memcpy(m.density.contents,grid.density.data(),bytes); m.stats={}; m.dirty=true;
}
void MetalBackend::step(const EnvironmentParams& params,uint32_t count) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize Metal before step");
    validateEnvironment(params); if (!count) return;
    @autoreleasepool {
        auto command=m.command(); const uint32_t size[2]={m.config.width,m.config.height};
        const NSUInteger tile=m.stepPipeline.maxTotalThreadsPerThreadgroup>=256 ? 16 : 8;
        for (uint32_t k=0;k<count;++k) {
            const int a=m.current,b=1-a;
            auto encoder=[command computeCommandEncoder]; [encoder setComputePipelineState:m.stepPipeline];
            [encoder setBuffer:m.state[a] offset:0 atIndex:0]; [encoder setBuffer:m.fuel[a] offset:0 atIndex:1];
            [encoder setBuffer:m.ticks[a] offset:0 atIndex:2]; [encoder setBuffer:m.state[b] offset:0 atIndex:3];
            [encoder setBuffer:m.fuel[b] offset:0 atIndex:4]; [encoder setBuffer:m.ticks[b] offset:0 atIndex:5];
            [encoder setBuffer:m.moisture offset:0 atIndex:6]; [encoder setBuffer:m.elevation offset:0 atIndex:7];
            [encoder setBuffer:m.vegetation offset:0 atIndex:8]; [encoder setBuffer:m.density offset:0 atIndex:9];
            auto p=params; p.timestep=static_cast<uint32_t>(m.stats.timestep);
            [encoder setBytes:&p length:sizeof(p) atIndex:10]; [encoder setBytes:size length:sizeof(size) atIndex:11];
            [encoder dispatchThreads:MTLSizeMake(m.config.width,m.config.height,1) threadsPerThreadgroup:MTLSizeMake(tile,tile,1)];
            [encoder endEncoding]; m.current=b; ++m.stats.timestep;
        }
        completed(command);
        m.stats.simulationMs=(command.GPUEndTime-command.GPUStartTime)*1000.0/count; m.dirty=true;
    }
}
void MetalBackend::downloadRenderState(RenderState& out) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize Metal before download");
    @autoreleasepool {
        const uint32_t count=m.config.width*m.config.height;
        auto command=m.command(); auto encoder=[command computeCommandEncoder];
        [encoder setComputePipelineState:m.compactPipeline];
        [encoder setBuffer:m.state[m.current] offset:0 atIndex:0]; [encoder setBuffer:m.fuel[m.current] offset:0 atIndex:1];
        [encoder setBuffer:m.compact offset:0 atIndex:2]; [encoder setBytes:&count length:sizeof(count) atIndex:3];
        [encoder dispatchThreads:MTLSizeMake(count,1,1) threadsPerThreadgroup:MTLSizeMake(256,1,1)];
        [encoder endEncoding]; completed(command);
        out.width=m.config.width; out.height=m.config.height; out.cellState.resize(count); out.fireIntensity.resize(count);
        const uint8_t* data=static_cast<const uint8_t*>(m.compact.contents);
        m.stats.noFuel=m.stats.unburned=m.stats.burning=m.stats.burned=0;
        for (uint32_t i=0;i<count;++i) {
            out.cellState[i]=data[i*2]; out.fireIntensity[i]=data[i*2+1];
            switch (data[i*2]) { case 0:++m.stats.noFuel;break; case 1:++m.stats.unburned;break; case 2:++m.stats.burning;break; case 3:++m.stats.burned;break; }
        }
        m.dirty=false;
    }
}
SimulationStats MetalBackend::stats() const {
    if (impl_->dirty) { RenderState temporary; const_cast<MetalBackend*>(this)->downloadRenderState(temporary); }
    return impl_->stats;
}
void MetalBackend::applyBrush(BrushTool tool,int x,int y,int radius,float strength) {
    auto& m=*impl_; if (!m.initialized) throw std::logic_error("Initialize Metal before brush");
    if (radius<0 || radius>30 || !std::isfinite(strength) || strength<0) throw std::invalid_argument("Invalid brush parameters");
    @autoreleasepool {
        const uint32_t size[2]={m.config.width,m.config.height}; const int brush[4]={x,y,radius,static_cast<int>(tool)};
        auto command=m.command(); auto encoder=[command computeCommandEncoder]; [encoder setComputePipelineState:m.brushPipeline];
        [encoder setBuffer:m.state[m.current] offset:0 atIndex:0]; [encoder setBuffer:m.fuel[m.current] offset:0 atIndex:1];
        [encoder setBuffer:m.ticks[m.current] offset:0 atIndex:2]; [encoder setBuffer:m.moisture offset:0 atIndex:3];
        [encoder setBytes:size length:sizeof(size) atIndex:4]; [encoder setBytes:brush length:sizeof(brush) atIndex:5];
        [encoder setBytes:&strength length:sizeof(strength) atIndex:6];
        const NSUInteger side=radius*2+1;
        [encoder dispatchThreads:MTLSizeMake(side,side,1) threadsPerThreadgroup:MTLSizeMake(16,16,1)];
        [encoder endEncoding]; completed(command); m.dirty=true;
    }
}
}

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <iostream>
int main() {
    @autoreleasepool {
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();
        if (!device) { std::cout << "SKIP: Metal device unavailable\n"; return 77; }
        NSError* error=nil;
        NSString* source=@"#include <metal_stdlib>\nusing namespace metal;\nkernel void increment(device const float* input [[buffer(0)]],device float* output [[buffer(1)]],uint i [[thread_position_in_grid]]){output[i]=input[i]+1;}";
        id<MTLLibrary> library=[device newLibraryWithSource:source options:nil error:&error];
        id<MTLFunction> function=[library newFunctionWithName:@"increment"];
        if (!function) { std::cerr << "Metal compile failed: " << error.description.UTF8String << '\n'; return 1; }
        id<MTLComputePipelineState> pipeline=[device newComputePipelineStateWithFunction:function error:&error];
        if (!pipeline) { std::cerr << "Metal pipeline failed\n"; return 1; }
        float input[16]; for (int i=0;i<16;++i) input[i]=i;
        id<MTLBuffer> a=[device newBufferWithBytes:input length:sizeof(input) options:MTLResourceStorageModeShared];
        id<MTLBuffer> b=[device newBufferWithLength:sizeof(input) options:MTLResourceStorageModeShared];
        id<MTLCommandQueue> queue=[device newCommandQueue];
        id<MTLCommandBuffer> command=[queue commandBuffer];
        id<MTLComputeCommandEncoder> encoder=[command computeCommandEncoder];
        [encoder setComputePipelineState:pipeline]; [encoder setBuffer:a offset:0 atIndex:0]; [encoder setBuffer:b offset:0 atIndex:1];
        [encoder dispatchThreads:MTLSizeMake(16,1,1) threadsPerThreadgroup:MTLSizeMake(16,1,1)];
        [encoder endEncoding]; [command commit]; [command waitUntilCompleted];
        if (command.status!=MTLCommandBufferStatusCompleted) { std::cerr << "Metal dispatch failed\n"; return 1; }
        const float* output=static_cast<const float*>(b.contents);
        for (int i=0;i<16;++i) if (output[i]!=input[i]+1) { std::cerr << "Metal increment mismatch\n"; return 1; }
        std::cout << "PASS: standalone Metal input[i]+1 on " << device.name.UTF8String << '\n'; return 0;
    }
}

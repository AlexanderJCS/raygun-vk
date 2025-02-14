#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require
#include "shaderCommon.h.glsl"
#include "../polyglot/common.h"

// Binding BINDING_IMAGEDATA in set 0 is a storage image with four 32-bit floating-point channels,
// defined using a uniform image2D variable.
layout(binding = 0, set = 0, rgba32f) uniform image2D storageImage;
layout(binding = 1, set = 0) uniform accelerationStructureEXT tlas;

// Ray payloads are used to send information between shaders.
layout(location = 0) rayPayloadEXT PassableInfo pld;

layout (push_constant) uniform PushConsts {
    PushConstantsStruct pushConstants;
};

// Uses the Box-Muller transform to return a normally distributed (centered
// at 0, standard deviation 1) 2D point.
vec2 randomGaussian(inout uint rngState) {
    // Almost uniform in (0, 1] - make sure the value is never 0:
    const float u1 = max(1e-5, stepAndOutputRNGFloat(rngState));
    const float u2 = stepAndOutputRNGFloat(rngState);  // In [0, 1]
    const float r = sqrt(-2.0 * log(u1));
    const float theta = 2 * k_pi * u2;  // Random in [0, 2pi]
    return r * vec2(cos(theta), sin(theta));
}

vec3 traceSegments(vec3 origin, vec3 direction) {
    vec3 accumulatedRayColor = vec3(1.0);
    vec3 incomingLight = vec3(0.0);

    for (int tracedSegments = 0; tracedSegments < 32; tracedSegments++) {
        traceRayEXT(
            tlas,                  // Top-level acceleration structure
            gl_RayFlagsOpaqueEXT,  // Ray flags, here saying "treat all geometry as opaque"
            0xFF,                  // 8-bit instance mask, here saying "trace against all instances"
            0,                     // SBT record offset
            0,                     // SBT record stride for offset
            0,                     // Miss index
            origin,                // Ray origin
            0.0,                   // Minimum t-value
            direction,             // Ray direction
            10000.0,               // Maximum t-value
            0                      // Location of payload
        );

        if (pld.rayHitSky) {
            incomingLight += pld.color * accumulatedRayColor;
            break;
        }

        incomingLight += pld.emission.xyz * pld.emission.w * accumulatedRayColor;
        accumulatedRayColor *= pld.color;

        origin = pld.rayOrigin;
        direction = pld.rayDirection;
    }

    return incomingLight;
}

vec3 computeSample(vec2 pixel, vec2 resolution, vec3 cameraOrigin, float fovVerticalSlope) {
    vec3 rayOrigin = cameraOrigin;

    const vec2 randomPixelCenter = vec2(pixel) + vec2(0.5) + 0.375 * randomGaussian(pld.rngState);
    const vec2 screenUV = vec2(
        (2.0 * randomPixelCenter.x - resolution.x) / resolution.y,
        (2.0 * randomPixelCenter.y - resolution.y) / resolution.y * -1
    );

    // Create a ray direction:
    vec3 rayDirection = vec3(fovVerticalSlope * screenUV.x, fovVerticalSlope * screenUV.y, -1.0);
    rayDirection = normalize(rayDirection);

    return traceSegments(rayOrigin, rayDirection);
}

void main() {
    const ivec2 resolution = imageSize(storageImage);
    const ivec2 pixel = ivec2(gl_LaunchIDEXT.xy);

    if ((pixel.x >= resolution.x) || (pixel.y >= resolution.y)) {
        return;
    }

    // State of the random number generator with an initial seed
    pld.rngState = uint((pushConstants.sampleBatch * resolution.y + pixel.y) * resolution.x + pixel.x);

    const vec3 cameraOrigin = vec3(0, 1, 10);
    const float fovVerticalSlope = 1.0 / 5;

    const int NUM_SAMPLES = 64;
    int actualSamples = 0;
    vec3 summedPixelColor = vec3(0.0);

    for (int sampleIdx = 0; sampleIdx < NUM_SAMPLES; sampleIdx++) {
        vec3 color = computeSample(pixel, resolution, cameraOrigin, fovVerticalSlope);

        if (any(isnan(color))) {
            continue;
        }

        actualSamples++;
        summedPixelColor += color;
    }

    vec3 finalColor = summedPixelColor / float(actualSamples);

    if (pushConstants.sampleBatch > 0) {
        vec3 prevColor = imageLoad(storageImage, pixel).rgb;
        finalColor = (prevColor * pushConstants.sampleBatch + finalColor) / float(pushConstants.sampleBatch + 1);
    }

    imageStore(storageImage, pixel, vec4(finalColor, 1));
}
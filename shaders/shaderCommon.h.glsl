#ifndef VK_MINI_PATH_TRACER_SHADER_COMMON_H
#define VK_MINI_PATH_TRACER_SHADER_COMMON_H

// define this to show normals on lambertian surfaces
// #define DEBUG_SHOW_NORMALS

struct PassableInfo
{
    vec3 color;         // The reflectivity of the surface.
    vec3 rayOrigin;     // The new ray origin in world-space.
    vec3 rayDirection;  // The new ray direction in world-space.
    uint rngState;      // State of the random number generator.
    bool rayHitSky;     // True if the ray hit the sky.
    vec4 emission;      // xyz: emission color, w: emission strength
    bool skip;          // If true, the raygen shader knows to skip this ray
};

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uint rngState) {
    // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
    rngState  = rngState * 747796405 + 1;
    uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
    word = (word >> 22) ^ word;
    return float(word) / 4294967295.0f;
}

const float k_pi = 3.14159265;

#endif  // #ifndef VK_MINI_PATH_TRACER_SHADER_COMMON_H
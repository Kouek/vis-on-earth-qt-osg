#version 430 core

#define SkipAlpha (.95f)

layout(binding = 0, rgba32f) uniform image2D colorImage;
layout(binding = 1, rgba32f) uniform image2D normalImage;
layout(binding = 2, rgba32f) uniform image2D depthImage;
layout(binding = 3, rgba16f) uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform int kernelSize;
uniform bool useSSAO;

float radius = 0.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1.0/4.0, 1.0/4.0);

void main() {
    float alpha = imageLoad(normalImage, ivec2(gl_FragCoord.xy)).a;
    if (!useSSAO || alpha < SkipAlpha) {
        gl_FragColor = imageLoad(colorImage, ivec2(gl_FragCoord.xy));
        return;
    }

    vec3 normal = imageLoad(normalImage, ivec2(gl_FragCoord.xy)).rgb;
    float depth = imageLoad(depthImage, ivec2(gl_FragCoord.xy)).r;

    vec3 randomVec = normalize(texture(texNoise, gl_FragCoord.xy * noiseScale).rgb);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i) {
        // get sample position
        vec3 samplePos = TBN * samples[i];// from tangent to view-space
        samplePos = samplePos * radius + vec3(gl_FragCoord.xy, depth);

        // get sample depth
        float sampleDepth = imageLoad(depthImage, ivec2(samplePos.xy)).r;

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(depth - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    gl_FragColor = imageLoad(colorImage, ivec2(gl_FragCoord.xy)) * vec4(occlusion);
}

#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba8ui, binding = 0) uniform uimage2D imgOutput;
uniform sampler2D volume;
uniform sampler1D transferFunction;

void main() {
    vec2 xy = vec2(gl_GlobalInvocationID.xy) / (imageSize(imgOutput) - 1);
    if (xy.x > 1.0 || xy.y > 1.0) return;
    xy.y = 1.0 - xy.y;// flip y-axis

    float scalar = texture(volume, xy).r;
    vec4 color = texture(transferFunction, scalar);

    imageStore(imgOutput, ivec2(gl_GlobalInvocationID.xy), uvec4(color.rgb * 255.0, 255));
}
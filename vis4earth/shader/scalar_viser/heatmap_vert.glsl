#version 130

uniform sampler1D tfTex;
uniform sampler2D volSliceTex;
uniform int volHeight;
uniform int height;
uniform int heightMapMode;
uniform float latitudeMin;
uniform float latitudeMax;
uniform float longtitudeMin;
uniform float longtitudeMax;
uniform float heightMin;
uniform float heightMax;

out vec3 vertex;
out vec3 color;

void main() {
    float scalar = texture(volSliceTex, gl_MultiTexCoord0.xy).r;
    vec4 rgba = texture(tfTex, scalar);
    color = rgba.rgb;

    float heightOffs = 0.f;
    if (heightMapMode == 1) // Surface
        heightOffs = rgba.a;
    else if (heightMapMode == 2) {
        // Pillar
        int subID = int(floor(gl_Vertex.z));
        if (subID % 2 == 1)
            heightOffs = rgba.a;
    }

    {
        float lon = longtitudeMin + gl_Vertex.x * (longtitudeMax - longtitudeMin);
        float lat = latitudeMin + gl_Vertex.y * (latitudeMax - latitudeMin);
        float h = heightMin + (1.f * height / volHeight + heightOffs) * (heightMax - heightMin);
        vertex.z = h * sin(lat);
        h *= cos(lat);
        vertex.y = h * sin(lon);
        vertex.x = h * cos(lon);
    }

    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.f);
}

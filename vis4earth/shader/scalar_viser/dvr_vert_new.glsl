#version 430 core

uniform mat4 MVP;

uniform float latitudeMin;
uniform float latitudeMax;
uniform float longtitudeMin;
uniform float longtitudeMax;
uniform float heightMin;
uniform float heightMax;

layout(location = 0) in vec3 inVertex;

out vec3 vertex;

void main() {
    {
        float lon = longtitudeMin + inVertex.x * (longtitudeMax - longtitudeMin);
        float lat = latitudeMin + inVertex.y * (latitudeMax - latitudeMin);
        float h = heightMin + inVertex.z * (heightMax - heightMin);
        vertex.z = h * sin(lat);
        h *= cos(lat);
        vertex.y = h * sin(lon);
        vertex.x = h * cos(lon);
    }

    gl_Position = MVP * vec4(vertex, 1.f);
}

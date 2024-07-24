#version 130 core

out vec3 vertex;
out vec3 normal;

void main() {
    vertex = gl_Vertex.xyz;
    normal = gl_Normal;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
#version 130 core

in vec3 vertex;
in vec3 normal;

void main() {
	gl_FragColor = vec4(abs(normal), 1); 
}
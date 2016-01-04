#version 430 core

in vec3 fNormal;

out vec4 FragColor;

vec3 encodeNormal(vec3 normal) {
    return (normalize(normal) + 1.0) * 0.5;
}

void main() {
	FragColor = vec4(encodeNormal(fNormal), 1);
}
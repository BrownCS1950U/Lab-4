#version 410 core

// Vertex attributes from VBO
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

// Uniform (Matrix)
uniform mat4 uMVP;

// Outputs for the fragment shader
out vec3 m_normal;
out vec4 m_vertex;
out vec2 m_texcoord;

void main() {
	gl_Position = uMVP * vec4(position, 1.0);
	m_normal = normal;
	m_vertex = vec4(position, 1.0);
	m_texcoord = texcoord;
}

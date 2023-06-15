#version 330

in vec4 color;

layout(location = 0) out vec4 frag_out0;

void main() { frag_out0 = color; }
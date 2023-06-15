#version 330

in vec4 edge_coords[];
// in float inBox[];
// out float isInBox;

layout(lines) in;
layout(line_strip, max_vertices = 2) out;

void main() {
  // isInBox = inBox[0];

  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  gl_Position = edge_coords[0];
  EmitVertex();

  EndPrimitive();
}
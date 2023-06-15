#version 330

uniform vec4 color;

// in float isInBox;

layout(location = 0) out vec4 frag_out0;

void main() {
  // frag_out0 = vec4(color.rgb, 0.05 + color.a * 0.95 * isInBox);
  frag_out0 = color;
}
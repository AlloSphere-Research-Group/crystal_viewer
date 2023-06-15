#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
uniform float scale;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 offset;
layout(location = 2) in vec4 colors;

out vec4 color;

// float insideBox(vec3 v) {
//     vec3 s = step(boxMin, v) - step(boxMax, v);
//     return s.x * s.y * s.z;
// }

void main() {
  vec4 p = vec4(scale * position + offset, 1.0);
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * p;

  // float ib = insideBox(offset);
  color = colors;
}
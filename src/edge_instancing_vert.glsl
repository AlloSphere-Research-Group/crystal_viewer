#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 start_offset;
layout(location = 2) in vec3 end_offset;

out vec4 edge_coords;
// out float inBox;

// float insideBox(vec3 v) {
//     vec3 s = step(boxMin, v) - step(boxMax, v);
//     return s.x * s.y * s.z;
// }

void main() {
  vec4 p = vec4(position + start_offset, 1.0);
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * p;

  vec4 p_end = vec4(position + end_offset, 1.0);

  edge_coords = al_ProjectionMatrix * al_ModelViewMatrix * p_end;
  // inBox = insideBox(offset) * insideBox(end_offset);
}
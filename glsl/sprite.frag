#version 430

#define PI 3.14159265359
#define SQRT2 0.70710678118654757

uniform vec2 window;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;

out vec4 out_col;

// Anti-aliased smoothstep.
float aastep(float lim, float val) {
  float diff = length(vec2(dFdx(val), dFdy(val))) * SQRT2;
  return smoothstep(lim - diff, lim + diff, val);
}

void main() {
  vec2 uvn = uv;
  float mag = 1.f - aastep(0.8f, length(uvn));
  out_col = color*mag;
}


#version 430

#define PI    3.14159265358979323
#define SQRT2 0.70710678118654757

uniform sampler2D tex;
uniform vec2 window;
uniform vec3 paper_col;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 frag;

// 2D hash.
float rand(vec2 co) {
  return fract(sin(dot(co.xy, vec2(12.9898,78.233))) *43758.5453);
}

// Film grain.
float grain(vec2 uv, float val) {
  return 1.f + (rand(uv - 0.5f)) * val;
}

// Anti-aliased smoothstep.
float aastep(float lim, float val) {
  float diff = length(vec2(dFdx(val), dFdy(val)))*SQRT2;
  return smoothstep(lim - diff, lim + diff, val);
}

// Rotate 2D vector.
vec2 rotate(vec2 p, float a) {
  return vec2(p.x*cos(a) - p.y*sin(a), p.y*cos(a) + p.x*sin(a));
}

// Render skewed rectangle.
float quad(vec2 p, vec2 sz, vec2 ang) {
  vec2 p1 = rotate(p, ang.x);
  float res = aastep(sz.x*0.5f, p1.x);
  res = max(res, aastep(sz.x*0.5f, -p1.x));

  vec2 p2 = rotate(p, ang.y);
  res = max(res, aastep(sz.y*0.5f, p2.x));
  res = max(res, aastep(sz.y*0.5f, -p2.x));
  return res;
}

// Three skewed rectangles to create a 3D microwave cavity.
float cavity(vec2 p, vec2 sz, float ang, float gap, float ygap) {
  float height = 0.4f;
  float diag = sz.x / cos(ang);
  float depth = diag * sin(ang*2.f);
  float off = sz.x * tan(ang);

  float res = quad(p + vec2(sz.x*0.5f+gap*0.5f, 0.f), sz, vec2(0.f, PI*0.5f+ang));
  res *= quad(p - vec2(sz.x*0.5f+gap*0.5f, 0.f), sz, vec2(0.f, PI*0.5f-ang));
  res *= quad(p - vec2(0.f, sz.y*0.5f+off*0.5f + ygap), vec2(depth, depth), vec2(PI*0.5f+ang, PI*0.5f-ang));
  return res;
}

void main() {
  vec2 uvn = uv*2.f-1.f;
  uvn.x *= window.x/window.y;

  vec2 uvinv = vec2(1.f-uv.x, uv.y); // Mirror image.
  vec4 raw = texture(tex, uvinv);

  vec3 col = paper_col;

  col *= 1.f - length(uvn) * 0.3f;
  col *= grain(uvn, 0.01f);

  float mag = cavity(uvn + vec2(0.f, 0.15f), vec2(0.4f, 0.15f), PI*0.18f, 0.02f, 0.03f);
  frag = mix(raw, vec4(col, 1.f), 0.1f + 0.9f*mag);
}

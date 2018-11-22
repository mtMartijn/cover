#version 430

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_col;

uniform vec2 window;
uniform sampler2D tex;
uniform vec2 dir;
uniform float radius;

void main() {
  float rad = radius/window.y;
  vec4 res = vec4(0.f);
  res += 6.f*texture(tex, vec2(in_uv.x - 2.f*dir.x*rad, in_uv.y - 2.f*dir.y*rad));
  res += 15.f*texture(tex, vec2(in_uv.x - 1.f*dir.x*rad, in_uv.y - 1.f*dir.y*rad));
  res += 20.f*texture(tex, in_uv);
  res += 15.f*texture(tex, vec2(in_uv.x + 1.f*dir.x*rad, in_uv.y + 1.f*dir.y*rad));
  res += 6.f*texture(tex, vec2(in_uv.x + 2.f*dir.x*rad, in_uv.y + 2.f*dir.y*rad));
  out_col = res/62.f;
}

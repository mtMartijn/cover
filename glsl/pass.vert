#version 430

layout(location = 0) in vec2 in_pos;

layout(location = 0) out vec2 out_box;

void main() {
  out_box = in_pos;
  gl_Position = vec4(in_pos*2.f-1.f, 0.f, 1.f);
}

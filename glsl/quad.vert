#version 430

uniform vec2 window;
uniform mat4 mv;
uniform mat4 proj;
uniform uint rotation;

layout(location = 0) in vec2 box;
layout(location = 1) in vec4 pos;
layout(location = 2) in vec4 cola;
layout(location = 3) in vec4 colb;
layout(location = 4) in vec4 attr;
layout(location = 5) in vec4 dir;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;

// These constants are necessary for continuity of luminance.
const float eps = 216.0/24389.0;
const float kap = 24389.0/27.0;
const vec2 D65 = vec2(0.31272, 0.32903);

// Convert XYZ to RGB colorspace.
vec3 xyz2rgb(vec3 c) {
  const mat3 cmat = mat3(
      3.24045, -1.53714, -0.49853,
      -0.96927, 1.87601, 0.04156,
      0.05564, -0.20403, 1.05723);
  return c * cmat;
}

// Convert CIELAB to XYZ colorspace.
vec3 lab2xyz(vec3 col) {
  float L = col.x;
  float a = col.y;
  float b = col.z;

  float fy = (L + 16.f) / 116.f;
  float fx = fy + a / 500.f;
  float fz = fy - b / 200.f;

  float y = (L > kap * eps) ? fy * fy * fy : L / kap;
  float x = (fx * fx * fx > eps) ? fx * fx * fx : (116.f * fx - 16.f) / kap;
  float z = (fz * fz * fz > eps) ? fz * fz * fz : (116.f * fz - 16.f) / kap;

  vec3 illum = vec3(D65.x, D65.y, 1.f - D65.x - D65.y) * (1.f / D65.y);
  return illum * vec3(x, y, z);
}

// Convert CIELAB to RGB colorspace.
vec3 lab2rgb(vec3 c) {
  return xyz2rgb(lab2xyz(c));
}

// Cylindrical to cartesian coordinates.
vec3 cart(vec3 c) {
  const float pi = 3.14159265359;
  float C = c.y;
  float hue = pi*c.z/180;
  return vec3(c.x, C*cos(hue), C*sin(hue));
}

// Rotate billboard.
vec3 rotate(vec3 pos, vec4 quat) {
    float s = quat.w;
    vec3 u = quat.xyz;
    return 2.f * dot(u, pos) * u + (s*s - dot(u, u)) * pos + 2.f * s * cross(pos, u);
}

void main() {
  float size = pos.w;
  vec3 uvc = vec3(box*2.f - 1.f, 0.f);
  vec3 quad = uvc * size;
  vec3 rotated = rotate(quad, dir);

  vec4 p;
  if (rotation) {
    p = mv * vec4(pos.xyz + rotated, 1.f);
  } else {
    vec4 eye_pos = mv * vec4(pos.xyz, 1.f);
    p = vec4(eye_pos.xyz + rotated, eye_pos.w);
  }
  gl_Position = proj * p;

  vec3 rgb = lab2rgb(cart(cola.xyz));
  float ad = clamp(1.f - colb.w, 0.f, 1.f);
  float op = clamp(cola.w, 0.f, 100.f);

  color = vec4(rgb, ad) * op; // Premultiplied alpha.
  uv = uvc.xy;
}

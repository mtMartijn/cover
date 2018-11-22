#version 430

#define SQRT2 0.70710678118654757

uniform vec2 window;
uniform vec3 sky_col;
uniform vec3 sun_pos;
uniform vec3 sun_col;
uniform vec3 cloud_col;
uniform float cloud_strength;
uniform float sun_radius;
uniform float sun_strength;
uniform float waterline;
uniform float exposure;
uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_col;

// 2D hash.
float rand(vec2 co) {
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// Lens vignette.
float vignette(vec2 center, vec2 uv, float rad) {
  return 1.f + length(uv-center)*rad;
}

// Film grain.
float grain(vec2 uv, float val) {
  return 1. + (rand(uv) - 0.5) * val;
}

// Anti-aliased smoothstep.
float aastep(float lim, float val) {
  float diff = length(vec2(dFdx(val), dFdy(val)))*SQRT2;
  return smoothstep(lim + diff, lim - diff, val);
}

// Hash for gradient noise. Replace with any hash that outputs to vec2[-1,1].
vec2 ghash(vec2 x) {
  const vec2 k = vec2(0.3183099, 0.3678794);
  x = x*k + k.yx;
  return -1.0 + 2.0*fract(16.0*k*fract(x.x*x.y*(x.x+x.y)));
}

// Gradient noise. From: https://www.shadertoy.com/view/XdXGW8.
float gnoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
	vec2 u = f*f*(3.0-2.0*f);

  return mix(mix(dot(ghash(i + vec2(0.0,0.0)), f - vec2(0.0,0.0)),
                 dot(ghash(i + vec2(1.0,0.0)), f - vec2(1.0,0.0)), u.x),
             mix(dot(ghash(i + vec2(0.0,1.0)), f - vec2(0.0,1.0)),
                 dot(ghash(i + vec2(1.0,1.0)), f - vec2(1.0,1.0)), u.x), u.y);
}

// Hash for value noise. Replace with any hash that outputs to [-1,1].
float vhash(vec2 p) {
  p = 50.0*fract(p*0.3183099 + vec2(0.71,0.113));
  return -1.0 + 2.0*fract(p.x*p.y*(p.x+p.y));
}

// Value noise. From: https://www.shadertoy.com/view/lsf3WH.
float vnoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
	vec2 u = f*f*(3.0-2.0*f);

  return mix(mix(vhash(i + vec2(0.0,0.0)),
                 vhash(i + vec2(1.0,0.0)), u.x),
             mix(vhash(i + vec2(0.0,1.0)),
                 vhash(i + vec2(1.0,1.0)), u.x), u.y);
}

// Pink gradient noise with tuneable frequency dependence.
float pgnoise(vec2 uv, float alpha, uint n) {
  const mat2 m = mat2(1.6, 1.2, -1.2, 1.6); // Rotate and double size.
  float amp = 1.f;
  float norm = 1.f;
  float f = amp*gnoise(uv);
  while (--n) {
    amp *= alpha;
    norm += amp;
    uv = m*uv;
    f += amp*gnoise(uv);
  }
  return f/norm;
}

// Linear gradient between two points.
float lingradient(vec2 uv, vec2 startp, vec2 endp) {
  float alpha = atan(-endp.y + startp.y, endp.x - startp.x);
  float startx = startp.x*cos(alpha) - startp.y*sin(alpha);
  float endx = endp.x*cos(alpha) - endp.y*sin(alpha);
  float xrot = uv.x*cos(alpha) - uv.y*sin(alpha);
  return smoothstep(startx, endx, xrot);
}

// "Cinematic" tone-map function.
vec3 cine_map(vec3 x) {
  float A = 0.15f, B = 0.5f, C = 0.1f, D = 0.2f, E = 0.02f, F = 0.3f, W = 11.2f;
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// Naive tone-map function.
vec3 tone_map(vec3 x) {
  return 1.f - exp(-x);
}

// Render stars.
float stars(vec2 uv) {
  const mat2 rot = mat2(7.f, 3.f, 6.f, 5.f);
  vec2 s = floor(uv);
  vec2 f = fract(uv);
	vec2 p = 0.5f + 0.35f*sin(11.*fract(sin(s*rot)*5.f))-f;
  float d = length(p);
	return smoothstep(0.f, d, sin(f.x+f.y)*0.01f);
}

// Apply gamma function.
vec3 gamma(vec3 c) {
  vec3 r;
  r.x = ( c.r > 0.0031308 ) ? (( 1.055 * pow( c.r, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * c.r;
  r.y = ( c.g > 0.0031308 ) ? (( 1.055 * pow( c.g, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * c.g;
  r.z = ( c.b > 0.0031308 ) ? (( 1.055 * pow( c.b, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * c.b;
  return r;
}

void main() {
  vec2 uv = in_uv * 2.0 - 1.0;
  uv.x *= window.x/float(window.y);
  vec2 uvc = in_uv;
  uvc.x *= window.x/window.y;

  // Coordinate manipulations.
  vec2 inuvc = in_uv * 2.f - 1.f;
  float offset = waterline;
  float under = aastep(offset, uv.y);
  inuvc.y = abs(inuvc.y - offset) + offset;
  inuvc = inuvc*0.5f + 0.5f;
  vec2 ns_uv = in_uv*8.f;
  ns_uv.y *= 8.f;
  float ns = pgnoise(ns_uv, 0.5f, 4);
  vec2 nudge = 0.12f*under*vec2(ns, 0.f);
  inuvc += inuvc.y*nudge;

  vec4 raw = texture(tex, inuvc);
  inuvc.x *= window.x/window.y;

  vec3 col = sky_col;
  float dist = length(sun_pos.xy - inuvc);
  dist = pow(dist, 3.f);
  col += pow(stars(inuvc*100.f + ns*200.f)*dist*20.f, 1.f);

  // Clouds.
  vec2 uvcl = inuvc;
  uvcl.y = 1.f / uvcl.y;

  // 2D Clouds: https://www.shadertoy.com/view/4tdSWr
  float scale = 5.1f;
  float cloudcover = 0.001f;
  float cloudalpha = 20.f;
  float clouddark = 0.0f;
  float cloudlight = 0.6f;
  float skytint = 0.9f;
  const mat2 m = mat2(1.6, 1.2, -1.2, 1.6); // Rotate and double size.

  // Ridged noise shape.
  float r = 0.f;
  vec2 uvcln = uvcl * scale;
  float weight = 0.8f;
  for (int i = 0; i < 8; ++i) {
    r += abs(weight*gnoise(uvcln));
    uvcln *= m;
    weight *= 0.7f;
  }

  // Noise shape.
  float f = 0.f;
  uvcln = uvcl * scale;
  weight = 0.7f;
  for (int i = 0; i < 8; ++i) {
    f += weight*gnoise(uvcln);
    uvcln *= m;
    weight *= 0.6f;
  }
  f *= r + f;

  // Noise colour.
  float c = 0.f;
  uvcln = uvcl * scale * 2.f;
  weight = 0.4f;
  for (int i = 0; i < 7; ++i) {
    c += weight*gnoise(uvcln);
    uvcln *= m;
    weight *= 0.6f;
  }

  // Noise ridge colour.
  float c1 = 0.f;
  uvcln = uvcl * scale * 3.f;
  weight = 0.4f;
  for (int i = 0; i < 7; ++i) {
    c1 += abs(weight*gnoise(uvcln));
    uvcln *= m;
    weight *= 0.6f;
  }
  c += c1;
  f = cloudcover + cloudalpha*f*r;
  vec3 cloudcolour = cloud_col * clamp((clouddark + cloudlight*c), 0.0, 1.0);
  vec3 cloudres = mix(col, clamp(skytint * col + cloudcolour, 0.f, 1.f), clamp(f+c, 0.f, 1.f));
  col = mix(col, cloudres, cloud_strength);

  // Sun.
  vec2 diff = sun_pos.xy - inuvc;
  float ang = atan(diff.y, diff.x);
  float len = length(diff);
  float angn = pgnoise(vec2(0.f, 2.f*ang), 0.8f, 4);
  col += (1.f + 0.7f*len*angn) * sun_strength * sun_col / max(len, sun_radius);

  // Tree. Manually blended.
  col += raw.rgb + (1.f - raw.a)*col;

  // Post.
  float v = vignette(sun_pos.xy, inuvc, -0.5f);
  col *= pow(v, 4.f);
  col *= (1.f - 0.02f*under);
  col *= grain(uv, 0.06f);

  col = tone_map(col*exposure);
  out_col = vec4(col, 1.0);
}

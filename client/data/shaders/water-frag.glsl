#version 300 es
precision highp float;

// water: waves bend the ground seen through it (refraction) and the mirror image of the scenery
// (reflection), mixed by the viewing angle; deep water takes on its own colour, foam where it's
// shallow and along walls, a glint of the sun on the waves

// red: depth (0..1 = 0..0.8 tiles), green: foam
uniform sampler2D u_data;
uniform sampler2D u_reflection;
uniform sampler2D u_refraction;

uniform float u_time;
uniform vec3 u_eye;
uniform vec3 u_deep;

// the air, as for the glTF scenery: the haze's rgb and strength, the box it's measured from, near and far
uniform vec4 u_haze;
uniform highp vec4 u_haze_box;
uniform vec2 u_haze_range;

// the viewport in window pixels
uniform vec2 u_origin;
uniform vec2 u_size;

in vec3 world;
in vec2 uv;

out vec4 o_color;

const vec3 LIGHT = normalize(vec3(-0.35, -0.55, 0.75));

float hash(vec2 p)
{
   return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
   vec2 i = floor(p);
   vec2 f = fract(p);
   vec2 u = f * f * (3.0 - 2.0 * f);
   return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

// slope of one travelling wave
vec2 wave(vec2 p, vec2 direction, float frequency, float speed, float amplitude)
{
   float phase = dot(p, direction) * frequency + u_time * speed;
   return direction * cos(phase) * frequency * amplitude;
}

void main()
{
   vec2 p = world.xy;
   vec2 slope = wave(p, normalize(vec2(1.0, 0.3)), 2.1, 1.3, 0.035) + wave(p, normalize(vec2(-0.4, 1.0)), 3.3, 1.9, 0.022) +
                wave(p, normalize(vec2(0.8, -0.7)), 5.7, 2.6, 0.012) + wave(p, normalize(vec2(-1.0, -0.2)), 9.1, 3.4, 0.006);
   slope += (vec2(noise(p * 4.0 + u_time * 0.4), noise(p * 4.0 - u_time * 0.35 + 7.0)) - 0.5) * 0.06;
   vec3 n = normalize(vec3(-slope, 1.0));

   vec4 data = texture(u_data, uv);
   float depth = data.r;
   vec2 screen = (gl_FragCoord.xy - u_origin) / u_size;

   // the ground below, bent by the waves, deeper water swallows it
   vec3 refraction = texture(u_refraction, screen + n.xy * 0.025 * (0.3 + depth)).rgb;
   refraction = mix(refraction * vec3(0.85, 0.95, 1.0), u_deep, smoothstep(0.05, 0.9, depth) * 0.8);

   vec3 reflection = texture(u_reflection, screen + n.xy * 0.035).rgb;

   vec3 view = normalize(u_eye - world);
   float facing = max(dot(view, n), 0.0);
   float fresnel = 0.2 + 0.8 * pow(1.0 - facing, 4.0);
   vec3 color = mix(refraction, reflection, clamp(fresnel * 1.6, 0.0, 0.85));

   // the sun on the waves
   vec3 half_vector = normalize(LIGHT + view);
   color += vec3(1.0, 0.97, 0.9) * pow(max(dot(n, half_vector), 0.0), 160.0) * 0.9;

   // foam, broken up and drifting
   float churn = noise(p * 3.5 + vec2(u_time * 0.25, -u_time * 0.2)) * 0.6 + noise(p * 9.0 - u_time * 0.5) * 0.4;
   float foam = smoothstep(0.45, 0.8, data.g * (0.55 + 0.7 * churn));
   color = mix(color, vec3(0.92, 0.96, 1.0), foam * 0.85);

   if (u_haze.a > 0.0)
   {
      highp vec2 outside = max(max(u_haze_box.xy - world.xy, world.xy - u_haze_box.zw), vec2(0.0));
      color = mix(color, u_haze.rgb, smoothstep(u_haze_range.x, u_haze_range.y, length(outside)) * u_haze.a);
   }

   o_color = vec4(color, 1.0);
}

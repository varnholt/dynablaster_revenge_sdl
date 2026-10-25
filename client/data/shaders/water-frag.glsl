#version 300 es
precision highp float;

// water: waves bend the ground seen through it (refraction) and the mirror image of the scenery
// (reflection), mixed by the viewing angle; deep water takes on its own colour, foam where it's
// shallow and along walls, a glint of the sun on the waves

// red: depth (0..1 = 0..0.8 tiles), green: foam, blue: the distance to the shore (0.02..1 = 0..SHORE_REACH
// tiles, 0 where the texture doesn't have it: the foam then stays as it was, without lapping and foam lines)
uniform sampler2D u_data;
uniform sampler2D u_reflection;
uniform sampler2D u_refraction;

uniform float u_time;
uniform vec3 u_eye;
uniform vec3 u_deep;

// units a second the water streams past along x, 0 for still water
uniform float u_flow;

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
const float SHORE_REACH = 3.0;

// turns each octave of noise against the last so the cells of the value noise don't line up into a grid
const mat2 TURN = mat2(0.8, -0.6, 0.6, 0.8);

float hash(vec2 p)
{
   return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
   vec2 i = floor(p);
   vec2 f = fract(p);
   // quintic fade: with the cubic one the cells' borders crease, sharpened into foam they show as squares
   vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
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
   // flowing water carries the churn along, stretched into streaks, and swells the foam in passing waves
   vec2 drift = vec2(u_flow * u_time, 0.0);
   vec2 q = (p - drift) * vec2(1.0 / (1.0 + 0.35 * abs(u_flow)), 1.0);
   float churn = noise(q * 3.5 + vec2(u_time * 0.25, -u_time * 0.2)) * 0.5 + noise(TURN * q * 7.0 - u_time * 0.4) * 0.3 +
                 noise(TURN * TURN * q * 13.0 + u_time * 0.5) * 0.2;
   float swell = 1.0 + 0.35 * sin(p.x * 1.3 - u_time * u_flow * 1.3) * step(0.01, abs(u_flow));

   // knowing how far the shore is, the foam comes alive: it laps against the shore, swelling out and drawing
   // back in a slow beat that wanders along the shore so it never pulses in unison, its outer edge frays and
   // shifts, and thin broken lines of foam leave it and drift outwards, fading as they go
   float alive = step(0.01, data.b);
   float shore = max(data.b - 0.02, 0.0) / 0.98 * SHORE_REACH;
   float beat = noise(p * 0.3) * 6.2832 + noise(p * 0.11 + 5.0) * 6.2832;
   float lap = sin(u_time * 0.6 + beat);
   swell *= 1.0 + alive * 0.4 * lap;
   // a soft edge, frayed by slow noise, rather than the hard rim of the foam mask
   float fray = (noise(q * 1.3 + vec2(u_time * 0.06, -u_time * 0.05)) - 0.5) * 0.3 * alive;
   float foam = smoothstep(0.45 - 0.1 * alive, 0.8 + 0.05 * alive, data.g * swell * (0.55 + 0.7 * churn) + fray);

   // a line is let go each time the band draws back, so the lines keep the beat of the lapping; a soft
   // gaussian profile, bent a little and broken into long pieces so they read as foam, not as stripes
   float travel = shore - u_time * 0.6 / 6.2832 - beat / 6.2832 + (noise(q * 0.9) - 0.5) * 0.35;
   float across = (fract(travel) - 0.5) / 0.07;
   float line = exp(-across * across);
   line *= smoothstep(0.3, 0.75, noise(TURN * q * vec2(1.1, 1.4) + vec2(-u_time * 0.05, u_time * 0.04)));
   line *= smoothstep(0.2, 0.7, shore) * smoothstep(2.2, 0.8, shore) * (1.0 - foam) * alive;
   foam = max(foam, line * 0.35);

   // the inside of the band isn't one sheet of white, it thins out here and there and fills in again
   float thin = noise(TURN * q * 3.0 + vec2(u_time * 0.15, u_time * 0.1)) * 0.6 + noise(TURN * TURN * q * 6.5 - u_time * 0.2) * 0.4;
   foam *= 1.0 - alive * 0.2 * smoothstep(0.4, 0.8, thin);
   color = mix(color, vec3(0.92, 0.96, 1.0), foam * 0.85);

   if (u_haze.a > 0.0)
   {
      highp vec2 outside = max(max(u_haze_box.xy - world.xy, world.xy - u_haze_box.zw), vec2(0.0));
      color = mix(color, u_haze.rgb, smoothstep(u_haze_range.x, u_haze_range.y, length(outside)) * u_haze.a);
   }

   o_color = vec4(color, 1.0);
}

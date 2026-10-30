#version 300 es
precision highp float;
precision highp int;

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

// the world's sun, as for the glTF scenery: towards it (z-up) and its colour
uniform vec3 u_sun_direction;
uniform vec3 u_sun;

// who stands in the water: xy where, zw how fast they walk (tiles a second), u_wader_count of them
const int MAX_WADERS = 16;
uniform vec4 u_waders[MAX_WADERS];
uniform float u_wader_count;

// the viewport in window pixels
uniform vec2 u_origin;
uniform vec2 u_size;

in vec3 world;
in vec2 uv;

out vec4 o_color;

const float SHORE_REACH = 3.0;

// turns each octave of noise against the last so the cells of the value noise don't line up into a grid
const mat2 TURN = mat2(0.8, -0.6, 0.6, 0.8);

// a random 0..1 for each lattice point p (whole numbers), from integer arithmetic (highp int, mediump
// is too narrow for it). The usual fract(sin(dot(p, k)) * 43758.5) doesn't hold up at world positions:
// at arguments of 1e4 and more, sin()'s rounding error, blown up by the 43758, turns into a different
// value whenever the compiler computes a corner a little differently, so two cells no longer agree on
// the corner they share and the noise jumps there: the foam broke into hard edged squares
float hash(vec2 p)
{
   uvec2 x = uvec2(ivec2(p) + 65536);
   uvec2 q = 1103515245u * ((x >> 1u) ^ x.yx);
   uint n = 1103515245u * (q.x ^ (q.y >> 3u));
   return float(n >> 8u) * (1.0 / 16777215.0);
}

float noise(vec2 p)
{
   vec2 i = floor(p);
   vec2 f = fract(p);
   // quintic fade: with the cubic one the cells' borders crease, sharpened into foam they show as squares
   vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
   return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

// how much of a wave of this frequency survives where one pixel covers footprint units: in the
// distance a pixel spans several crests, they'd alias into moiré bands, so they fade out
float resolved(float frequency, float footprint)
{
   return 1.0 - smoothstep(1.0, 3.0, footprint * frequency);
}

// the slope of the sea: many waves, each smaller and shorter than the last and each running its own
// way, with sharp crests and wide flat troughs (exp(sin) rather than a plain sine, which looks like a
// tin roof). Every wave pushes the point along its slope before the next one is taken, so the small
// waves ride on the big ones and crowd together on their crests, as on real water
const int WAVES = 18;
const float WAVE_HEIGHT = 0.7;
const float DRAG = 0.38;

vec2 sea(vec2 p, float footprint)
{
   float angle = 0.0;
   float frequency = 0.8;
   float weight = 1.0;
   float total = 0.0;
   vec2 slope = vec2(0.0);
   for (int i = 0; i < WAVES; ++i)
   {
      vec2 direction = vec2(sin(angle), cos(angle));
      // deep water: a wave's speed goes with the square root of its length
      float x = dot(direction, p) * frequency + u_time * 1.2 * sqrt(frequency);
      float height = exp(sin(x) - 1.0);
      float rise = height * cos(x);
      slope += direction * rise * frequency * weight * resolved(frequency, footprint);
      total += weight;
      p -= direction * rise * weight * DRAG;
      weight *= 0.82;
      frequency *= 1.18;
      // a far apart, unrelated turn for each wave
      angle += 1232.399963;
   }
   return slope / total * WAVE_HEIGHT;
}

// the water round someone's feet: a ring of foam hugging them, ripples running off in rings, behind a walker
// a wake spreading out in a V; returns the foam, adds the ripples' slope
float wading(vec2 p, inout vec2 slope)
{
   float foam = 0.0;
   for (int i = 0; i < MAX_WADERS; ++i)
   {
      if (float(i) >= u_wader_count)
      {
         break;
      }

      vec2 offset = p - u_waders[i].xy;
      float d = length(offset);
      if (d > 2.0)
      {
         continue;
      }

      vec2 velocity = u_waders[i].zw;
      float speed = length(velocity);
      float walking = smoothstep(0.3, 1.5, speed);
      float seed = float(i) * 1.7;

      // the ring at their feet, broken up, pushed out ahead of a walker
      vec2 ahead = speed > 0.01 ? velocity / speed : vec2(0.0);
      float bow = dot(offset / max(d, 0.001), ahead) * walking;
      // a little wider than the feet, the body hides the inner part
      float radius = 0.36 + 0.03 * sin(u_time * 3.0 + seed) + 0.08 * bow;
      float churn = noise(offset * 9.0 + vec2(u_time * 0.8, seed)) * 0.6 + noise(offset * 17.0 - u_time * 1.1) * 0.4;
      float ring = exp(-pow((d - radius) / (0.04 + 0.02 * walking), 2.0)) * (0.3 + 0.35 * churn);
      ring += smoothstep(radius, radius - 0.08, d) * 0.2 * churn;

      // ripples, one going out every second or so, thinning as they spread
      for (int k = 0; k < 2; ++k)
      {
         float age = fract(u_time * (0.7 + 0.3 * walking) + float(k) * 0.5 + seed);
         float at = radius + age * 1.3;
         float across = (d - at) / 0.04;
         float wave = exp(-across * across) * (1.0 - age) * (1.0 - age);
         slope += offset / max(d, 0.001) * wave * -across * 0.05;
         ring += wave * 0.12 * smoothstep(0.35, 0.7, churn);
      }

      // the wake: two arms trailing behind at about twenty degrees, lumpy foam along them
      if (walking > 0.0)
      {
         float behind = -dot(offset, ahead);
         float side = abs(offset.x * ahead.y - offset.y * ahead.x);
         float arm = (side - behind * 0.36) / (0.05 + behind * 0.06);
         float trail = exp(-arm * arm) * smoothstep(0.1, 0.35, behind) * smoothstep(1.8, 0.5, behind);
         float lumps = noise(vec2(behind * 6.0 - u_time * speed * 1.5, side * 8.0 + seed));
         ring += trail * walking * (0.2 + 0.35 * lumps);
         // churned water straight behind
         ring += exp(-side * side / 0.02) * smoothstep(0.2, 0.3, behind) * smoothstep(1.0, 0.3, behind) * walking * churn * 0.2;
      }

      foam = max(foam, ring);
   }
   return clamp(foam, 0.0, 1.0);
}

void main()
{
   vec2 p = world.xy;
   vec2 footprint = fwidth(p);
   float pixel = max(footprint.x, footprint.y);

   // some stretches of the sea are calmer, others choppier
   float rough = 0.6 + 0.8 * noise(TURN * p * 0.06 + vec2(u_time * 0.015, -u_time * 0.01));
   vec2 slope = sea(p, pixel) * rough;
   float waded = wading(p, slope);
   slope += (vec2(noise(p * 4.0 + u_time * 0.4), noise(p * 4.0 - u_time * 0.35 + 7.0)) - 0.5) * 0.04 * resolved(25.0, pixel);
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

   // the sun on the waves: a sharp glint, and around it, where the waves turn the sun's light towards the eye,
   // glitter: a tiny facet of the ripples here and there catches it and flashes for a moment, each in a beat of its
   // own, drifting with the stream; too small to resolve far away they would only flicker, so they fade out there
   vec3 half_vector = normalize(normalize(u_sun_direction) + view);
   float towards = max(dot(n, half_vector), 0.0);
   color += u_sun * pow(towards, 160.0) * 1.1;
   vec2 sparkles = (p - vec2(u_flow * u_time, 0.0)) * 11.0;
   vec2 cell = floor(sparkles);
   float seed = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
   vec2 spot = vec2(fract(seed * 7.13), fract(seed * 13.7)) * 0.6 + 0.2;
   float flash = pow(max(sin(u_time * (1.5 + 2.5 * seed) + seed * 40.0), 0.0), 40.0);
   float glitter = flash * smoothstep(0.22, 0.05, length(fract(sparkles) - spot)) * step(0.45, fract(seed * 3.3));
   glitter *= pow(towards, 8.0) * resolved(11.0, pixel);
   color += mix(u_sun, vec3(1.0), 0.5) * glitter * 4.0;

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
   foam = max(foam, waded * 0.5);
   color = mix(color, vec3(0.92, 0.96, 1.0), foam * 0.85);

   if (u_haze.a > 0.0)
   {
      highp vec2 outside = max(max(u_haze_box.xy - world.xy, world.xy - u_haze_box.zw), vec2(0.0));
      color = mix(color, u_haze.rgb, smoothstep(u_haze_range.x, u_haze_range.y, length(outside)) * u_haze.a);
   }

   o_color = vec4(color, 1.0);
}

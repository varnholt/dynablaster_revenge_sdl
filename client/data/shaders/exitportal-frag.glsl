#version 300 es
precision highp float;

// which piece is drawn, see ExitPortal::Part
uniform int part;

// seconds
uniform float time;

// 0 sealed, 1 open
uniform float openness;

// radius of the iris' opening
uniform float aperture;

in vec3 position;
in vec3 normal;
in vec2 uv;

out vec4 o_color;

const float PI = 3.14159265;
const vec3 LIGHT = normalize(vec3(-0.35, 0.45, 1.0));
const vec3 VIOLET = vec3(0.5, 0.3, 1.0);
const vec3 SEAM = vec3(0.3, 0.6, 1.0);

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

float lit(vec3 n)
{
   return 0.35 + 0.65 * max(dot(normalize(n), LIGHT), 0.0);
}

// seams flare up while the portal opens
float flare()
{
   return sin(clamp(openness, 0.0, 1.0) * PI);
}

void main()
{
   gl_FragDepth = gl_FragCoord.z;

   float angle = atan(position.y, position.x);
   float r = length(position.xy);

   if (part == 0)
   {
      // rim: weathered stone, glowing runes along its crest
      float grain = 0.8 + 0.2 * noise(vec2(angle * 12.0, uv.y * 6.0));
      vec3 stone = vec3(0.32, 0.31, 0.34) * grain * lit(normal);

      float crest = 1.0 - smoothstep(0.0, 0.18, abs(uv.y - 0.5));
      float runes = step(0.55, fract(angle / (2.0 * PI) * 12.0)) * crest;
      float pulse = 0.75 + 0.25 * sin(time * 3.0 - angle * 2.0);
      float glow = runes * (0.25 + 0.75 * openness + 1.5 * flare()) * pulse;

      o_color = vec4(stone + SEAM * glow, 1.0);
   }
   else if (part == 1)
   {
      // funnel: spiral arms twisting into the depth (Black Hole Sun, Jaenam, shadertoy wcjyDK)
      float depth = uv.y;
      float twist = angle * 3.0 + depth * 9.0 - time * 2.5;
      float arms = 0.5 + 0.5 * sin(twist + 1.5 * noise(vec2(angle * 3.0, depth * 4.0 - time)));
      arms = pow(arms, 3.0);

      vec3 wall = mix(vec3(0.06, 0.04, 0.12), vec3(0.01, 0.0, 0.02), depth);
      vec3 emission = VIOLET * arms * (1.6 - depth) * 1.4;
      vec3 core = vec3(0.85, 0.75, 1.0) * exp((depth - 1.0) * 7.0) * 1.5;

      o_color = vec4(min(wall + emission + core, vec3(1.0)), 1.0);
   }
   else if (part == 2)
   {
      // lid: an iris of six blades
      float blade = fract((angle + r * 4.0) / (2.0 * PI) * 6.0);
      float iris_edge = aperture * (1.0 + 0.12 * (blade - 0.5));

      if (r < iris_edge)
      {
         discard;
      }

      float seam = 1.0 - smoothstep(0.0, 0.06, min(blade, 1.0 - blade));
      float ring = 1.0 - smoothstep(0.0, 0.015, abs(r - 0.3));
      float grain = 0.85 + 0.15 * noise(position.xy * 30.0);
      vec3 stone = vec3(0.2, 0.2, 0.24) * grain * (0.75 + 0.25 * r / 0.36);

      float pulse = 0.6 + 0.4 * sin(time * 2.0);
      float glow = (seam + ring) * (0.2 * pulse + 1.5 * flare());

      o_color = vec4(stone + SEAM * glow, 1.0);
   }
   else if (part == 3)
   {
      // glow spilling onto the floor around the open portal
      float falloff = exp(-r * 4.0) * smoothstep(1.25, 0.6, r);
      float pulse = 0.85 + 0.15 * sin(time * 3.0);
      float a = falloff * pulse * openness * 1.6;
      o_color = vec4(VIOLET * a, 1.0);
   }
   else
   {
      // the floor's depth is cleared inside the opening so the funnel below shows through
      gl_FragDepth = 1.0;
      o_color = vec4(0.0);
   }
}

#version 300 es
precision mediump float;

// a hot core in a soft halo, added onto what's behind; or a fire's flame: a tapered tongue of
// turbulent noise rising through it, white-yellow inside, orange and red at its frayed edges,
// on a brazier with a few sparks drifting up

in vec2 corner;
in vec3 color;
in highp vec3 fire;

out vec4 o_color;

uniform float u_brightness;

highp float hash(highp vec2 p)
{
   return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

highp float valueNoise(highp vec2 p)
{
   highp vec2 i = floor(p);
   highp vec2 f = fract(p);
   f = f * f * (3.0 - 2.0 * f);
   return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

vec3 flameColor(float heat)
{
   vec3 c = mix(vec3(0.6, 0.07, 0.01), vec3(1.0, 0.36, 0.05), smoothstep(0.0, 0.3, heat));
   c = mix(c, vec3(1.0, 0.7, 0.18), smoothstep(0.3, 0.62, heat));
   return mix(c, vec3(1.0, 0.95, 0.75), smoothstep(0.72, 0.95, heat));
}

void main()
{
   // the halos are only added (alpha 0), a flame also covers what's behind it a little so its orange stays
   // orange on a bright floor (premultiplied: src + dst * (1 - alpha))
   if (fire.x < 0.5 || fire.x > 1.5)
   {
      float r2 = dot(corner, corner);
      float halo = exp(-r2 * 4.0) * (1.0 - smoothstep(0.6, 1.0, r2));
      float core = fire.x > 1.5 ? 0.0 : exp(-r2 * 45.0);
      o_color = vec4(color * (halo * 0.7 + core * 1.4) * u_brightness, 0.0);
      return;
   }

   highp float t = fire.y;
   float x = corner.x;

   // the flame fills the lower 80%, sparks fly in the rest
   float y = (corner.y * 0.5 + 0.5) / 0.8;

   // noise rising through the flame, the phase in t keeps neighbours apart
   highp vec2 q = vec2(x * 1.6, y * 1.6 - t * 2.4);
   float n = valueNoise(q * 2.0) * 0.6 + valueNoise(q * 4.7 + vec2(7.3, t * 0.9)) * 0.4;

   // licks sideways more the higher up
   float bent = x + (n - 0.5) * 0.8 * y + sin(t * 3.1 + y * 5.0) * 0.08 * y;

   // a teardrop: round below, pointed at the tip, its top torn into tongues by the noise
   float width = 0.95 * pow(max(1.0 - y, 0.0), 0.65) * sqrt(clamp(y / 0.18, 0.0, 1.0));
   float d = abs(bent) / max(width, 0.001);
   float heat = (1.0 - d) * (1.2 - y * 0.8) - (n - 0.4) * 0.8 * y;
   heat = clamp(heat, 0.0, 1.0) * step(y, 1.0);
   float body = smoothstep(0.0, 0.22, heat);

   // white-yellow only deep inside, mostly yellow and orange, red at the fraying edge
   vec3 rgb = flameColor(heat * 0.95) * body;
   float alpha = body * 0.55;

   // sparks off a brazier: a few points rising out of the flame, fading as they go
   if (fire.z > 0.5)
   {
      vec2 at = vec2(x, corner.y * 0.5 + 0.5);
      for (int i = 0; i < 4; i++)
      {
         float k = float(i);
         highp float rise = t * (0.45 + k * 0.07) + k * 0.31;
         float life = fract(rise);
         float seed = mod(floor(rise), 97.0) + k * 17.0;
         vec2 spark = vec2((hash(vec2(seed, 1.0)) - 0.5) * 0.9 + sin(life * 6.0 + k) * 0.12, 0.3 + life * 0.68);
         vec2 offset = (at - spark) * vec2(1.0, 3.0);
         rgb += vec3(1.0, 0.62, 0.22) * exp(-dot(offset, offset) * 900.0) * (1.0 - life) * 1.6;
      }
   }

   o_color = vec4(rgb * u_brightness, alpha * u_brightness);
}

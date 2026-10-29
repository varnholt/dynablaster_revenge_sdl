#version 300 es
precision mediump float;

// lava (WaterSurface's lava mode): molten rock lit by itself. Dark crust floes drift slowly on it and break
// up, the melt glowing through the cracks between them; the heat breathes, hot spots well up and fade; along the
// shores it cools into crust with a bright seam where it meets the rock. The scenery's water data drives it:
// red the depth (0..1 = 0..0.8 tiles), blue the distance to the shore (0.02..1 = 0..SHORE_REACH tiles).

uniform sampler2D u_data;
uniform float u_time;
uniform vec3 u_deep;
uniform vec4 u_haze;
uniform highp vec4 u_haze_box;
uniform vec2 u_haze_range;

in vec3 world;
in vec2 uv;
out vec4 o_color;

const float SHORE_REACH = 3.0;

highp float hash(highp vec2 p)
{
   return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

highp float valueNoise(highp vec2 p)
{
   highp vec2 i = floor(p);
   highp vec2 f = fract(p);
   f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
   return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

highp float fbm(highp vec2 p)
{
   highp float sum = 0.0;
   highp float amplitude = 0.5;
   for (int k = 0; k < 4; k++)
   {
      sum += valueNoise(p) * amplitude;
      p = mat2(1.6, 1.2, -1.2, 1.6) * p + 11.7;
      amplitude *= 0.5;
   }
   return sum;
}

// the distance to the nearest of a cell grid's jittered points and to the next one: crust floes, their seams
highp vec2 cells(highp vec2 p)
{
   highp vec2 i = floor(p);
   highp vec2 f = fract(p);
   highp float near = 9.0;
   highp float next = 9.0;
   for (int y = -1; y <= 1; y++)
   {
      for (int x = -1; x <= 1; x++)
      {
         highp vec2 g = vec2(float(x), float(y));
         highp vec2 o = vec2(hash(i + g), hash(i + g + 17.3));
         // the floes rock a little as they drift
         o = 0.5 + 0.4 * sin(u_time * 0.35 + 6.2831 * o);
         highp float d = length(g + o - f);
         if (d < near)
         {
            next = near;
            near = d;
         }
         else if (d < next)
         {
            next = d;
         }
      }
   }
   return vec2(near, next);
}

// molten rock by its heat, 0 cold crust .. 1 white hot
vec3 molten(float heat)
{
   vec3 crust = vec3(0.07, 0.03, 0.025);
   vec3 red = vec3(0.65, 0.07, 0.02);
   vec3 orange = vec3(1.0, 0.38, 0.05);
   vec3 yellow = vec3(1.0, 0.82, 0.35);
   vec3 color = mix(crust, red, smoothstep(0.0, 0.35, heat));
   color = mix(color, orange, smoothstep(0.3, 0.65, heat));
   return mix(color, yellow, smoothstep(0.65, 1.0, heat));
}

void main()
{
   vec4 data = texture(u_data, uv);
   float shore = step(0.01, data.b) * max(data.b - 0.02, 0.0) / 0.98 * SHORE_REACH + (1.0 - step(0.01, data.b)) * SHORE_REACH;

   highp vec2 p = world.xy;
   // the melt creeps along, slowly turning on itself
   highp vec2 drift = vec2(fbm(p * 0.25 + u_time * 0.03), fbm(p * 0.25 - u_time * 0.025 + 5.0)) * 2.0;
   highp vec2 q = p * 0.9 + drift + vec2(u_time * 0.05, u_time * 0.02);

   // crust floes: dark plates, the melt glowing in the seams between them, the seams wider where it's hotter
   highp vec2 c = cells(q);
   float seam = c.y - c.x;
   float hot = fbm(p * 0.4 + vec2(-u_time * 0.04, u_time * 0.03));
   float open = 0.14 + 0.3 * smoothstep(0.3, 0.7, hot);
   float crust = smoothstep(open * 0.6, open, seam);
   // whole stretches of open magma between the floes, drifting and changing
   crust *= smoothstep(0.42, 0.58, fbm(q * 0.35 + 3.0));
   // the floes cool towards the shore: wider plates, thinner seams; right at the edge a bright seam on the rock
   crust = mix(crust, 1.0, smoothstep(1.6, 0.4, shore) * 0.55);

   // the heat breathes, hot spots welling up and fading
   float pulse = 0.5 + 0.5 * sin(u_time * 1.1 + fbm(p * 0.7) * 9.0);
   float melt = 0.55 + 0.35 * hot + 0.15 * pulse;
   // the crust's own faint glow from beneath, a cracked surface texture on it
   float plate = 0.08 + 0.12 * fbm(q * 3.0) + 0.1 * (1.0 - smoothstep(0.0, 0.5, c.x));
   float heat = mix(melt, plate, crust);
   heat = max(heat, smoothstep(0.25, 0.0, shore) * 0.95);

   vec3 color = molten(clamp(heat, 0.0, 1.0)) * 1.15;

   // the cave's smoke over the lava further out, as over the rest of the scenery
   if (u_haze.a > 0.0)
   {
      highp vec2 outside = max(max(u_haze_box.xy - world.xy, world.xy - u_haze_box.zw), vec2(0.0));
      float haze = smoothstep(u_haze_range.x, u_haze_range.y, length(outside)) * u_haze.a;
      color = mix(color, u_haze.rgb, haze * 0.6);
   }

   o_color = vec4(color, 1.0);
}

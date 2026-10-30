#version 300 es
precision mediump float;

// lava (WaterSurface's lava mode): molten rock lit by itself. Dark crust floes drift slowly on it and break
// up, the melt glowing through the cracks between them; the heat breathes, hot spots well up and fade; along the
// shores it cools into crust with a bright seam where it meets the rock. The scenery's water data drives it:
// red the depth (0..1 = 0..0.8 tiles), blue the distance to the shore (0.02..1 = 0..SHORE_REACH tiles).

uniform sampler2D u_data;
uniform highp float u_time;
uniform vec3 u_deep;
uniform vec4 u_haze;
uniform highp vec4 u_haze_box;
uniform vec2 u_haze_range;

in highp vec3 world;
in vec2 uv;
out vec4 o_color;

const float SHORE_REACH = 3.0;

// the radius the way round the field's corners is measured at, the same for the whole width of the stream: measured
// at each point's own distance, the corners before it would add more way the further out it lies, and along the
// edges the pattern would be sheared across the stream into stripes
const float BEND = 2.5;

// without sin(): along the ring the noise's coordinates grow into the hundreds, where a sin() hash runs out of
// precision and its values line up into stripes
highp float hash(highp vec2 p)
{
   highp vec3 p3 = fract(vec3(p.xyx) * 0.1031);
   p3 += dot(p3, p3.yzx + 33.33);
   return fract((p3.x + p3.y) * p3.z);
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

// the melt's heat at p (0 crust .. 1 white hot), its floes and seams carried along by flow
float lavaHeat(highp vec2 p, highp vec2 flow, float shore)
{
   highp vec2 drift = vec2(fbm(p * 0.25 + u_time * 0.03), fbm(p * 0.25 - u_time * 0.025 + 5.0)) * 2.0;
   highp vec2 q = (p - flow) * 0.9 + drift;

   // crust floes: dark plates, the melt glowing in the seams between them, the seams wider where it's hotter
   highp vec2 c = cells(q);
   float seam = c.y - c.x;
   float hot = fbm((p - flow) * 0.4);
   float open = 0.14 + 0.3 * smoothstep(0.3, 0.7, hot);
   float crust = smoothstep(open * 0.6, open, seam);
   // whole stretches of open magma between the floes
   crust *= smoothstep(0.3, 0.45, fbm(q * 0.35 + 3.0));
   // the floes cool towards the shore: wider plates, thinner seams
   crust = mix(crust, 1.0, smoothstep(1.6, 0.4, shore) * 0.55);

   // the heat breathes, hot spots welling up and fading
   float pulse = 0.5 + 0.5 * sin(u_time * 1.1 + fbm(p * 0.7) * 9.0);
   float melt = 0.55 + 0.35 * hot + 0.15 * pulse;
   float plate = 0.08 + 0.12 * fbm(q * 3.0) + 0.1 * (1.0 - smoothstep(0.0, 0.5, c.x));
   return mix(melt, plate, crust);
}

void main()
{
   vec4 data = texture(u_data, uv);
   float shore = step(0.01, data.b) * max(data.b - 0.02, 0.0) / 0.98 * SHORE_REACH + (1.0 - step(0.01, data.b)) * SHORE_REACH;

   // the lava streams round the field (u_haze_box, the field's extent): its pattern is laid out in the moat's own
   // coordinates, the way along the ring round the field and the distance out from it, and slides along the ring;
   // nothing is pushed about per pixel, so nothing smears and the floes keep their shape
   highp vec2 lo = u_haze_box.xy;
   highp vec2 hi = u_haze_box.zw;
   highp vec2 size = hi - lo;
   highp vec2 nearest = clamp(world.xy, lo, hi);
   highp vec2 out_of = world.xy - nearest;
   highp float r = length(out_of);
   // the way along the box's outline to the nearest point, anticlockwise from the bottom left corner, and round each
   // corner the arc at BEND
   highp float quarter = 1.5707963 * BEND;
   // edges where the point lies straight out from one (clamp leaves the other axis exactly 0), corners in between
   highp float way;
   if (out_of.x == 0.0 && out_of.y <= 0.0)
      way = nearest.x - lo.x;  // bottom
   else if (out_of.x > 0.0 && out_of.y < 0.0)
      way = size.x + atan(out_of.x, -out_of.y) * BEND;  // bottom right
   else if (out_of.x > 0.0 && out_of.y == 0.0)
      way = size.x + quarter + nearest.y - lo.y;  // right
   else if (out_of.x > 0.0 && out_of.y > 0.0)
      way = size.x + quarter + size.y + atan(out_of.y, out_of.x) * BEND;  // top right
   else if (out_of.x == 0.0 && out_of.y > 0.0)
      way = size.x + 2.0 * quarter + size.y + hi.x - nearest.x;  // top
   else if (out_of.x < 0.0 && out_of.y > 0.0)
      way = 2.0 * size.x + 2.0 * quarter + size.y + atan(-out_of.x, out_of.y) * BEND;  // top left
   else if (out_of.x < 0.0 && out_of.y == 0.0)
      way = 2.0 * size.x + 3.0 * quarter + size.y + hi.y - nearest.y;  // left
   else
      way = 2.0 * size.x + 3.0 * quarter + 2.0 * size.y + atan(-out_of.y, -out_of.x) * BEND;  // bottom left
   highp float ring = 2.0 * (size.x + size.y) + 4.0 * quarter;

   // the whole stream at one speed: slower by the shores, the stream would shear its pattern further with every
   // second, drawing the floes out into stripes along the ring (and the shore distance, 8 bits, into steps); the
   // drag only sways it a little, back and forth, out of step from the inner shore out
   highp float u = mod(way - u_time * 1.1 + 0.6 * sin(u_time * 0.35 + r * 1.3), ring);
   // where the ring closes the pattern runs into its own start: the last stretch fades over into it
   float close = smoothstep(ring - 2.0, ring, u);
   float heat = mix(lavaHeat(vec2(u, r), vec2(0.0), shore), lavaHeat(vec2(u - ring, r), vec2(0.0), shore), close);
   // and a slow churning where it is, not carried along with the stream, so it doesn't slide past like a belt
   heat += (fbm(world.xy * 0.3 + vec2(u_time * 0.05, -u_time * 0.04)) - 0.5) * 0.3;
   // right at the edge a bright seam where it meets the rock
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

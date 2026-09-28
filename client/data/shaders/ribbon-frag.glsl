#version 300 es
// highp: time and circleOffset are shared with the vertex shader
precision highp float;

uniform float time;
uniform float circleOffset;
uniform sampler2D texturemap;

in vec2 uv;
out vec4 o_color;

const float duration = 1.5;
const float fade_out_time = 1.5;
const float cut_offset = 0.05;

float getRadius(float u)
{
   float normalized_time = min(time / duration, 1.0);

   float radius = 0.0;

   if (u + cut_offset < normalized_time)
   {
      radius = 1.0;
   }
   else if (u < normalized_time)
   {
      radius = (normalized_time - u) / cut_offset;
   }

   return radius;
}

void main()
{
   vec4 col = vec4(uv.x, uv.x, 1.0, uv.x * 0.5);
   vec4 texture_color = texture(texturemap, vec2(uv.x - time * 0.03, 0.1 * uv.y));
   col *= texture_color;

   // fade out
   if (time > duration)
   {
      float fade_out_factor = 1.0 - (time - duration) / fade_out_time;
      col.a = uv.x * 0.5 * fade_out_factor;
   }

   col.a *= getRadius(uv.x);

   float vertical_alpha = uv.y >= 0.5 ? (1.0 - uv.y) : uv.y;
   col.a *= 2.0 * vertical_alpha;

   col.r = circleOffset * 0.03;

   o_color = col;
}

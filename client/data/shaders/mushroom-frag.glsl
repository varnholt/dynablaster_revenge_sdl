#version 300 es
precision highp float;

uniform float intensity;
uniform float time;
uniform sampler2D texture0;

in vec2 uv;
out vec4 o_color;

const float scale = 0.02;

vec2 getOffset(float t, vec2 uv_temp)
{
   float a = 1.0 + 0.5 * sin(t + uv_temp.x * 10.0);
   float b = 1.0 + 0.5 * cos(t + uv_temp.y * 10.0);

   return scale * vec2(a + sin(b), b + cos(a));
}

void main()
{
   vec4 orig_color = texture(texture0, uv);

   float speed = 5.0;
   float prev_time = speed * (time - 1.0);

   vec2 offset = getOffset(time, uv);
   vec2 prev_offset = getOffset(prev_time, uv);

   // motion vector from previous to current frame
   vec2 delta = offset - prev_offset;

   vec2 uv_temp = uv + offset;

   vec4 color = vec4(0.0);

   // unweighted blur along the motion vector
   const int steps = 20;
   float factor = 1.0 / float(steps);

   for (int i = 0; i < steps; i++)
   {
      color += texture(texture0, uv_temp);
      uv_temp += delta * factor;
   }

   o_color = intensity * (color * factor) + (1.0 - intensity) * orig_color;
}

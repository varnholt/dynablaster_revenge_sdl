#version 300 es
precision mediump float;

uniform float alpha;
uniform float intensity;
uniform vec2 motionDir;
uniform sampler2D tex;

in vec2 uv;
out vec4 o_color;

void main()
{
   vec4 origColor = texture(tex, uv);

   vec4 color = vec4(0.0);

   const int steps = 20;
   float factor = 1.0 / float(steps);

   vec2 offset = vec2(0.0);
   vec2 delta = motionDir * factor;

   for (int i = 0; i < steps; i++)
   {
      color += texture(tex, uv + offset);
      offset += delta;
   }

   vec4 result = intensity * factor * color + (1.0 - intensity) * origColor;
   o_color = vec4(result.rgb, result.a * alpha);
}

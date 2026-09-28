#version 300 es
precision highp float;

uniform sampler2D positionmap;
uniform sampler2D colormap;

uniform mat4 proj;
uniform float stop;

in vec2 pos2d;

out vec4 o_color;

void main()
{
   vec4 result = vec4(0.0, 0.0, 0.0, 0.0);

   vec4 pos = texture(positionmap, pos2d);

   if (pos.w >= 1.0 && stop < 0.5)
   {
      vec4 p = proj * vec4(pos.xyz, 1.0);
      float t = 1.0 / p.w;
      vec2 uv = (p.xy * t + 1.0) * 0.5;

      result = texture(colormap, uv);
   }

   // replaces the original's fixed-function alpha test (GL_GREATER, 0.001)
   if (result.a <= 0.001)
   {
      discard;
   }

   o_color = result;
}

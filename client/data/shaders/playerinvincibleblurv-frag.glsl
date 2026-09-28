#version 300 es
precision mediump float;

uniform sampler2D texturemap;
uniform vec2 texelOffset;
uniform float radius;
uniform float kernel[32];

in vec2 uv;
out vec4 o_color;

void main()
{
   vec2 pos = uv;
   vec2 neg = uv;

   vec4 col = texture(texturemap, pos);

   float alpha = col.a * kernel[0];

   for (int i = 1; i < int(radius); i++)
   {
      neg -= texelOffset;
      float a2 = texture(texturemap, neg).a;
      alpha += a2 * kernel[i];
   }

   for (int i = 1; i < int(radius); i += 2)
   {
      pos += texelOffset;
      float a1 = texture(texturemap, pos).a;
      alpha += a1 * kernel[i];
   }

   col = (col + texture(texturemap, uv + texelOffset) + texture(texturemap, uv - texelOffset)) * 0.33333;

   alpha = alpha * 3.0;

   o_color = vec4(col.r * alpha, col.r * alpha * 0.5, 0.0, alpha);
}

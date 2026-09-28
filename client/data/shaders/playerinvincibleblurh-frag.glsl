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
   vec4 col = texture(texturemap, uv);

   float alpha = col.a * kernel[0];

   vec2 pos = uv;
   vec2 neg = uv;
   for (int i = 1; i < int(radius); i++)
   {
      pos += texelOffset;
      neg -= texelOffset;

      float a1 = texture(texturemap, pos).a;
      float a2 = texture(texturemap, neg).a;

      alpha += (a1 + a2) * kernel[i];
   }

   col = (col + texture(texturemap, uv + texelOffset) + texture(texturemap, uv - texelOffset)) * 0.33333;

   o_color = vec4(1.0 - col.a, 0.0, 0.0, alpha);
}

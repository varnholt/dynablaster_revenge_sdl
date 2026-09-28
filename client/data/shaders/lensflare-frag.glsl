#version 300 es
precision highp float;

uniform sampler2D texturemap;

in float alpha;
in vec2 uv;
in vec3 color;

out vec4 o_color;

void main()
{
   vec4 texture_color = texture(texturemap, uv);

   // the original's GL_ALPHA_TEST GL_GREATER 0
   if (texture_color.a <= 0.0)
   {
      discard;
   }

   o_color = vec4(texture_color.rgb * color * alpha, texture_color.a);
}

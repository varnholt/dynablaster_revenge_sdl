#version 300 es
precision highp float;

uniform sampler2D texturemap;
uniform vec4 color;

in vec2 uv1;
in vec2 uv2;

out vec4 o_color;

void main()
{
   vec4 col1 = texture(texturemap, uv1);
   vec4 col2 = texture(texturemap, uv2);

   o_color = vec4((col1.rgb + col2.rgb) * 0.5, color.a);
}

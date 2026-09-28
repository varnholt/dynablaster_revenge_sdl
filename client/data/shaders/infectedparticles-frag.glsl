#version 300 es
precision mediump float;

uniform sampler2D texturemap;

in vec4 color;

out vec4 o_color;

void main()
{
   vec4 maskColor = texture(texturemap, gl_PointCoord.xy);
   o_color = maskColor * color;
}

#version 300 es
precision mediump float;

uniform sampler2D texturemap;

out vec4 o_color;

const float alpha = 0.35;

void main()
{
   o_color = texture(texturemap, gl_PointCoord) * alpha;
}

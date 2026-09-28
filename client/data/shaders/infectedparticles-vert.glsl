#version 300 es

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;

uniform float particleSize;
uniform mat4 u_projection;

out vec4 color;

void main()
{
   vec4 p = u_projection * vec4(a_position.xyz, 1.0);

   float alpha = a_position.w;
   gl_PointSize = particleSize * sqrt(max(alpha, 0.0)) / p.w;

   color = a_color;

   gl_Position = p;
}

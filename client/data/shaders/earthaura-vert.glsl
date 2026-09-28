#version 300 es

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

uniform mat4 u_modelViewProjection;

out vec4 color;

void main()
{
   color = a_color;
   gl_Position = u_modelViewProjection * vec4(a_position, 1.0);
}

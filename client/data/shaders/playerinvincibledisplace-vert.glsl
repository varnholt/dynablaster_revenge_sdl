#version 300 es

layout(location = 0) in vec3 a_position;

uniform mat4 u_modelViewProjection;

out vec4 pos;

void main()
{
   pos = u_modelViewProjection * vec4(a_position, 1.0);
   gl_Position = pos;
}

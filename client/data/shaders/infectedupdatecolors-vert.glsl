#version 300 es

layout(location = 0) in vec2 a_position;

out vec2 pos2d;

void main()
{
   gl_Position = vec4(a_position, 0.0, 1.0);
   pos2d = (a_position + 1.0) * 0.5;
}

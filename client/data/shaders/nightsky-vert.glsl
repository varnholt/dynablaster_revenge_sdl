#version 300 es

// the night sky: one full screen triangle behind everything, the fragments work out where they look

layout(location = 0) in vec2 a_position;

out vec2 clip;

void main()
{
   clip = a_position;
   gl_Position = vec4(a_position, 1.0, 1.0);
}

#version 300 es

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord0;
layout(location = 2) in vec3 a_normal;

uniform mat4 u_modelViewProjection;

// the portal's local space, centered on the tile
out vec3 position;
out vec3 normal;
out vec2 uv;

// the camera in the portal's local space
flat out vec3 eye;

void main()
{
   position = a_position;
   normal = a_normal;
   uv = a_texcoord0;

   // the point every clip space z line runs through
   vec4 camera = inverse(u_modelViewProjection) * vec4(0.0, 0.0, -1.0, 0.0);
   eye = camera.xyz / camera.w;

   gl_Position = u_modelViewProjection * vec4(a_position, 1.0);
}

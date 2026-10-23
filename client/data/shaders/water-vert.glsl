#version 300 es

// water surfaces of glTF scenery, see water-frag.glsl

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord0;

uniform mat4 u_modelViewProjection;

// the world transform, the camera is folded into the projection
uniform mat4 u_modelView;

out vec3 world;
out vec2 uv;

void main()
{
   world = (u_modelView * vec4(a_position, 1.0)).xyz;
   uv = a_texcoord0;
   gl_Position = u_modelViewProjection * vec4(a_position, 1.0);
}

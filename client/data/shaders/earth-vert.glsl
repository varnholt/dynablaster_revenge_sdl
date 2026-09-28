#version 300 es

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_tangent;
layout(location = 2) in vec2 a_texcoord0;

uniform mat4 u_modelViewProjection;
uniform vec3 osLightPos;
uniform vec3 osCameraPos;

out vec2 uv;
out vec3 lightDir;
out vec3 viewDir;

void main()
{
   uv = a_texcoord0;

   // tangent space
   vec3 z = normalize(a_position);
   vec3 x = a_tangent;
   vec3 y = cross(x, z);
   mat3 tangent_space = mat3(x, y, z);

   lightDir = normalize(a_position - osLightPos) * tangent_space;
   viewDir = normalize(a_position - osCameraPos) * tangent_space;

   gl_Position = u_modelViewProjection * vec4(a_position, 1.0);
}

#version 300 es

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord0;

uniform mat4 u_modelViewProjection;
uniform vec3 textureScroll;

out vec2 uv1;
out vec2 uv2;

void main()
{
   vec2 uv_scale = vec2(2.0, 1.0);
   uv1 = a_texcoord0 * uv_scale + vec2(textureScroll.x, 0.0);
   uv2 = a_texcoord0 * uv_scale + vec2(textureScroll.y, 0.0);

   gl_Position = u_modelViewProjection * vec4(a_position, 1.0);
}

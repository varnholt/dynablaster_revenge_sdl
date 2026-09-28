#version 300 es

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord0;
layout(location = 3) in vec3 a_direction;
layout(location = 4) in vec2 a_speedIndex;

uniform vec3 field;
uniform vec3 color;
uniform float time;
uniform vec3 camera;
uniform mat4 u_modelViewProjection;

out float v_alpha;
out float v_index;
out vec2 v_uv;

mat3 rotateX(float f)
{
   float c = cos(f);
   float s = sin(f);
   return mat3(1.0, 0.0, 0.0, 0.0, c, s, 0.0, -s, c);
}

mat3 rotateY(float f)
{
   float c = cos(f);
   float s = sin(f);
   return mat3(c, 0.0, -s, 0.0, 1.0, 0.0, s, 0.0, c);
}

mat3 rotateZ(float f)
{
   float c = cos(f);
   float s = sin(f);
   return mat3(c, s, 0.0, -s, c, 0.0, 0.0, 0.0, 1.0);
}

void main()
{
   v_uv = a_texcoord0;

   float speed = a_speedIndex.x;
   v_index = a_speedIndex.y;

   mat3 rot = rotateX(time + v_index) * rotateY(time + v_index) * rotateZ(time + v_index);

   vec3 pos = rot * a_position;

   vec3 n = rot * a_normal;
   v_alpha = abs(dot(n, camera));

   vec3 center = (time * speed) * a_direction;
   center += field;
   pos += center;

   pos.z += -0.5 * time * time;

   gl_Position = u_modelViewProjection * vec4(pos, 1.0);
}

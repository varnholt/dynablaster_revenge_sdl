#version 300 es

// x, y, z: position in snow space; w: point size in px at 1920 px width
layout(location = 0) in vec4 a_position;

uniform mat4 u_modelViewProjection;
uniform float sizeFactor;

void main()
{
   vec3 pos = a_position.xyz;

   // random motion
   pos.x += sin(a_position.z * 26.0 - a_position.y * 11.0) * 0.004;
   pos.y += cos(a_position.x * 22.0 - a_position.z * 14.0) * 0.004;

   pos.x += sin(a_position.x * 6.0 - a_position.z * 2.5) * 0.01;
   pos.y += cos(a_position.y * 3.5 - a_position.z * 5.0) * 0.01;

   // varies the fall speed
   pos.z += sin(a_position.y * 4.0 - a_position.x * 3.0) * 0.05;

   vec4 p = u_modelViewProjection * vec4(pos, 1.0);
   gl_Position = p;
   gl_PointSize = a_position.w * sizeFactor / p.w;
}

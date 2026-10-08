#version 300 es

// glTF models (story mode enemies and stage pieces): optional linear blend skinning with up to
// 16 joints, joint matrices are in model space

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord0;
layout(location = 3) in vec4 a_joints;
layout(location = 4) in vec4 a_weights;

uniform mat4 u_modelViewProjection;
uniform mat4 u_model;
uniform mat4 u_joints[16];
uniform float u_skinned;

out vec2 uv;
out vec3 nrm;

void main()
{
   mat4 skin = mat4(1.0);

   if (u_skinned > 0.5)
   {
      skin = u_joints[int(a_joints.x)] * a_weights.x + u_joints[int(a_joints.y)] * a_weights.y +
             u_joints[int(a_joints.z)] * a_weights.z + u_joints[int(a_joints.w)] * a_weights.w;
   }

   uv = a_texcoord0;
   nrm = normalize(mat3(u_model) * mat3(skin) * a_normal);
   gl_Position = u_modelViewProjection * skin * vec4(a_position, 1.0);
}

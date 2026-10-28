#version 300 es

// glTF models (story mode enemies and stage pieces): optional linear blend skinning with up to
// 40 joints, joint matrices are in model space; cloth waves in the wind by its a_wave

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord0;
layout(location = 3) in vec4 a_joints;
layout(location = 4) in vec4 a_weights;
layout(location = 5) in vec4 a_wave;

uniform mat4 u_modelViewProjection;
uniform mat4 u_model;
uniform mat4 u_joints[40];
uniform float u_skinned;

// seconds
uniform float u_time;

// a fur shell: x how far out along the normal, y the layer (0 the skin itself, 1 the tips), z strands per unit
uniform vec4 u_shell;

out vec2 uv;
out vec3 nrm;

// world z, for the clip height of mirror images
out float height;

// world position, where the detail textures are tiled
out highp vec3 world;

// the position on the bare model, where a fur's strands stand
out highp vec3 local;

void main()
{
   mat4 skin = mat4(1.0);

   if (u_skinned > 0.5)
   {
      skin = u_joints[int(a_joints.x)] * a_weights.x + u_joints[int(a_joints.y)] * a_weights.y +
             u_joints[int(a_joints.z)] * a_weights.z + u_joints[int(a_joints.w)] * a_weights.w;
   }

   // gusts running through the cloth
   float phase = dot(a_position, vec3(0.85, 0.45, 0.6));
   float gust = 0.6 * sin(u_time * 2.7 - phase) + 0.4 * sin(u_time * 4.3 - phase * 1.9 + 1.3);
   // fur shells pushed out along the normal and, the further out the more, sagging under their weight (the
   // model's up is +y, glTF's)
   vec3 fur = a_normal * u_shell.x - vec3(0.0, 0.45, 0.0) * u_shell.x * u_shell.y;
   vec4 position = vec4(a_position + a_wave.xyz * gust + fur, 1.0);
   local = a_position;

   uv = a_texcoord0;
   nrm = normalize(mat3(u_model) * mat3(skin) * a_normal);
   world = (u_model * skin * position).xyz;
   height = world.z;
   gl_Position = u_modelViewProjection * skin * position;
}

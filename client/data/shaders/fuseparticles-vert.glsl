#version 300 es

// Point-sprite vertex shader for FuseParticleSystem (bomb fuse sparks). One vertex per particle,
// no index/UV buffer needed - gl_PointCoord supplies the per-fragment texture coordinate.
// a_position.w carries the particle's remaining life (1.0 -> 0.0), driving both point size and
// brightness, same convention as deathparticles-vert.glsl.

layout(location = 0) in vec4 a_position;

uniform float particleSize;
uniform mat4 u_projection;

out float v_life;

void main()
{
   vec4 p = u_projection * vec4(a_position.xyz, 1.0);

   v_life = a_position.w;
   gl_PointSize = particleSize * a_position.w / p.w;

   gl_Position = p;
}

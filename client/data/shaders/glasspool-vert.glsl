#version 300 es

// the light a glass block throws onto the floor round its foot (BlockMaterial::drawLightPools): a quad on the
// ground in the block's own space, a_position -1..1 across it

layout(location = 0) in vec3 a_position;

uniform vec3 centre;
uniform vec2 extent;
uniform float floorHeight;
uniform mat4 u_modelView;
uniform mat4 u_projection;

out vec2 across;

void main()
{
   across = a_position.xy;
   vec3 position = vec3(centre.xy + a_position.xy * extent, floorHeight);
   gl_Position = u_projection * (u_modelView * vec4(position, 1.0));
}

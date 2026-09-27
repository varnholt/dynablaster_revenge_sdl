#version 300 es
precision mediump float;

// See fuseparticles-vert.glsl. Additive spark: brightness scales with remaining life so sparks
// dim as they cool, matching the size shrink already driven by gl_PointSize.

uniform sampler2D texturemap;

in float v_life;

out vec4 o_color;

void main()
{
   vec4 mask = texture(texturemap, gl_PointCoord.xy);
   o_color = mask * v_life;
}

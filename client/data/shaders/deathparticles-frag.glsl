#version 300 es
precision mediump float;

// GLES3 port of client/data/shaders/deathparticles-frag.glsl. gl_PointCoord is a real GLES3
// built-in (unchanged). texture2D -> texture, gl_FragColor -> o_color.

uniform sampler2D texturemap;
uniform sampler2D colormap;

in vec2 uv;

out vec4 o_color;

void main()
{
   vec4 maskColor = texture(texturemap, gl_PointCoord.xy);
   vec4 color = texture(colormap, uv);

   o_color = maskColor * color;
}

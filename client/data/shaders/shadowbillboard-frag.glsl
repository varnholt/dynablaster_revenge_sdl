#version 300 es
precision mediump float;

// GLES3 port of client/data/shaders/shadowbillboard-frag.glsl (texture2D -> texture,
// varying -> in, gl_FragColor -> an explicit out).

uniform sampler2D texturemap;

// how dark the shadow falls, 1 as painted (it's multiplied onto what's below)
uniform float strength;

in vec2 uv;
out vec4 o_color;

void main()
{
   vec4 shadow = texture(texturemap, uv);
   o_color = vec4(mix(vec3(1.0), shadow.rgb, strength), shadow.a);
}

#version 300 es
precision mediump float;

// PSD layer draw tinted by "color", like the original's glColor modulation
uniform vec3 color;
uniform float alpha;
uniform sampler2D tex;

in vec2 uv;
out vec4 o_color;

void main()
{
   vec4 texel = texture(tex, uv);
   o_color = vec4(texel.rgb * color, texel.a * alpha);
}

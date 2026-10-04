#version 300 es
precision mediump float;

// PSD layer draw that cuts the texture off right of "progress" (0..1 in u)
uniform float progress;
uniform float alpha;
uniform sampler2D tex;

in vec2 uv;
out vec4 o_color;

void main()
{
   vec4 color = texture(tex, uv);
   o_color = vec4(color.rgb, uv.x < progress ? color.a * alpha : 0.0);
}

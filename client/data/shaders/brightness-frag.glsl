#version 300 es
precision highp float;

uniform float gamma;
uniform sampler2D texture0;

in vec2 uv;
out vec4 o_color;

void main()
{
   const float inv_gamma = 1.0 / 2.2;

   vec4 col = texture(texture0, uv);

   // undo the display gamma, then apply the configured one
   vec3 linear = pow(col.rgb, vec3(inv_gamma));
   o_color = vec4(pow(linear, vec3(gamma)), col.a);
}

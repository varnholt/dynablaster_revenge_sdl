#version 300 es
precision highp float;

uniform float time;
uniform sampler2D texturemap;
uniform vec3 color;

in float v_alpha;
in float v_index;
in vec2 v_uv;

out vec4 o_color;

vec3 getColor()
{
   vec3 white = vec3(1.0);
   float val = 0.5 * (1.0 + sin(v_index));
   return (1.0 - val) * white + val * color;
}

void main()
{
   vec4 tex = texture(texturemap, v_uv);
   float alphaDec = v_alpha - (time / 10.0);
   vec3 mixedColor = getColor();
   o_color = vec4(tex.rgb * mixedColor, tex.a * alphaDec);
}

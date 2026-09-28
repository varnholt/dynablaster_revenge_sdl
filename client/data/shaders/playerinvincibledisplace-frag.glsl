#version 300 es
precision mediump float;

uniform sampler2D texturemap;
uniform sampler2D displace;
uniform vec2 texelOffset;
uniform vec4 displaceOffset;
uniform vec4 sourceRect;
uniform float fade;
uniform vec3 center;

in vec4 pos;
out vec4 o_color;

void main()
{
   vec2 uv = pos.xy / pos.w;

   vec2 d1 = texture(displace, (uv - center.xy) * 0.70 + texelOffset * displaceOffset.xy).rg - 0.5;
   vec2 d2 = texture(displace, (uv - center.xy) * 0.65 + texelOffset * displaceOffset.yz).rg - 0.5;
   vec2 d3 = texture(displace, (uv - center.xy) * 0.60 + texelOffset * displaceOffset.zw).rg - 0.5;

   d1 *= texelOffset * 0.04;
   d2 *= texelOffset * 0.04;
   d3 *= texelOffset * 0.04;

   vec4 c1 = texture(texturemap, (uv - sourceRect.xy) * sourceRect.zw + d1);
   vec4 c2 = texture(texturemap, (uv - sourceRect.xy) * sourceRect.zw + d2);
   vec4 c3 = texture(texturemap, (uv - sourceRect.xy) * sourceRect.zw + d3);

   o_color = (c1 + c2 + c3) * 0.4 * fade;
}

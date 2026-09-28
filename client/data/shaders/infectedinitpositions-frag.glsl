#version 300 es
precision highp float;

uniform sampler2D depthmap;
uniform mat4 invProj;
uniform vec4 srcRect;

in vec2 pos;

out vec4 o_color;

float frands(vec2 p, float minimum, float maximum)
{
   float d = dot(p.xy, vec2(12.9898, 78.233));
   float n = fract(sin(d) * 43758.5453);
   return minimum + n * (maximum - minimum);
}

void main()
{
   vec2 p2d = vec2(frands(pos.xy, srcRect.x, srcRect.z), frands(pos.yx, srcRect.y, srcRect.w));

   float z = texture(depthmap, (p2d + 1.0) * 0.5).r;

   vec4 v = invProj * vec4(p2d.x, p2d.y, z * 2.0 - 1.0, 1.0);
   float t = 1.0 / v.w;

   o_color = vec4(v.xyz * t, 1.0);
}

#version 300 es
precision highp float;

uniform sampler2D vertexPosTexture;
uniform sampler2D vertexParamTexture;
uniform sampler2D flowfieldTexture;
uniform sampler2D depthTexture;

uniform vec3 center;
uniform float fieldScale;
uniform float timeDelta;
uniform vec4 srcRect;
uniform mat4 invProj;
uniform float stop;

in vec2 currentPos;

out vec4 o_color;

float frands(vec2 p, float minimum, float maximum)
{
   float d = dot(p.xy, vec2(12.9898, 78.233));
   float n = fract(sin(d) * 43758.5453);
   return minimum + n * (maximum - minimum);
}

void main()
{
   vec4 pos = texture(vertexPosTexture, currentPos);
   vec4 param = texture(vertexParamTexture, currentPos);

   pos.w -= param.x * timeDelta;

   if (pos.w > 0.0)
   {
      vec2 offset = param.yz;
      vec2 uv = (pos.xz - center.xz) * fieldScale + offset;
      vec4 dir = texture(flowfieldTexture, uv) - 0.5;

      dir.x = dir.x * 1.8;
      dir.z = dir.z * 1.8;

      pos.xyz += dir.xyz * timeDelta * 0.05;
   }
   else if (stop < 0.5)
   {
      vec2 p2d = vec2(frands(pos.xy, srcRect.x, srcRect.z), frands(pos.yz, srcRect.y, srcRect.w));

      float z = texture(depthTexture, (p2d + 1.0) * 0.5).r;

      vec4 p3d = invProj * vec4(p2d.x, p2d.y, z * 2.0 - 1.0, 1.0);
      float t = 1.0 / p3d.w;

      pos = vec4(p3d.xyz * t, 1.0);
   }
   else
   {
      pos.w = 0.0;
   }

   o_color = pos;
}

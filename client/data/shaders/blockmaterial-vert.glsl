#version 300 es

// GLES3 port of client/data/shaders/blockmaterial-vert.glsl.
// gl_Vertex/gl_Normal/gl_NormalMatrix/gl_ModelViewMatrix/gl_ProjectionMatrix/gl_MultiTexCoord0
// (fixed-function built-ins) become explicit attributes/uniforms. u_modelView here is the plain
// object-space world matrix (matches this port's GLDevice::uploadTransformUniforms() convention,
// not a real combined model*view) and u_projection already carries view*projection combined -
// same split this legacy renderer itself used, just under different uniform names - so
// u_projection * (u_modelView * vertex) still produces the correct final clip position, and
// u_modelView * vertex still gives the same WORLD-space position the shadowCamera matrix expects.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord0;

uniform vec3 camera;
uniform vec3 uvOffset;
uniform mat4 shadowCamera;
uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_normalMatrix;
// 1: every block shows its colour turned its own way (the story's worlds), 0: all alike (the original levels)
uniform float variation;

out vec3 eye;
out vec3 nrm;
out vec2 uv1;
out vec2 uv2;
out vec2 uv3;
out float faceShadow;
out vec3 world;
flat out float turns;
flat out float story;
// glass: the block's own space, where the view ray is followed through it
out vec3 objectPos;
flat out vec3 objectCamera;

const float texWidth = 192.0;
const float texHeight = 256.0;
const float texPadding = 4.0;

void main()
{
   vec4 pos = u_modelView * vec4(a_position, 1.0);
   vec3 n = mat3(u_normalMatrix) * a_normal;
   vec3 tmp = vec3(shadowCamera[0].z, shadowCamera[1].z, shadowCamera[2].z);

   // vertex-normal faces shadow camera?
   faceShadow = clamp(0.5 - dot(tmp, n), 0.0, 1.0);

   nrm = n;
   eye = normalize(mat3(u_normalMatrix) * (a_position - camera));
   vec4 shadowPos = shadowCamera * pos;

   uv1 = a_texcoord0;
   // compensate padding between individual uv sets:
   uv2 = vec2(
      (a_texcoord0.x * texWidth + uvOffset.x * (texWidth + texPadding) + 1.0) / 1024.0,
      (a_texcoord0.y * texHeight + uvOffset.y * (texHeight + texPadding) + 1.0) / 2048.0
   );

   uv3 = vec2(shadowPos.x, -shadowPos.y) * 0.5 + 0.5;
   world = pos.xyz;
   story = variation;
   objectPos = a_position;
   objectCamera = camera;
   // quarter turns 0..3 picked by where the block stands, steady from frame to frame
   vec2 centre = floor((u_modelView * vec4(0.0, 0.0, 0.0, 1.0)).xy * 2.0 + 0.5);
   turns = floor(fract(sin(dot(centre, vec2(12.9898, 78.233))) * 43758.5453) * 4.0) * variation;
   gl_Position = u_projection * pos;
}

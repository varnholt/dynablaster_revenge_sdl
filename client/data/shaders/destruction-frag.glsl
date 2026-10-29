#version 300 es
precision mediump float;

// GLES3 port of client/data/shaders/destruction-frag.glsl (texture2D -> texture, varying -> in,
// gl_FragColor -> an explicit out).

uniform sampler2D texturemap;
uniform sampler2D diffusemap;
uniform sampler2D specularmap;
uniform sampler2D shadowmap;
// glass blocks' debris (DestructionMaterial::setGlass): shards of their block's tint
uniform float glass;
uniform vec3 tint;

in vec3 eye;
in vec2 uv;
in vec3 nrm;
in vec2 uv3;
in float faceShadow;
out vec4 o_color;

vec4 textureSphere(sampler2D map, vec3 dir)
{
   vec3 t = dir + vec3(0.0, 0.0, 1.0);
   float m = 0.5 / sqrt(dot(t, t));
   t = vec3(0.5, 0.5, 0.5) + vec3(t.x, -t.y, t.z) * m;
   return texture(map, t.xy);
}

void main()
{
   vec3 n = normalize(nrm);
   vec3 e = normalize(eye);
   vec3 refl = e - n * 2.0 * dot(n, e);

   vec4 spec = textureSphere(specularmap, refl);
   vec4 diff = textureSphere(diffusemap, n);
   vec4 tex = texture(texturemap, uv);

   if (glass > 0.5)
   {
      // a shard of the block: its tint lit from inside, brighter where light falls on it, its faces seen edge on
      // flashing white as it tumbles
      vec3 light = normalize(vec3(-0.45, 0.35, 0.82));
      float fresnel = pow(1.0 - clamp(abs(dot(n, e)), 0.0, 1.0), 2.0);
      vec3 shard = tint * (0.7 + 0.7 * max(dot(n, light), 0.0)) + mix(tint, vec3(1.0), 0.65) * fresnel * 1.1;
      o_color = vec4(shard, 1.0);
      return;
   }

   vec4 shad = texture(shadowmap, uv3);

   // blend shadow map with facing factor
   // 1.0 -> full shadow map
   // 0.0 -> completely blending to white
   shad = shad * faceShadow + vec4(1.0, 1.0, 1.0, 1.0) * (1.0 - faceShadow);

   vec4 final = (diff * tex * shad) + (spec * tex.a);

   o_color = vec4(final.rgb, 1.0);
}

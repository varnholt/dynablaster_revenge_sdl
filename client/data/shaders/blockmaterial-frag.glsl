#version 300 es
precision mediump float;

// GLES3 port of client/data/shaders/blockmaterial-frag.glsl (texture2D -> texture, varying -> in,
// gl_FragColor -> an explicit out).

uniform sampler2D specularmap;
uniform sampler2D diffusemap;
uniform sampler2D shadowmap;
uniform sampler2D texturemap;
uniform sampler2D ambientmap;

in vec3 eye;
in vec3 nrm;
in vec2 uv1;
in vec2 uv2;
in vec2 uv3;
in float faceShadow;
in highp vec3 world;
flat in float turns;
flat in float story;
in highp vec3 objectPos;
flat in highp vec3 objectCamera;
// which of the tints this block takes (BlockMaterial::glassTint)
uniform float tintChoice;

// glass blocks (the story's crystal blocks, BlockMaterial::setGlass)
uniform float glass;
uniform vec3 tint0;
uniform vec3 tint1;
uniform vec3 tint2;
uniform vec4 viewport;
uniform highp vec3 boxMin;
uniform highp vec3 boxMax;
uniform sampler2D refraction;
out vec4 o_color;

vec4 textureSphere(sampler2D map, vec3 dir)
{
   vec3 t = dir + vec3(0.0, 0.0, 1.0);
   float m = 0.5 / sqrt(dot(t, t));
   vec2 uv = vec2(0.5 + t.x * m, 0.5 - t.y * m);
   return texture(map, uv);
}

// the colour's cross (171 x 128 texel cells on 512: a column of four at x 170..341, one more either side of its
// second) seen turned by turns quarter turns: the top shows its cell turned, the sides seen from the field (south,
// east, west, the cells of the cross' arms' row) show one another's painting, mirrored on odd turns. Only the
// colour moves: the occlusion between neighbours (uv2) stays with the faces it belongs to.
vec2 turned(vec2 uv, vec3 face)
{
   if (turns < 0.5)
   {
      return uv;
   }

   const vec3 edges = vec3(0.0, 170.0 / 512.0, 341.0 / 512.0);
   float column = uv.x < edges.y ? 0.0 : (uv.x < edges.z ? 1.0 : 2.0);
   float row = floor(uv.y * 4.0);
   vec2 origin = vec2(column == 0.0 ? edges.x : (column == 1.0 ? edges.y : edges.z), row * 0.25);
   vec2 size = vec2((column == 1.0 ? edges.z : (column == 0.0 ? edges.y : 1.0)) - origin.x, 0.25);
   vec2 st = clamp((uv - origin) / size, 0.0, 1.0);

   if (face.z > 0.5)
   {
      for (float k = 0.0; k < turns; k += 1.0)
      {
         st = vec2(st.y, 1.0 - st.x);
      }
   }
   else if (abs(face.z) < 0.5 && face.y < 0.5)
   {
      column = mod(column + turns, 3.0);
      origin.x = column == 0.0 ? edges.x : (column == 1.0 ? edges.y : edges.z);
      size.x = (column == 1.0 ? edges.z : (column == 0.0 ? edges.y : 1.0)) - origin.x;
      if (mod(turns, 2.0) > 0.5)
      {
         st.x = 1.0 - st.x;
      }
   }
   else
   {
      return uv;
   }

   return origin + st * size;
}

highp float hash3(highp vec3 p)
{
   return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

highp float valueNoise3(highp vec3 p)
{
   highp vec3 i = floor(p);
   highp vec3 f = fract(p);
   f = f * f * (3.0 - 2.0 * f);
   return mix(mix(mix(hash3(i), hash3(i + vec3(1, 0, 0)), f.x), mix(hash3(i + vec3(0, 1, 0)), hash3(i + vec3(1, 1, 0)), f.x), f.y),
              mix(mix(hash3(i + vec3(0, 0, 1)), hash3(i + vec3(1, 0, 1)), f.x), mix(hash3(i + vec3(0, 1, 1)), hash3(i + vec3(1, 1, 1)), f.x), f.y),
              f.z);
}

// how far p (on the box's surface) is from the box's nearest edge: of its distances to the faces the smallest is
// the face it lies on, the middle one the edge
highp float edgeDistance(highp vec3 p)
{
   highp vec3 d = min(p - boxMin, boxMax - p);
   return d.x + d.y + d.z - min(min(d.x, d.y), d.z) - max(max(d.x, d.y), d.z);
}

// distance to a box of halfSize size halfSize with its edges rounded by radius, from p (relative to its centre)
highp float roundedBox(highp vec3 p, highp vec3 halfSize, highp float radius)
{
   highp vec3 q = abs(p) - (halfSize - radius);
   return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - radius;
}

// a block of coloured glass lit from inside, after Haujobb's BBQ invitation: what's behind it shows through, tinted
// deeper the longer the way through; the view ray followed through the box shows its far edges inside as lines of
// light, its near edges catch a rim; light drifts in clouds inside
vec3 glassBlock(vec3 face, vec3 e)
{
   vec3 tint = tintChoice < 0.5 ? tint0 : (tintChoice < 1.5 ? tint1 : tint2);
   highp vec3 size = boxMax - boxMin;
   highp float unit = max(max(size.x, size.y), size.z);
   highp vec3 ray = normalize(objectPos - objectCamera);

   // the glass is the mesh's box with its edges ground off wide, like the BBQ's cubes: the ray is followed from the
   // box's face to the bevelled shape inside it, and where it misses that the box's corner isn't there
   highp vec3 centre = (boxMin + boxMax) * 0.5;
   highp vec3 halfSize = size * 0.5;
   highp float bevelRadius = unit * 0.12;
   highp float t = 0.0;
   bool hit = false;
   for (int k = 0; k < 24; k++)
   {
      highp float d = roundedBox(objectPos + ray * t - centre, halfSize, bevelRadius);
      if (d < unit * 0.0008)
      {
         hit = true;
         break;
      }
      t += d;
      if (t > unit * 0.5)
      {
         break;
      }
   }
   if (!hit)
   {
      discard;
   }
   highp vec3 surface = objectPos + ray * t;
   // the shape's normal there: straight out of a face, slanted on the bevel
   highp vec3 corner = max(abs(surface - centre) - (halfSize - bevelRadius), 0.0);
   highp vec3 normal = length(corner) > 1e-6 ? normalize(sign(surface - centre) * corner) : face;
   float bevel = smoothstep(0.03, 0.25, 1.0 - max(max(abs(normal.x), abs(normal.y)), abs(normal.z)));

   // from just inside the face it enters, to the face it leaves by: per axis the way to the side the ray heads for
   // (at a grazing angle the face it enters gave a way of nothing, and the exit flickered onto the entry)
   highp vec3 start = clamp(surface, boxMin + unit * 0.002, boxMax - unit * 0.002);
   highp vec3 side = mix(boxMin, boxMax, step(0.0, ray));
   highp vec3 far = (side - start) / (sign(ray) * max(abs(ray), vec3(1e-5)));
   highp float through = min(min(far.x, far.y), far.z);
   highp vec3 exit = start + ray * through;
   float depth = clamp(through / unit, 0.0, 1.7);

   float cloud = 0.0;
   for (int k = 0; k < 6; k++)
   {
      highp vec3 p = (start + ray * through * (float(k) + 0.5) / 6.0 - boxMin) / unit;
      cloud += valueNoise3(p * 2.2) * 0.75 + valueNoise3(p * 4.1 + 7.0) * 0.25;
   }
   cloud /= 6.0;

   vec2 screen = (gl_FragCoord.xy - viewport.xy) / viewport.zw;
   // frosted: what's behind seen blurred, more so the deeper it lies
   vec2 at = screen - normal.xy * 0.03 * (0.4 + depth);
   vec2 spread = vec2(0.004, 0.006) * (0.6 + depth);
   vec3 behind = (texture(refraction, clamp(at, 0.0, 1.0)).rgb * 2.0 +
                  texture(refraction, clamp(at + vec2(spread.x, spread.y), 0.0, 1.0)).rgb +
                  texture(refraction, clamp(at + vec2(-spread.x, spread.y), 0.0, 1.0)).rgb +
                  texture(refraction, clamp(at + vec2(spread.x, -spread.y), 0.0, 1.0)).rgb +
                  texture(refraction, clamp(at - spread, 0.0, 1.0)).rgb) / 6.0;

   // the glass takes on its colour, what's behind fades into it the deeper it lies: deep in, only the glow
   vec3 color = behind * tint * mix(1.2, 0.45, clamp(depth, 0.0, 1.0));
   // the light inside, in clouds filling the body, saturated, and hot cores where they're densest, as in the BBQ
   float glowing = smoothstep(0.3, 0.8, cloud);
   color += tint * (0.5 + 1.5 * glowing) * (0.45 + 0.75 * depth);
   color += mix(tint, vec3(1.0), 0.35) * smoothstep(0.62, 0.9, cloud) * depth * 0.9;
   // the far edges seen through it, ground off as wide as the near ones
   float inner = smoothstep(0.16, 0.04, edgeDistance(exit) / unit);
   color += mix(tint, vec3(1.0), 0.35) * inner * inner * 0.7;
   // the bevels catch the light (from the top left, above), lit glass the brightest of the block
   highp vec3 light = normalize(vec3(-0.45, 0.35, 0.82));
   color += mix(tint, vec3(1.0), 0.55) * bevel * (0.35 + 0.9 * max(dot(normal, light), 0.0));
   float fresnel = pow(1.0 - clamp(abs(dot(normal, e)), 0.0, 1.0), 3.0);
   // the sheen at grazing angles: a smooth reflection, the blocks' specular sphere map is grainy and sparkled all
   // over the faces seen edge on
   color += mix(tint, vec3(1.0), 0.7) * fresnel * 0.35;
   return color;
}

void main()
{
   vec3 n = normalize(nrm);
   vec3 e = normalize(eye);
   vec3 refl = e - n * 2.0 * dot(n, e);

   vec4 diff = textureSphere(diffusemap, n);
   vec4 spec = textureSphere(specularmap, refl);
   // the face's own normal (the mesh's are smoothed over its edges), which way it looks decides its turn
   vec3 face = normalize(cross(dFdx(world), dFdy(world)));
   vec4 tex = textureGrad(texturemap, turned(uv1, face), dFdx(uv1), dFdy(uv1));
   vec4 ambi = texture(ambientmap, uv2);

   if (glass > 0.5)
   {
      o_color = vec4(glassBlock(face, e), 1.0);
      return;
   }

   vec4 shad = texture(shadowmap, uv3);

   shad = shad * faceShadow + vec4(1.0, 1.0, 1.0, 1.0) * (1.0 - faceShadow);

   vec4 final = (ambi.r * diff * tex * shad) + (spec * tex.a);

   // the story's skins mark what glows of its own (crystals, embers, lamps) with alpha below 1: its colour shines on
   // top of the light it gets, so it glows in the shade too; the original levels' skins are opaque, stay as they were
   float glow = story * (1.0 - tex.a);
   final.rgb += tex.rgb * glow * 1.6;

   o_color = vec4(final.rgb, 1.0);
}

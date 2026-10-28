#version 300 es
precision mediump float;

// painted colour times baked ambient occlusion, lit by one soft key light from the camera side

uniform sampler2D u_color;
uniform sampler2D u_occlusion;

// the colour by night and how much of the day's colour shows (1 by day)
uniform sampler2D u_night;
uniform float u_has_night;
uniform float u_daylight;
uniform float u_flash;
uniform float u_alpha;
uniform vec3 u_tint;

// fragments above this height are dropped (mirror images)
uniform highp float u_clip;

// cutouts (alphaMode MASK): fragments whose colour's alpha is below this are dropped, 0 for opaque materials;
// double sided ones are lit on their back with the normal flipped
uniform float u_alpha_cutoff;
uniform float u_double_sided;

// a fur shell (see gltf-vert.glsl), its layer 0 for everything else
uniform highp vec4 u_shell;

// fine surface detail tiled in world space: up to four detail images, the mask's rgba (at the colour's uvs) how
// much of each shows where, their repeats per unit; images average mid grey, so far away (in the mipmaps) they
// leave the colour as it is
uniform sampler2D u_detail_mask;
uniform sampler2D u_detail0;
uniform sampler2D u_detail1;
uniform sampler2D u_detail2;
uniform sampler2D u_detail3;
uniform vec4 u_detail_scales;
uniform float u_detail_strength;

// the air: the land fades into the haze's rgb (its a how much at most) from near to far outside the box
// (x0, y0, x1, y1, the field), the ambient light's and the rim light's colours
uniform vec4 u_haze;
uniform highp vec4 u_haze_box;
uniform vec2 u_haze_range;
uniform vec3 u_ambient;
uniform vec3 u_rim;

in vec2 uv;
in vec3 nrm;
in highp float height;
in highp vec3 world;
in highp vec3 local;
out vec4 o_color;

highp float hash(highp vec3 p)
{
   p = fract(p * vec3(0.1031, 0.1030, 0.0973));
   p += dot(p, p.yxz + 33.33);
   return fract((p.x + p.y) * p.z);
}

// how far a fur shell's point is from the strand of its cell, relative to the strand's thickness there: below 1
// it's on the strand. Strands stand on a jittered grid in the bare model's space, each its own length, thinning
// to their tips; two grids half a cell apart, so the surface cuts through enough of them
float strand(highp vec3 p, float layer)
{
   float nearest = 9.0;
   for (int grid = 0; grid < 2; ++grid)
   {
      highp vec3 q = p + float(grid) * 0.5;
      highp vec3 cell = floor(q);
      highp vec3 centre = vec3(hash(cell), hash(cell + 17.0), hash(cell + 31.0)) * 0.6 + 0.2;
      float reach = 0.55 + 0.45 * hash(cell + 53.0);
      float thickness = 0.42 * (1.0 - layer / reach);
      if (layer < reach)
      {
         nearest = min(nearest, length(q - cell - centre) / max(thickness, 0.001));
      }
   }
   return nearest;
}

// one detail image at a point, at two unrelated scales mixed so its repeats don't show
vec3 detailAt(sampler2D image, highp vec2 point, highp float scale)
{
   vec3 mixed = (texture(image, point * scale).rgb + texture(image, point * scale * 0.37 + vec2(0.31, 0.73)).rgb) * 0.5;
   return vec3(0.5) + (mixed - vec3(0.5)) * 1.4;
}

// projected along the axes and blended by the normal, so walls and cliffs don't smear
vec3 triplanar(sampler2D image, highp float scale, vec3 weights)
{
   vec3 result = vec3(0.0);
   if (weights.z > 0.01)
   {
      result += detailAt(image, world.xy, scale) * weights.z;
   }
   if (weights.y > 0.01)
   {
      result += detailAt(image, world.xz, scale) * weights.y;
   }
   if (weights.x > 0.01)
   {
      result += detailAt(image, world.yz, scale) * weights.x;
   }
   return result;
}

void main()
{
   if (height > u_clip)
   {
      discard;
   }

   if (u_alpha_cutoff > 0.0)
   {
      // the mipmaps average the blades with the gaps between them, their alpha sinks below the cutoff and the
      // cutout would thin out with the distance; scaled up by the mipmap level it keeps its coverage
      highp vec2 texels = uv * vec2(textureSize(u_color, 0));
      highp float level = log2(max(max(length(dFdx(texels)), length(dFdy(texels))), 1.0));
      if (texture(u_color, uv).a * (1.0 + level * 0.25) < u_alpha_cutoff)
      {
         discard;
      }
   }

   float shell = u_shell.y;
   if (shell > 0.0 && (texture(u_color, uv).a < 0.5 || strand(local * u_shell.z, shell) > 1.0))
   {
      discard;
   }

   vec3 light = normalize(vec3(-0.35, -0.55, 0.75));
   vec3 n = normalize(nrm);
   if (u_double_sided > 0.5 && !gl_FrontFacing)
   {
      n = -n;
   }
   float diffuse = max(dot(n, light), 0.0);
   float rim = pow(1.0 - max(n.z, 0.0), 3.0) * 0.15;

   vec3 albedo = texture(u_color, uv).rgb;
   if (u_has_night > 0.5)
   {
      albedo = mix(texture(u_night, uv).rgb, albedo, u_daylight);
   }

   // detail only where it can be resolved: on faces seen edge on or far away (a few times the repeats per pixel
   // of the ground under the game's camera) it only shimmers as the camera moves
   highp float footprint = max(length(dFdx(world)), length(dFdy(world))) * max(max(u_detail_scales.x, u_detail_scales.y),
                                                                             max(u_detail_scales.z, u_detail_scales.w));
   float resolved = 1.0 - smoothstep(0.03, 0.08, footprint);

   if (u_detail_strength > 0.0 && resolved > 0.0)
   {
      vec3 weights = pow(abs(n), vec3(4.0));
      weights /= weights.x + weights.y + weights.z;
      vec4 mask = texture(u_detail_mask, uv);
      vec3 detail = triplanar(u_detail0, u_detail_scales.x, weights) * mask.r +
                    triplanar(u_detail1, u_detail_scales.y, weights) * mask.g +
                    triplanar(u_detail2, u_detail_scales.z, weights) * mask.b +
                    triplanar(u_detail3, u_detail_scales.w, weights) * mask.a;
      detail += vec3(0.5) * max(1.0 - dot(mask, vec4(1.0)), 0.0);
      albedo *= mix(vec3(1.0), detail * 2.0, u_detail_strength * resolved);
   }
   // the fur darker down at its roots, where little light gets in
   if (shell > 0.0)
   {
      albedo *= 0.7 + 0.45 * shell;
   }
   albedo *= u_tint;
   float occlusion = texture(u_occlusion, uv).r;

   vec3 color = albedo * occlusion * (0.45 * u_ambient + 0.75 * diffuse) + rim * u_rim;

   if (u_haze.a > 0.0)
   {
      highp vec2 outside = max(max(u_haze_box.xy - world.xy, world.xy - u_haze_box.zw), vec2(0.0));
      color = mix(color, u_haze.rgb, smoothstep(u_haze_range.x, u_haze_range.y, length(outside)) * u_haze.a);
   }
   color = mix(color, vec3(1.0), u_flash);

   o_color = vec4(color, u_alpha);
}

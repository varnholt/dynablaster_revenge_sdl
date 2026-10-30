#version 300 es
precision mediump float;

// painted colour lit by the light baked into the scenery, or by the world's sun and ambient light over the baked
// ambient occlusion

uniform sampler2D u_color;
uniform sampler2D u_occlusion;

// the colour by night and how much of the day's colour shows (1 by day)
uniform sampler2D u_night;
uniform float u_has_night;
uniform float u_daylight;
uniform float u_flash;
uniform float u_alpha;
uniform vec3 u_tint;

// a screen (u_screen 1): live static between its dark and bright colour, scanlines, a bar rolling up, unlit;
// u_time is the scenery's clock in seconds
uniform float u_screen;
uniform vec3 u_screen_dark;
uniform vec3 u_screen_bright;
uniform highp float u_time;

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

// the world's sun: towards it (z-up) and its colour
uniform vec3 u_sun_direction;
uniform vec3 u_sun;

// the light the water throws (GltfModel's "caustics"): x strength (0 for none), y cells per unit, z speed, w the
// units above and below the water level they fade out over; their colour, the water's height (world z) and the
// game's clock in seconds
uniform vec4 u_caustics;
uniform vec3 u_caustic_color;
uniform highp float u_water_level;
uniform highp float u_clock;

// the light baked into the scenery in colour, a scale-th of it (GltfModel's "lightmap"); 0 scale for none, the
// sun and the ambient light over the occlusion then
uniform sampler2D u_lightmap;
uniform float u_lightmap_scale;

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
      float reach = 0.45 + 0.55 * hash(cell + 53.0);
      // thick at the root, a fine point at the tip: soft, not a bristle
      float thickness = 0.62 * pow(max(1.0 - layer / reach, 0.0), 1.4);
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

highp float screenHash(highp vec2 p)
{
   p = fract(p * vec2(0.1031, 0.1030));
   p += dot(p, p.yx + 33.33);
   return fract((p.x + p.y) * p.x);
}

// one layer of the caustics' web: the thin lines where a wavy field crosses zero, the field warped as it drifts
highp float causticLayer(highp vec2 p, highp float t)
{
   p += 0.5 * vec2(sin(p.y * 1.3 + t * 1.1), sin(p.x * 1.2 - t * 0.9));
   highp float field = sin(p.x * 2.1 + t) + sin(p.y * 1.9 - t * 0.8) + sin((p.x - p.y) * 1.3 + t * 0.6);
   return exp(-abs(field) * 3.5);
}

// how much of the water's light falls on a point (world space): two layers drifting their own ways, brightest where
// their lines cross; walls catch it too, their height shifts the pattern
float caustics(highp vec3 at)
{
   if (u_caustics.x <= 0.0)
   {
      return 0.0;
   }

   highp vec2 p = (at.xy + vec2(0.6, -0.6) * at.z) * u_caustics.y * 3.1416;
   highp float t = u_clock * u_caustics.z;
   float web = causticLayer(p, t) + causticLayer(mat2(0.8, -0.6, 0.6, 0.8) * p * 1.37 + 3.1, t * 1.23);
   web = web * (0.6 + 0.4 * web);

   // too fine to resolve far away it would only flicker, it evens out to a faint glow
   highp vec2 footprint = fwidth(p);
   web = mix(web, 0.15, smoothstep(0.4, 1.2, max(footprint.x, footprint.y)));

   float near = exp(-abs(at.z - u_water_level) / max(u_caustics.w, 0.001));
   return web * near * u_caustics.x;
}

void main()
{
   if (height > u_clip)
   {
      discard;
   }

   if (u_screen > 0.5)
   {
      // static in coarse cells, a new pattern twelve times a second, over a slow blotchy picture
      highp float frame = floor(u_time * 12.0);
      highp vec2 cell = floor(uv * vec2(96.0, 64.0));
      float grain = screenHash(cell + frame * 17.0);
      highp vec2 blot = floor(uv * vec2(8.0, 5.0));
      float picture = 0.5 + 0.5 * sin(u_time * 0.7 + screenHash(blot) * 6.28);
      float scan = 0.82 + 0.18 * sin(uv.y * 340.0);
      float bar = smoothstep(0.08, 0.0, abs(fract(uv.y * 0.6 - u_time * 0.15) - 0.5)) * 0.35;
      float level = clamp(mix(picture, grain, 0.55) * scan + bar, 0.0, 1.0);
      vec3 screen = mix(u_screen_dark, u_screen_bright, level);
      // dimmer towards the edges, like a tube
      screen *= 1.0 - 0.35 * pow(length(uv - 0.5) * 1.4, 3.0);
      o_color = vec4(screen, u_alpha);
      return;
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
   // strands lean together in tufts further out: each tuft's strands are looked up nearer their tuft's middle
   highp vec3 tuft = (local * u_shell.z * 0.25);
   highp vec3 lean = (vec3(hash(floor(tuft)), hash(floor(tuft) + 7.0), hash(floor(tuft) + 13.0)) - 0.5) * 2.0;
   if (shell > 0.0 && (texture(u_color, uv).a < 0.5 || strand(local * u_shell.z + lean * shell * 0.8, shell) > 1.0))
   {
      discard;
   }

   vec3 light = normalize(u_sun_direction);
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
      // the light caught in the tips, each strand a shade of its own
      albedo *= (0.78 + 0.4 * shell) * (0.94 + 0.12 * hash(floor(local * u_shell.z)));
   }
   albedo *= u_tint;
   float occlusion = texture(u_occlusion, uv).r;

   vec3 color = albedo * occlusion * (0.45 * u_ambient + diffuse * u_sun) + rim * u_rim;
   if (u_lightmap_scale > 0.0)
   {
      color = albedo * texture(u_lightmap, uv).rgb * u_lightmap_scale + rim * u_rim;
   }
   color += albedo * u_caustic_color * caustics(world);

   if (u_haze.a > 0.0)
   {
      highp vec2 outside = max(max(u_haze_box.xy - world.xy, world.xy - u_haze_box.zw), vec2(0.0));
      color = mix(color, u_haze.rgb, smoothstep(u_haze_range.x, u_haze_range.y, length(outside)) * u_haze.a);
   }
   color = mix(color, vec3(1.0), u_flash);

   o_color = vec4(color, u_alpha);
}

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
out vec4 o_color;

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

   vec3 light = normalize(vec3(-0.35, -0.55, 0.75));
   vec3 n = normalize(nrm);
   float diffuse = max(dot(n, light), 0.0);
   float rim = pow(1.0 - max(n.z, 0.0), 3.0) * 0.15;

   vec3 albedo = texture(u_color, uv).rgb;
   if (u_has_night > 0.5)
   {
      albedo = mix(texture(u_night, uv).rgb, albedo, u_daylight);
   }
   if (u_detail_strength > 0.0)
   {
      vec3 weights = pow(abs(n), vec3(4.0));
      weights /= weights.x + weights.y + weights.z;
      vec4 mask = texture(u_detail_mask, uv);
      vec3 detail = triplanar(u_detail0, u_detail_scales.x, weights) * mask.r +
                    triplanar(u_detail1, u_detail_scales.y, weights) * mask.g +
                    triplanar(u_detail2, u_detail_scales.z, weights) * mask.b +
                    triplanar(u_detail3, u_detail_scales.w, weights) * mask.a;
      detail += vec3(0.5) * max(1.0 - dot(mask, vec4(1.0)), 0.0);
      albedo *= mix(vec3(1.0), detail * 2.0, u_detail_strength);
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

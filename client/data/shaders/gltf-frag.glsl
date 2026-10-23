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

in vec2 uv;
in vec3 nrm;
in highp float height;
out vec4 o_color;

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
   albedo *= u_tint;
   float occlusion = texture(u_occlusion, uv).r;

   vec3 color = albedo * occlusion * (0.45 + 0.75 * diffuse) + rim;
   color = mix(color, vec3(1.0), u_flash);

   o_color = vec4(color, u_alpha);
}

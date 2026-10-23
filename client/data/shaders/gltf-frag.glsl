#version 300 es
precision mediump float;

// painted colour times baked ambient occlusion, lit by one soft key light from the camera side

uniform sampler2D u_color;
uniform sampler2D u_occlusion;
uniform float u_flash;
uniform float u_alpha;
uniform vec3 u_tint;

in vec2 uv;
in vec3 nrm;
out vec4 o_color;

void main()
{
   vec3 light = normalize(vec3(-0.35, -0.55, 0.75));
   vec3 n = normalize(nrm);
   float diffuse = max(dot(n, light), 0.0);
   float rim = pow(1.0 - max(n.z, 0.0), 3.0) * 0.15;

   vec3 albedo = texture(u_color, uv).rgb * u_tint;
   float occlusion = texture(u_occlusion, uv).r;

   vec3 color = albedo * occlusion * (0.45 + 0.75 * diffuse) + rim;
   color = mix(color, vec3(1.0), u_flash);

   o_color = vec4(color, u_alpha);
}

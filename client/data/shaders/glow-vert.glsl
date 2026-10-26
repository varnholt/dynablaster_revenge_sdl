#version 300 es

// light halos: a camera facing quad per light, sized in world units, pulled towards the camera so
// the lamp it belongs to and the wall behind don't cut it. A fire adds a second quad for its flame:
// standing on the light's position, upright along the world's z axis as far as the camera shows it,
// swaying and flickering in height; its halo floats in the flame's middle and flickers with it.

layout(location = 0) in vec3 a_center;
layout(location = 1) in vec2 a_corner;
layout(location = 2) in vec3 a_color;

// size, mode (0 steady, 1 flame, 2 blink, 3 pulse, 4 fire, 5 a fire's flame), phase
layout(location = 3) in vec3 a_light;

uniform mat4 u_modelViewProjection;
uniform float u_time;

// viewport width / height
uniform float u_aspect;

out vec2 corner;
out vec3 color;

// x: 1 on a flame, 2 on a fire's halo, y: the time, z: 1 when the flame throws sparks
out highp vec3 fire;

// how high a fire burns right now, about 0.6..1; its halo and flame share it
float flicker(float t)
{
   return 0.82 + 0.09 * sin(t * 11.0) + 0.06 * sin(t * 23.0 + 1.7) + 0.05 * sin(t * 5.3 + 0.4);
}

void main()
{
   float t = u_time + a_light.z;
   float mode = a_light.y;
   corner = a_corner;
   color = a_color;
   fire = vec3(0.0, mod(t, 1000.0), 0.0);

   if (mode > 4.5)
   {
      // the flame: its base on the light's position, its tip size * 0.75 above, size * 0.2 to either side
      float size = a_light.x;
      vec4 base = u_modelViewProjection * vec4(a_center, 1.0);
      vec4 tip = u_modelViewProjection * vec4(a_center + vec3(0.0, 0.0, size * 0.75), 1.0);
      vec4 side = u_modelViewProjection * vec4(a_center + vec3(size * 0.2, 0.0, 0.0), 1.0);
      vec2 scale = vec2(u_aspect, 1.0);
      vec2 b = base.xy / base.w * scale;
      vec2 up = tip.xy / tip.w * scale - b;
      float half_width = length(side.xy / side.w * scale - b);

      // seen from above, z shortens: never squatter than this
      float height = max(length(up), half_width * 3.0);
      up = length(up) > 1e-5 ? normalize(up) : vec2(0.0, 1.0);
      vec2 across = vec2(up.y, -up.x);

      float v = a_corner.y * 0.5 + 0.5;
      float sway = 0.35 * sin(t * 1.9) + 0.2 * sin(t * 4.7 + 0.8);
      vec2 p = b + up * v * height * flicker(t) + across * (a_corner.x + sway * v * v) * half_width;

      float pull = min(size * 0.25, base.w * 0.5);
      float w = base.w - pull;
      gl_Position = vec4(p / scale * w, base.z - pull, w);
      fire = vec3(1.0, mod(t, 1000.0), size > 1.1 ? 1.0 : 0.0);
      return;
   }

   vec3 position_world = a_center;
   float strength = 1.0;
   if (mode > 3.5)
   {
      // a fire's halo around the flame's middle, breathing with it
      position_world.z += a_light.x * 0.3;
      strength = flicker(t) * 0.8;
      fire.x = 2.0;
   }
   else if (mode > 2.5)
   {
      strength = 0.65 + 0.35 * sin(t * 2.5);
   }
   else if (mode > 1.5)
   {
      strength = fract(t * 0.7) < 0.55 ? 1.0 : 0.12;
   }
   else if (mode > 0.5)
   {
      strength = 0.8 + 0.12 * sin(t * 13.0) + 0.08 * sin(t * 31.0 + 1.3);
   }

   vec4 center = u_modelViewProjection * vec4(position_world, 1.0);
   vec4 side = u_modelViewProjection * vec4(position_world + vec3(a_light.x, 0.0, 0.0), 1.0);
   float radius = length(side.xy / side.w - center.xy / center.w);

   color = a_color * strength;

   vec2 position = center.xy / center.w + a_corner * vec2(radius, radius * u_aspect);
   float pull = min(a_light.x * 1.5, center.w * 0.5);
   float w = center.w - pull;
   gl_Position = vec4(position * w, center.z - pull, w);
}

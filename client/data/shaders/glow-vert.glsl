#version 300 es

// light halos: a camera facing quad per light, sized in world units, pulled towards the camera so
// the lamp it belongs to and the wall behind don't cut it

layout(location = 0) in vec3 a_center;
layout(location = 1) in vec2 a_corner;
layout(location = 2) in vec3 a_color;

// size, mode (0 steady, 1 flame, 2 blink, 3 pulse), phase
layout(location = 3) in vec3 a_light;

uniform mat4 u_modelViewProjection;
uniform float u_time;

// viewport width / height
uniform float u_aspect;

out vec2 corner;
out vec3 color;

void main()
{
   vec4 center = u_modelViewProjection * vec4(a_center, 1.0);
   vec4 side = u_modelViewProjection * vec4(a_center + vec3(a_light.x, 0.0, 0.0), 1.0);
   float radius = length(side.xy / side.w - center.xy / center.w);

   float t = u_time + a_light.z;
   float strength = 1.0;
   if (a_light.y > 2.5)
   {
      strength = 0.65 + 0.35 * sin(t * 2.5);
   }
   else if (a_light.y > 1.5)
   {
      strength = fract(t * 0.7) < 0.55 ? 1.0 : 0.12;
   }
   else if (a_light.y > 0.5)
   {
      strength = 0.8 + 0.12 * sin(t * 13.0) + 0.08 * sin(t * 31.0 + 1.3);
   }

   corner = a_corner;
   color = a_color * strength;

   vec2 position = center.xy / center.w + a_corner * vec2(radius, radius * u_aspect);
   float pull = min(a_light.x * 1.5, center.w * 0.5);
   float w = center.w - pull;
   gl_Position = vec4(position * w, center.z - pull, w);
}

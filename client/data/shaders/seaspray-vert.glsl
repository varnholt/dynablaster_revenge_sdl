#version 300 es

// sea spray (SeaSpray): a camera facing quad per droplet or puff of mist, sized in world units

layout(location = 0) in vec3 a_center;
layout(location = 1) in vec2 a_corner;

// x: size, y: opacity, negative for mist
layout(location = 2) in vec2 a_droplet;

uniform mat4 u_modelViewProjection;

// viewport width / height
uniform float u_aspect;

out vec2 corner;

// x: opacity, y: 1 for mist
out vec2 kind;

void main()
{
   corner = a_corner;
   kind = vec2(abs(a_droplet.y), a_droplet.y < 0.0 ? 1.0 : 0.0);

   vec4 center = u_modelViewProjection * vec4(a_center, 1.0);
   vec4 side = u_modelViewProjection * vec4(a_center + vec3(a_droplet.x, 0.0, 0.0), 1.0);
   // never smaller than a pixel or so, a droplet far off would flicker in and out
   float radius = max(length(side.xy / side.w - center.xy / center.w), 0.0015);

   vec2 position = center.xy / center.w + a_corner * vec2(radius, radius * u_aspect);
   gl_Position = vec4(position * center.w, center.z, center.w);
}

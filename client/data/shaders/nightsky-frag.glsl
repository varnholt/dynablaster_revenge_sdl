#version 300 es
precision highp float;

// the night sky by the direction a fragment looks in (z up): the soft sky from an equirectangular map, sharp
// twinkling stars and the moon on top

uniform mat4 u_inverse;
uniform sampler2D u_sky;
uniform float u_time;
uniform float u_brightness;

// towards the moon, its size (radians)
uniform vec3 u_moon;
uniform float u_moon_size;

in vec2 clip;
out vec4 o_color;

const float PI = 3.14159265;

float hash(vec3 p)
{
   p = fract(p * vec3(443.897, 441.423, 437.195));
   p += dot(p, p.yzx + 19.19);
   return fract((p.x + p.y) * p.z);
}

// stars in cells of a grid over the directions, at most one per cell
vec3 stars(vec3 direction, float density, float size)
{
   vec3 p = direction * density;
   vec3 cell = floor(p);
   float chance = hash(cell);

   if (chance > 0.12)
   {
      return vec3(0.0);
   }

   vec3 centre = cell + 0.2 + 0.6 * vec3(hash(cell + 1.7), hash(cell + 3.1), hash(cell + 5.3));
   float distance = length(p - centre);

   // a pixel or two whatever the resolution
   float pixel = fwidth(p.x) + fwidth(p.y);
   float disc = 1.0 - smoothstep(size * 0.5, size + pixel, distance);
   float twinkle = 0.7 + 0.3 * sin(u_time * (1.5 + 3.0 * hash(cell + 7.7)) + 40.0 * chance);
   float brightness = pow(hash(cell + 9.1), 4.0) * 2.2 + 0.15;
   vec3 tint = mix(vec3(0.75, 0.82, 1.0), vec3(1.0, 0.92, 0.8), hash(cell + 2.2));
   return tint * disc * twinkle * brightness;
}

void main()
{
   vec4 near = u_inverse * vec4(clip, -1.0, 1.0);
   vec4 far = u_inverse * vec4(clip, 1.0, 1.0);
   vec3 direction = normalize(far.xyz / far.w - near.xyz / near.w);

   float elevation = asin(clamp(direction.z, -1.0, 1.0));
   float azimuth = atan(direction.y, direction.x);
   vec3 color = texture(u_sky, vec2(azimuth / (2.0 * PI) + 0.5, elevation / PI + 0.5)).rgb;

   // fewer and fainter down in the haze over the horizon
   float clear = smoothstep(0.02, 0.3, direction.z);
   color += (stars(direction, 110.0, 0.09) + 0.45 * stars(direction.zxy, 220.0, 0.1)) * clear;

   // the moon: a crescent with a soft halo
   float angle = acos(clamp(dot(direction, u_moon), -1.0, 1.0));
   vec3 shadow = normalize(u_moon + vec3(0.012, -0.004, 0.006));
   float lit = 1.0 - smoothstep(u_moon_size * 0.9, u_moon_size, angle);
   float dark = 1.0 - smoothstep(u_moon_size * 0.85, u_moon_size, acos(clamp(dot(direction, shadow), -1.0, 1.0)));
   color = mix(color, vec3(1.0, 0.97, 0.86), lit * (1.0 - dark * 0.92));
   color += vec3(0.25, 0.3, 0.45) * exp(-angle / (u_moon_size * 3.0)) * 0.35;

   o_color = vec4(color * u_brightness, 1.0);
}

#version 300 es
precision mediump float;

// a hot core in a soft halo, added onto what's behind

in vec2 corner;
in vec3 color;

out vec4 o_color;

void main()
{
   float r2 = dot(corner, corner);
   float halo = exp(-r2 * 4.0) * (1.0 - smoothstep(0.6, 1.0, r2));
   float core = exp(-r2 * 45.0);
   o_color = vec4(color * (halo * 0.7 + core * 1.4), 1.0);
}

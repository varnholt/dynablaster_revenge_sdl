#version 300 es
precision mediump float;

// a droplet: a small bright bead; mist: a soft, frayed puff

in vec2 corner;
in vec2 kind;

uniform vec3 u_color;

out vec4 o_color;

void main()
{
   float d = length(corner);
   if (d > 1.0)
   {
      discard;
   }

   float shape;
   if (kind.y > 0.5)
   {
      shape = pow(1.0 - d, 1.6) * (0.75 + 0.25 * sin(corner.x * 7.0 + corner.y * 5.0));
   }
   else
   {
      shape = smoothstep(1.0, 0.45, d);
   }

   o_color = vec4(u_color, shape * kind.x);
}

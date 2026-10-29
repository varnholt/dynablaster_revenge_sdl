#version 300 es
precision mediump float;

// a glass block's light on the floor: brightest at its foot, fading out in a rounded square round it, added to what's
// there (the blend adds), so the floor seems lit by the glass

uniform vec3 tint;
uniform float strength;

in vec2 across;
out vec4 o_color;

void main()
{
   // 0 at the block's foot (the inner 40 % of the quad), 1 at the quad's edge
   float d = length(max(abs(across) - 0.4, 0.0)) / 0.6;
   float light = 1.0 - smoothstep(0.0, 1.0, d);
   o_color = vec4(tint * light * light * strength, 1.0);
}

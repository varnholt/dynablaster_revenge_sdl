#version 300 es
precision mediump float;

// PSD layer draw that fades the texture towards its luminance, "intensity" 0..1 (dead players'
// icons in the game HUD)
uniform float intensity;
uniform float alpha;
uniform sampler2D tex;

in vec2 uv;
out vec4 o_color;

void main()
{
   vec4 color = texture(tex, uv);
   float grey = dot(color.rgb, vec3(0.3, 0.59, 0.11));
   o_color = vec4(mix(color.rgb, vec3(grey), intensity), color.a * alpha);
}

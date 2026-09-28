#version 300 es

// x, y: corner offset; z: position along the sun ray; w: rotation speed
layout(location = 0) in vec4 a_position;
// x, y: offset (only y is used); z, w: uv
layout(location = 1) in vec4 a_offset_uv;
layout(location = 2) in vec3 a_color;

uniform mat4 u_modelViewProjection;
uniform vec2 sun;
uniform float time;
uniform float normalizedlength;

out vec2 uv;
out vec3 color;
out float alpha;

vec2 rotate(vec2 p, float speed)
{
   float r = speed * time;
   mat2 rotation = mat2(vec2(cos(r), sin(r)), vec2(-sin(r), cos(r)));
   return rotation * p;
}

void main()
{
   color = a_color;
   vec2 offset = a_offset_uv.xy;
   uv = a_offset_uv.zw;

   vec2 pos = rotate(a_position.xy, a_position.w);

   alpha = normalizedlength;

   // shift along the ray
   pos += sun * (1.0 - a_position.z);

   // shift perpendicular to the ray
   pos += vec2(sun.y, -sun.x) * offset.y;

   gl_Position = u_modelViewProjection * vec4(pos, 0.0, 1.0);
}

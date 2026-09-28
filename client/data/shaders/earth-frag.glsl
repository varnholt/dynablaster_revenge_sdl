#version 300 es
precision highp float;

uniform sampler2D texturemap;  // color + specular (in alpha)
uniform sampler2D normalmap;   // normal + clouds (in alpha)
uniform vec3 moveCloud;

in vec2 uv;
in vec3 lightDir;
in vec3 viewDir;

out vec4 o_color;

const vec4 specular_color = vec4(1.0, 0.8, 0.8, 1.0);

void main()
{
   vec4 col = vec4(0.0);

   vec4 tex_color = texture(texturemap, uv);

   float cloud_color = texture(normalmap, uv + moveCloud.xy).a;
   float cloud_shadow = texture(normalmap, uv + moveCloud.xy + vec2(lightDir.x, -lightDir.y) * 0.0025).a;

   // less specular on clouds
   float specular_value = tex_color.a * 0.5 + 0.5;
   specular_value *= max(1.0 - cloud_color * cloud_color * 1.5, 0.0);

   vec3 normal = texture(normalmap, uv).xyz * 2.0 - 1.0;

   vec3 ld = normalize(lightDir);
   vec3 vd = normalize(viewDir);

   float intensity = max(dot(normal, ld) * 1.5, 0.0);
   col += vec4(tex_color.rgb, 1.0) * intensity;

   vec3 refl = vd - normal * 2.0 * dot(normal, vd);
   float specular = dot(refl, ld);

   // 0.8^30 is about 0
   if (specular > 0.8)
   {
      col += specular_color * pow(specular, 30.0) * specular_value;
   }

   col = col * (1.0 - cloud_shadow * 0.5);

   float fresnel = 1.0 - vd.z * vd.z;
   if (fresnel > 0.5)
   {
      float f = pow(fresnel, 10.0);
      col = col * (1.0 - f) + vec4(0.7, 0.9, 1.5, 1.0) * f;
   }

   col += cloud_color;

   o_color = col;
}

#version 330

in vec3 v_world_normal;
in vec2 v_vertex_uv;

out vec4 o_color;

uniform sampler2D u_texture;



void main()
{
  vec3 normal = normalize(v_world_normal);
  vec3 alpha = (normal + 1.) * .5;
  vec2 bt = mix(vec2(.3, .7), vec2(.4, .8), alpha.xz);
  vec3 ambient = mix(vec3(bt.x, .3, bt.x), vec3(bt.y, .8, bt.y), alpha.y);
  
  o_color = vec4(ambient, 1.) * texture(u_texture, v_vertex_uv);
}
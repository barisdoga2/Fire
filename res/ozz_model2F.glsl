#version 330

in vec2 v_vertex_uv;
in mat3 v_TBN;

out vec4 o_color;

uniform sampler2D u_texture;



void main()
{
  o_color = vec4(texture(u_texture, v_vertex_uv).rgb + vec3(0,0,v_TBN[0][0]), 1.0);
}
#version 330



layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_normal;

out vec3 v_world_normal;
out vec2 v_vertex_uv;

uniform mat4 u_model;
uniform mat4 u_viewproj;



void main()
{
  mat4 world_matrix = u_model;
  
  vec4 vertex = vec4(a_position.xyz, 1.);
  
  gl_Position = u_viewproj * world_matrix * vertex;
  
  mat3 cross_matrix = mat3(
    cross(world_matrix[1].xyz, world_matrix[2].xyz),
    cross(world_matrix[2].xyz, world_matrix[0].xyz),
    cross(world_matrix[0].xyz, world_matrix[1].xyz));
  
  float invdet = 1.0 / dot(cross_matrix[2], world_matrix[2].xyz);
  
  mat3 normal_matrix = cross_matrix * invdet;
  
  v_world_normal = normal_matrix * a_normal;
    
  v_vertex_uv = a_uv;
}
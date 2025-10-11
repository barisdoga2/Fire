#version 330

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;
layout (location = 4) in uvec4 a_boneIds;
layout (location = 5) in vec4 a_weights;

out vec2 v_vertex_uv;
out vec3 v_frag_pos;
out mat3 v_TBN;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_bones[128];

void main()
{
    // Calculate skinning matrix from bone weights
    mat4 skinMat =
        a_weights.x * u_bones[a_boneIds.x] +
        a_weights.y * u_bones[a_boneIds.y] +
        a_weights.z * u_bones[a_boneIds.z] +
        a_weights.w * u_bones[a_boneIds.w];

    // Transform position
    vec4 worldPos = u_model * skinMat * vec4(a_position, 1.0);
    v_frag_pos = worldPos.xyz;
	
    // --- Transform normal and tangent ---
    mat3 normalMat = mat3(u_model * skinMat);
    vec3 N = normalize(normalMat * a_normal);
    vec3 T = normalize(normalMat * a_tangent);
    vec3 B = normalize(cross(N, T)); // bitangent

    v_TBN = mat3(T, B, N);

    v_vertex_uv = a_uv;

    gl_Position = u_proj * u_view * worldPos;
}
#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 worldPos;
    vec3 worldNormal;
} gs_in[];

out vec4 gColor;

uniform mat4 view;
uniform mat4 proj;
uniform float normalLength = 0.2;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        vec3 p = gs_in[i].worldPos;
        vec3 n = normalize(gs_in[i].worldNormal);

        // start of normal
        gl_Position = proj * view * vec4(p, 1.0);
        gColor = vec4(1.0, 0.0, 0.0, 1.0);
        EmitVertex();

        // end of normal
        gl_Position = proj * view * vec4(p + n * normalLength, 1.0);
        gColor = vec4(0.0, 1.0, 0.0, 1.0);
        EmitVertex();

        EndPrimitive();
    }
}

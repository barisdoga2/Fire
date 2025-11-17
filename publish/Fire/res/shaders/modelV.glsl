#version 400 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 boneMatrices[200];
uniform int animated;

out vec2 pass_uv;
out vec3 pass_normal;
out vec3 pass_worldPos;

void main()
{
    pass_uv = uv;

    vec4 skinnedPos  = vec4(0.0);
    vec3 skinnedNorm = vec3(0.0);

    if (animated == 1)
    {
        for (int i = 0; i < 4; i++)
        {
            int id = boneIds[i];
            float w = weights[i];

            if (id < 0) continue;
            if (id >= 200) continue;

            mat4 bone = boneMatrices[id];

            skinnedPos  += bone * vec4(position, 1.0) * w;
            skinnedNorm += mat3(bone) * normal * w;
        }
    }
    else
    {
        skinnedPos  = vec4(position, 1.0);
        skinnedNorm = normal;
    }

    vec4 worldPos = model * skinnedPos;
    pass_worldPos = worldPos.xyz;

    pass_normal = normalize(mat3(model) * skinnedNorm);

    gl_Position = proj * view * worldPos;
}

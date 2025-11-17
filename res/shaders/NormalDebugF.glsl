#version 330 core

in vec3 vNormalVS;
out vec4 FragColor;

void main()
{
    vec3 n = normalize(vNormalVS) * 0.5 + 0.5; // [-1,1] -> [0,1]
    FragColor = vec4(n, 1.0);
}

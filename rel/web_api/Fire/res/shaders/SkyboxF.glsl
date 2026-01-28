#version 330 core

out vec4 FragColor;

void main()
{
    // Solid sky color
    vec3 skyColor = vec3(0.5, 0.7, 1.0); // choose any color

    FragColor = vec4(skyColor, 1.0);
}

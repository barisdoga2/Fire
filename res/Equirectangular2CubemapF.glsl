#version 330 core

const vec2 invAtan = vec2(0.1591, 0.3183);


in vec3 pass_worldpos;

uniform sampler2D equirectangularMap;

out vec4 out_Color;



vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(pass_worldpos));
    vec3 color = texture(equirectangularMap, vec2(uv.x, 1-uv.y)).rgb;
    
    out_Color = vec4(color, 1.0);
}
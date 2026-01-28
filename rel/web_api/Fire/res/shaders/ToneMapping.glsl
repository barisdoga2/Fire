vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v*(v + 0.0245786) - 0.000090537;
    vec3 b = v*(0.983729*v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 Reinhard(vec3 v)
{
    return v / (v + vec3(1.0));
}

vec3 ToneMapping(vec3 v)
{
    //return Reinhard(v);
    return RRTAndODTFit(v);
}
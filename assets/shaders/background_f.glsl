#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outFragColor;

uniform sampler2D equirectangularMap;
uniform float exposure;
const vec2 invAtan = vec2(0.1591, 0.3183);
uniform float day_time;
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 N = normalize(inWorldPos);
    N.y = (N.y+1.f) * 0.5f;
//    vec2 uv = SampleSphericalMap(normalize(inWorldPos)); // make sure to normalize localPos

    vec3 color = texture(equirectangularMap,vec2(day_time,N.y)).rgb;
//    color = color /(color+ vec3(1.f));
//    color = vec3(1.f) - exp(-color * exposure);
//    color = pow(color,vec3(1.f/2.2f));
    outFragColor = vec4(color, 1.0);
}
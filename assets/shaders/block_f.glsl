#version 460 core
layout(location = 0) in vec3 iFragPos;
layout(location = 1) in vec3 iFragNormal;
layout(location = 2) in vec2 uv;
layout(location = 3) in float ao;
layout(location = 4) in float fog_factor;
layout(location = 5) in float iDiffuse;
layout(location = 0) out vec4 oFragColor;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}


uniform sampler2D BlockTexture;
uniform sampler2D equirectangularMap;
uniform vec3 view_pos;
uniform float day_time;
uniform float day_light;

void main(){
    vec3 N = normalize(iFragPos-view_pos);
    N.y = (N.y+1.f) * 0.5f;
    vec3 view_dir = normalize(iFragPos - view_pos);
    vec3 sky_color = texture(equirectangularMap,vec2(day_time,N.y)).rgb;
    vec3 color = texture(BlockTexture,uv).rgb;
    if(color == vec3(1.f,0.f,1.f)) discard;
    bool cloud = color == vec3(1.f,1.f,1.f);
    float _diffuse = cloud ? 1.f - iDiffuse * 0.2 : iDiffuse;
    float _ao = cloud ? 1.f - (1.0 - ao) * 0.2:ao;
    vec3 light_color = vec3(day_light*0.3+0.2);
    vec3 ambient = vec3(day_light*0.3+0.2);
    vec3 light = light_color * _diffuse + ambient;
    vec3 block_color = clamp(light * _ao * color,vec3(0.f),vec3(1.f));
    block_color.rgb = mix(block_color.rgb,sky_color,fog_factor);
    oFragColor = vec4(block_color.rgb,1.f);

}
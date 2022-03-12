#version 460 core
layout(location = 0) in vec3 iFragPos;
layout(location = 1) in vec3 iFragNormal;
layout(location = 2) in vec2 uv;
layout(location = 3) in float ao;
layout(location = 0) out vec4 oFragColor;

uniform sampler2D BlockTexture;

void main(){
    vec4 block_color = texture(BlockTexture,uv).rgba;
    if(block_color.rgb == vec3(1.f,0.f,1.f)) discard;
    block_color = block_color * ao;
    oFragColor = vec4(block_color.rgb,1.f);

}
#version 460 core
layout(location = 0) in vec3 iFragPos;
layout(location = 1) in vec3 iFragNormal;
layout(location = 2) in vec2 uv;
layout(location = 0) out vec4 oFragColor;
void main(){
    oFragColor = vec4(1.f,0.5f,0.25f,1.f);
}
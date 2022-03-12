#version 460 core

layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec3 TexCoord;

layout(location = 0) out vec3 oVertexPos;
layout(location = 1) out vec3 oVertexNormal;
layout(location = 2) out vec2 oTexCoord;
layout(location = 3) out float oAO;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main(){
    gl_Position = proj * view * model * vec4(VertexPos,1.f);
    oVertexPos = vec3(model*vec4(VertexPos,1.f));
    oVertexNormal = vec3(model*vec4(VertexNormal,0.f));
    oTexCoord = TexCoord.xy;
    oAO = 0.3 + (1.0 -TexCoord.z) * 0.7;
}
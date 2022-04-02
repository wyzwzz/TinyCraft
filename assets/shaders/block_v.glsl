#version 460 core

layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec3 TexCoord;

layout(location = 0) out vec3 oVertexPos;
layout(location = 1) out vec3 oVertexNormal;
layout(location = 2) out vec2 oTexCoord;
layout(location = 3) out float oAO;
layout(location = 4) out float oFogFactor;
layout(location = 5) out float oDiffuse;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 view_pos;
uniform float fog_distance;
const vec3 light_direction = normalize(vec3(-1.0, 1.0, -1.0));
void main(){
    gl_Position = proj * view * model * vec4(VertexPos,1.f);
    oVertexPos = vec3(model*vec4(VertexPos,1.f));
    oVertexNormal = vec3(model*vec4(VertexNormal,0.f));
    oTexCoord = TexCoord.xy;
    oAO = 0.3 + (1.0 -TexCoord.z) * 0.7;
    float view_dist = distance(view_pos,oVertexPos);
    oFogFactor = pow(clamp(view_dist / fog_distance,0.f,1.f),4.f);
    oDiffuse = max(0.f,dot(normalize(oVertexNormal),normalize(light_direction)));
}
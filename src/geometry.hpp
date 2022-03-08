#pragma once
#include "math.hpp"

struct Vertex{
    float3 pos;
    float3 normal;
    float2 uv;
};
struct Triangle{
    Vertex vertices[3];
};

struct Frustum{

};

struct BoundBox3D{
    float3 min_p;
    float3 max_p;
};
inline bool FrustumIntersectWithBoundBox(const Frustum& frustum,const BoundBox3D& box){
    return false;
}
inline void ExtractFrustumFromProjViewMatrix(const mat4& vp,Frustum& frustum){

}

inline BoundBox3D GetBoundBoxFromFrustum(const Frustum& frustum){
    return {};
}

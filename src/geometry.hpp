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

};
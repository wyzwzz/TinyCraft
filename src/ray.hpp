//
// Created by wyz on 2022/3/2.
//
#pragma once
#include "camera.hpp"
class Ray{
public:
    Ray(const Camera& camera){
        this->origin = camera.position;
        this->direction = normalize(camera.front);
        this->radius = 10.f;
        this->step = 0.01f;
    }
    float3 origin;
    float3 direction;
    float radius;
    float step;
};
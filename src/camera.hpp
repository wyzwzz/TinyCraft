#pragma once
#include "math.hpp"
#include "define.hpp"
class Camera{
public:
    Matrix4 getViewMatrix() const{
        return lookAt(position,target,up);
    }
    Matrix4 getProjMatrix() const{
        return perspective(radians(zoom),aspect,z_near,z_far);
    }

    float3 position{0.5f,-0.5f,10.5f};
    float3 target{0.1f,0.1f,0.f};
    float3 up{0.f,1.f,0.f};
    float3 right{1.f,0.f,0.f};
    float3 front{0.f,0.f,-1.f};
    float yaw{-90.f};
    float pitch{0.f};
    float z_near{0.01f};
    float z_far{300.f};
    float zoom{65.f};//15.f
    float3 world_up{0.f,1.f,0.f};
    float move_sense{0.05f};
    float move_speed{3.f};
    static constexpr float aspect = static_cast<float>(ScreenWidth)/static_cast<float>(ScreenHeight);

};
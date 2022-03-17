#pragma once
#include <string>

const int ScreenWidth = 1280;
const int ScreenHeight = 720;
constexpr const float ScreenAspect = static_cast<float>(ScreenWidth)/static_cast<float>(ScreenHeight);
const std::string AssetsPath = "../assets/";
const std::string ShaderPath = "../assets/shaders/";


#define VSYNC 1

#define SCROLL_THRESHOLD 0.1
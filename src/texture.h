// #define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <iostream>
#include <string>
#include <vector>
enum CubeMapType {
    SHADOW_MAP,
    HDR_MAP,
    PREFILTER_MAP
};
const unsigned int NumSidesInCube = 6;

unsigned int load_texture(char const* path);
unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int loadHDRTexture(char const* path);
unsigned int generateCubeMap(const int width, const int height, CubeMapType cubeType);

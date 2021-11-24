#pragma once
#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

class DirShadow
{
public:
	unsigned int depthMapFBO;
	unsigned int depthMap;
	DirShadow();
};

class PointShadow
{
public:
	unsigned int depthMapFBO;
	unsigned int depthCubemap;
	PointShadow();
};

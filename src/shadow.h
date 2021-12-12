#pragma once
#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

class DirShadow
{
private:
	unsigned int depthMapFBO;
public:
	unsigned int depthMap;
	DirShadow();
	void Bind();
};

class PointShadow
{
private:
	unsigned int depthMapFBO;
public:
	unsigned int depthCubemap;
	PointShadow();
	void Bind();
};


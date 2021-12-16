#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <iostream>
#include <glad/glad.h>
#include "camera.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const unsigned int SHADOW_WIDTH = 2000, SHADOW_HEIGHT = 2000;

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

